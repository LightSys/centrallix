#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "multiquery.h"
#include "cxlib/mtsession.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	multiq_equijoin.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 8, 1999    					*/
/* Description:	Provides support for equi-joins, where the join is 	*/
/*		fully resolved on a given data field, and is not a	*/
/*		cartesian-product type of join.				*/
/*		April 5, 2000 - added <n>-orquery optimization		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: multiq_equijoin.c,v 1.7 2007/02/17 04:18:14 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_equijoin.c,v $

    $Log: multiq_equijoin.c,v $
    Revision 1.7  2007/02/17 04:18:14  gbeeley
    - (bugfix) SQL engine was not properly setting ObjCoverageMask on
      expression trees built from components of the where clause, thus
      expressions tended to not get re-evaluated when new values were
      available.

    Revision 1.6  2005/09/17 01:31:33  gbeeley
    - Proper error return values from queries containing joins when one of
      the join halves doesn't open correctly.

    Revision 1.5  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.4  2004/12/31 04:19:43  gbeeley
    - bug fix for 'cannot be inner and outer member of an outer join'
    - bug fix for WHERE expression not being completely reevaluated

    Revision 1.3  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.2  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.1.1.1  2001/08/13 18:00:54  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:57  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** This tuning parameter affects how a join is performed.  The first of
 *** the parameters, MQJ_ENABLE_PREFETCH, turns on a mechanism by which the
 *** join mechanism will read <n> rows from the "master" query at a time and
 *** then build a long "thing=valueA or thing=valueB or thing=valueC" etc.
 *** type of additional-constraint to send to the slave query.  This process
 *** will repeat for each group of <n> or fewer rows until the master query
 *** is exhausted.  Note that this is typically only valid for equijoins, not
 *** for joins on greater-than or less-than.
 ***
 *** The idea is to improve performance with intelligent datasources (like
 *** Sybase) so that only 1/nth of the number of queries actually need to be
 *** sent to the intelligent data source.  Performance should be about the 
 *** same with non-intelligent data sources (like CSV files).
 ***/
#define MQJ_ENABLE_PREFETCH	0
#define MQJ_MAX_ORQUERY		16


/*** Globals ***/
struct
    {
    pQueryDriver        ThisDriver;
    }
    MQJINF;


/*** "private data" structure for mqj-specific operations, including the
 *** n-row-prefetch algorithm.
 ***/
typedef struct
    {
    pObject 		PrefetchObjs[MQJ_MAX_ORQUERY];
    int			PrefetchMatchCnt[MQJ_MAX_ORQUERY];
    int 		nPrefetch;
    int			CurItem;
    pExpression		SlaveExpr;
    }
    MqjJoinData, *pMqjJoinData;



/*** mqjAnalyze - take a given query syntax structure (qs) and scans it for
 *** equijoin operations, by first scanning the Where clause for the various
 *** join statements, and then piecing them together to form a set of joins
 *** done in the 'proper' order :)
 ***/
