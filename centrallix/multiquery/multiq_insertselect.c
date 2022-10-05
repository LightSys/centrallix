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
#include "cxlib/strtcpy.h"


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
/* Module: 	multiq_insertselect.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 13, 2008   					*/
/* Description:	Provides support for a INSERT INTO ... SELECT construct	*/
/************************************************************************/



struct
    {
    pQueryDriver	ThisDriver;
    }
    MQISINF;


/*** mqisAnalyze - take a given query syntax structure and see if it has
 *** both an INSERT and a SELECT clause.  If so, jump in there and do our
 *** thing.
 ***/
int
mqisAnalyze(pQueryStatement stmt)
    {
    pQueryStructure select_qs, insert_qs;
    pQueryElement qe;
    int i;

    	/** Search for an INSERT and a SELECT statement... **/
	select_qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	insert_qs = mq_internal_FindItem(stmt->QTree, MQ_T_INSERTCLAUSE, NULL);

	/** We get to sit on the bench this time? **/
	if (!select_qs || !insert_qs)
	    return 0;

	/** Bad news if other drivers haven't found a SELECT tree **/
	if (!stmt->Tree)
	    return -1;

	/** Alloc a queryexec node **/
	qe = mq_internal_AllocQE();
	qe->Driver = MQISINF.ThisDriver;

	/** link in with insert clause **/
	insert_qs->QELinkage = qe;
	qe->QSLinkage = insert_qs;

	/** Link with select items **/
	for(i=0;i<stmt->Tree->AttrNames.nItems;i++)
	    {
	    xaAddItem(&qe->AttrNames, stmt->Tree->AttrNames.Items[i]);
	    xaAddItem(&qe->AttrExprPtr, stmt->Tree->AttrExprPtr.Items[i]);
	    xaAddItem(&qe->AttrCompiledExpr, stmt->Tree->AttrCompiledExpr.Items[i]);
	    }

	/** Link the qe into the multiquery **/
	xaAddItem(&stmt->Trees, qe);
	xaAddItem(&qe->Children, stmt->Tree);
	stmt->Tree->Parent = qe;
	stmt->Tree = qe;

    return 0;
    }


/*** mqisStart - starts the query operation for the insert.  This actually
 *** does all of the work of iterating the selected rows and doing the
 *** resulting inserts.  Fetch/NextItem always just returns NULL.
 ***/
