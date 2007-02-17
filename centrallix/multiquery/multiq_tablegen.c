#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
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
/* Module: 	multiq_tablegen.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 26, 1999					*/
/* Description:	Provides support for a Tabular Output operation, which	*/
/*		forms the framework around a join/projection operation	*/
/*		and allows for constants to be in the query.  This	*/
/*		module is _always_ used by queries.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: multiq_tablegen.c,v 1.5 2007/02/17 04:18:14 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_tablegen.c,v $

    $Log: multiq_tablegen.c,v $
    Revision 1.5  2007/02/17 04:18:14  gbeeley
    - (bugfix) SQL engine was not properly setting ObjCoverageMask on
      expression trees built from components of the where clause, thus
      expressions tended to not get re-evaluated when new values were
      available.

    Revision 1.4  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.3  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.2  2002/11/14 03:46:39  gbeeley
    Updated some files that were depending on the old xaAddItemSorted() to
    use xaAddItemSortedInt32() because these uses depend on sorting on a
    binary integer field, which changes its physical byte ordering based
    on the architecture of the machine's CPU.

    Revision 1.1.1.1  2001/08/13 18:00:54  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:58  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/** Private data structure for doing groupby, etc **/
typedef struct
    {
    unsigned char	GroupByBuf[2][2048];
    unsigned char*	GroupByPtr;
    int			GroupByLen;
    pExpression		GroupByItems[16];
    int			nGroupByItems;
    pObject		SavedObjList[16];
    int			nObjects;
    int			IsLastRow;
    int			AggLevel;
    pExpression		ListItems[16];
    int			ListCount[16];
    int			nListItems;
    }
    MQTData, *pMQTData;


/** GLOBALS for this module **/
struct
    {
    pQueryDriver	ThisDriver;
    }
    MQTINF;


/*** mqt_internal_CheckGroupBy -- build a binary image of the group-by 
 *** columns and compare with the last row.  If different (or first row),
 *** return 1 else return 0.  The new binary image for the row will be
 *** in the alternate binary row buffer.  Returns -1 on error.
 ***/
int
mqt_internal_CheckGroupBy(pQueryElement qe, pMultiQuery mq, pMQTData md, unsigned char** new_ptr)
    {
    unsigned char* cur_buf;
    unsigned char* new_buf;
    unsigned char* ptr;
    int i;
    pExpression exp;

    	/** Figure out which buffer is which. **/
	if (!md->GroupByPtr || md->GroupByPtr == md->GroupByBuf[1])
	    {
	    cur_buf = md->GroupByBuf[1];
	    new_buf = md->GroupByBuf[0];
	    }
	else
	    {
	    cur_buf = md->GroupByBuf[0];
	    new_buf = md->GroupByBuf[1];
	    }

	/** Build the binary image of the group by expression results **/
	ptr = new_buf;
	for(i=0;i<md->nGroupByItems;i++)
	    {
	    /** Evaluate the item **/
	    exp = md->GroupByItems[i];
	    if (expEvalTree(exp, mq->QTree->ObjList) < 0)
	        {
		mssError(0,"MQT","Error evaluating group-by item #%d",i+1);
		return -1;
		}

	    /** Pick the data type and copy the stinkin thing **/
	    if (exp->Flags & EXPR_F_NULL)
	        {
		*(ptr++) = '0';
		}
	    else
	        {
		*(ptr++) = '1';
		switch(exp->DataType)
		    {
		    case DATA_T_INTEGER:
		        memcpy(ptr, &(exp->Integer), 4);
			ptr += 4;
			break;

		    case DATA_T_STRING:
		        memcpy(ptr, exp->String, strlen(exp->String));
			ptr += strlen(exp->String);
			break;

		    case DATA_T_DATETIME:
		        memcpy(ptr, &(exp->Types.Date), sizeof(DateTime));
			ptr += sizeof(DateTime);
			break;

		    case DATA_T_MONEY:
		        memcpy(ptr, &(exp->Types.Money), sizeof(MoneyType));
			ptr += sizeof(MoneyType);
			break;

		    case DATA_T_DOUBLE:
		        memcpy(ptr, &(exp->Types.Double), sizeof(double));
			ptr += sizeof(double);
			break;
		    }
		}
	    }

	/** Ok, figure out if the group by columns changed **/
	*new_ptr = new_buf;
	if (md->GroupByPtr == NULL) return 1;
	if (memcmp(cur_buf, new_buf, ptr - new_buf)) return 1;

    return 0;
    }