int
mqjAnalyze(pMultiQuery mq)
    {
    pQueryElement qe;
    pQueryElement master=NULL,slave=NULL;
    pQueryStructure from_qs = NULL;
    pQueryStructure select_qs = NULL;
    pQueryStructure where_qs = NULL;
    pQueryStructure select_item;
    pQueryStructure where_item;
    pQueryStructure from_item=NULL;
    int i,n=0,j,found,m;
    pExpression new_exp;
    unsigned short join_mask[16];
    unsigned short join_outer[16];
    signed char join_obj1[16];
    signed char join_obj2[16];
    unsigned char join_spec[16];
    unsigned char join_map[16];
    unsigned short joined_objects;
    unsigned char join_used[16];
    pQueryStructure from_sources[16];
    unsigned char from_srcmap[16];
    int n_joins = 0, n_joins_used;

    	/** Search for WHERE clauses with join operations... **/
	while((where_qs = mq_internal_FindItem(mq->QTree, MQ_T_WHERECLAUSE, where_qs)) != NULL)
	    {
	    /** Build a list of join expressions available in the where clause. **/
	    for(i=0;i<where_qs->Children.nItems;i++)
	        {
		where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		if (where_item->ObjCnt == 2)
		    {
		    /** Already seen this join combination? **/
		    for(n=-1,j=0;j<n_joins;j++)
		        {
			if (join_mask[j] == where_item->Expr->ObjCoverageMask) 
			    {
			    n = j;
			    if (join_outer[n] != where_item->Expr->ObjOuterMask)
			        {
				mssError(1,"MQJ","An entity cannot be both an inner and outer member of an outer join");
				return -1;
				}
			    break;
			    }
			}

		    /** If not seen it, add it to our list **/
		    if (n < 0)
		        {
			n = n_joins;
			join_outer[n_joins] = where_item->Expr->ObjOuterMask;
			join_mask[n_joins++] = where_item->Expr->ObjCoverageMask;
			}
		    }
		}

	    /** No joins in this where clause? **/
	    if (n_joins == 0) continue;
	
	    /** Get the select clause **/
	    select_qs = mq_internal_FindItem(where_qs->Parent, MQ_T_SELECTCLAUSE, NULL);

	    /** Build a list of the FROM sources, and init the source mapping **/
	    from_qs = mq_internal_FindItem(where_qs->Parent, MQ_T_FROMCLAUSE, NULL);
	    for(i=0;i<mq->QTree->ObjList->nObjects;i++)
	        {
		/** init the map - which we'll use for sorting the from items by specificity. **/
		from_srcmap[i] = i;

		/** Search for the object in the FROM clause **/
		found = -1;
		for(j=0;j<from_qs->Children.nItems;j++)
		    {
		    from_item = (pQueryStructure)(from_qs->Children.Items[j]);
		    if (!strcmp(mq->QTree->ObjList->Names[i], 
		        from_item->Presentation[0]?(from_item->Presentation):(from_item->Source)))
			{
			found = j;
			break;
			}
		    }
		if (found < 0) 
		    {
		    mssError(1,"MQE","Join: referenced object not found in FROM clause");
		    return -1;
		    }
		from_sources[i] = from_item;
		}

	    /** Ok, now sort by specificity **/
	    for(i=0;i<mq->QTree->ObjList->nObjects;i++)
	        {
		for(j=i+1;j<mq->QTree->ObjList->nObjects;j++)
		    {
		    if (from_sources[from_srcmap[i]]->Specificity < from_sources[from_srcmap[j]]->Specificity)
		        {
			n = from_srcmap[i];
			from_srcmap[i] = from_srcmap[j];
			from_srcmap[j] = n;
			}
		    }
		}

	    /** Make it easier to find out which two objects are in each join. **/
	    for(i=0;i<n_joins;i++)
	        {
		join_obj1[i] = -1;
		join_obj2[i] = -1;
		j = 0;
		m = join_mask[i];
		while(join_obj1[i] < 0) 
		    {
		    if (m & 1) join_obj1[i] = j;
		    m >>= 1;
		    j++;
		    }
		while(join_obj2[i] < 0)
		    {
		    if (m & 1) join_obj2[i] = j;
		    m >>= 1;
		    j++;
		    }
		}

	    /** Build the join specificity array **/
	    for(i=0;i<n_joins;i++)
	        {
		join_spec[i] = from_sources[join_obj1[i]]->Specificity + from_sources[join_obj2[i]]->Specificity + 1;
		join_map[i] = i;
		join_used[i] = 0;
		}

	    /** Adjust the specificity to properly order according to outer-inner relationships **/
	    for(i=0;i<n_joins;i++)
	        {
		for(j=0;j<n_joins;j++)
		    {
		    if (i==j) continue;
		    if (join_obj1[i] == join_obj1[j] && (join_outer[i] & (1<<join_obj1[i])) && !(join_outer[j] & (1<<join_obj1[j])))
		        {
			join_spec[j] += join_spec[i];
			}
		    else if (join_obj1[i] == join_obj2[j] && (join_outer[i] & (1<<join_obj1[i])) && !(join_outer[j] & (1<<join_obj2[j])))
		        {
			join_spec[j] += join_spec[i];
			}
		    else if (join_obj2[i] == join_obj1[j] && (join_outer[i] & (1<<join_obj2[i])) && !(join_outer[j] & (1<<join_obj1[j])))
		        {
			join_spec[j] += join_spec[i];
			}
		    else if (join_obj2[i] == join_obj2[j] && (join_outer[i] & (1<<join_obj2[i])) && !(join_outer[j] & (1<<join_obj2[j])))
		        {
			join_spec[j] += join_spec[i];
			}
		    }
		}

	    /** Sort the joins by specificity **/
	    for(i=0;i<n_joins;i++)
	        {
		for(j=i+1;j<n_joins;j++)
		    {
		    if (join_spec[join_map[i]] < join_spec[join_map[j]])
		        {
			n = join_map[i];
			join_map[i] = join_map[j];
			join_map[j] = n;
			}
		    }
		}

	    /** Ok, got list of join expressions.  Now create a JOIN from each one. **/
	    joined_objects = 0;
	    n_joins_used = 0;
	    do  {
	        /** Find a high-specificity join that can hook into any already processed joins **/
	        found = -1;
		for(i=0;i<n_joins;i++) if (!join_used[join_map[i]])
		    {
		    if (n_joins_used == 0 || (joined_objects & join_mask[join_map[i]]))
		        {
			found = join_map[i];
			break;
			}
		    }

		/** Create the new QueryElement and link into the exec tree... **/
		qe = mq_internal_AllocQE();
		qe->Driver = MQJINF.ThisDriver;
		if (join_outer[found]) qe->Flags |= MQ_EF_OUTERJOIN;

		/** Check master/slave ordering of the join. **/
		if (qe->Flags & MQ_EF_OUTERJOIN)
		    {
		    /** If outerjoin, the ordering is preset.  Forget about specificity. **/
		    if (join_outer[found] & (1<<join_obj2[found]))
		        {
		        n = join_obj1[found];
		        join_obj1[found] = join_obj2[found];
		        join_obj2[found] = n;
			}
		    }
		else if (from_sources[join_obj1[found]]->Specificity < from_sources[join_obj2[found]]->Specificity)
		    {
		    n = join_obj1[found];
		    join_obj1[found] = join_obj2[found];
		    join_obj2[found] = n;
		    }

		/** Verify that the master object connects with existing join(s) **/
		if (n_joins_used > 0 && !(joined_objects & (1<<join_obj1[found])))
		    {
		    if (qe->Flags & MQ_EF_OUTERJOIN)
		        {
			/** Can't reverse outer join!  Error! **/
			mssError(1,"MQJ","Bark!  Internal join order problem!");
			mq_internal_FreeQE(qe);
			return -1;
			}
		    n = join_obj1[found];
		    join_obj1[found] = join_obj2[found];
		    join_obj2[found] = n;
		    }

		/** Find the master QE from the MQ->Trees array if 1st join, otherwise use MQ->Tree. **/
		if (n_joins_used == 0)
		    {
		    for(i=0;mq->Trees[i];i++)
		        {
			master = mq->Trees[i];
			if (((pQueryStructure)(master->QSLinkage)) == from_sources[join_obj1[found]]) break;
			}
		    }
		else
		    {
		    master = mq->Tree;
		    }
		qe->SrcIndex = join_obj1[found];
		qe->SrcIndexSlave = join_obj2[found];

		/** Find the slave QE, from the MQ->Trees array, whether 1st or nth join. **/
		for(i=0;mq->Trees[i];i++)
		    {
		    slave = mq->Trees[i];
		    if (((pQueryStructure)(slave->QSLinkage)) == from_sources[join_obj2[found]]) break;
		    }

		/** 'steal' the qe-linkages from the select items for any sub-objects. **/
		for(i=0;i<select_qs->Children.nItems;i++)
		    {
		    select_item = (pQueryStructure)(select_qs->Children.Items[i]);
		    if (select_item->QELinkage == slave || select_item->QELinkage == master)
		        select_item->QELinkage = qe;
		    }

		/** Setup the attribute list in the queryelement structure **/
		for(i=0;i<select_qs->Children.nItems;i++)
		    {
		    select_item = (pQueryStructure)(select_qs->Children.Items[i]);
		    if ((select_item->Expr->ObjCoverageMask & ~(joined_objects | join_mask[found])) == 0)
		        {
			xaAddItem(&qe->AttrNames, (void*)select_item->Presentation);
			xaAddItem(&qe->AttrExprPtr, (void*)select_item->RawData.String);
			xaAddItem(&qe->AttrCompiledExpr, (void*)select_item->Expr);
			xaAddItem(&qe->AttrDeriv, (void*)(select_item->QELinkage));
			select_item->QELinkage = qe;
			}
		    }

		/** Grab up the WHERE expressions for this join... **/
		qe->Constraint = NULL;
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		    if (where_item->Expr->ObjCoverageMask == join_mask[found])
		        {
			if (qe->Constraint)
			    {
			    new_exp = expAllocExpression();
			    new_exp->NodeType = EXPR_N_AND;
			    expAddNode(new_exp, qe->Constraint);
			    expAddNode(new_exp, where_item->Expr);
			    qe->Constraint = new_exp;
			    }
			else
			    {
			    qe->Constraint = where_item->Expr;
			    }
			xaRemoveItem(&where_qs->Children,i);
			where_item->Expr = NULL;
			mq_internal_FreeQS(where_item);
			i--;
			continue;
			}
		    }

		/** Add the master and slave below this qe **/
		if (!master || !slave)
		    {
		    mssError(1,"MQJ","Bark!  Could not locate master/slave query component(s)!");
		    mq_internal_FreeQE(qe);
		    return -1;
		    }
		xaAddItem(&qe->Children,(void*)master);
		xaAddItem(&qe->Children,(void*)slave);
		mq->Tree = qe;
		qe->QSLinkage = NULL;
		qe->PrivateData = (void*)nmMalloc(sizeof(MqjJoinData));

		/** Set the joined_objects bitmask to include the objects we have here **/
		joined_objects |= join_mask[found];

		/** Mark the one we found used. **/
		join_used[found] = 1;
		n_joins_used++;
	        } while (n_joins_used < n_joins);
	    }

    return 0;
    }