int
mqisStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    pQueryElement sel;
    int rval = -1;
    int exp_rval;
    int sel_rval = 0;
    char pathname[OBJSYS_MAX_PATH];
    char new_pathname[OBJSYS_MAX_PATH];
    char* new_objname;
    pObject new_obj = NULL;
    pObject reopen_obj;
    pObject old_newobj;
    pObject parent_obj = NULL;
    int old_newobj_id;
    int is_started = 0;
    int attrid, astobjid;
    char* attrname;
    ObjData od;
    int t;
    int use_attrid;
    int hc_rval;
    handle_t collection;

	/** Prepare for the inserts **/
	if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_FROMOBJECT)
	    {
	    strtcpy(pathname, ((pQueryStructure)qe->QSLinkage)->Source, sizeof(pathname));
	    }
	else
	    {
	    if (strlen(((pQueryStructure)qe->QSLinkage)->Source) + 2 + 1 > sizeof(pathname))
		{
		mssError(1, "MQIS", "Pathname too long for INSERT destination");
		goto error;
		}
	    snprintf(pathname, sizeof(pathname), "%s/*", ((pQueryStructure)qe->QSLinkage)->Source);
	    }
    
	/** Replace the previous __inserted object with NULL, in case insert fails **/
	old_newobj_id = expLookupParam(stmt->Query->ObjList, "__inserted", 0);
	if (old_newobj_id >= 0)
	    {
	    old_newobj = stmt->Query->ObjList->Objects[old_newobj_id];
	    if (old_newobj)
		objClose(old_newobj);
	    expModifyParam(stmt->Query->ObjList, "__inserted", NULL);
	    }

	/** Start the SELECT query **/
	sel = (pQueryElement)(qe->Children.Items[0]);
	if (sel->Driver->Start(sel, stmt, NULL) < 0)
	    goto error;
	is_started = 1;

	/** If we're working with a collection, open its parent so we can
	 ** later do objOpenChild() calls to create the child objects.
	 **/
	if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_COLLECTION)
	    {
	    collection = mq_internal_FindCollection(stmt->Query, ((pQueryStructure)qe->QSLinkage)->Source);
	    if (collection == XHN_INVALID_HANDLE)
		{
		mssError(1,"MQIS","Could not find destination collection '%s' for SQL insert", ((pQueryStructure)qe->QSLinkage)->Source);
		goto error;
		}
	    parent_obj = objOpenTempObject(stmt->Query->SessionID, collection, OBJ_O_RDONLY);
	    if (!parent_obj)
		{
		mssError(1,"MQIS","Could not open destination collection '%s' for SQL insert", ((pQueryStructure)qe->QSLinkage)->Source);
		goto error;
		}
	    }

	/** Select all items in the result set **/
	while((sel_rval = sel->Driver->NextItem(sel, stmt)) == 1)
	    {
	    /** check HAVING clause **/
	    hc_rval = mq_internal_EvalHavingClause(stmt, NULL);
	    if (hc_rval < 0)
		goto error;
	    else if (hc_rval == 0)
		continue;

	    /** open a new object **/
	    if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_COLLECTION)
		{
		new_obj = objOpenChild(parent_obj, "*", OBJ_O_RDWR | OBJ_O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");
		}
	    else
		{
		new_obj = objOpen(stmt->Query->SessionID, pathname, OBJ_O_RDWR | OBJ_O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");
		}
	    if (!new_obj)
		goto error;
	    objUnmanageObject(stmt->Query->SessionID, new_obj);
	   
	    /** Set all of the SELECTed attributes **/
	    attrid = 0;
	    astobjid = -1;
	    while(1)
		{
		use_attrid = attrid;
		attrname = mq_internal_QEGetNextAttr(stmt->Query, sel, stmt->Query->ObjList, &attrid, &astobjid);
		if (!attrname) break;
		if (astobjid >= 0)
		    {
		    /** attr available direct from object via SELECT * **/
		    t = objGetAttrType(stmt->Query->ObjList->Objects[astobjid], attrname);
		    if (t <= 0)
			continue;
		    if (objGetAttrValue(stmt->Query->ObjList->Objects[astobjid], attrname, t, &od) != 0)
			continue;
		    }
		else
		    {
		    /** attr available through SELECT item list **/
		    if (expEvalTree((pExpression)sel->AttrCompiledExpr.Items[use_attrid], stmt->Query->ObjList) < 0)
			goto error;
		    t = ((pExpression)sel->AttrCompiledExpr.Items[use_attrid])->DataType;
		    if (t <= 0)
			continue;
		    exp_rval = expExpressionToPod((pExpression)(sel->AttrCompiledExpr.Items[use_attrid]), t, &od);
		    if (exp_rval < 0)
			goto error;
		    else if (exp_rval == 1) /* null */
			continue;
		    }

		/** Set the attribute on the newly inserted object **/
		if (objSetAttrValue(new_obj, attrname, t, &od) < 0)
		    goto error;
		}

	    /** Commit and get new object name **/
	    objCommitObject(new_obj);
	    if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_FROMOBJECT)
		{
		strtcpy(new_pathname, pathname, sizeof(new_pathname));
		}
	    else
		{
		new_objname = NULL;
		objGetAttrValue(new_obj, "name", DATA_T_STRING, POD(&new_objname));
		if (!new_objname)
		    {
		    mssError(0, "MQIS", "Could not INSERT new object");
		    goto error;
		    }
		if (strlen(((pQueryStructure)qe->QSLinkage)->Source) + 1 + strlen(new_objname) + 1 > sizeof(new_pathname))
		    {
		    mssError(1, "MQIS", "Pathname too long for newly INSERTed object %s", new_objname);
		    goto error;
		    }
		snprintf(new_pathname, sizeof(new_pathname), "%s/%s", ((pQueryStructure)qe->QSLinkage)->Source, new_objname);
		}

	    /** Link the new object as the __inserted object in the object list.**/
	    if (!(stmt->Query->Flags & MQ_F_NOINSERTED))
		{
		/** Don't reopen for inserts into collections (won't find it) **/
		if (!(((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_COLLECTION))
		    {
		    reopen_obj = objOpen(stmt->Query->SessionID, new_pathname, OBJ_O_RDONLY, 0600, "system/object");
		    if (reopen_obj)
			{
			objUnmanageObject(stmt->Query->SessionID, reopen_obj);
			objClose(new_obj);
			new_obj = reopen_obj;
			ASSERTMAGIC(new_obj, MGK_OBJECT);
			}
		    }

		/** Replace the previous __inserted object with our newly created one **/
		old_newobj_id = expLookupParam(stmt->Query->ObjList, "__inserted", 0);
		if (old_newobj_id >= 0)
		    {
		    old_newobj = stmt->Query->ObjList->Objects[old_newobj_id];
		    if (old_newobj)
			objClose(old_newobj);
		    expModifyParam(stmt->Query->ObjList, "__inserted", objLinkTo(new_obj));
		    }
		}

	    /** Close up and go on to next object to be inserted. **/
	    objClose(new_obj);
	    new_obj = NULL;

	    /** Yield, if necessary **/
	    mq_internal_CheckYield(stmt->Query);

	    /** Insert into object?  Only use the first row **/
	    if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_FROMOBJECT)
		break;
	    }

	if (sel_rval < 0)
	    goto error;

	rval = 0;

    error:
	if (parent_obj)
	    objClose(parent_obj);

	/** Close the SELECT **/
	if (is_started)
	    if (sel->Driver->Finish(sel, stmt) < 0)
		return -1;
	if (new_obj)
	    objClose(new_obj);

    return rval;
    }


/*** mqisNextItem - retrieves the first/next item.  This always returns
 *** 0 (end of results) because an insert does not generate a result set.
 ***/
int
mqisNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqisFinish - ends the operation.
 ***/
int
mqisFinish(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqisRelease - release any private data allocated (none)
 ***/
int
mqisRelease(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqisInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqisInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQIS - MultiQuery InsertSelect Data Module");
	drv->Precedence = 5000;
	drv->Flags = 0;
	drv->Analyze = mqisAnalyze;
	drv->Start = mqisStart;
	drv->NextItem = mqisNextItem;
	drv->Finish = mqisFinish;
	drv->Release = mqisRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQISINF.ThisDriver = drv;

    return 0;
    }