/*** mqtAnalyze - take a given query syntax structure, locate some SELECT
 *** clauses, and build a tablegen item for each select clause, possibly
 *** linking to the constant expressions within the querystructure items.
 ***/
int
mqtAnalyze(pMultiQuery mq)
    {
    pQueryStructure qs = NULL, item, subitem, where_qs, where_item;
    pQueryElement qe,recent;
    pExpression new_exp;
    int i;
    int n = 0;
    pMQTData md;

    	/** Search for SELECT statements... **/
	while ((qs = mq_internal_FindItem(mq->QTree, MQ_T_SELECTCLAUSE, qs)) != NULL)
	    {
	    /** Allocate a new query-element **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQTINF.ThisDriver;

	    /** Allocate the private data stuff **/
	    qe->PrivateData = (void*)nmMalloc(sizeof(MQTData));
	    memset(qe->PrivateData, 0, sizeof(MQTData));
	    md = (pMQTData)(qe->PrivateData);
	    md->nListItems = 0;

	    /** Does this have a set rowcount? Store it in SlaveIterCnt. **/
	    qe->SlaveIterCnt = 0;
	    for(i=0;i<qs->Parent->Children.nItems;i++)
	        {
		item = (pQueryStructure)(qs->Parent->Children.Items[i]);
		if (item->NodeType == MQ_T_SETOPTION && !strcmp(item->Name,"rowcount"))
		    {
		    qe->SlaveIterCnt = strtol(item->Source, NULL, 10);
		    break;
		    }
		}

	    /** Need to link in with each of the select-items. **/
	    recent = NULL;
	    md->AggLevel = 0;
	    for(i=0;i<qs->Children.nItems;i++)
	        {
		item = (pQueryStructure)(qs->Children.Items[i]);
		xaAddItem(&qe->AttrNames, (void*)item->Presentation);
		xaAddItem(&qe->AttrExprPtr, (void*)item->RawData.String);
		xaAddItem(&qe->AttrCompiledExpr, (void*)item->Expr);
		if (item->Expr->AggLevel > md->AggLevel) md->AggLevel = item->Expr->AggLevel;
		if (item->QELinkage)
		    {
		    if (item->QELinkage != recent)
		        {
			if (xaFindItem(&qe->Children, (void*)item->QELinkage) < 0)
			    xaAddItem(&qe->Children, (void*)item->QELinkage);
			}
		    recent = item->QELinkage;
		    xaAddItem(&qe->AttrDeriv, (void*)item->QELinkage);
		    }
		else
		    {
		    /** No linkage but we can't handle it??? **/
		    if (item->ObjCnt > 0)
		        {
			nmFree(qe->PrivateData,sizeof(MQTData));
			nmFree(qe,sizeof(QueryElement));
			mssError(1,"MQT","Bark! Unhandled SELECT item in query - aborting");
			return -1;
			}

		    /** Is it a list?  If so, make a note of it. **/
		    if (md->nListItems >= 16)
		        {
			nmFree(qe->PrivateData,sizeof(MQTData));
			nmFree(qe,sizeof(QueryElement));
			mssError(1,"MQT","Bark! Too many lists in select query");
			return -1;
			}

		    /** Ok, grab this one... **/
		    xaAddItem(&qe->AttrDeriv, (void*)NULL);
		    item->QELinkage = qe;
		    }
		}

	    /** No items linked via SELECT items, but a mq->Tree is set? **/
	    if (qe->Children.nItems == 0 && mq->Tree != NULL)
	        {
		xaAddItem(&qe->Children, (void*)(mq->Tree));
		}

	    /** Find the WHERE items that didn't specifically match anything else **/
	    qe->Constraint = NULL;
	    where_qs = mq_internal_FindItem(qs->Parent, MQ_T_WHERECLAUSE, NULL);
	    if (where_qs)
	        {
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		    if (1)
		        {
			/** Add this expression into the constraint for this element **/
			if (qe->Constraint == NULL)
			    {
			    qe->Constraint = where_item->Expr;
			    }
			else
			    {
			    new_exp = expAllocExpression();
			    new_exp->NodeType = EXPR_N_AND;
			    expAddNode(new_exp, qe->Constraint);
			    expAddNode(new_exp, where_item->Expr);
			    qe->Constraint = new_exp;
			    }

			/** Release the where item's structure **/
			where_item->Expr = NULL;
			mq_internal_FreeQS(where_item);

			/** Remove this from the WHERE clause and keep looking for more. **/
			xaRemoveItem(&where_qs->Children,i);
			i--;
			continue;
			}
		    }
		}
#if 0
	    /** Look for a having clause to apply to the whole select results **/
	    item = mq_internal_FindItem(qs->Parent, MQ_T_HAVINGCLAUSE, NULL);
	    if (item && item->Expr)
	        {
		/** Add this expression into the constraint for this element **/
		if (qe->Constraint == NULL)
		    {
		    qe->Constraint = item->Expr;
		    }
		else
		    {
		    new_exp = expAllocExpression();
		    new_exp->NodeType = EXPR_N_AND;
		    xaAddItem(&new_exp->Children, (void*)qe->Constraint);
		    xaAddItem(&new_exp->Children, (void*)item->Expr);
		    qe->Constraint = new_exp;
		    }

		/** Note that we 'used' the expression... **/
		item->Expr = NULL;
		}
#endif
	    /** Look for a group-by clause as well **/
	    item = mq_internal_FindItem(qs->Parent, MQ_T_GROUPBYCLAUSE, NULL);
	    if (item)
	        {
		for(i=0;i<item->Children.nItems;i++)
		    {
		    subitem = (pQueryStructure)(item->Children.Items[i]);
		    md->GroupByItems[md->nGroupByItems++] = subitem->Expr;
		    }
		}

	    /** Link the qe into the multiquery **/
	    while(mq->Trees[n]) n++;
	    mq->Trees[n] = qe;
	    mq->Tree = qe;
	    }

    return 0;
    }


