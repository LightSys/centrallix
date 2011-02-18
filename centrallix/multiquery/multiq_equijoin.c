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

    $Id: multiq_equijoin.c,v 1.17 2011/02/18 03:47:46 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_equijoin.c,v $

    $Log: multiq_equijoin.c,v $
    Revision 1.17  2011/02/18 03:47:46  gbeeley
    enhanced ORDER BY, IS NOT NULL, bug fix, and MQ/EXP code simplification

    - adding multiq_orderby which adds limited high-level order by support
    - adding IS NOT NULL support
    - bug fix for issue involving object lists (param lists) in query
      result items (pseudo objects) getting out of sorts
    - as a part of bug fix above, reworked some MQ/EXP code to be much
      cleaner

    Revision 1.16  2010/09/08 22:22:43  gbeeley
    - (bugfix) DELETE should only mark non-provided objects as null.
    - (bugfix) much more intelligent join dependency checking, as well as
      fix for queries containing mixed outer and non-outer joins
    - (feature) support for two-level aggregates, as in select max(sum(...))
    - (change) make use of expModifyParamByID()
    - (change) disable RequestNotify mechanism as it needs to be reworked.

    Revision 1.15  2010/01/10 07:51:06  gbeeley
    - (feature) SELECT ... FROM OBJECT /path/name selects a specific object
      rather than subobjects of the object.
    - (feature) SELECT ... FROM WILDCARD /path/name*.ext selects from a set of
      objects specified by the wildcard pattern.  WILDCARD and OBJECT can be
      combined.
    - (feature) multiple statements per SQL query now allowed, with the
      statements terminated by semicolons.

    Revision 1.14  2009/06/26 16:04:26  gbeeley
    - (feature) adding DELETE support
    - (change) HAVING clause now honored in INSERT ... SELECT
    - (bugfix) some join order issues resolved
    - (performance) cache 0 or 1 row result sets during a join
    - (feature) adding INCLUSIVE option to SUBTREE selects
    - (bugfix) switch to qprintf for building RawData sql data
    - (change) some minor refactoring

    Revision 1.13  2008/06/25 18:39:47  gbeeley
    - (bugfix) include the "*" in a SELECT * in the topmost join's list of
      attributes, rather than causing squawkage about the query not being set
      up right...

    Revision 1.12  2008/04/06 20:53:49  gbeeley
    - (bugfix) under some conditions, the NULL values in a row resulting from
      an outerjoin were not showing up as NULL.

    Revision 1.11  2008/03/19 07:30:53  gbeeley
    - (feature) adding UPDATE statement capability to the multiquery module.
      Note that updating was of course done previously, but not via SQL
      statements - it was programmatic via objSetAttrValue.
    - (bugfix) fixes for two bugs in the expression module, one a memory leak
      and the other relating to null values when copying expression values.
    - (bugfix) the Trees array in the main multiquery structure could
      overflow; changed to an xarray.

    Revision 1.10  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.9  2007/09/18 17:59:07  gbeeley
    - (change) permit multiple WHERE clauses in the SQL.  They are automatically
      combined using AND.  This permits more flexible building of dynamic SQL
      (no need to do fancy text processing in order to add another WHERE
      constraint to the query).
    - (bugfix) fix for crash when using "SELECT *" with a join.
    - (change) permit the specification of one FROM source to be an "IDENTITY"
      data source for the query.  That data source will be the one affected by
      any inserting and deleting through the query.

    Revision 1.8  2007/07/31 17:39:59  gbeeley
    - (feature) adding "SELECT *" capability, rather than having to name each
      attribute in every query.  Note - "select *" does result in a query
      result set in which each row may differ in what attributes it has,
      depending on the data source(s) used.

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
#define MQJ_MAX_PREFETCH	16