/*** mqjStart - starts the join operation.  This function mostly does 
 *** some initialization, but does not fetch any rows.
 ***/
int
mqjStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    pQueryElement master;
    pMqjJoinData md;

    	/** Initialize the iteration counts. **/
	qe->IterCnt = 0;
	qe->SlaveIterCnt = 0;
	qe->Flags &= ~MQ_EF_SLAVESTART;
	md = (pMqjJoinData)(qe->PrivateData);
	md->nPrefetch = 0;
	md->CurItem = 0;

	/** Start the query on the master side. **/
	master = (pQueryElement)(qe->Children.Items[0]);
	if (master->Driver->Start(master, mq, NULL) < 0) return -1;

    return 0;
    }


/*** mqj_internal_NextItemPrefetch - retrieves the first/next item in the
 *** joined result set, using the prefetch-n-rows method detailed at the
 *** top of this source code.  Only enabled if MQJ_ENABLE_PREFETCH is set
 *** to 1 (instead of 0).
 ***/
int
mqj_internal_NextItemPrefetch(pQueryElement qe, pMultiQuery mq)
    {
    pQueryElement master,slave;
    pMqjJoinData md;

	/** Determine master and slave sides of query **/
	master = (pQueryElement)(qe->Children.Items[0]);
	slave = (pQueryElement)(qe->Children.Items[1]);
	md = (pMqjJoinData)(qe->PrivateData);

    return 1;
    }


