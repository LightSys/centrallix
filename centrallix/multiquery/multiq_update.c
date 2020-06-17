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
#include "cxlib/xarray.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2008 LightSys Technology Services, Inc.		*/
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
/* Module: 	multiq_update.c  	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 18, 2008   					*/
/* Description:	Provides support for UPDATE statements.			*/
/************************************************************************/



struct
    {
    pQueryDriver	ThisDriver;
    }
    MQUINF;


/*** mquAnalyze - take a given query syntax structure, locate the UPDATE
 *** clause, and if there is one, we're in business.
 ***/
int
mquAnalyze(pQueryStatement stmt)
    {
    pQueryStructure qs = NULL, item, where_qs, where_item, from_qs;
    pQueryElement qe,recent;
    pExpression new_exp;
    int i;
    int src_idx;

    	/** Search for an UPDATE statement... **/
	if ((qs = mq_internal_FindItem(stmt->QTree, MQ_T_UPDATECLAUSE, NULL)) != NULL)
	    {
	    /** Allocate a new query-element **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQUINF.ThisDriver;

	    /** Does this have a set rowcount? Store it in SlaveIterCnt. **/
	    qe->SlaveIterCnt = 0;
	    for(i=0;i<qs->Parent->Children.nItems;i++)
	        {
		item = (pQueryStructure)(qs->Parent->Children.Items[i]);
		if (item->NodeType == MQ_T_SETOPTION && !strcmp(item->Name,"rowcount"))
		    {
		    qe->SlaveIterCnt = strtoi(item->Source, NULL, 10);
		    break;
		    }
		}

	    /** Determine which object id is for updating **/
	    from_qs = mq_internal_FindItem(stmt->QTree, MQ_T_FROMCLAUSE, NULL);
	    src_idx = -1;
	    if (from_qs)
		{
		for(i=0;i<from_qs->Children.nItems;i++)
		    {
		    item = (pQueryStructure)(from_qs->Children.Items[i]);
		    if (from_qs->Children.nItems == 1 || (item->Flags & MQ_SF_IDENTITY))
			{
			src_idx = expLookupParam(stmt->Query->ObjList, 
					item->Presentation[0]?(item->Presentation):(item->Source),
					0);
			}
		    }
		}
	    if (src_idx == -1)
		{
		mssError(1, "MQU", "UPDATE must have exactly one updatable source");
		mq_internal_FreeQE(qe);
		return -1;
		}
	    qe->SrcIndex = src_idx;

	    /** mq->Tree is set? **/
	    if (stmt->Tree != NULL)
	        {
		xaAddItem(&qe->Children, (void*)(stmt->Tree));
		stmt->Tree->Parent = qe;
		}
	    else
		{
		mssError(1, "MQU", "UPDATE clause must update items from data source");
		mq_internal_FreeQE(qe);
		return -1;
		}

	    /** Link the qe into the multiquery **/
	    xaAddItem(&stmt->Trees, qe);
	    stmt->Tree = qe;

	    /** Need to link in with each of the update-items. **/
	    recent = NULL;
	    for(i=0;i<qs->Children.nItems;i++)
	        {
		item = (pQueryStructure)(qs->Children.Items[i]);
		xaAddItem(&qe->AttrNames, (void*)item->Name);
		xaAddItem(&qe->AttrExprPtr, (void*)item->RawData.String);
		xaAddItem(&qe->AttrCompiledExpr, (void*)item->Expr);
		xaAddItem(&qe->AttrAssignExpr, (void*)item->AssignExpr);

		/** Ok, grab this one... **/
		xaAddItem(&qe->AttrDeriv, (void*)NULL);
		item->QELinkage = qe;
		}

	    /** Find the WHERE items that didn't specifically match anything else **/
	    qe->Constraint = NULL;
	    where_qs = mq_internal_FindItem(qs->Parent, MQ_T_WHERECLAUSE, NULL);
	    if (where_qs)
	        {
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);

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

    return 0;
    }


/*** mqu_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqu_internal_CheckConstraint(pQueryElement qe, pQueryStatement stmt)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, stmt->Query->ObjList);
	    if (qe->Constraint->DataType != DATA_T_INTEGER)
	        {
	        mssError(1,"MQU","WHERE clause item must have a boolean/integer value.");
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


/*** mquStart - for an update query, much as for a insert query, we will
 *** run the entire operation within mquStart() and return no rows for
 *** mquNextItem.
 ***/
int
mquStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    int i,j;
    pQueryElement cld;
    pExpression exp, assign_exp;
    int rval = -1;
    int cld_rval;
    int is_started = 0;
    int t;
    pXArray objects_to_update = NULL;
    pParamObjects objlist;
    /*ObjData od;*/

	/** Now, 'trickle down' the Start operation to the child item(s). **/
	cld = (pQueryElement)(qe->Children.Items[0]);
	if (cld->Driver->Start(cld, stmt, NULL) < 0) 
	    {
	    mssError(0,"MQU","Failed to start child join/projection operation");
	    goto error;
	    }
	is_started = 1;

	/** Set iteration cnt to 0 **/
	qe->IterCnt = 0;

	objects_to_update = xaNew(16);
	if (!objects_to_update) goto error;

	/** Retrieve matching records **/
	while((!qe->SlaveIterCnt || qe->IterCnt < qe->SlaveIterCnt) && (cld_rval = cld->Driver->NextItem(cld, stmt)) == 1)
	    {
	    /** Does this row match the where clause criteria? **/
	    if (mqu_internal_CheckConstraint(qe, stmt) == 1)
		{
		/** Got a matching row, count it and update it **/
		qe->IterCnt++;

		/** Save it for later **/
		objlist = expCreateParamList();
		if (!objlist) goto error;
		expCopyList(stmt->Query->ObjList, objlist, -1);
		expLinkParams(objlist, stmt->Query->nProvidedObjects, -1);
		xaAddItem(objects_to_update, (void*)objlist);
		}
	    }

	is_started = 0;
	if (cld->Driver->Finish(cld, stmt) < 0)
	    goto error;

	if (cld_rval < 0)
	    goto error;

	/** Update the retrieved records **/
	expUnlinkParams(stmt->Query->ObjList, stmt->Query->nProvidedObjects, -1);
	for(j=0;j<objects_to_update->nItems;j++)
	    {
	    objlist = (pParamObjects)xaGetItem(objects_to_update, j);
	    expCopyList(objlist, stmt->Query->ObjList, -1);
	    //expCopyParams(objlist, stmt->Query->ObjList, stmt->Query->nProvidedObjects, -1);

	    /** Loop through list of values to set **/
	    for(i=0;i<qe->AttrNames.nItems;i++)
		{
		if (qe->AttrDeriv.Items[i] == NULL && ((pExpression)(qe->AttrCompiledExpr.Items[i]))->AggLevel == 0)
		    {
		    exp = (pExpression)(qe->AttrCompiledExpr.Items[i]);
		    assign_exp = (pExpression)(qe->AttrAssignExpr.Items[i]);

		    /** Get the value to be assigned **/
		    if (expEvalTree(exp, stmt->Query->ObjList) < 0) 
			{
			mssError(0,"MQU","Could not evaluate UPDATE expression's value");
			goto error;
			}
		    t = exp->DataType;
		    if (t <= 0)
			{
			mssError(1,"MQU","Could not evaluate UPDATE expression's value");
			goto error;
			}

		    /** Set the attribute on the object **/
		    if (expCopyValue(exp, assign_exp, 1) < 0)
			{
			mssError(1,"MQU","Could not handle UPDATE expression's value");
			goto error;
			}
		    if (expReverseEvalTree(assign_exp, stmt->Query->ObjList) < 0)
			goto error;
		    }
		}
	    }

	for(i=stmt->Query->nProvidedObjects;i<stmt->Query->ObjList->nObjects;i++)
	    expModifyParamByID(stmt->Query->ObjList, i, NULL);
	    /*stmt->Query->ObjList->Objects[i] = NULL;*/

	rval = 0;

    error:
	if (objects_to_update)
	    {
	    for(i=0;i<objects_to_update->nItems;i++)
		{
		objlist = (pParamObjects)xaGetItem(objects_to_update, i);
		if (objlist)
		    {
		    expUnlinkParams(objlist, stmt->Query->nProvidedObjects, -1);
		    expFreeParamList(objlist);
		    }
		}
	    xaFree(objects_to_update);
	    }

	/** Close the SELECT **/
	if (is_started)
	    if (cld->Driver->Finish(cld, stmt) < 0)
		return -1;

    return rval;
    }


/*** mquNextItem - an update query returns no rows.
 ***/
int
mquNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mquFinish - clean up.
 ***/
int
mquFinish(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mquRelease - does nothing for an update statement.
 ***/
int
mquRelease(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mquInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mquInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQU - MultiQuery UPDATE Statement Module");
	drv->Precedence = 5000;
	drv->Flags = 0;
	drv->Analyze = mquAnalyze;
	drv->Start = mquStart;
	drv->NextItem = mquNextItem;
	drv->Finish = mquFinish;
	drv->Release = mquRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQUINF.ThisDriver = drv;

    return 0;
    }