#define MQJ_MAX_JOIN		(EXPR_MAX_PARAMS)


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
    pObject 		PrefetchObjs[MQJ_MAX_PREFETCH];
    int			PrefetchMatchCnt[MQJ_MAX_PREFETCH];
    int 		nPrefetch;
    int			PrefetchUsesSlave;	/* boolean */
    int			PrefetchReachedEnd;	/* boolean */
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
mqjAnalyze(pQueryStatement stmt)
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
    unsigned int join_mask[MQJ_MAX_JOIN];
    unsigned int join_outer[MQJ_MAX_JOIN];
    signed char join_obj1[MQJ_MAX_JOIN];
    signed char join_obj2[MQJ_MAX_JOIN];
    unsigned char join_spec[MQJ_MAX_JOIN];
    unsigned char join_map[MQJ_MAX_JOIN];
    unsigned int joined_objects;
    unsigned char join_used[MQJ_MAX_JOIN];
    pQueryStructure from_sources[MQJ_MAX_JOIN];
    unsigned char from_srcmap[MQJ_MAX_JOIN];
    int n_joins = 0, n_joins_used;
    int min_objlist = stmt->Query->nProvidedObjects;

    	/** Search for WHERE clauses with join operations... **/
	while((where_qs = mq_internal_FindItem(stmt->QTree, MQ_T_WHERECLAUSE, where_qs)) != NULL)
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
	    for(i=min_objlist;i<stmt->Query->ObjList->nObjects;i++)
	        {
		/** init the map - which we'll use for sorting the from items by specificity. **/
		from_srcmap[i] = i;

		/** Search for the object in the FROM clause **/
		found = -1;
		for(j=0;j<from_qs->Children.nItems;j++)
		    {
		    from_item = (pQueryStructure)(from_qs->Children.Items[j]);
		    if (!strcmp(stmt->Query->ObjList->Names[i], 
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
	    for(i=min_objlist;i<stmt->Query->ObjList->nObjects;i++)
	        {
		for(j=i+1;j<stmt->Query->ObjList->nObjects;j++)
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
	    do  {  /** do ... while (n_joins_used < n_joins); **/
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

		/** This join already covered? **/
		if ((join_mask[found] & joined_objects) == join_mask[found])
		    {
		    n_joins_used++;
		    continue;
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
		else if (from_sources[join_obj1[found]]->QELinkage->OrderPrio != from_sources[join_obj2[found]]->QELinkage->OrderPrio)
		    {
		    /** ORDER BY clause trumps specificity at present **/
		    if (from_sources[join_obj1[found]]->QELinkage->OrderPrio > from_sources[join_obj2[found]]->QELinkage->OrderPrio)
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
		    for(i=0;i < xaCount(&stmt->Trees);i++)
		        {
			master = (pQueryElement)xaGetItem(&stmt->Trees, i);
			if (((pQueryStructure)(master->QSLinkage)) == from_sources[join_obj1[found]]) break;
			}
		    }
		else
		    {
		    master = stmt->Tree;
		    }
		qe->SrcIndex = join_obj1[found];
		qe->SrcIndexSlave = join_obj2[found];

		/** Find the slave QE, from the MQ->Trees array, whether 1st or nth join. **/
		for(i=0;i<xaCount(&stmt->Trees);i++)
		    {
		    slave = (pQueryElement)xaGetItem(&stmt->Trees, i);
		    if (((pQueryStructure)(slave->QSLinkage)) == from_sources[join_obj2[found]]) break;
		    }

		/** 'steal' the qe-linkages from the select items for any sub-objects. **/
		if (select_qs)
		    {
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
			if ((select_item->Expr && (select_item->Expr->ObjCoverageMask & ~(stmt->Query->ProvidedObjMask | joined_objects | join_mask[found])) == 0) || ((select_item->Flags & MQ_SF_ASTERISK) && n_joins_used == n_joins-1))
			    {
			    if (select_item->Flags & MQ_SF_ASTERISK)
				stmt->Flags |= MQ_TF_ASTERISK;
			    xaAddItem(&qe->AttrNames, (void*)select_item->Presentation);
			    xaAddItem(&qe->AttrExprPtr, (void*)select_item->RawData.String);
			    xaAddItem(&qe->AttrCompiledExpr, (void*)select_item->Expr);
			    xaAddItem(&qe->AttrDeriv, (void*)(select_item->QELinkage));
			    select_item->QELinkage = qe;
			    }
			}
		    }

		/** Grab up the WHERE expressions for this join... **/
		qe->Constraint = NULL;
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		    /*if (where_item->Expr->ObjCoverageMask == join_mask[found])*/
		    if ((where_item->Expr->ObjCoverageMask & (joined_objects | join_mask[found])) == where_item->Expr->ObjCoverageMask)
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
		master->Parent = qe;
		xaAddItem(&qe->Children,(void*)slave);
		slave->Parent = qe;
		stmt->Tree = qe;
		qe->QSLinkage = NULL;
		qe->PrivateData = (void*)nmMalloc(sizeof(MqjJoinData));
		memset(qe->PrivateData, 0, sizeof(MqjJoinData));
		qe->OrderPrio = (slave->OrderPrio < master->OrderPrio)?slave->OrderPrio:master->OrderPrio;

		/** Set the joined_objects bitmask to include the objects we have here **/
		joined_objects |= join_mask[found];

		/** Mark the one we found used. **/
		join_used[found] = 1;
		n_joins_used++;
	        } while (n_joins_used < n_joins);  /** do ... while **/
	    }

    return 0;
    }


int
mqj_internal_ClosePrefetch(pQueryElement qe)
    {
    int i;
    pMqjJoinData md = (pMqjJoinData)(qe->PrivateData);

	for(i=0;i<md->nPrefetch;i++)
	    {
	    if (md->PrefetchObjs[i])
		{
		objClose(md->PrefetchObjs[i]);
		md->PrefetchObjs[i] = NULL;
		}
	    }
	md->nPrefetch = 0;

    return 0;
    }


/*** mqj_internal_DoPrefetch() - grab up to N rows from the given child
 *** source, and put them in the prefetch array in the PrivateData.
 ***/
int
mqj_internal_DoPrefetch(pQueryElement qe, pQueryStatement stmt)
    {
    pMqjJoinData md = (pMqjJoinData)(qe->PrivateData);
    pQueryElement prefetch_cld;
    int rval;

	/** are we prefetching on master or slave side of the join? **/
	if (md->PrefetchUsesSlave)
	    prefetch_cld = (pQueryElement)(qe->Children.Items[1]);
	else
	    prefetch_cld = (pQueryElement)(qe->Children.Items[0]);

	/** close any existing open prefetch objects **/
	mqj_internal_ClosePrefetch(qe);

	md->PrefetchReachedEnd = 0;

	/** Get at most MQJ_MAX_PREFETCH objects **/
	while(md->nPrefetch < MQJ_MAX_PREFETCH)
	    {
	    rval = prefetch_cld->Driver->NextItem(prefetch_cld, stmt);
	    if (rval < 0)
		{
		/** error from prefetch_cld **/
		return rval;
		}
	    if (rval == 0)
		{
		/** no more rows from prefetch_cld **/
		md->PrefetchReachedEnd = 1;
		break;
		}
	    md->PrefetchObjs[md->nPrefetch] = stmt->Query->ObjList->Objects[prefetch_cld->SrcIndex];
	    if (md->PrefetchObjs[md->nPrefetch])
		objLinkTo(md->PrefetchObjs[md->nPrefetch]);
	    md->nPrefetch++;
	    }

    return 0;
    }


/*** mqjStart - starts the join operation.  This function mostly does 
 *** some initialization, but does not fetch any rows.
 ***/
int
mqjStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
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
	if (master->Driver->Start(master, stmt, NULL) < 0) return -1;

    return 0;
    }


/*** mqj_internal_NextItemPrefetch - retrieves the first/next item in the
 *** joined result set, using the prefetch-n-rows method detailed at the
 *** top of this source code.  Only enabled if MQJ_ENABLE_PREFETCH is set
 *** to 1 (instead of 0).
 ***/
int
mqj_internal_NextItemPrefetch(pQueryElement qe, pQueryStatement stmt)
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
mqjNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement master,slave;
    int rval;
    int i;
    int nullouter;

    	/** Do query in multiple-OR-fetch mode? **/
	if (MQJ_ENABLE_PREFETCH) return mqj_internal_NextItemPrefetch(qe,stmt);

    	/** Get master, slave ptrs **/
	master = (pQueryElement)(qe->Children.Items[0]);
	slave = (pQueryElement)(qe->Children.Items[1]);

	/** Loop until we get a row. **/
	while(1)
	    {
	    /** IF slave cnt is 0, get a new master row and start the slave... **/
	    if (qe->SlaveIterCnt == 0)
	        {
	        rval = master->Driver->NextItem(master, stmt);
	        if (rval == 0 || rval < 0) return rval;
	        qe->IterCnt++;
		/*expFreezeEval(qe->Constraint, stmt->Query->ObjList, slave->SrcIndex);*/
		/*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL)
		    {*/
		    /** If slave only depends on NULL outer elements, then don't run it **/
		    nullouter = 0;
		    for(i=stmt->Query->nProvidedObjects;i<stmt->Query->ObjList->nObjects;i++)
			if (stmt->Query->ObjList->Objects[i] == NULL)
			    nullouter |= (1<<i);
		    if (slave->DependencyMask && (slave->DependencyMask & ~nullouter) == 0)
			{
			rval = 1;
			break;
			}

		    /** Start the slave **/
		    if (slave->Driver->Start(slave, stmt, qe->Constraint) < 0)
			return -1;
		    qe->Flags |= MQ_EF_SLAVESTART;
		    /*}*/
	        }

	    /** Ok, retrieve a slave row. **/
	    /*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL) */
	        rval = slave->Driver->NextItem(slave, stmt);
	    /*else
	        rval = 0;*/
	    if ((rval == 0 || rval < 0) && (qe->SlaveIterCnt > 0 || !(qe->Flags & MQ_EF_OUTERJOIN)))
	        {
		/*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL) 
		    {*/
		    slave->Driver->Finish(slave, stmt);
		    qe->Flags &= ~MQ_EF_SLAVESTART;
		    /*}*/
		qe->SlaveIterCnt = 0;
		continue;
		}
	    else if (rval == 0 && (qe->Flags & MQ_EF_OUTERJOIN))
		{
		/** outer join with NULL row from slave **/
		/*expModifyParam(stmt->Query->ObjList, stmt->Query->ObjList->Names[slave->SrcIndex], NULL);*/
		rval = 1;
		}
	    qe->SlaveIterCnt++;
	    break;
	    }

    return rval;
    }


/*** mqjFinish - ends the join operation and cleans up after itself...
 ***/
int
mqjFinish(pQueryElement qe, pQueryStatement stmt)
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
	    slave->Driver->Finish(slave, stmt);
	    }

    	/** Complete the query on the master side **/
	master = (pQueryElement)(qe->Children.Items[0]);
	master->Driver->Finish(master, stmt);

    return 0;
    }


/*** mqjRelease - deallocate any private data structures that were used
 *** in the Analyze phase.  Just returns without doing anything for this
 *** function.
 ***/
int
mqjRelease(pQueryElement qe, pQueryStatement stmt)
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
