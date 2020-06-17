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
/* Module: 	multiq_delete.c  	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 11, 2008					*/
/* Description:	Provides support for DELETE statements.			*/
/************************************************************************/



struct
    {
    pQueryDriver	ThisDriver;
    }
    MQDINF;


/** keeping track of an object to delete **/
typedef struct
    {
    char	Name[256];
    pObject	Obj;
    }
    MqdDeletable, *pMqdDeletable;


/*** mqdAnalyze - take a given query syntax structure, locate the DELETE
 *** clause, and if there is one, we're in business.
 ***/
int
mqdAnalyze(pQueryStatement stmt)
    {
    pQueryStructure qs = NULL, item, where_qs, where_item, from_qs;
    pQueryElement qe;
    pExpression new_exp;
    int i;
    int src_idx;

    	/** Search for a DELETE statement... **/
	if ((qs = mq_internal_FindItem(stmt->QTree, MQ_T_DELETECLAUSE, NULL)) != NULL)
	    {
	    /** Allocate a new query-element **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQDINF.ThisDriver;

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

	    /** Determine which object id is for deleting **/
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
		mssError(1, "MQD", "DELETE must have exactly one deletable source");
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
		mssError(1, "MQD", "DELETE clause must delete items from data source(s)");
		mq_internal_FreeQE(qe);
		return -1;
		}

	    /** Link the qe into the multiquery **/
	    xaAddItem(&stmt->Trees, qe);
	    stmt->Tree = qe;

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


/*** mqd_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqd_internal_CheckConstraint(pQueryElement qe, pQueryStatement stmt)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, stmt->Query->ObjList);
	    if (qe->Constraint->DataType != DATA_T_INTEGER)
	        {
	        mssError(1,"MQD","WHERE clause item must have a boolean/integer value.");
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


/*** mqdStart - for a delete query, much as for a insert query, we will
 *** run the entire operation within mqdStart() and return no rows for
 *** mqdNextItem.
 ***/
int
mqdStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    int i,j;
    pQueryElement cld;
    int rval = -1;
    int cld_rval;
    int is_started = 0;
    pXArray objects_to_delete = NULL;
    pXHashTable objects_hash = NULL;
    pMqdDeletable del;
    char* name;

	/** Now, 'trickle down' the Start operation to the child item(s). **/
	cld = (pQueryElement)(qe->Children.Items[0]);
	if (cld->Driver->Start(cld, stmt, NULL) < 0) 
	    {
	    mssError(0,"MQD","Failed to start child join/projection operation");
	    goto error;
	    }
	is_started = 1;

	/** Set iteration cnt to 0 **/
	qe->IterCnt = 0;

	objects_to_delete = (pXArray)nmMalloc(sizeof(XArray));
	if (!objects_to_delete) goto error;
	xaInit(objects_to_delete, 16);
	objects_hash = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (!objects_hash)
	    goto error;
	xhInit(objects_hash, 257, 0);

	/** Retrieve matching records **/
	while((!qe->SlaveIterCnt || qe->IterCnt < qe->SlaveIterCnt) && (cld_rval = cld->Driver->NextItem(cld, stmt)) == 1)
	    {
	    /** Does this row match the where clause criteria? **/
	    if (mqd_internal_CheckConstraint(qe, stmt) == 1)
		{
		/** Got a matching row, count it and delete it **/
		qe->IterCnt++;

		/** Save it for later deleting, if not already seen... **/
		objGetAttrValue(stmt->Query->ObjList->Objects[qe->SrcIndex], "name", DATA_T_STRING, POD(&name));
		del = (pMqdDeletable)nmMalloc(sizeof(MqdDeletable));
		if (!del) goto error;
		strtcpy(del->Name, name, sizeof(del->Name));
		if (xhAdd(objects_hash, del->Name, (void*)del) < 0)
		    {
		    nmFree(del, sizeof(MqdDeletable));
		    }
		else
		    {
		    del->Obj = objLinkTo(stmt->Query->ObjList->Objects[qe->SrcIndex]);
		    xaAddItem(objects_to_delete, (void*)del);
		    }
		}
	    }

	is_started = 0;
	if (cld->Driver->Finish(cld, stmt) < 0)
	    goto error;

	if (cld_rval < 0)
	    goto error;

	/** Delete the retrieved records **/
	expUnlinkParams(stmt->Query->ObjList, stmt->Query->nProvidedObjects, -1);
	for(j=objects_to_delete->nItems-1; j>=0; j--)
	    {
	    del = (pMqdDeletable)xaGetItem(objects_to_delete, j);

	    /** Delete it **/
	    objDeleteObj(del->Obj);
	    /*printf("Deleting %s\n", del->Name); objClose(del->Obj);*/
	    del->Obj = NULL;
	    }

	for(i=0;i<stmt->Query->ObjList->nObjects;i++)
	    {
	    if (stmt->Query->ObjList->Objects[i] && i >= stmt->Query->nProvidedObjects)
		stmt->Query->ObjList->Objects[i] = NULL;
	    }

	rval = 0;

    error:
	if (objects_to_delete)
	    {
	    for(i=0; i<objects_to_delete->nItems; i++)
		{
		del = (pMqdDeletable)xaGetItem(objects_to_delete, i);
		if (del)
		    {
		    if (del->Obj)
			{
			objClose(del->Obj);
			del->Obj = NULL;
			}
		    nmFree(del, sizeof(MqdDeletable));
		    }
		}
	    xaDeInit(objects_to_delete);
	    nmFree(objects_to_delete, sizeof(XArray));
	    objects_to_delete = NULL;
	    }

	if (objects_hash)
	    {
	    xhClear(objects_hash, NULL, NULL);
	    xhDeInit(objects_hash);
	    nmFree(objects_hash, sizeof(XHashTable));
	    objects_hash = NULL;
	    }

	/** Close the SELECT **/
	if (is_started)
	    if (cld->Driver->Finish(cld, stmt) < 0)
		return -1;

    return rval;
    }


/*** mqdNextItem - a delete query returns no rows.
 ***/
int
mqdNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqdFinish - clean up.
 ***/
int
mqdFinish(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqdRelease - does nothing for a delete statement.
 ***/
int
mqdRelease(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqdInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqdInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	nmRegister(sizeof(MqdDeletable), "MqdDeletable");

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQD - MultiQuery DELETE Statement Module");
	drv->Precedence = 5000;
	drv->Flags = 0;
	drv->Analyze = mqdAnalyze;
	drv->Start = mqdStart;
	drv->NextItem = mqdNextItem;
	drv->Finish = mqdFinish;
	drv->Release = mqdRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQDINF.ThisDriver = drv;

    return 0;
    }