/*** mqtStart - starts the query operation for a given tabular query
 *** element.  This evaluates the constant expressions and gets them
 *** ready for retrieval.
 ***/
int
mqtStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    int i;
    pQueryElement cld;

    	/** First, evaluate all of the attributes that we 'own' **/
	mq->QTree->ObjList->Session = mq->SessionID;
	for(i=0;i<qe->AttrNames.nItems;i++) if (qe->AttrDeriv.Items[i] == NULL && ((pExpression)(qe->AttrCompiledExpr.Items[i]))->AggLevel == 0)
	    {
	    if (expEvalTree((pExpression)qe->AttrCompiledExpr.Items[i], mq->QTree->ObjList) < 0) 
	        {
		mssError(0,"MQT","Could not evaluate SELECT item's value");
		return -1;
		}
	    }

	/** Now, 'trickle down' the Start operation to the child item(s). **/
	for(i=0;i<qe->Children.nItems;i++)
	    {
	    cld = (pQueryElement)(qe->Children.Items[i]);
	    if (cld->Driver->Start(cld, mq, NULL) < 0) 
	        {
		mssError(0,"MQT","Failed to start child join/projection operation");
		return -1;
		}
	    }

	/** Set iteration cnt to 0 **/
	qe->IterCnt = 0;

    return 0;
    }


/*** mqt_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqt_internal_CheckConstraint(pQueryElement qe, pMultiQuery mq)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, mq->QTree->ObjList);
	    if (qe->Constraint->DataType != DATA_T_INTEGER)
	        {
	        mssError(1,"MQT","WHERE clause item must have a boolean/integer value.");
	        return -1;
	        }
	    if (!(qe->Constraint->Flags & EXPR_F_NULL) && qe->Constraint->Integer != 0) return 1;
	    }
        else
	    {
            return 1;
	    }

    return 0;
    }


/*** mqtNextItem - retrieves the first/next item in the result set for the
 *** tablular query.  This driver only runs its own loop once through, but
 *** within that one iteration may be multiple row result sets handled by
 *** lower-level drivers, like a projection or join operation.
 ***/