/*** mqjNextItem - retrieves the first/next item in the joined result set.
 *** This function must iterate through each item of the master's result
 *** set, and for each one, call a start, nextitem{}, and finish for the
 *** slave's result set.  Return 1 if valid row, 0 if no more rows, -1 if
 *** error.
 ***/
int
mqjNextItem(pQueryElement qe, pMultiQuery mq)
    {
    pQueryElement master,slave;
    int rval;

    	/** Do query in multiple-OR-fetch mode? **/
	if (MQJ_ENABLE_PREFETCH) return mqj_internal_NextItemPrefetch(qe,mq);

    	/** Get master, slave ptrs **/
	master = (pQueryElement)(qe->Children.Items[0]);
	slave = (pQueryElement)(qe->Children.Items[1]);

	/** Loop until we get a row. **/
	while(1)
	    {
	    /** IF slave cnt is 0, get a new master row and start the slave... **/
	    if (qe->SlaveIterCnt == 0)
	        {
	        rval = master->Driver->NextItem(master, mq);
	        if (rval == 0 || rval < 0) return rval;
	        qe->IterCnt++;
		expFreezeEval(qe->Constraint, mq->QTree->ObjList, slave->SrcIndex);
		if (mq->QTree->ObjList->Objects[qe->SrcIndex] != NULL)
		    {
		    if (slave->Driver->Start(slave, mq, qe->Constraint) < 0)
			return -1;
		    qe->Flags |= MQ_EF_SLAVESTART;
		    }
	        }

	    /** Ok, retrieve a slave row. **/
	    if (mq->QTree->ObjList->Objects[qe->SrcIndex] != NULL) 
	        rval = slave->Driver->NextItem(slave, mq);
	    else
	        rval = 0;
	    if ((rval == 0 || rval < 0) && (qe->SlaveIterCnt > 0 || !(qe->Flags & MQ_EF_OUTERJOIN)))
	        {
		if (mq->QTree->ObjList->Objects[qe->SrcIndex] != NULL) 
		    {
		    slave->Driver->Finish(slave, mq);
		    qe->Flags &= ~MQ_EF_SLAVESTART;
		    }
		qe->SlaveIterCnt = 0;
		continue;
		}
	    qe->SlaveIterCnt++;
	    break;
	    }

    return 1;
    }


