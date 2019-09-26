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
#include "cxlib/util.h"


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


#define MQT_MAX_OBJECTS	    (EXPR_MAX_PARAMS)

/** Private data structure for doing groupby, etc **/
typedef struct
    {
    unsigned char	GroupByBuf[2][2048];
    unsigned char*	GroupByPtr;
    int			GroupByLen;
    pExpression		GroupByItems[MQT_MAX_OBJECTS];
    int			nGroupByItems;
    pObject		SavedObjList[MQT_MAX_OBJECTS];
    int			nObjects;
    int			IsLastRow;
    int			BypassChecked;
    int			AggLevel;
    pExpression		ListItems[MQT_MAX_OBJECTS];
    int			ListCount[MQT_MAX_OBJECTS];
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
mqt_internal_CheckGroupBy(pQueryElement qe, pQueryStatement stmt, pMQTData md, unsigned char** new_ptr)
    {
    unsigned char* cur_buf;
    unsigned char* new_buf;
    int oldlen;

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
#if 00
	ptr = new_buf;
	for(i=0;i<md->nGroupByItems;i++)
	    {
	    /** Evaluate the item **/
	    exp = md->GroupByItems[i];
	    if (expEvalTree(exp, stmt->Query->ObjList) < 0)
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
		        memcpy(ptr, exp->String, strlen(exp->String)+1);
			ptr += (strlen(exp->String)+1);
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
#endif /** 00 **/

	/** Ok, figure out if the group by columns changed **/
	oldlen = md->GroupByLen;
	/*md->GroupByLen = (ptr - new_buf);*/
	md->GroupByLen = objBuildBinaryImage((char*)new_buf, sizeof(md->GroupByBuf[0]), md->GroupByItems, md->nGroupByItems, stmt->Query->ObjList, 0);
	if (md->GroupByLen < 0) return -1;
	*new_ptr = new_buf;
	if (md->GroupByPtr == NULL) return 1;
	if (md->GroupByLen != oldlen || memcmp(cur_buf, new_buf, md->GroupByLen) != 0) return 1;

    return 0;
    }


/*** mqtAnalyze - take a given query syntax structure, locate some SELECT
 *** clauses, and build a tablegen item for each select clause, possibly
 *** linking to the constant expressions within the querystructure items.
 ***/
int
mqtAnalyze(pQueryStatement stmt)
    {
    pQueryStructure qs = NULL, item, subitem, where_qs, where_item;
    pQueryElement qe,recent;
    pExpression new_exp;
    int i;
    pMQTData md;

    	/** Search for SELECT statements... **/
	/*while ((qs = mq_internal_FindItem(mq->QTree, MQ_T_SELECTCLAUSE, qs)) != NULL || (qs = mq_internal_FindItem(mq->QTree, MQ_T_UPDATECLAUSE, qs)) != NULL)*/
	while ((qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, qs)) != NULL)
	    {
	    /** Allocate a new query-element **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQTINF.ThisDriver;

	    /** Allocate the private data stuff **/
	    qe->PrivateData = (void*)nmMalloc(sizeof(MQTData));
	    qe->Constraint = NULL;
	    qe->SlaveIterCnt = 0;
	    memset(qe->PrivateData, 0, sizeof(MQTData));

	    md = (pMQTData)(qe->PrivateData);
	    md->nListItems = 0;
	    md->AggLevel = 0;

	    /** Need to link in with each of the select-items.  This operates both
	     ** on SELECT items and on the RHS of SET items in an UPDATE clause.
	     **/
	    recent = NULL;
	    for(i=0;i<qs->Children.nItems;i++)
		{
		item = (pQueryStructure)(qs->Children.Items[i]);
		xaAddItem(&qe->AttrNames, (void*)item->Presentation);
		xaAddItem(&qe->AttrExprPtr, (void*)item->RawData.String);
		xaAddItem(&qe->AttrCompiledExpr, (void*)item->Expr);
		if (item->Expr && item->Expr->AggLevel > md->AggLevel) md->AggLevel = item->Expr->AggLevel;
		if (item->QELinkage)
		    {
		    /*if (item->QELinkage != recent)
			{
			if (xaFindItem(&qe->Children, (void*)item->QELinkage) < 0)
			    xaAddItem(&qe->Children, (void*)item->QELinkage);
			}
		    recent = item->QELinkage;*/
		    xaAddItem(&qe->AttrDeriv, (void*)item->QELinkage);
		    }
		else
		    {
		    /** No linkage but we can't handle it??? **/
		    if (!(item->Flags & MQ_SF_ASTERISK) && item->ObjCnt > 0 && !(item->Expr->ObjCoverageMask & EXPR_MASK_INDETERMINATE))
			{
			nmFree(qe->PrivateData,sizeof(MQTData));
			nmFree(qe,sizeof(QueryElement));
			mssError(1,"MQT","Bark! Unhandled SELECT item in query - aborting");
			return -1;
			}

		    /** Is it a list?  If so, make a note of it. **/
		    if (md->nListItems >= MQT_MAX_OBJECTS)
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

	    /** Don't do certain things if we're layering in an UPDATE query **/
	    if (qs->NodeType == MQ_T_SELECTCLAUSE)
		{
		/** Does this have a set rowcount? Store it in SlaveIterCnt. **/
		for(i=0;i<qs->Parent->Children.nItems;i++)
		    {
		    item = (pQueryStructure)(qs->Parent->Children.Items[i]);
		    if (item->NodeType == MQ_T_SETOPTION && !strcmp(item->Name,"rowcount"))
			{
			qe->SlaveIterCnt = strtoi(item->Source, NULL, 10);
			break;
			}
		    }

		/** No items linked via SELECT items, but a stmt->Tree is set? **/
		if (qe->Children.nItems == 0 && stmt->Tree != NULL)
		    {
		    xaAddItem(&qe->Children, (void*)(stmt->Tree));
		    stmt->Tree->Parent = qe;
		    }

		/** Find the WHERE items that didn't specifically match anything else **/
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
	    xaAddItem(&stmt->Trees, qe);
	    stmt->Tree = qe;
	    }

    return 0;
    }


int
mqt_internal_ResetAggregates(pQueryStatement stmt, pQueryElement qe, int level)
    {
    int i;
    pExpression exp;

	/** Reset the SELECT item expressions **/
	for(i=0;i<qe->AttrCompiledExpr.nItems;i++) 
	    {
	    exp = (pExpression)(qe->AttrCompiledExpr.Items[i]);
	    if (exp) expResetAggregates(exp, -1, level);
	    }

	/** Reset any ORDER BY expressions in the parent **/
	if (qe->Parent)
	    {
	    for(i=0;i<24;i++)
		if (qe->Parent->OrderBy[i])
		    expResetAggregates(qe->Parent->OrderBy[i], -1, level);
		else
		    break;
	    }
    
	/** Reset HAVING clause **/
	if (stmt->HavingClause)
	    {
	    expResetAggregates(stmt->HavingClause, -1, level);
	    /*id = -1;
	    if (expLookupParam(objlist, "this") < 0)
		id = expAddParamToList(objlist, "this", NULL, 0);
	    expUnlockAggregates(stmt->HavingClause, level);
	    expEvalTree(stmt->HavingClause, objlist);
	    if (id >= 0)
		expRemoveParamFromList(objlist, "this");*/
	    }

    return 0;
    }


int
mqt_internal_UpdateAggregates(pQueryStatement stmt, pQueryElement qe, int level, pParamObjects objlist)
    {
    int i, id;
    pExpression exp;

	/** Update our SELECT item expressions **/
	for(i=0;i<qe->AttrCompiledExpr.nItems;i++)
	    {
	    exp = (pExpression)(qe->AttrCompiledExpr.Items[i]);
	    //if (exp /* && (exp->AggLevel != 0 || qe->AttrDeriv.Items[i] != NULL) */ )
	    if (exp && (exp->AggLevel != 0 || qe->AttrDeriv.Items[i] != NULL))
		{
		expUnlockAggregates(exp, level);
		expEvalTree(exp, objlist);
		}
	    }

	/** Update any ORDER BY expressions in the parent **/
	if (qe->Parent)
	    {
	    for(i=0;i<24;i++)
		if (qe->Parent->OrderBy[i])
		    {
		    expUnlockAggregates(qe->Parent->OrderBy[i], level);
		    expEvalTree(qe->Parent->OrderBy[i], objlist);
		    }
		else
		    break;
	    }

	/** Update HAVING clause **/
	if (stmt->HavingClause)
	    {
	    id = -1;
	    if (expLookupParam(objlist, "this", 0) < 0)
		id = expAddParamToList(objlist, "this", NULL, 0);
	    expUnlockAggregates(stmt->HavingClause, level);
	    expEvalTree(stmt->HavingClause, objlist);
	    if (id >= 0)
		expRemoveParamFromList(objlist, "this");
	    }

    return 0;
    }


/*** mqtStart - starts the query operation for a given tabular query
 *** element.  This evaluates the constant expressions and gets them
 *** ready for retrieval.
 ***/
int
mqtStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    int i;
    pQueryElement cld;
    pMQTData md = (pMQTData)(qe->PrivateData);

    	/** First, evaluate all of the attributes that we 'own' **/
	for(i=0;i<qe->AttrNames.nItems;i++) if (qe->AttrDeriv.Items[i] == NULL && qe->AttrCompiledExpr.Items[i] && ((pExpression)(qe->AttrCompiledExpr.Items[i]))->AggLevel == 0)
	    {
	    if (expEvalTree((pExpression)qe->AttrCompiledExpr.Items[i], stmt->Query->ObjList) < 0) 
	        {
		mssError(0,"MQT","Could not evaluate SELECT item's value");
		return -1;
		}
	    }

	/** Clear aggregates - level 2 **/
	mqt_internal_ResetAggregates(stmt, qe, 2);

	/** Now, 'trickle down' the Start operation to the child item(s). **/
	for(i=0;i<qe->Children.nItems;i++)
	    {
	    cld = (pQueryElement)(qe->Children.Items[i]);
	    if (cld->Driver->Start(cld, stmt, NULL) < 0) 
	        {
		mssError(0,"MQT","Failed to start child join/projection operation");
		return -1;
		}
	    }

	/** Set iteration cnt to 0 **/
	qe->IterCnt = 0;
	md->BypassChecked = 0;

    return 0;
    }


/*** mqt_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqt_internal_CheckConstraint(pQueryElement qe, pQueryStatement stmt)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, stmt->Query->ObjList);
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


/*** mqt_internal_SwapObjList() - exchange the active query objects in the objlist
 *** with those in the saved list, optionally closing the objects that are placed
 *** into the saved list.
 ***/
int
mqt_internal_SwapObjList(pMQTData md, pQueryStatement stmt, int do_close)
    {
    pObject tmp_obj;
    int i;

	for(i=stmt->Query->nProvidedObjects;i<md->nObjects;i++)
	    {
	    tmp_obj = md->SavedObjList[i];
	    md->SavedObjList[i] = stmt->Query->ObjList->Objects[i];
	    stmt->Query->ObjList->Objects[i] = tmp_obj;
	    if (do_close && md->SavedObjList[i])
		{
		objClose(md->SavedObjList[i]);
		md->SavedObjList[i] = NULL;
		}
	    }

    return 0;
    }


/*** mqt_internal_NextChildItem - retrieve the next child item underlying
 *** this query.
 ***/
int
mqt_internal_NextChildItem(pQueryElement parent, pQueryElement child, pQueryStatement stmt)
    {
    int rval;
    int ck;
    pMQTData md = (pMQTData)(parent->PrivateData);

	/** If our constraint is false and depends only on provided objects, we're done now. **/
	if (md->BypassChecked == 0 && parent->Constraint && parent->Constraint->ObjCoverageMask == (parent->Constraint->ObjCoverageMask & stmt->Query->ProvidedObjMask))
	    {
	    md->BypassChecked = 1;
	    ck = mqt_internal_CheckConstraint(parent, stmt);
	    if (ck == 0)
		return 0;
	    }

	rval = child->Driver->NextItem(child, stmt);

	//mqt_internal_UpdateAggregates(stmt, parent, 0, stmt->Query->ObjList);

    return rval;
    }


/*** mqtNextItem - retrieves the first/next item in the result set for the
 *** tablular query.  This driver only runs its own loop once through, but
 *** within that one iteration may be multiple row result sets handled by
 *** lower-level drivers, like a projection or join operation.
 ***/
int
mqtNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement cld;
    int rval;
    int fetch_rval = 0;
    int ck;
    int i;
    pMQTData md = (pMQTData)(qe->PrivateData);
    unsigned char* bptr;

    	/** Check the setrowcount... **/
	if (qe->SlaveIterCnt > 0 && qe->IterCnt >= qe->SlaveIterCnt) return 0;

    	/** Pass the NextItem on to the child, otherwise just 1 row. **/
	cld = (pQueryElement)(qe->Children.Items[0]);
	qe->IterCnt++;

	/** Check to see if we're doing group-by **/
	if (md->nGroupByItems == 0 && md->AggLevel == 0)
	    {
	    /** Next, retrieve until end or until end of group **/
	    if (qe->Children.nItems > 0 && cld)
	        {
	        while(1)
	            {
		    rval = mqt_internal_NextChildItem(qe, cld, stmt);
		    if (rval <= 0) break;
		    ck = mqt_internal_CheckConstraint(qe, stmt);
		    if (ck < 0) return ck;
		    if (ck == 1) break;
		    }
		}
	    else
	        {
		ck = mqt_internal_CheckConstraint(qe, stmt);
		if (ck < 0) return ck;
		if (ck == 0) return 0;
	        rval = (qe->IterCnt==1)?1:0;
		}
	    return rval;
	    }
	else
	    {
	    /** Then, reset all aggregate counters/sums/etc - level 1 **/
	    mqt_internal_ResetAggregates(stmt, qe, 1);

	    /** Restore a saved object list? **/
	    if (md->nObjects != 0 && qe->IterCnt > 1)
	        {
		mqt_internal_SwapObjList(md, stmt, 1 /* close */);
		expAllObjChanged(stmt->Query->ObjList);
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
	        if (qe->Children.nItems > 0 && cld)
		    {
	            while(1)
	                {
			rval = mqt_internal_NextChildItem(qe, cld, stmt);
			if (rval <= 0) break;
		        ck = mqt_internal_CheckConstraint(qe, stmt);
		        if (ck < 0) return ck;
		        if (ck == 1) break;
		        }
		    }
	        else
		    {
	            rval = (qe->IterCnt==1)?1:0;
		    ck = mqt_internal_CheckConstraint(qe, stmt);
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
		mqt_internal_CheckGroupBy(qe, stmt, md, &(md->GroupByPtr));
		}

	    /** This is the loop for a query with group-by and two-level aggregates **/
	    while(1)
		{
		/** This is the group-by loop. **/
		while(1)
		    {
		    /** Update all aggregate counters - level 1 **/
		    mqt_internal_UpdateAggregates(stmt, qe, 1, stmt->Query->ObjList);

		    /** Link to all objects in the current object list **/
		    memcpy(md->SavedObjList + stmt->Query->nProvidedObjects, stmt->Query->ObjList->Objects + stmt->Query->nProvidedObjects, (stmt->Query->ObjList->nObjects - stmt->Query->nProvidedObjects)*sizeof(pObject));
		    md->nObjects = stmt->Query->ObjList->nObjects;
		    for(i=stmt->Query->nProvidedObjects;i<md->nObjects;i++) 
			if (md->SavedObjList[i]) 
			    objLinkTo(md->SavedObjList[i]);

		    /** Next, retrieve until end or until end of group **/
		    if (qe->Children.nItems > 0 && cld)
			{
			while(1)
			    {
			    rval = mqt_internal_NextChildItem(qe, cld, stmt);
			    if (rval <= 0) break;
			    ck = mqt_internal_CheckConstraint(qe, stmt);
			    if (ck < 0) return ck;
			    if (ck == 1) break;
			    }
			}
		    else
			{
			rval = (qe->IterCnt==1)?1:0;
			ck = mqt_internal_CheckConstraint(qe, stmt);
			if (ck < 0) return ck;
			if (ck == 0) rval = 0;
			}

		    /** Last one? **/
		    if (rval == 0) 
			{
			mqt_internal_SwapObjList(md, stmt, 0 /* no close */);
			expAllObjChanged(stmt->Query->ObjList);
			md->IsLastRow = 1;
			fetch_rval = 1;
			break;
			}

		    /** Is this a group-end?  Return now if so. **/
		    if (mqt_internal_CheckGroupBy(qe, stmt, md, &bptr) == 1 || qe->Children.nItems == 0 || !cld)
			{
			/** Restore saved objects **/
			mqt_internal_SwapObjList(md, stmt, 0 /* no close */);
			expAllObjChanged(stmt->Query->ObjList);
			md->GroupByPtr = bptr;
			if (qe->Children.nItems == 0 || !cld) md->IsLastRow = 1;
			fetch_rval = 1;
			break;
			}
		    else
			{
			/** Not end of group, unlink our saved objects and go around again **/
			for(i=stmt->Query->nProvidedObjects;i<md->nObjects;i++)
			    {
			    if (md->SavedObjList[i]) 
				{
				objClose(md->SavedObjList[i]);
				md->SavedObjList[i] = NULL;
				}
			    }
			md->nObjects = 0;
			}
		    }

		/** One-level grouping?  Return now if so. **/
		if (md->AggLevel < 2)
		    break;

		/** Got a row with 2-level grouping? **/
		if (fetch_rval == 1 && md->AggLevel == 2)
		    {
		    /** Re-eval second level group **/
		    mqt_internal_UpdateAggregates(stmt, qe, 2, stmt->Query->ObjList);
		    }

		/** 2-level group and this is the last row?  If so, return. **/
		if (md->AggLevel == 2 && fetch_rval == 1 && md->IsLastRow)
		    break;

		/** Reset all aggregate counters/sums/etc - level 1 **/
		mqt_internal_ResetAggregates(stmt, qe, 1);

		/** Force recalc **/
		if (md->nObjects != 0)
		    {
		    mqt_internal_SwapObjList(md, stmt, 1 /* close */);
		    expAllObjChanged(stmt->Query->ObjList);
		    md->nObjects = 0;
		    }
		}
	    }

    return fetch_rval;
    }


/*** mqtFinish - ends the tabular data operation and frees up any memory used
 *** in the process of running the "query".
 ***/
int
mqtFinish(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement cld;
    int i;
    pMQTData md = (pMQTData)(qe->PrivateData);

	/** Restore a saved object list? **/
	if (md->nObjects != 0 && qe->IterCnt > 0)
	    {
	    mqt_internal_SwapObjList(md, stmt, 1 /* close */);
	    expAllObjChanged(stmt->Query->ObjList);
	    md->nObjects = 0;
	    }

#if 00
	/** Unlink saved objects, if query got interrupted **/
	for(i=0;i<md->nObjects;i++)
	    {
	    if (md->SavedObjList[i] && i >= stmt->Query->nProvidedObjects) 
		{
		objClose(md->SavedObjList[i]);
		md->SavedObjList[i] = NULL;
		}
	    md->nObjects = 0;
	    }
#endif

    	/** Trickle down the Finish to the child objects **/
	for(i=0;i<qe->Children.nItems;i++)
	    {
	    cld = (pQueryElement)(qe->Children.Items[i]);
	    if (cld->Driver->Finish(cld, stmt) < 0) return -1;
	    }

    return 0;
    }


/*** mqtRelease - release the private data structure MQTData allocated
 *** during the Analyze phase.
 ***/
int
mqtRelease(pQueryElement qe, pQueryStatement stmt)
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

	nmRegister(sizeof(MQTData), "MQTData");

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