int
mqtNextItem(pQueryElement qe, pMultiQuery mq)
    {
    pQueryElement cld;
    int rval,ck;
    int i;
    pExpression exp;
    pMQTData md = (pMQTData)(qe->PrivateData);
    unsigned char* bptr;
    pObject tmp_obj;

    	/** Check the setrowcount... **/
	if (qe->SlaveIterCnt > 0 && qe->IterCnt >= qe->SlaveIterCnt) return 0;

    	/** Pass the NextItem on to the child, otherwise just 1 row. **/
	cld = (pQueryElement)(qe->Children.Items[0]);
	qe->IterCnt++;

	/** Check to see if we're doing group-by **/
	if (md->nGroupByItems == 0 && md->AggLevel == 0)
	    {
	    /** Next, retrieve until end or until end of group **/
	    if (cld && qe->Children.nItems > 0)
	        {
	        while(1)
	            {
	            rval = cld->Driver->NextItem(cld, mq);
		    if (rval <= 0) break;
		    ck = mqt_internal_CheckConstraint(qe, mq);
		    if (ck < 0) return ck;
		    if (ck == 1) break;
		    }
		}
	    else
	        {
		ck = mqt_internal_CheckConstraint(qe, mq);
		if (ck < 0) return ck;
		if (ck == 0) return 0;
	        rval = (qe->IterCnt==1)?1:0;
		}
	    return rval;
	    }
	else
	    {
	    /** First, reset all aggregate counters/sums/etc **/
	    for(i=0;i<qe->AttrCompiledExpr.nItems;i++) 
	        {
	        exp = (pExpression)(qe->AttrCompiledExpr.Items[i]);
	        expResetAggregates(exp, -1);
	        }

	    /** Restore a saved object list? **/
	    if (md->nObjects != 0 && qe->IterCnt > 1)
	        {
		mq->QTree->ObjList->SeqID++;
		for(i=0;i<md->nObjects;i++)
		    {
		    tmp_obj = md->SavedObjList[i];
		    md->SavedObjList[i] = mq->QTree->ObjList->Objects[i];
		    mq->QTree->ObjList->Objects[i] = tmp_obj;
		    mq->QTree->ObjList->SeqIDs[i] = mq->QTree->ObjList->SeqID;
		    if (md->SavedObjList[i]) objClose(md->SavedObjList[i]);
		    }
		md->nObjects = 0;
		}

	    /** That was the last row? **/
	    if (md->IsLastRow == 1)
	        {
		return 0;
		}

	    /** Prime with the first row if necessary **/
	    if (qe->IterCnt == 1)
	        {
	        if (cld && qe->Children.nItems > 0)
		    {
	            while(1)
	                {
	                rval = cld->Driver->NextItem(cld, mq);
			if (rval <= 0) break;
		        ck = mqt_internal_CheckConstraint(qe, mq);
		        if (ck < 0) return ck;
		        if (ck == 1) break;
		        }
		    }
	        else
		    {
	            rval = (qe->IterCnt==1)?1:0;
		    ck = mqt_internal_CheckConstraint(qe, mq);
		    if (ck < 0) return ck;
		    if (ck == 0) rval = 0;
		    }
		if (rval == 0)
		    {
		    if (md->AggLevel > 0)
		        {
		        md->IsLastRow = 1;
		        return 1;
			}
		    else
		        {
			return 0;
			}
		    }
		mqt_internal_CheckGroupBy(qe, mq, md, &(md->GroupByPtr));
		}

	    /** This is the group-by loop. **/
	    while(1)
	        {
		/** Update all aggregate counters **/
		for(i=0;i<qe->AttrCompiledExpr.nItems;i++)
		    {
	            exp = (pExpression)(qe->AttrCompiledExpr.Items[i]);
		    if (exp->AggLevel != 0 || qe->AttrDeriv.Items[i] != NULL)
		        {
		        expUnlockAggregates(exp);
		        expEvalTree(exp, mq->QTree->ObjList);
			}
		    }

		/** Link to all objects in the current object list **/
		memcpy(md->SavedObjList, mq->QTree->ObjList->Objects, mq->QTree->ObjList->nObjects*sizeof(pObject));
		md->nObjects = mq->QTree->ObjList->nObjects;
		for(i=0;i<md->nObjects;i++) if (md->SavedObjList[i]) objLinkTo(md->SavedObjList[i]);

	        /** Next, retrieve until end or until end of group **/
	        if (cld && qe->Children.nItems > 0)
		    {
	            while(1)
	                {
	                rval = cld->Driver->NextItem(cld, mq);
			if (rval <= 0) break;
		        ck = mqt_internal_CheckConstraint(qe, mq);
		        if (ck < 0) return ck;
		        if (ck == 1) break;
		        }
		    }
	        else
		    {
	            rval = (qe->IterCnt==1)?1:0;
		    ck = mqt_internal_CheckConstraint(qe, mq);
		    if (ck < 0) return ck;
		    if (ck == 0) rval = 0;
		    }

		/** Last one? **/
		if (rval == 0) 
		    {
		    mq->QTree->ObjList->SeqID++;
		    for(i=0;i<md->nObjects;i++)
		        {
			tmp_obj = md->SavedObjList[i];
			md->SavedObjList[i] = mq->QTree->ObjList->Objects[i];
			mq->QTree->ObjList->Objects[i] = tmp_obj;
			mq->QTree->ObjList->SeqIDs[i] = mq->QTree->ObjList->SeqID;
			}
		    md->IsLastRow = 1;
		    return 1;
		    }

		/** Is this a group-end?  Return now if so. **/
		if (mqt_internal_CheckGroupBy(qe, mq, md, &bptr) == 1 || !cld || qe->Children.nItems == 0)
		    {
		    mq->QTree->ObjList->SeqID++;
		    for(i=0;i<md->nObjects;i++)
		        {
			tmp_obj = md->SavedObjList[i];
			md->SavedObjList[i] = mq->QTree->ObjList->Objects[i];
			mq->QTree->ObjList->Objects[i] = tmp_obj;
			mq->QTree->ObjList->SeqIDs[i] = mq->QTree->ObjList->SeqID;
			}
		    md->GroupByPtr = bptr;
		    if (!cld || qe->Children.nItems == 0) md->IsLastRow = 1;
		    return 1;
		    }
		else
		    {
		    for(i=0;i<md->nObjects;i++) if (md->SavedObjList[i]) objClose(md->SavedObjList[i]);
		    }
		}
	    }

    return 0;
    }


/*** mqtFinish - ends the tabular data operation and frees up any memory used
 *** in the process of running the "query".
 ***/
int
mqtFinish(pQueryElement qe, pMultiQuery mq)
    {
    pQueryElement cld;
    int i;

    	/** Trickle down the Finish to the child objects **/
	for(i=0;i<qe->Children.nItems;i++)
	    {
	    cld = (pQueryElement)(qe->Children.Items[i]);
	    if (cld->Driver->Finish(cld, mq) < 0) return -1;
	    }

    return 0;
    }


/*** mqtRelease - release the private data structure MQTData allocated
 *** during the Analyze phase.
 ***/
int
mqtRelease(pQueryElement qe, pMultiQuery mq)
    {
    	
	/** Free the group by structure **/
	nmFree(qe->PrivateData, sizeof(MQTData));

    return 0;
    }


/*** mqtInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqtInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQT - MultiQuery Tabular Data Module");
	drv->Precedence = 4000;
	drv->Flags = 0;
	drv->Analyze = mqtAnalyze;
	drv->Start = mqtStart;
	drv->NextItem = mqtNextItem;
	drv->Finish = mqtFinish;
	drv->Release = mqtRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQTINF.ThisDriver = drv;

    return 0;
    }