/*** mqjFinish - ends the join operation and cleans up after itself...
 ***/
int
mqjFinish(pQueryElement qe, pMultiQuery mq)
    {
    pQueryElement master;
    pQueryElement slave;
    pMqjJoinData md;

    	/** Need to complete the slave-side query? (early query cancel) **/
	md = (pMqjJoinData)(qe->PrivateData);
	if (qe->Flags & MQ_EF_SLAVESTART)
	    {
	    qe->Flags &= ~MQ_EF_SLAVESTART;
	    slave = (pQueryElement)(qe->Children.Items[1]);
	    slave->Driver->Finish(slave, mq);
	    }

    	/** Complete the query on the master side **/
	master = (pQueryElement)(qe->Children.Items[0]);
	master->Driver->Finish(master, mq);

    return 0;
    }


/*** mqjRelease - deallocate any private data structures that were used
 *** in the Analyze phase.  Just returns without doing anything for this
 *** function.
 ***/
int
mqjRelease(pQueryElement qe, pMultiQuery mq)
    {

    	if (qe->PrivateData) nmFree(qe->PrivateData, sizeof(MqjJoinData));

    return 0;
    }


/*** mqjInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqjInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQJ - MultiQuery Equi-Join Module");
	drv->Precedence = 3000;
	drv->Flags = 0;
	drv->Analyze = mqjAnalyze;
	drv->Start = mqjStart;
	drv->NextItem = mqjNextItem;
	drv->Finish = mqjFinish;
	drv->Release = mqjRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQJINF.ThisDriver = drv;

    return 0;
    }
