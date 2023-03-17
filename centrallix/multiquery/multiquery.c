#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <syslog.h>
#include "ptod.h"
#include "obj.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "multiquery.h"
#include "cxlib/mtsession.h"
#include "application.h"
#include "cxlib/xhandle.h"
#include "centrallix.h"

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
/* Module: 	multiquery.h, multiquery.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 19, 1999					*/
/* Description:	Provides support for multiple-object queries (such as	*/
/*		join and union operations, and subqueries).  Used by	*/
/*		the objMultiQuery routine (obj_query.c).		*/
/************************************************************************/



/*** Globals ***/
struct
    {
    XArray		Drivers;
    }
    MQINF;

int mqSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData value, pObjTrxTree* oxt);
int mqGetAttrValue(void* inf_v, char* attrname, int datatype, void* value, pObjTrxTree* oxt);
int mqGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt);
int mq_internal_QueryClose(pMultiQuery qy, pObjTrxTree* oxt);


/*** mq_internal_CheckYield() - see if we need to do a thYield() to other
 *** threads due to a long-running query.
 ***/
void
mq_internal_CheckYield(pMultiQuery mq)
    {
    unsigned int ms;

	ms = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	if (ms - mq->StartMsec > 1000 && mq->YieldMsec == 0)
	    {
	    thYield();
	    mq->YieldMsec = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	    }
	else if (ms - mq->YieldMsec > 100)
	    {
	    thYield();
	    mq->YieldMsec = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	    }

    return;
    }


/*** mq_internal_FreeQS - releases memory used by the query syntax tree.
 ***/
int 
mq_internal_FreeQS(pQueryStructure qstree)
    {
    int i;

    	/** First, release memory held by child items **/
	for(i=0;i<qstree->Children.nItems;i++)
	    {
	    mq_internal_FreeQS((pQueryStructure)(qstree->Children.Items[i]));
	    }
	
	/** Release some memory held by structure items. **/
	xaDeInit(&qstree->Children);
	xsDeInit(&qstree->RawData);
	xsDeInit(&qstree->AssignRawData);
	if (qstree->Expr) expFreeExpression(qstree->Expr);
	if (qstree->AssignExpr) expFreeExpression(qstree->AssignExpr);

	/** Free this structure **/
	nmFree(qstree,sizeof(QueryStructure));

    return 0;
    }


/*** mq_internal_FreeQE - release a query exec tree structure...
 ***/
int
mq_internal_FreeQE(pQueryElement qetree)
    {
    int i;

    	/** Scan through child subtrees first... **/
	for(i=0;i<qetree->Children.nItems;i++)
	    {
	    mq_internal_FreeQE((pQueryElement)(qetree->Children.Items[i]));
	    }

	/** Tell the driver to release this **/
	qetree->Driver->Release(qetree, NULL);

	/** Release memory held by the arrays, etc **/
	xaDeInit(&qetree->Children);
	xaDeInit(&qetree->AttrNames);
	xaDeInit(&qetree->AttrDeriv);
	xaDeInit(&qetree->AttrExprPtr);
	xaDeInit(&qetree->AttrCompiledExpr);
	xaDeInit(&qetree->AttrAssignExpr);

	/** Release expression **/
	if (qetree->Constraint) expFreeExpression(qetree->Constraint);

	/** Release memory for the node itself **/
	nmFree(qetree, sizeof(QueryElement));

    return 0;
    }


/*** mq_internal_AllocQE - allocate a qe structure and initialize its fields
 ***/
pQueryElement
mq_internal_AllocQE()
    {
    pQueryElement qe;

    	/** Allocate and initialize the arrays, etc **/
	qe = (pQueryElement)nmMalloc(sizeof(QueryElement));
	if (!qe) return NULL;
	memset(qe, 0, sizeof(QueryElement));
	xaInit(&qe->Children,16);
	xaInit(&qe->AttrNames,16);
	xaInit(&qe->AttrDeriv,16);
	xaInit(&qe->AttrExprPtr,16);
	xaInit(&qe->AttrCompiledExpr,16);
	xaInit(&qe->AttrAssignExpr,16);
	qe->Parent = NULL;
	qe->Constraint = NULL;
	qe->Flags = 0;
	qe->PrivateData = NULL;
	qe->QSLinkage = NULL;

    return qe;
    }


/*** Syntax Parser States ***/
typedef enum
    {
    /** when waiting for main clause keyword like SELECT, FROM, WHERE **/
    LookForClause,

    /** when looking at a clause item **/
    SelectItem,
    FromItem,
    WhereItem,
    OrderByItem,
    GroupByItem,
    HavingItem,
    UpdateItem,
    OnDupItem,
    OnDupUpdateItem,

    /** Misc **/
    ParseDone,
    ParseError
    }
    ParserState;


/*** mq_internal_AllocQS - allocate a new qs with a given type.
 ***/
pQueryStructure
mq_internal_AllocQS(int type)
    {
    pQueryStructure this;

    	/** Allocate **/
	this = (pQueryStructure)nmMalloc(sizeof(QueryStructure));
	if (!this) return NULL;
	memset(this, 0, sizeof(QueryStructure));

	/** Fill in the structure **/
	this->NodeType = type;
	this->QELinkage = NULL;
	this->Expr = NULL;
	this->AssignExpr = NULL;
	this->Specificity = 0;
	this->Flags = 0;
	xaInit(&this->Children,16);
	xsInit(&this->RawData);
	xsInit(&this->AssignRawData);

    return this;
    }


/*** mq_internal_SetChainedReferences - for order by and group by clauses, the
 *** field lists might reference the result set column names rather than the
 *** source object attributes.  Here, we set those up so that the REFERENCED
 *** flag reflects the actual underlying objects.
 ***/
int
mq_internal_SetChainedReferences(pQueryStructure qs, pQueryStructure clause)
    {
    /*pQueryStructure clause_item;
    int i,cnt,j;*/

    return 0;
    }


/*** mq_internal_ExprToPresentation - see if we can determine the name of this
 *** data field based on the expression tree - a top level object or property
 *** node can yield the name for us.  However, don't do this if the name is
 *** "objcontent" since that is a special reserved value.
 ***/
int
mq_internal_ExprToPresentation(pExpression exp, char* pres, int maxlen)
    {

	if (exp->NodeType == EXPR_N_OBJECT && strcmp(((pExpression)(exp->Children.Items[0]))->Name,"objcontent"))
	    {
	    strtcpy(pres, ((pExpression)(exp->Children.Items[0]))->Name, maxlen);
	    return 0;
	    }
	else if (exp->NodeType == EXPR_N_PROPERTY && strcmp(exp->Name,"objcontent"))
	    {
	    strtcpy(pres, exp->Name, maxlen);
	    return 0;
	    }

    return -1;
    }


/*** mq_internal_SetCoverage - walk the QE tree and determine what parts
 *** of the tree contain various object references.
 ***/
int
mq_internal_SetCoverage(pQueryStatement stmt, pQueryElement tree)
    {
    int i;
    pQueryElement subtree;

	tree->CoverageMask = 0;

	/** Determine dependencies on all subtrees **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pQueryElement)(tree->Children.Items[i]);
	    mq_internal_SetCoverage(stmt, subtree);
	    tree->CoverageMask |= subtree->CoverageMask;
	    }

	/** No children?  projection module - grab the src index **/
	if (tree->Children.nItems == 0)
	    tree->CoverageMask |= (1<<tree->SrcIndex);

    return 0;
    }


/*** mq_internal_SetDependencies - walk the QE tree and determine what the
 *** dependencies are for various projections and joins, as far as the 
 *** constraints go.  This is mainly for outerjoin purposes, so we know
 *** when we don't have to run the slave side of a join.
 ***/
int
mq_internal_SetDependencies(pQueryStatement stmt, pQueryElement tree, pQueryElement parent)
    {
    int i;
    pQueryElement subtree;

	return 0; /* now done by join analysis */

	/** Parent's dependency mask applies to this too **/
	if (parent)
	    tree->DependencyMask = parent->DependencyMask;
	else
	    tree->DependencyMask = 0;

	/** Determine dependencies based on constraint expressions **/
	if (tree->Constraint)
	    tree->DependencyMask |= tree->Constraint->ObjCoverageMask;

	/** Determine dependencies on all subtrees **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pQueryElement)(tree->Children.Items[i]);
	    mq_internal_SetDependencies(stmt, subtree, tree);
	    }

	/** Don't include *ourselves* in the dependency mask **/
	tree->DependencyMask &= ~tree->CoverageMask;

    return 0;
    }


/*** mq_qdo_GetAttrType() - get the type of a declared object's attribute.
 ***/
int
mq_qdo_GetAttrType(pQueryDeclaredObject qdo, char* attrname)
    {
    pStructInf attr;

	attr = stLookup(qdo->Data, attrname);
	if (!attr)
	    return DATA_T_UNAVAILABLE; /* null / unset */

    return stGetAttrType(attr, 0);
    }


/*** mq_qdo_GetAttrValue() - get the value of a declared object's attribute.
 ***/
int
mq_qdo_GetAttrValue(pQueryDeclaredObject qdo, char* attrname, int data_type, pObjData val)
    {
    pStructInf attr;

	attr = stLookup(qdo->Data, attrname);
	if (!attr)
	    return 1; /* null / unset */

    return stGetAttrValue(attr, data_type, val, 0);
    }


/*** mq_qdo_SetAttrValue() - does nothing at present.
 ***/
int
mq_qdo_SetAttrValue(pQueryDeclaredObject qdo, char* attrname, int data_type, pObjData val)
    {
    return -1;
    }


/*** mq_internal_AddDeclaredObject() - make a declared object available in the object list
 ***/
int
mq_internal_AddDeclaredObject(pMultiQuery query, pQueryDeclaredObject qdo)
    {

	/** Add to master object list too **/
	if (expLookupParam(query->ObjList, qdo->Name, 0) >= 0)
	    return -1;
	if (expAddParamToList(query->ObjList, qdo->Name, (void*)qdo, 0) >= 0)
	    {
	    expSetParamFunctions(query->ObjList, qdo->Name, mq_qdo_GetAttrType, mq_qdo_GetAttrValue, mq_qdo_SetAttrValue);
	    query->nProvidedObjects++;
	    query->ProvidedObjMask = (1<<(query->nProvidedObjects)) - 1;
	    }

    return 0;
    }


/*** mq_internal_PostProcess - performs some additional processing on the
 *** SELECT, FROM, ORDER-BY, and WHERE clauses after the initial parse has been
 *** completed.
 ***/
int
mq_internal_PostProcess(pQueryStatement stmt, pQueryStructure qs, pQueryStructure sel, pQueryStructure from, pQueryStructure where, 
		 	pQueryStructure ob, pQueryStructure gb, pQueryStructure ct, pQueryStructure up, pQueryStructure dc,
			pQueryStructure odc, pQueryStructure dec)
    {
    int i,j,cnt,n_assign,exists;
    pQueryStructure subtree;
    pQueryStructure having;
    char* ptr;
    int has_identity;
    pQueryDeclaredObject qdo;
    pQueryDeclaredCollection qdc;
    pQueryAppData appdata;

	/** Set up a declared object or collection? **/
	if (dec)
	    {
	    stmt->Flags |= MQ_TF_IMMEDIATE;

	    /** Lookup application scope data **/
	    appdata = appLookupAppData("MQ:appdata");

	    /** Object vs Collection **/
	    if (dec->Flags & MQ_SF_COLLECTION)
		{
		/** Check to see if it already exists **/
		exists = 0;
		for(i=0; i<appdata->DeclaredCollections.nItems; i++)
		    {
		    qdc = (pQueryDeclaredCollection)appdata->DeclaredCollections.Items[i];
		    if (!strcmp(qdc->Name, dec->Name))
			exists = 1;
		    }

		if (!exists)
		    {
		    /** New Collection **/
		    qdc = (pQueryDeclaredCollection)nmMalloc(sizeof(QueryDeclaredCollection));
		    if (qdc)
			{
			/** Create it **/
			strtcpy(qdc->Name, dec->Name, sizeof(qdc->Name));
			qdc->Collection = objCreateTempObject(stmt->Query->SessionID);
			if (qdc->Collection == XHN_INVALID_HANDLE)
			    {
			    mssError(1, "MQ", "DECLARE COLLECTION: could not allocate temporary collection '%s'", qdc->Name);
			    nmFree(qdc, sizeof(QueryDeclaredCollection));
			    return -1;
			    }

			/** Query vs Application scope **/
			if (dec->Flags & MQ_SF_APPSCOPE)
			    xaAddItem(&appdata->DeclaredCollections, (void*)qdc);
			else
			    xaAddItem(&stmt->Query->DeclaredCollections, (void*)qdc);
			}
		    }
		}
	    else
		{
		/** Check to see if the object already exists **/
		exists = 0;
		for(i=0; i<appdata->DeclaredObjects.nItems; i++)
		    {
		    qdo = (pQueryDeclaredObject)appdata->DeclaredObjects.Items[i];
		    if (!strcmp(qdo->Name, dec->Name))
			exists = 1;
		    }

		if (!exists)
		    {
		    /** New object **/
		    qdo = (pQueryDeclaredObject)nmMalloc(sizeof(QueryDeclaredObject));
		    if (qdo)
			{
			/** Create it **/
			strtcpy(qdo->Name, dec->Name, sizeof(qdo->Name));
			qdo->Data = stAllocInf();
			if (mq_internal_AddDeclaredObject(stmt->Query, qdo) < 0)
			    {
			    mssError(1, "MQ", "DECLARE OBJECT: '%s' already exists in query or query parameter", qdo->Name);
			    return -1;
			    }

			/** Query vs App scope **/
			if (appdata && (dec->Flags & MQ_SF_APPSCOPE))
			    xaAddItem(&appdata->DeclaredObjects, (void*)qdo);
			else
			    xaAddItem(&stmt->Query->DeclaredObjects, (void*)qdo);
			}
		    }
		}
	    }

    	/** First, build the object list from the FROM clause **/
	has_identity = 0;
	if (from)
	    {
	    for(i=0;i<from->Children.nItems;i++)
		{
		subtree = (pQueryStructure)(from->Children.Items[i]);
		if (subtree->Flags & MQ_SF_EXPRESSION && !subtree->Presentation[0])
		    snprintf(subtree->Presentation, sizeof(subtree->Presentation), "from_%d", i);
		if (subtree->Flags & MQ_SF_COLLECTION && !subtree->Presentation[0])
		    strtcpy(subtree->Presentation, subtree->Source, sizeof(subtree->Presentation));
		if (subtree->Presentation[0])
		    ptr = subtree->Presentation;
		else
		    ptr = subtree->Source;
		if (subtree->Flags & MQ_SF_IDENTITY)
		    has_identity = 1;
		if (expLookupParam(stmt->Query->ObjList, ptr, 0) >= 0)
		    {
		    mssError(1, "MQ", "Data source '%s' already exists in query or query parameter", ptr);
		    return -1;
		    }
		subtree->ObjID = expAddParamToList(stmt->Query->ObjList, ptr, NULL, (i==0 || (subtree->Flags & MQ_SF_IDENTITY))?EXPR_O_CURRENT:0);

		/** Compile FROM clause expression if needed **/
		if (subtree->Flags & MQ_SF_EXPRESSION)
		    {
		    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
		    if (!subtree->Expr) 
			{
			mssError(0,"MQ","Error in FROM list expression <%s>", subtree->RawData.String);
			return -1;
			}
		    }
		}
	    if (from->Children.nItems > 1 && !has_identity && up)
		{
		mssError(1, "MQ", "UPDATE statement with multiple data sources must have one marked as the IDENTITY source.");
		return -1;
		}
	    if (from->Children.nItems > 0 && !has_identity && dc)
		{
		/** Mark the first one as the identity source **/
		subtree = (pQueryStructure)(from->Children.Items[0]);
		subtree->Flags |= MQ_SF_IDENTITY;
		}
	    if (from->Children.nItems == 1 && !has_identity)
		{
		/** If only one source, it is the identity source **/
		subtree = (pQueryStructure)(from->Children.Items[0]);
		subtree->Flags |= MQ_SF_IDENTITY;
		}
	    }

	/** Ok, got object list.  Now compile the SELECT expressions **/
	n_assign = 0;
	if (sel) for(i=0;i<sel->Children.nItems;i++)
	    {
	    /** Compile the expression **/
	    subtree = (pQueryStructure)(sel->Children.Items[i]);
	    if (subtree->Flags & MQ_SF_ASSIGNMENT)
		{
		if (stmt->Query->Flags & MQ_F_NOUPDATE)
		    {
		    mssError(1, "MQ", "SELECT assignments cannot be used in this query context");
		    return -1;
		    }
		n_assign++;
		}
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    if (!strcmp(subtree->RawData.String,"*") || !strcmp(subtree->RawData.String,"* "))
		{
		subtree->Expr = NULL;
		subtree->ObjCnt = stmt->Query->ObjList->nObjects - stmt->Query->nProvidedObjects;
		strtcpy(subtree->Presentation, "*", sizeof(subtree->Presentation));
		sel->Flags |= MQ_SF_ASTERISK;
		subtree->Flags |= MQ_SF_ASTERISK;
		if (subtree->ObjCnt == 0)
		    {
		    mssError(0,"MQ","Cannot use 'SELECT *' without at least one 'FROM' data source", subtree->RawData.String);
		    return -1;
		    }
		for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++)
		    {
		    subtree->ObjFlags[j] = EXPR_O_REFERENCED;
		    }
		}
	    else
		{
		subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
		if (!subtree->Expr) 
		    {
		    mssError(0,"MQ","Error in SELECT list expression <%s>", subtree->RawData.String);
		    return -1;
		    }
		cnt = 0;
		for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
		    {
		    subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		    if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		    }
		subtree->ObjCnt = cnt;

		/** Determine if we need to assign it a generic name **/
		if (subtree->Presentation[0] == '\0')
		    {
		    if (mq_internal_ExprToPresentation(subtree->Expr, subtree->Presentation, sizeof(subtree->Presentation)) < 0)
			{
			snprintf(subtree->Presentation, sizeof(subtree->Presentation), "column_%3.3d", i);
			}
		    }
		else if (!strcmp(subtree->Presentation, "objcontent"))
		    {
		    /** Explicitly labeled "objcontent" **/
		    stmt->Flags |= MQ_TF_OBJCONTENT;
		    }
		}
	    }
	if (n_assign && sel && n_assign == sel->Children.nItems)
	    stmt->Flags |= MQ_TF_ALLASSIGN;
	if (n_assign)
	    stmt->Flags |= MQ_TF_ONEASSIGN;

	/** The on-duplicate clause items **/
	if (odc) for(i=0;i<odc->Children.nItems;i++)
	    {
	    /** Compile the on-duplicate clause expression **/
	    subtree = (pQueryStructure)(odc->Children.Items[i]);
	    if (subtree->NodeType != MQ_T_ONDUPITEM)
		continue;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->OneObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->Expr) 
		{
		mssError(0,"MQ","Error in ON DUPLICATE expression <%s>", subtree->RawData.String);
		return -1;
		}
	    }

	/** Compile the order by expressions **/
	if (ob) for(i=0;i<ob->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(ob->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in ORDER BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    for(j=0;j<stmt->Query->nProvidedObjects;j++)
		{
		if (expFreezeOne(subtree->Expr, stmt->Query->ObjList, j) < 0)
		    {
		    mssError(0, "MQ", "Error evaluating query's ORDER BY expression <%s>", subtree->RawData.String);
		    return -1;
		    }
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the group-by expressions **/
	if (gb) for(i=0;i<gb->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(gb->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in GROUP BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the update expressions **/
	if (up) for(i=0;i<up->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(up->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in UPDATE expression <%s>", subtree->RawData.String);
		return -1;
		}
	    subtree->AssignExpr = expCompileExpression(subtree->AssignRawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->AssignExpr)
	        {
		mssError(0,"MQ","Error in UPDATE assignment expression <%s>", subtree->AssignRawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the on-duplicate update expressions **/
	if (odc) for(i=0;i<odc->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(odc->Children.Items[i]);
	    if (subtree->NodeType != MQ_T_ONDUPUPDATEITEM)
		continue;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in UPDATE SET expression <%s>", subtree->RawData.String);
		return -1;
		}
	    subtree->AssignExpr = expCompileExpression(subtree->AssignRawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->AssignExpr)
	        {
		mssError(0,"MQ","Error in UPDATE SET assignment expression <%s>", subtree->AssignRawData.String);
		return -1;
		}
	    }

	/** Merge WHERE clauses if there are more than one **/
	cnt = xaCount(&qs->Children);
	where = NULL;
	for(i=0;i<cnt;i++)
	    {
	    subtree = (pQueryStructure)xaGetItem(&qs->Children, i);
	    if (subtree->NodeType == MQ_T_WHERECLAUSE)
		{
		if (!where)
		    {
		    where = subtree;
		    }
		else
		    {
		    /** merge **/
		    ptr = nmSysStrdup(where->RawData.String);
		    xsQPrintf(&where->RawData, "( %STR ) and ( %STR )", ptr, subtree->RawData.String);
		    nmSysFree(ptr);

		    /** remove the extra where clause **/
		    xaRemoveItem(&qs->Children,i);
		    i--;
		    cnt--;
		    mq_internal_FreeQS(subtree);
		    }
		}
	    }

	/** Merge HAVING clauses if there are more than one **/
	cnt = xaCount(&qs->Children);
	having = NULL;
	for(i=0;i<cnt;i++)
	    {
	    subtree = (pQueryStructure)xaGetItem(&qs->Children, i);
	    if (subtree->NodeType == MQ_T_HAVINGCLAUSE)
		{
		if (!having)
		    {
		    having = subtree;
		    }
		else
		    {
		    /** merge **/
		    ptr = nmSysStrdup(having->RawData.String);
		    xsQPrintf(&having->RawData, "( %STR ) and ( %STR )", ptr, subtree->RawData.String);
		    nmSysFree(ptr);

		    /** remove the extra having clause **/
		    xaRemoveItem(&qs->Children,i);
		    i--;
		    cnt--;
		    mq_internal_FreeQS(subtree);
		    }
		}
	    }

    return 0;
    }


/*** mq_internal_DetermineCoverage - determine which objects 'cover' which parts
 *** of the query's where clause, so the various drivers can intelligently apply
 *** filters...
 ***/
int
mq_internal_DetermineCoverage(pQueryStatement stmt, pExpression where_clause, pQueryStructure qs_where, pQueryStructure qs_select, pQueryStructure qs_update, int level)
    {
    int i,v;
    int sum_objmask = 0;
    int sum_outermask = 0;
    int is_covered = 1;
    int min_id;
    pExpression exp;
    pQueryStructure where_item = NULL;
    pQueryStructure select_item = NULL;
    char presentation[64];

	/** Check coverage mask for leaf nodes first **/
	if (qs_select && where_clause->NodeType == EXPR_N_PROPERTY && where_clause->ObjID == EXPR_OBJID_CURRENT)
	    {
	    /** If no object specified, make sure we have the obj id and thus coverage mask correct **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    min_id = 255;
	    for(i=0;i<qs_select->Children.nItems;i++)
	        {
		select_item = (pQueryStructure)(qs_select->Children.Items[i]);
		if (select_item->Expr && mq_internal_ExprToPresentation(select_item->Expr, presentation, sizeof(presentation)) == 0)
		    {
		    if (!strcmp(presentation, where_clause->Name) && select_item->Expr->ObjID < min_id)
			{
			sum_objmask = where_clause->ObjCoverageMask = select_item->Expr->ObjCoverageMask;
			min_id = where_clause->ObjID = select_item->Expr->ObjID;
			}
		    }
		}
	    }
	else if (qs_update && where_clause->NodeType == EXPR_N_PROPERTY && where_clause->ObjID == EXPR_OBJID_CURRENT)
	    {
	    /** Just leave it alone for now...  we may need to change
	     ** this to refer to the IDENTITY source later on.
	     **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else if (where_clause->Children.nItems == 0)
	    {
	    /** IF this is a non-property leaf node, just grab the coverage mask. **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else if (where_clause->NodeType == EXPR_N_OBJECT)
	    {
	    /** If an object node, just grab the coverage mask **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else
	    {
    	    /** First, call this routine on sub-expressions **/
	    for(i=0;i<where_clause->Children.nItems;i++)
	        {
		/** Call sub-expression **/
	        exp = (pExpression)(where_clause->Children.Items[i]);
	        mq_internal_DetermineCoverage(stmt, exp, qs_where, qs_select, qs_update, level+1);

		/** Sum up the object mask of objs involved, and determine outer members **/
	        sum_objmask |= exp->ObjCoverageMask;
		if (where_clause->NodeType == EXPR_N_COMPARE)
		    {
		    if (((where_clause->Flags & EXPR_F_LOUTERJOIN) && i==0) ||
		        ((where_clause->Flags & EXPR_F_ROUTERJOIN) && i==1))
			{
		        sum_outermask |= exp->ObjCoverageMask;
			}
		    }
		else
		    {
		    sum_outermask |= exp->ObjOuterMask;
		    }
	        }
	    where_clause->ObjCoverageMask = sum_objmask;
	    where_clause->ObjOuterMask = sum_outermask;

	    /** Now check to see if all objects can be covered by this. **/
	    for(i=0;i<where_clause->Children.nItems;i++)
	        {
	        exp = (pExpression)(where_clause->Children.Items[i]);
	        if (exp->ObjCoverageMask && ((exp->ObjCoverageMask & ~EXPR_MASK_EXTREF) != (sum_objmask & ~EXPR_MASK_EXTREF)))
	            {
		    is_covered = 0;
		    break;
	   	    }
	        }

	    /** If this isn't an AND node, it actually is covered... **/
	    if (where_clause->NodeType != EXPR_N_AND) is_covered = 1;
	    }

	/** IF NOT covered, take node's children and make where items from them. **/
	if (!is_covered)
	    {
	    /** Grab out the elements of the AND clause. **/
	    while(where_clause->Children.nItems)
	        {
	        exp = (pExpression)(where_clause->Children.Items[0]);
		where_item = mq_internal_AllocQS(MQ_T_WHEREITEM);
		where_item->Parent = qs_where;
		xaAddItem(&qs_where->Children, (void*)where_item);
		where_item->Expr = exp;
		where_item->Presentation[0] = 0;
		where_item->Name[0] = 0;
		where_item->Source[0] = 0;
		where_item->Expr->Parent = NULL;
		xaRemoveItem(&where_clause->Children,0);

		/** Set the object cnt **/
		if (where_item)
	    	    {
	    	    where_item->ObjCnt = 0;
	    	    v = where_item->Expr->ObjCoverageMask;
	    	    for(i=0;i<EXPR_MAX_PARAMS;i++) 
	        	{
			if ((v & 1) && i >= stmt->Query->nProvidedObjects) where_item->ObjCnt++;
			v >>= 1;
			}
	    	    }
		}

	    /** Convert the AND clause to an INTEGER node with value 1 **/
	    where_clause->NodeType = EXPR_N_INTEGER;
	    where_clause->DataType = DATA_T_INTEGER;
	    where_clause->ObjCoverageMask = 0;
	    where_clause->ObjOuterMask = 0;
	    where_clause->Integer = 1;
	    }

	/** If covered and at top-level, add this as one where-item. **/
	if (is_covered && level == 0)
	    {
	    where_clause->Flags |= EXPR_F_CVNODE;
	    where_item = mq_internal_AllocQS(MQ_T_WHEREITEM);
	    where_item->Parent = qs_where;
	    xaAddItem(&qs_where->Children, (void*)where_item);
	    where_item->Expr = where_clause;
	    where_item->Presentation[0] = 0;
	    where_item->Name[0] = 0;
	    where_item->Source[0] = 0;

	    /** Set the object cnt **/
	    if (where_item)
	    	{
	    	where_item->ObjCnt = 0;
	    	v = where_item->Expr->ObjCoverageMask;
	    	for(i=0;i<EXPR_MAX_PARAMS;i++) 
	            {
		    if ((v & 1) && i >= stmt->Query->nProvidedObjects) where_item->ObjCnt++;
		    v >>= 1;
		    }
	        }
	    }

    return 0;
    }


/*** mq_internal_OptimizeExpr - take the various WHERE expressions and perform
 *** little optimizations on them (like removing (x) AND TRUE types of things)
 ***/
int
mq_internal_OptimizeExpr(pExpression *expr)
    {
    pExpression expr_tmp;
    pExpression i0, i1;
    int i;

	if ((*expr)->Children.nItems == 0) return 0;

    	/** Perform AND TRUE optimization **/
	if ((*expr)->Children.nItems == 2)
	    {
	    /** Do some 'shortcut' assignments **/
	    i0 = (pExpression)((*expr)->Children.Items[0]);
	    i1 = (pExpression)((*expr)->Children.Items[1]);

	    /** Weed out ___ AND TRUE or TRUE AND ___ **/
	    if (i1 && (*expr)->NodeType == EXPR_N_AND && i0->NodeType == EXPR_N_INTEGER && i0->Integer == 1)
		{
		expr_tmp = *expr;
		*expr = i1;
		(*expr)->Parent = expr_tmp->Parent;
		xaRemoveItem(&(expr_tmp->Children),1);
		expFreeExpression(expr_tmp);
		}
	    else if (i1 && (*expr)->NodeType == EXPR_N_AND && i1->NodeType == EXPR_N_INTEGER && i1->Integer == 1)
		{
		expr_tmp = *expr;
		*expr = i0;
		(*expr)->Parent = expr_tmp->Parent;
		xaRemoveItem(&(expr_tmp->Children),0);
		expFreeExpression(expr_tmp);
		}
	    }

	/** Optimize sub-expressions **/
	for(i=0;i<(*expr)->Children.nItems;i++)
	    {
	    mq_internal_OptimizeExpr((pExpression*)&((*expr)->Children.Items[i]));
	    }

    return 0;
    }


/*** mq_internal_ProcessWhere - take the where expression, and remove all OR
 *** components from it by converting them to UNION DISTINCT types of operations,
 *** giving multiple queries, unless the OR is within a subtree that is covered
 *** completely by one directory source.  Then take the remaining elements of
 *** the WHERE expression, and add them as WHERE items in the query syntax
 *** structure.  Whew!!!
 ***/
int
mq_internal_ProcessWhere(pExpression where_clause, pExpression search_at, pQueryStructure *qs)
    {
    /*pQueryStructure new_qs;
    pQueryStructure sub_qs[16];*/

    	/** Is _this_ one an 'OR' expression node? **/
	if (search_at->NodeType == EXPR_N_OR)
	    {
	    }

    return 0;
    }


/*** mq_internal_ParseSelectItem - create a single SELECT item.  This is also
 *** used by the Upsert logic to create ON DUPLICATE items, which have an
 *** identical syntax.
 ***/
int
mq_internal_ParseSelectItem(pQueryStructure item_qs, pLxSession lxs)
    {
    int t;
    int parenlevel;
    int n_tok;
    char* ptr;
    int col_alias_state = 0;
    int col_assign_state = 0;

	/** Copy the entire item literally to the RawData for later compilation **/
	item_qs->Presentation[0] = 0;
	item_qs->Name[0] = 0;
	parenlevel = 0;
	n_tok = 0;
	while(1)
	    {
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
		break;
	    n_tok++;
	    if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
		break;
	    if (t == MLX_TOK_OPENPAREN) 
		parenlevel++;
	    if (t == MLX_TOK_CLOSEPAREN)
		{
		parenlevel--;
		if (parenlevel < 0)
		    break;
		}

	    /** Copy it to the raw data **/
	    ptr = mlxStringVal(lxs,NULL);
	    if (!ptr) break;
	    if (t == MLX_TOK_STRING)
		xsConcatQPrintf(&item_qs->RawData, "%STR&DQUOT", ptr);
	    else
		xsConcatenate(&item_qs->RawData,ptr,-1);
	    xsConcatenate(&item_qs->RawData," ",1);

	    /** Handle column aliases (SELECT a = :obj:attr) **/
	    if (col_alias_state == 1 && n_tok == 2)
		{
		if (t == MLX_TOK_EQUALS)
		    {
		    col_alias_state++;
		    xsCopy(&item_qs->RawData,"",-1);
		    }
		else
		    col_alias_state = -1;
		}
	    else if (col_alias_state == 0 && n_tok == 1)
		{
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    col_alias_state++;
		    strtcpy(item_qs->Presentation, ptr, sizeof(item_qs->Presentation));
		    }
		else
		    col_alias_state = -1;
		}

	    /** Handle column assignments (SELECT :obj:attr = :obj2:attr2) **/
	    if (col_assign_state == 0 && n_tok == 1)
		{
		if (t == MLX_TOK_COLON)
		    col_assign_state++;
		else
		    col_assign_state = -1;
		}
	    else if (col_assign_state == 1 && n_tok == 2)
		{
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    col_assign_state++;
		    strtcpy(item_qs->Name, ptr, sizeof(item_qs->Name));
		    }
		else
		    col_assign_state = -1;
		}
	    else if (col_assign_state == 2 && n_tok == 3)
		{
		if (t == MLX_TOK_COLON)
		    col_assign_state++;
		else
		    col_assign_state = -1;
		}
	    else if (col_assign_state == 3 && n_tok == 4)
		{
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    col_assign_state++;
		    strtcpy(item_qs->Presentation, ptr, sizeof(item_qs->Presentation));
		    }
		else
		    col_assign_state = -1;
		}
	    else if (col_assign_state == 4 && n_tok == 5)
		{
		if (t == MLX_TOK_EQUALS)
		    {
		    col_assign_state++;
		    xsCopy(&item_qs->RawData,"",-1);
		    item_qs->Flags |= MQ_SF_ASSIGNMENT;
		    }
		else
		    col_assign_state = -1;
		}
	    }

	/** Clean up presentation/name stuff if unneeded **/
	if (col_assign_state != 5)
	    item_qs->Name[0] = '\0';
	if (col_assign_state != 5 && col_alias_state != 2)
	    item_qs->Presentation[0] = '\0';

    return t;
    }


/*** mq_internal_ParseUpdateItem - parse one assignment in an UPDATE ... SET
 *** clause.  This is used both by traditional UPDATE ... SET statements as
 *** well as by ON DUPLICATE ... UPDATE SET ... statements.
 ***/
int
mq_internal_ParseUpdateItem(pQueryStructure item_qs, pLxSession lxs)
    {
    int t, prev_t;
    int parenlevel;
    int in_assign;
    char* ptr;

	/** Copy the entire item literally to the RawData for later compilation **/
	item_qs->Presentation[0] = 0;
	item_qs->Name[0] = 0;
	parenlevel = 0;
	in_assign = 1;
	while(1)
	    {
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
		break;
	    if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
		break;
	    if (t == MLX_TOK_OPENPAREN) 
		parenlevel++;
	    if (t == MLX_TOK_CLOSEPAREN)
		parenlevel--;
	    if (t == MLX_TOK_EQUALS && parenlevel == 0 && in_assign)
		{
		in_assign = 0;
		continue;
		}
	    ptr = mlxStringVal(lxs,NULL);
	    if (!ptr) break;
	    if (t == MLX_TOK_STRING)
		{
		if (in_assign)
		    xsConcatQPrintf(&item_qs->AssignRawData, "%STR&DQUOT", ptr);
		else
		    xsConcatQPrintf(&item_qs->RawData, "%STR&DQUOT", ptr);
		}
	    else
		{
		if (in_assign)
		    xsConcatenate(&item_qs->AssignRawData,ptr,-1);
		else
		    xsConcatenate(&item_qs->RawData,ptr,-1);
		}
	    xsConcatenate(&item_qs->RawData," ",1);
	    }

	/** Check for IF MODIFIED modifier **/
	if (t == MLX_TOK_RESERVEDWD)
	    {
	    ptr = mlxStringVal(lxs, NULL);
	    if (!strcasecmp(ptr, "if"))
		{
		prev_t = t;
		t = mlxNextToken(lxs);
		if (t == MLX_TOK_RESERVEDWD)
		    {
		    ptr = mlxStringVal(lxs, NULL);
		    if (!strcasecmp(ptr, "modified"))
			{
			item_qs->Flags |= MQ_SF_IFMODIFIED;
			t = mlxNextToken(lxs);
			}
		    else
			{
			mlxHoldToken(lxs);
			t = prev_t;
			}
		    }
		else
		    {
		    mlxHoldToken(lxs);
		    t = prev_t;
		    }
		}
	    }

	if (!item_qs->RawData.String[0] || !item_qs->AssignRawData.String[0])
	    {
	    mssError(1, "MQ", "UPDATE assignment must have form '{assign-expr} = {value-expr}'");
	    mlxNotePosition(lxs);
	    return -1;
	    }

    return t;
    }


/*** mq_internal_CopyLiteral() - copy lexer data to a string raw, for later
 *** analysis or compilation.  Returns the token type of the last token
 *** encountered (i.e., the token that terminated the copy operation).
 ***/
int
mq_internal_CopyLiteral(pLxSession lxs, pXString xs)
    {
    int t, parenlevel;
    char* ptr;

	parenlevel = 0;
	while(1)
	    {
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
		break;
	    if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON || t == MLX_TOK_COMMA) && parenlevel <= 0)
		break;
	    if (t == MLX_TOK_OPENPAREN) parenlevel++;
	    if (t == MLX_TOK_CLOSEPAREN) parenlevel--;
	    ptr = mlxStringVal(lxs,NULL);
	    if (!ptr) break;
	    if (t == MLX_TOK_STRING)
		{
		xsConcatQPrintf(xs, "%STR&DQUOT", ptr);
		}
	    else
		{
		xsConcatenate(xs, ptr, -1);
		}
	    xsConcatenate(xs, " ", 1);
	    }

	/** Mismatched parentheses **/
	if (parenlevel != 0)
	    {
	    return -t;
	    }

    return t;
    }


/*** mq_internal_SyntaxParse - parse the syntax of the SQL used for this
 *** query.
 ***/
pQueryStructure
mq_internal_SyntaxParse(pLxSession lxs, pQueryStatement stmt)
    {
    pQueryStructure qs, new_qs, select_cls=NULL, from_cls=NULL, where_cls=NULL, orderby_cls=NULL, groupby_cls=NULL, crosstab_cls=NULL, having_cls=NULL;
    pQueryStructure insert_cls=NULL, update_cls=NULL, delete_cls=NULL;
    /* pQueryStructure delete_cls=NULL, update_cls=NULL;*/
    pQueryStructure limit_cls = NULL;
    pQueryStructure ondup_cls = NULL;
    pQueryStructure declare_cls = NULL;
    ParserState state = LookForClause;
    ParserState next_state = ParseError;
    int t,parenlevel,subtr,identity,inclsubtr,wildcard,fromobject,prunesubtr,expfrom,collfrom,nonempty,paged;
    int is_object;
    char* ptr;
    char* str;
    char cmd[10];
    pXString xs, param;
    pTObjData ptod;
    static char* reserved_wds[] = {"where","select","from","order","by","set","rowcount","group",
    				   "crosstab","as","having","into","update","delete","insert",
				   "values","with","limit","for","on","duplicate", "declare",
				   "showplan", "multistatement", "if", "modified", "exec", 
				   "log", "print", NULL};

    	/** Setup reserved words list for lexical analyzer **/
	mlxSetReservedWords(lxs, reserved_wds);

    	/** Allocate a structure to work with **/
	qs = mq_internal_AllocQS(MQ_T_QUERY);

    	/** Enter finite state machine parser. **/
	while(state != ParseDone && state != ParseError)
	    {
	    switch(state)
	        {
		case ParseDone:
		case ParseError:
		    /** added these to shut up the -Wall gcc switch **/
		    /** never reached - see while() statement above **/
		    break;

	        case LookForClause:
		    if ((t=mlxNextToken(lxs)) != MLX_TOK_RESERVEDWD)
		        {
			if ((t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON) && (select_cls || update_cls || delete_cls || declare_cls))
			    {
			    mlxHoldToken(lxs);
			    next_state = ParseDone;
			    break;
			    }
			next_state = ParseError;
			mssError(1,"MQ","Query must begin with SELECT");
			mlxNoteError(lxs);
			}
		    else
		        {
			ptr = mlxStringVal(lxs,NULL);
			if (!strcmp("select",ptr))
			    {
			    if (update_cls || delete_cls)
				{
				mssError(1, "MQ", "Query cannot contain both a SELECT and an UPDATE or DELETE clause");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
			    if (select_cls)
				{
				mssError(1, "MQ", "Query must contain only one SELECT clause");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
			    select_cls = mq_internal_AllocQS(MQ_T_SELECTCLAUSE);
			    xaAddItem(&qs->Children, (void*)select_cls);
			    select_cls->Parent = qs;
		            next_state = SelectItem;
			    }
			else if (!strcmp("from",ptr))
			    {
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    }
			else if (!strcmp("where",ptr))
			    {
			    where_cls = mq_internal_AllocQS(MQ_T_WHERECLAUSE);
			    xaAddItem(&qs->Children, (void*)where_cls);
			    where_cls->Parent = qs;
		            next_state = WhereItem;
			    }
			else if (!strcmp("order",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after ORDER");
				mlxNoteError(lxs);
				}
			    else
			        {
				orderby_cls = mq_internal_AllocQS(MQ_T_ORDERBYCLAUSE);
				xaAddItem(&qs->Children, (void*)orderby_cls);
				orderby_cls->Parent = qs;
			        next_state = OrderByItem;
				}
			    }
			else if (!strcmp("group",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after GROUP");
				mlxNoteError(lxs);
				}
			    else
			        {
				groupby_cls = mq_internal_AllocQS(MQ_T_GROUPBYCLAUSE);
				xaAddItem(&qs->Children, (void*)groupby_cls);
				groupby_cls->Parent = qs;
			        next_state = GroupByItem;
				}
			    }
			else if (!strcmp("with",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_KEYWORD)
			        {
			        mssError(1,"MQ","Expected keyword after WITH");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else
			        {
				new_qs = mq_internal_AllocQS(MQ_T_WITHCLAUSE);
				xaAddItem(&qs->Children, (void*)new_qs);
				new_qs->Parent = qs;
				mlxCopyToken(lxs,new_qs->Name,31);
				next_state = LookForClause;
				}
			    }
			else if (!strcmp("limit",ptr))
			    {
			    if (limit_cls)
				{
			        mssError(1,"MQ","Duplicate LIMIT clause found");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				{
			        mssError(1,"MQ","Expected number after LIMIT");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else
				{
				limit_cls = mq_internal_AllocQS(MQ_T_LIMITCLAUSE);
				xaAddItem(&qs->Children, (void*)limit_cls);
				limit_cls->Parent = qs;
				limit_cls->IntVals[0] = mlxIntVal(lxs);
				if (mlxNextToken(lxs) == MLX_TOK_COMMA)
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
					{
					mssError(1,"MQ","Expected numeric START,CNT after LIMIT");
					mlxNoteError(lxs);
					next_state = ParseError;
					}
				    limit_cls->IntVals[1] = mlxIntVal(lxs);
				    next_state = LookForClause;
				    }
				else
				    {
				    mlxHoldToken(lxs);
				    limit_cls->IntVals[1] = limit_cls->IntVals[0];
				    limit_cls->IntVals[0] = 0;
				    }
				}
			    }
			else if (!strcmp("crosstab",ptr))
			    {
			    /** Allocate the main node for the crosstab clause **/
			    crosstab_cls = mq_internal_AllocQS(MQ_T_CROSSTABCLAUSE);
			    xaAddItem(&qs->Children, (void*)crosstab_cls);
			    crosstab_cls->Parent = qs;

			    /** Determine column to be crosstab'd **/
			    if (mlxNextToken(lxs) != MLX_TOK_KEYWORD)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected column name after CROSSTAB");
				mlxNoteError(lxs);
				break;
				}
			    mlxCopyToken(lxs, crosstab_cls->Name, 32);
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after CROSSTAB <%s>", crosstab_cls->Name);
				mlxNoteError(lxs);
				break;
				}

			    /** Determine crosstab criteria **/
			    while(1)
			        {
				t = mlxNextToken(lxs);
				if (t == MLX_TOK_ERROR || t == MLX_TOK_RESERVEDWD || t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON) break;
				if (crosstab_cls->RawData.String[0]) xsConcatenate(&crosstab_cls->RawData," ",1);
				if (t == MLX_TOK_STRING) xsConcatenate(&crosstab_cls->RawData,"\"",1);
				ptr = mlxStringVal(lxs,NULL);
				if (!ptr) break;
				xsConcatenate(&crosstab_cls->RawData,ptr,-1);
				if (t == MLX_TOK_STRING) xsConcatenate(&crosstab_cls->RawData,"\"",1);
				}
			    if (t == MLX_TOK_ERROR)
			        {
				mssError(1,"MQ","Expected End-of-SQL, reserved word, or AS after CROSSTAB ... BY ... ");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
			    if (t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON)
			        {
				mlxHoldToken(lxs);
				next_state = ParseDone;
				break;
				}

			    /** See if we have an AS clause for the crosstab **/
			    if (strcmp(mlxStringVal(lxs,NULL),"as"))
			        {
				mlxHoldToken(lxs);
				break;
				}
			    mlxSetOptions(lxs,MLX_F_IFSONLY);
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_STRING)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected column header name after CROSSTAB ... AS");
				mlxNoteError(lxs);
				break;
				}
			    mlxCopyToken(lxs,crosstab_cls->Presentation,32);
			    mlxUnsetOptions(lxs,MLX_F_IFSONLY);
			    t = mlxNextToken(lxs);
			    if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON) mlxHoldToken(lxs);
			    else if (t == MLX_TOK_EOF) next_state = ParseDone;
			    else if (t == MLX_TOK_ERROR) 
			        {
				mssError(1,"MQ","Expected End-of-SQL or reserved word after CROSSTAB ... BY ... AS");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    }
			else if (!strcmp("declare",ptr))
			    {
			    if (declare_cls)
				{
				mssError(1,"MQ","Only one DECLARE clause allowed per statement");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    if (select_cls || insert_cls || delete_cls || update_cls)
				{
				mssError(1,"MQ","DECLARE cannot be used with any other clause");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_KEYWORD || (ptr = mlxStringVal(lxs,NULL)) == NULL || (strcasecmp(ptr, "object") != 0 && strcasecmp(ptr, "collection") != 0))
				{
				mssError(1,"MQ","Expected keyword OBJECT or COLLECTION after DECLARE");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    if (!strcasecmp(ptr, "object"))
				is_object = 1;
			    else
				is_object = 0;
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_KEYWORD && t != MLX_TOK_RESERVEDWD && t != MLX_TOK_STRING)
				{
				mssError(1,"MQ","Expected object name after DECLARE OBJECT/COLLECTION");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    declare_cls = mq_internal_AllocQS(MQ_T_DECLARECLAUSE);
			    xaAddItem(&qs->Children, (void*)declare_cls);
			    declare_cls->Parent = qs;
			    mlxCopyToken(lxs, declare_cls->Name, sizeof(declare_cls->Name));
			    if (!is_object)
				declare_cls->Flags |= MQ_SF_COLLECTION;
			    if (!strcmp(declare_cls->Name, "") || !strcasecmp(declare_cls->Name, "this") || strlen(declare_cls->Name) >= 31)
				{
				mssError(1,"MQ","Invalid object name after DECLARE OBJECT/COLLECTION");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else
				{
				/** Look for scope indication **/
				t = mlxNextToken(lxs);
				if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs, NULL)) != NULL && !strcasecmp(ptr, "scope"))
				    {
				    /** Valid scopes are QUERY and APPLICATION **/
				    t = mlxNextToken(lxs);
				    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs, NULL)) != NULL && (!strcasecmp(ptr, "application") || !strcasecmp(ptr, "query")))
					{
					if (!strcasecmp(ptr, "application"))
					    declare_cls->Flags |= MQ_SF_APPSCOPE;
					}
				    else
					{
					mssError(1,"MQ","Invalid scope type for DECLARE OBJECT/COLLECTION ... SCOPE ...");
					mlxNoteError(lxs);
					next_state = ParseError;
					}
				    }
				else
				    {
				    mlxHoldToken(lxs);
				    }
				next_state = LookForClause;
				}
			    }
			else if (!strcmp("set",ptr))
			    {
			    if (update_cls)
				{
				next_state = UpdateItem;
				break;
				}

			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
			        {
				next_state = ParseError;
				mssError(1,"MQ","SET requires one of rowcount, multistatement, or showplan");
				mlxNoteError(lxs);
				}
			    else
			        {
				ptr = mlxStringVal(lxs,NULL);
				if (!strcmp(ptr,"rowcount"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				        {
					next_state = ParseError;
			                mssError(1,"MQ","SET ROWCOUNT requires INTEGER parameter");
					mlxNoteError(lxs);
					}
				    else
				        {
					new_qs = mq_internal_AllocQS(MQ_T_SETOPTION);
					new_qs->Parent = qs;
					xaAddItem(&qs->Children, (void*)new_qs);
					mlxCopyToken(lxs,new_qs->Source,255);
					strcpy(new_qs->Name,"rowcount");
					next_state = LookForClause;
					}
				    }
				else if (!strcmp(ptr,"ms") || !strcmp(ptr,"multistatement"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				        {
					next_state = ParseError;
			                mssError(1,"MQ","SET MULTISTATEMENT expects 0 or 1 following");
					mlxNoteError(lxs);
					}
				    else
				        {
					if (mlxIntVal(lxs))
					    {
					    stmt->Query->Flags |= MQ_F_MULTISTATEMENT;
					    next_state = LookForClause;
					    }
					else
					    {
					    if (stmt->Query->Flags & MQ_F_ONESTATEMENT)
						{
						next_state = ParseError;
						mssError(1,"MQ","SET MULTISTATEMENT disabled in this context");
						mlxNoteError(lxs);
						}
					    else
						{
						stmt->Query->Flags &= ~MQ_F_MULTISTATEMENT;
						next_state = LookForClause;
						}
					    }
					}
				    }
				else if (!strcmp(ptr,"showplan"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				        {
					next_state = ParseError;
			                mssError(1,"MQ","SET SHOWPLAN expects 0 or 1 following");
					mlxNoteError(lxs);
					}
				    else
				        {
					if (!mlxIntVal(lxs))
					    {
					    stmt->Query->Flags &= ~MQ_F_SHOWPLAN;
					    next_state = LookForClause;
					    }
					else
					    {
					    if (stmt->Query->Flags & MQ_F_ONESTATEMENT)
						{
						next_state = ParseError;
						mssError(1,"MQ","SET SHOWPLAN disabled in this context");
						mlxNoteError(lxs);
						}
					    else
						{
						stmt->Query->Flags |= MQ_F_SHOWPLAN;
						next_state = LookForClause;
						}
					    }
					}
				    }
				else
				    {
				    next_state = ParseError;
			            mssError(1,"MQ","Unknown SET parameter <%s>", ptr);
				    mlxNoteError(lxs);
				    }
				}
			    }
			else if (!strcmp("having",ptr))
			    {
			    having_cls = mq_internal_AllocQS(MQ_T_HAVINGCLAUSE);
			    xaAddItem(&qs->Children, (void*)having_cls);
			    having_cls->Parent = qs;
		            next_state = HavingItem;
			    }
			else if (!strcmp("delete",ptr))
			    {
			    if (stmt->Query->Flags & MQ_F_NOUPDATE)
				{
				next_state = ParseError;
				mssError(1,"MQ","DELETE statement disallowed because data changes are forbidden");
				mlxNoteError(lxs);
				break;
				}
			    if (select_cls || insert_cls || update_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have DELETE after a SELECT, INSERT, or UPDATE clause");
				mlxNoteError(lxs);
				break;
				}

			    /** Create the main delete clause **/
			    delete_cls = mq_internal_AllocQS(MQ_T_DELETECLAUSE);
			    xaAddItem(&qs->Children, (void*)delete_cls);
			    delete_cls->Parent = qs;

			    /** Optional FROM keyword **/
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_RESERVEDWD || ((ptr = mlxStringVal(lxs,NULL)) && strcasecmp("from", ptr)))
				{
				mlxHoldToken(lxs);
				}

			    /** Create a "from" clause too, the user didn't actually type it,
			     ** but this is sortof like DELETE FROM /data/source, and SUBTREE
			     ** is supported as are joins.
	 		     **/
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    break;
			    }
			else if (!strcmp("insert",ptr))
			    {
			    if (stmt->Query->Flags & MQ_F_NOUPDATE)
				{
				next_state = ParseError;
				mssError(1,"MQ","INSERT statement disallowed because data changes are forbidden");
				mlxNoteError(lxs);
				break;
				}
			    if (select_cls || update_cls || delete_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have INSERT after a SELECT, UPDATE, or DELETE clause");
				mlxNoteError(lxs);
				break;
				}

			    /** Create the main insert clause **/
			    insert_cls = mq_internal_AllocQS(MQ_T_INSERTCLAUSE);
			    xaAddItem(&qs->Children, (void*)insert_cls);
			    insert_cls->Parent = qs;

			    /** Check for optional "INTO" and then table name **/
			    collfrom = 0;
			    t = mlxNextToken(lxs);
		    	    if (t == MLX_TOK_RESERVEDWD && (ptr = mlxStringVal(lxs,NULL)) != NULL && !strcasecmp(ptr, "into"))
			        {
				t = mlxNextToken(lxs);
				}
			    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs, NULL)) != NULL && !strcasecmp(ptr, "collection"))
				{
				insert_cls->Flags |= MQ_SF_COLLECTION;
				t = mlxNextToken(lxs);
				}
			    else if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs, NULL)) != NULL && !strcasecmp(ptr, "object"))
				{
				insert_cls->Flags |= MQ_SF_FROMOBJECT;
				t = mlxNextToken(lxs);
				}
			    if (t != MLX_TOK_FILENAME && t != MLX_TOK_STRING && (!(insert_cls->Flags & MQ_SF_COLLECTION) || t != MLX_TOK_KEYWORD))
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected pathname after INSERT [INTO]");
				mlxNoteError(lxs);
				break;
				}
		    	    mlxCopyToken(lxs, insert_cls->Source, 256);

			    t = mlxNextToken(lxs);

			    /** Check for a keyword, which provides an identifier for the insert path **/
			    if (t == MLX_TOK_KEYWORD || t == MLX_TOK_STRING)
				{
				ptr = mlxStringVal(lxs,NULL);
				if (ptr)
				    strtcpy(insert_cls->Presentation, ptr, sizeof(insert_cls->Presentation));
				t = mlxNextToken(lxs);
				}

			    /** Ok, either a paren (for column list), VALUES, or SELECT is next. **/
			    if (t == MLX_TOK_RESERVEDWD)
			        {
				/** VALUES or SELECT **/
				ptr = mlxStringVal(lxs, NULL);
				if (ptr && !strcmp(ptr, "select"))
				    {
				    if (select_cls)
					{
					mssError(1, "MQ", "INSERT INTO ... SELECT: Query must contain only one SELECT clause");
					mlxNoteError(lxs);
					next_state = ParseError;
					break;
					}
				    select_cls = mq_internal_AllocQS(MQ_T_SELECTCLAUSE);
				    xaAddItem(&qs->Children, (void*)select_cls);
				    select_cls->Parent = qs;
				    next_state = SelectItem;
				    }
				}
			    else if (t == MLX_TOK_OPENPAREN)
			        {
				/** Column list **/
				}
			    }
			else if (!strcmp("update",ptr))
			    {
			    if (stmt->Query->Flags & MQ_F_NOUPDATE)
				{
				next_state = ParseError;
				mssError(1,"MQ","UPDATE statement disallowed because data changes are forbidden");
				mlxNoteError(lxs);
				break;
				}
			    if (select_cls || insert_cls || delete_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have UPDATE after a SELECT, INSERT, or DELETE clause");
				mlxNoteError(lxs);
				break;
				}

			    /** Duh, this comes as no surprise. **/
			    stmt->Flags |= MQ_TF_ALLOWUPDATE;

			    /** Create the main update clause **/
			    update_cls = mq_internal_AllocQS(MQ_T_UPDATECLAUSE);
			    xaAddItem(&qs->Children, (void*)update_cls);
			    update_cls->Parent = qs;

			    /** Create a "from" clause too, the user didn't actually type it,
			     ** but this is sortof like UPDATE FROM /data/source, and SUBTREE
			     ** is supported as are joins.
	 		     **/
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    break;
			    }
			else if (!strcmp("for",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected UPDATE after FOR");
				mlxNoteError(lxs);
				break;
				}
			    ptr = mlxStringVal(lxs,NULL);
			    if (ptr && !strcmp(ptr,"update"))
				{
				if (!select_cls)
				    {
				    next_state = ParseError;
				    mssError(1,"MQ","FOR UPDATE allowed only with SELECT");
				    mlxNoteError(lxs);
				    break;
				    }
				if (stmt->Query->Flags & MQ_F_NOUPDATE)
				    {
				    next_state = ParseError;
				    mssError(1,"MQ","SELECT ... FOR UPDATE disallowed because data changes are forbidden");
				    mlxNoteError(lxs);
				    break;
				    }
				select_cls->Flags |= MQ_SF_FORUPDATE;
				next_state = LookForClause;
				}
			    else
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected UPDATE after FOR");
				mlxNoteError(lxs);
				break;
				}
			    }
			else if (!strcmp("on",ptr))
			    {
			    /** Various sorts of ON clauses go here **/
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected DUPLICATE after ON");
				mlxNoteError(lxs);
				break;
				}
			    ptr = mlxStringVal(lxs,NULL);
			    if (ptr && strcmp(ptr,"duplicate") != 0)
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected DUPLICATE after ON");
				mlxNoteError(lxs);
				break;
				}
			    if (insert_cls == NULL)
				{
				next_state = ParseError;
				mssError(1,"MQ","ON DUPLICATE...UPDATE is only valid with an INSERT statement");
				mlxNoteError(lxs);
				break;
				}
			    ondup_cls = mq_internal_AllocQS(MQ_T_ONDUPCLAUSE);
			    xaAddItem(&qs->Children, (void*)ondup_cls);
			    ondup_cls->Parent = qs;
			    next_state = OnDupItem;
			    }
			else if (!strcmp("exec", ptr))
			    {
			    /** EXEC is a form of SELECT from a specific source with parameters **/
			    t = mlxNextToken(lxs);
			    if (t == MLX_TOK_FILENAME || t == MLX_TOK_STRING)
				{
				/** We handle this by setting up a SELECT * FROM /path/name?param=value **/
				ptr = mlxStringVal(lxs, NULL);
				select_cls = mq_internal_AllocQS(MQ_T_SELECTCLAUSE);
				xaAddItem(&qs->Children, (void*)select_cls);
				select_cls->Parent = qs;
				new_qs = mq_internal_AllocQS(MQ_T_SELECTITEM);
				xaAddItem(&select_cls->Children, (void*)new_qs);
				new_qs->Parent = select_cls;
				new_qs->Expr = NULL;
				strtcpy(new_qs->Presentation, "*", sizeof(new_qs->Presentation));
				xsCopy(&new_qs->RawData, "*", 1);
				new_qs->Flags |= MQ_SF_ASTERISK;
				new_qs->ObjCnt = 1;
				from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
				xaAddItem(&qs->Children, (void*)from_cls);
				from_cls->Parent = qs;
				new_qs = mq_internal_AllocQS(MQ_T_FROMSOURCE);
				xaAddItem(&from_cls->Children, (void*)new_qs);
				new_qs->Parent = from_cls;
				new_qs->Presentation[0] = 0;
				new_qs->Name[0] = 0;
				xs = xsNew();
				xsCopy(xs, ptr, -1);

				/** Check for parameters **/
				int paramcnt = 0;
				while(1)
				    {
				    t = mlxNextToken(lxs);
				    if (t != MLX_TOK_STRING && t != MLX_TOK_RESERVEDWD && t != MLX_TOK_KEYWORD)
					{
					mlxHoldToken(lxs);
					break;
					}
				    paramcnt++;
				    ptr = mlxStringVal(lxs, NULL);
				    xsConcatQPrintf(xs, "%STR%STR&URL", (paramcnt == 1)?"?":"&", ptr);
				    t = mlxNextToken(lxs);
				    if (t != MLX_TOK_EQUALS)
					{
					next_state = ParseError;
					mssError(1,"MQ","Expected equals after EXEC parameter");
					mlxNoteError(lxs);
					xsFree(xs);
					break;
					}

				    /** Parameter value **/
				    param = xsNew();
				    t = mq_internal_CopyLiteral(lxs, param);
				    if (t < 0)
					{
					next_state = ParseError;
					mssError(1,"MQ","Error in EXEC parameter");
					mlxNoteError(lxs);
					xsFree(xs);
					xsFree(param);
					break;
					}
				    ptod = expCompileAndEval(param->String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
				    if (!ptod)
					{
					next_state = ParseError;
					mssError(1,"MQ","Could not evaluate EXEC parameter");
					mlxNoteError(lxs);
					xsFree(xs);
					xsFree(param);
					break;
					}
				    ptr = ptodToStringTmp(ptod);
				    xsConcatQPrintf(xs, "=%STR&URL", ptr);
				    if (t != MLX_TOK_COMMA)
					{
					mlxHoldToken(lxs);
					break;
					}
				    }

				strtcpy(new_qs->Source, xs->String, sizeof(new_qs->Source));
				next_state = LookForClause;
				}
			    else
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected path name after EXEC");
				mlxNoteError(lxs);
				break;
				}
			    }
			else if (!strcmp("log", ptr) || !strcmp("print", ptr))
			    {
			    strtcpy(cmd, ptr, sizeof(cmd));
			    xs = xsNew();
			    param = xsNew();
			    next_state = LookForClause;
			    if (xs && param)
				{
				while(1)
				    {
				    t = mq_internal_CopyLiteral(lxs, param);

				    if (t < 0)
					{
					next_state = ParseError;
					mssError(1,"MQ","Error in %s statement parameter", cmd);
					mlxNoteError(lxs);
					xsFree(xs);
					xsFree(param);
					break;
					}
				    else
					{
					ptod = expCompileAndEval(param->String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
					if (!ptod)
					    {
					    next_state = ParseError;
					    mssError(1,"MQ","Could not evaluate %s parameter", cmd);
					    mlxNoteError(lxs);
					    xsFree(xs);
					    xsFree(param);
					    break;
					    }
					str = ptodToStringTmp(ptod);
					xsConcatenate(xs, str, -1);
					}

				    if (t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON)
					{
					mlxHoldToken(lxs);
					next_state = ParseDone;
					break;
					}

				    if (t != MLX_TOK_COMMA)
					{
					mlxHoldToken(lxs);
					break;
					}
				    else
					{
					xsConcatenate(xs, " ", 1);
					xsCopy(param, "", -1);
					}
				    }
				if (next_state != ParseError)
				    {
				    if (!strcmp("log", cmd))
					mssLog(LOG_INFO, xs->String);
				    else
					printf("%s\n", xs->String);
				    xsFree(xs);
				    xsFree(param);
				    stmt->Flags |= MQ_TF_IMMEDIATE;
				    }
				}
			    else
				{
				next_state = ParseError;
				mssError(1,"MQ","Memory exhausted");
				mlxNoteError(lxs);
				if (xs) xsFree(xs);
				if (param) xsFree(param);
				}
			    }
			else
			    {
			    next_state = ParseError;
			    mssError(1,"MQ","Unknown reserved word or clause name <%s>", ptr);
			    mlxNoteError(lxs);
			    }
			}
		    break;

		case OnDupItem:
		    new_qs = mq_internal_AllocQS(MQ_T_ONDUPITEM);
		    xaAddItem(&ondup_cls->Children, (void*)new_qs);
		    new_qs->Parent = ondup_cls;

		    t = mq_internal_ParseSelectItem(new_qs, lxs);

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD && (ptr = mlxStringVal(lxs, NULL)) && !strcmp(ptr,"update"))
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_RESERVEDWD && (ptr = mlxStringVal(lxs, NULL)) && !strcmp(ptr,"set"))
			    {
			    next_state = OnDupUpdateItem;
			    break;
			    }
			else
			    {
			    next_state = ParseError;
			    mssError(1,"MQ","Expected on-duplicate item or UPDATE SET clause after ON DUPLICATE");
			    mlxNoteError(lxs);
			    break;
			    }
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected on-duplicate item or UPDATE SET clause after ON DUPLICATE");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case OnDupUpdateItem:
		    new_qs = mq_internal_AllocQS(MQ_T_ONDUPUPDATEITEM);
		    xaAddItem(&ondup_cls->Children, (void*)new_qs);
		    new_qs->Parent = ondup_cls;

		    t = mq_internal_ParseUpdateItem(new_qs, lxs);
		    if (t < 0)
			{
			next_state = ParseError;
			break;
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected update item or end-of-query after UPDATE SET");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case GroupByItem:
		    new_qs = mq_internal_AllocQS(MQ_T_GROUPBYITEM);
		    new_qs->Parent = groupby_cls;
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&groupby_cls->Children, (void*)new_qs);

		    /** Copy the entire item literally to the RawData for later compilation **/
		    t = mq_internal_CopyLiteral(lxs, &new_qs->RawData);

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or group-by item after GROUP BY clause");
			mlxNoteError(lxs);
			break;
			}
		    break;


		case OrderByItem:
		    new_qs = mq_internal_AllocQS(MQ_T_ORDERBYITEM);
		    new_qs->Parent = orderby_cls;
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&orderby_cls->Children, (void*)new_qs);

		    /** Copy the entire item literally to the RawData for later compilation **/
		    t = mq_internal_CopyLiteral(lxs, &new_qs->RawData);

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or order-by item after ORDER BY clause");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case SelectItem:
		    new_qs = mq_internal_AllocQS(MQ_T_SELECTITEM);
		    xaAddItem(&select_cls->Children, (void*)new_qs);
		    new_qs->Parent = select_cls;

		    t = mq_internal_ParseSelectItem(new_qs, lxs);

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected select item, FROM clause, or end-of-query after SELECT");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case FromItem:
		    t = mlxNextToken(lxs);
		    subtr = 0;
		    identity = 0;
		    inclsubtr = 0;
		    prunesubtr = 0;
		    wildcard = 0;
		    fromobject = 0;
		    expfrom = 0;
		    collfrom = 0;
		    nonempty = 0;
		    paged = 0;
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("identity", ptr))
			{
			t = mlxNextToken(lxs);
			identity = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("nonempty", ptr))
			{
			t = mlxNextToken(lxs);
			nonempty = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("object", ptr))
			{
			t = mlxNextToken(lxs);
			fromobject = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("pruned", ptr))
			{
			t = mlxNextToken(lxs);
			prunesubtr = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("inclusive", ptr))
			{
			t = mlxNextToken(lxs);
			inclsubtr = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("subtree", ptr))
			{
			t = mlxNextToken(lxs);
			subtr = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("wildcard", ptr))
			{
			t = mlxNextToken(lxs);
			wildcard = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("paged", ptr))
			{
			t = mlxNextToken(lxs);
			paged = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("expression", ptr))
			{
			t = mlxNextToken(lxs);
			expfrom = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("collection", ptr))
			{
			t = mlxNextToken(lxs);
			collfrom = 1;
			}
		    if (prunesubtr && !subtr)
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: PRUNED keyword is invalid without SUBTREE keyword");
			mlxNoteError(lxs);
			break;
			}
		    if (inclsubtr && !subtr)
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: INCLUSIVE keyword is invalid without SUBTREE keyword");
			mlxNoteError(lxs);
			break;
			}
		    if (fromobject && subtr)
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: OBJECT keyword conflicts with SUBTREE keyword");
			mlxNoteError(lxs);
			break;
			}
		    if (collfrom && (expfrom | wildcard | fromobject))
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: COLLECTION keyword cannot be used with EXPRESSION, WILDCARD, or OBJECT");
			mlxNoteError(lxs);
			break;
			}
		    if (expfrom && t != MLX_TOK_OPENPAREN)
			{
			next_state = ParseError;
			mssError(1,"MQ","Expected open parenthesis after EXPRESSION keyword in FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    else if (collfrom && t != MLX_TOK_STRING && t != MLX_TOK_KEYWORD)
			{
			next_state = ParseError;
			mssError(1,"MQ","Expected collection name after COLLECTION keyword in FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    else if (!collfrom && !expfrom && (t != MLX_TOK_FILENAME && t != MLX_TOK_STRING))
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected data source filename in FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    new_qs = mq_internal_AllocQS(MQ_T_FROMSOURCE);
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    new_qs->Source[0] = 0;
		    if (subtr) new_qs->Flags |= MQ_SF_FROMSUBTREE;
		    if (inclsubtr) new_qs->Flags |= MQ_SF_INCLSUBTREE;
		    if (prunesubtr) new_qs->Flags |= MQ_SF_PRUNESUBTREE;
		    if (identity) new_qs->Flags |= MQ_SF_IDENTITY;
		    if (wildcard) new_qs->Flags |= MQ_SF_WILDCARD;
		    if (fromobject) new_qs->Flags |= MQ_SF_FROMOBJECT;
		    if (expfrom) new_qs->Flags |= MQ_SF_EXPRESSION;
		    if (collfrom) new_qs->Flags |= MQ_SF_COLLECTION;
		    if (nonempty) new_qs->Flags |= MQ_SF_NONEMPTY;
		    if (paged) new_qs->Flags |= MQ_SF_PAGED;
		    xaAddItem(&from_cls->Children, (void*)new_qs);
		    new_qs->Parent = from_cls;
		    parenlevel = 0;
		    if (expfrom)
			{
			do /* while (parenlevel > 0) */
			    {
			    if (t == MLX_TOK_OPENPAREN) parenlevel++;
			    if (t == MLX_TOK_CLOSEPAREN) parenlevel--;
			    if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
				break;
			    if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
				break;
			    ptr = mlxStringVal(lxs, NULL);
			    if (!ptr) break;
			    if (t == MLX_TOK_STRING)
				xsConcatQPrintf(&new_qs->RawData, "%STR&DQUOT", ptr);
			    else
				xsConcatenate(&new_qs->RawData, ptr, -1);
			    xsConcatenate(&new_qs->RawData," ",1);
			    t = mlxNextToken(lxs);
			    }
			    while (parenlevel > 0);
			}
		    else
			{
			mlxCopyToken(lxs,new_qs->Source,256);
			t = mlxNextToken(lxs);
			}
		    if (t == MLX_TOK_KEYWORD)
		        {
			mlxCopyToken(lxs,new_qs->Presentation,32);
			t = mlxNextToken(lxs);
			}
		    if (t == MLX_TOK_COMMA) 
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
			if (delete_cls && (ptr = mlxStringVal(lxs, NULL)) && !strcasecmp(ptr, "from"))
			    {
			    /** FROM keyword can be used to delimit sources in DELETE clause **/
			    next_state = state;
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    next_state = LookForClause;
			    }
			break;
			}
		    else if (t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query, FROM source, or WHERE/ORDER BY after FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case WhereItem:
		    /** Copy the whole where clause first **/
		    t = mq_internal_CopyLiteral(lxs, &where_cls->RawData);

		    /** We'll break the where clause out later.  Now should be end of SQL. **/
		    if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			}
		    else if (t < 0)
			{
			next_state = ParseError;
			mssError(1,"MQ","Unexpected end-of-query in WHERE clause");
			mlxNoteError(lxs);
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
		        next_state = LookForClause;
			mlxHoldToken(lxs);
			}
		    else
		    	{
		        next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query, GROUP BY, ORDER BY, or HAVING after WHERE clause");
			mlxNoteError(lxs);
			}
		    break;

		case HavingItem:
		    /** Copy the whole having clause first **/
		    t = mq_internal_CopyLiteral(lxs, &having_cls->RawData);

		    /** We'll break the having clause out later. **/
		    if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
		        next_state = ParseDone;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
		        next_state = LookForClause;
			mlxHoldToken(lxs);
			}
		    else
		    	{
		        next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or ORDER BY after HAVING clause");
			mlxNoteError(lxs);
			}
		    break;

		case UpdateItem:
		    new_qs = mq_internal_AllocQS(MQ_T_UPDATEITEM);
		    xaAddItem(&update_cls->Children, (void*)new_qs);
		    new_qs->Parent = update_cls;

		    t = mq_internal_ParseUpdateItem(new_qs, lxs);
		    if (t < 0)
			{
			next_state = ParseError;
			break;
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected update item, WHERE clause, or end-of-query after UPDATE");
			mlxNoteError(lxs);
			break;
			}
		    break;

		default:
		    /** This should not be reachable **/
		    next_state = ParseError;
		    mssError(1, "MQ", "Bark! Unhandled SQL parser state");
		    break;
		}

	    /** Set the next state **/
	    state = next_state;
	    }

	/** Error? **/
	if (state == ParseError)
	    {
	    mq_internal_FreeQS(qs);
	    mssError(0,"MQ","Could not parse multiquery");
	    return NULL;
	    }

	/** Ok, postprocess the expression trees, etc. **/
	if (mq_internal_PostProcess(stmt, qs, select_cls, from_cls, where_cls, orderby_cls, groupby_cls, crosstab_cls, update_cls, delete_cls, ondup_cls, declare_cls) < 0)
	    {
	    mq_internal_FreeQS(qs);
	    mssError(0,"MQ","Could not postprocess multiquery");
	    return NULL;
	    }

    return qs;
    }


/*** mq_internal_DumpQS - prints the QS tree out for debugging purposes.
 ***/
int
mq_internal_DumpQS(pQueryStructure tree, int level)
    {
    int i;

    	/** Print this item **/
	switch(tree->NodeType)
	    {
	    case MQ_T_QUERY: printf("%*.*sQUERY:\n",level*4,level*4,""); break;
	    case MQ_T_SELECTCLAUSE: printf("%*.*sSELECT:\n",level*4,level*4,""); break;
	    case MQ_T_FROMCLAUSE: printf("%*.*sFROM:\n",level*4,level*4,""); break;
	    case MQ_T_FROMSOURCE: printf("%*.*s%s (%s)\n",level*4,level*4,"",tree->Source,tree->Presentation); break;
	    case MQ_T_SELECTITEM: printf("%*.*s%s (%s)\n",level*4,level*4,"",tree->RawData.String,tree->Presentation); break;
	    case MQ_T_WHERECLAUSE: printf("%*.*sWHERE: %s\n",level*4,level*4,"",tree->RawData.String); break;
	    case MQ_T_UPDATECLAUSE: printf("%*.*sUPDATE:\n",level*4,level*4,""); break;
	    case MQ_T_UPDATEITEM: printf("%*.*s%s <-- %s\n",level*4,level*4,"",tree->Name,tree->RawData.String); break;
	    default: printf("%*.*sunknown\n",level*4,level*4,"");
	    }

	/** Print child items **/
	for(i=0;i<tree->Children.nItems;i++) 
	    mq_internal_DumpQS((pQueryStructure)(tree->Children.Items[i]),level+1);

    return 0;
    }


/*** mq_internal_DumpQE - print the QE tree out for debug purposes.
 ***/
int
mq_internal_DumpQE(pQueryElement tree, int level)
    {
    int i;

    	/** print the driver type handling this tree **/
	printf("%*.*sDRIVER=%s, CPTR=%8.8lx, NC=%d, FLAG=%d, COV=0x%X, DEP=0x%X, CCOV=0x%X",level*4,level*4,"",tree->Driver->Name,
	    (unsigned long)(tree->Children.Items),tree->Children.nItems,tree->Flags, tree->CoverageMask, tree->DependencyMask,
	    tree->Constraint?tree->Constraint->ObjCoverageMask:0);

	if (strncmp(tree->Driver->Name, "MQP", 3) == 0 && tree->QSLinkage)
	    printf(", IDX=%d, REF=%s, SRC=%s", tree->SrcIndex, ((pQueryStructure)tree->QSLinkage)->Presentation, ((pQueryStructure)tree->QSLinkage)->Source);

	printf("\n");

	/** print child items **/
	for(i=0;i<tree->Children.nItems;i++)
	    mq_internal_DumpQE((pQueryElement)(tree->Children.Items[i]), level+1);

    return 0;
    }


/*** mq_internal_DumpQEWithExpr - print the QE tree out for debug purposes.
 ***/
int
mq_internal_DumpQEWithExpr(pQueryElement tree, int level)
    {
    int i;

    	/** print the driver type handling this tree **/
	printf("%*.*sDRIVER=%s, CPTR=%8.8lx, NC=%d, FLAG=%d, COV=0x%X, DEP=0x%X, CCOV=0x%X",level*4,level*4,"",tree->Driver->Name,
	    (unsigned long)(tree->Children.Items),tree->Children.nItems,tree->Flags, tree->CoverageMask, tree->DependencyMask,
	    tree->Constraint?tree->Constraint->ObjCoverageMask:0);

	if (strncmp(tree->Driver->Name, "MQP", 3) == 0 && tree->QSLinkage)
	    printf(", IDX=%d, REF=%s, SRC=%s", tree->SrcIndex, ((pQueryStructure)tree->QSLinkage)->Presentation, ((pQueryStructure)tree->QSLinkage)->Source);

	printf("\n");
	if (tree->Constraint)
	    expDumpExpression(tree->Constraint);
	else
	    printf("(no constraint)\n");

	/** print child items **/
	for(i=0;i<tree->Children.nItems;i++)
	    mq_internal_DumpQEWithExpr((pQueryElement)(tree->Children.Items[i]), level+1);

    return 0;
    }


/*** mq_internal_FindItem_r - internal recursive version of the below.
 ***/
pQueryStructure
mq_internal_FindItem_r(pQueryStructure tree, int type, pQueryStructure next, int* found_next)
    {
    pQueryStructure qs;
    int i;

    	/** Is this structure itself it? **/
	if (tree->NodeType == type) 
	    {
	    if (tree == next) 
	        *found_next = 1;
	    else if (*found_next || !next)
	        return tree;
	    }

	/** Otherwise, try children. **/
	for(qs=NULL,i=0;i<tree->Children.nItems;i++)
	    {
	    qs = (pQueryStructure)(tree->Children.Items[i]);
	    qs = mq_internal_FindItem_r(qs,type,next, found_next);
	    if (qs) break;
	    }

    return qs;
    }


/*** mq_internal_FindItem - finds a query structure subtree within the
 *** main tree, where the type of the structure is the given type.
 ***/
pQueryStructure
mq_internal_FindItem(pQueryStructure tree, int type, pQueryStructure next)
    {
    int found_next = 0;
    return mq_internal_FindItem_r(tree, type, next, &found_next);
    }



/*** mq_internal_CkSetObjList - checks to see whether the object list serial
 *** id number is the same as the query's serial id number, and if not, sets
 *** the given object list as the active one for determining the query's src
 *** object set (for expr evaluation).
 ***/
int
mq_internal_CkSetObjList(pMultiQuery mq, pPseudoObject p)
    {

    	/** Check serial id # **/
	if (mq->CurSerial == p->Serial) return 0;

	/** Ok, need to update... **/
	/*expSyncSeqID(&(p->ObjList), mq->QTree->ObjList);*/
	memcpy(mq->ObjList, &p->ObjList, sizeof(ParamObjects));
	/*mq->QTree->ObjList->MainFlags |= EXPR_MO_RECALC;*/
	mq->CurSerial = p->Serial;

    return 1;
    }



/*** mq_internal_UpdateNotify() - a callback function that is used by the
 *** OSML's request-notify (Rn) mechanism to let the multiquery layer know
 *** that an attribute of an underlying object has changed.  In response, we 
 *** flag the object on the objlist as having changed, and generate an event to
 *** the OSML to let it know that the composite object has been modified - which
 *** possibly will result in Notifications (Rn-style) to whatever has the
 *** multi-query open.
 ***/
int
mq_internal_UpdateNotify(void* v)
    {
    pObjNotification n = (pObjNotification)v;
    pPseudoObject p = (pPseudoObject)(n->Context);
    int objid;
    pExpression exp;
    int i;

	/** We're about to update... sync the seq ids **/
	/*expSyncSeqID(&(p->ObjList), p->Query->QTree->ObjList);*/

	/** Track down the entry in the objlist **/
	objid = expObjChanged(p->ObjList, n->Obj);
	/*objid = expObjChanged(&(p->Query->QTree->ObjList), n->Obj);*/
	if (objid < 0) return -1;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Find attributes affected by it **/
	for(i=0;i<p->Stmt->Tree->AttrCompiledExpr.nItems;i++)
	    {
	    if (!strcmp(p->Stmt->Tree->AttrNames.Items[i], "*"))
		{
		objDriverAttrEvent(p->Obj, n->Name, NULL, 1);
		}
	    else
		{
		exp = (pExpression)(p->Stmt->Tree->AttrCompiledExpr.Items[i]);
		if (expContainsAttr(exp, objid, n->Name))
		    {
		    /** Got one.  Trigger an event on this. **/
		    objDriverAttrEvent(p->Obj, p->Stmt->Tree->AttrNames.Items[i], NULL, 1);
		    }
		}
	    }

    return 0;
    }


/*** mq_internal_FinishStatement() - issue the Finish() call to the underlying
 *** query modules for one statement.
 ***/
int
mq_internal_FinishStatement(pQueryStatement stmt)
    {
    pMultiQuery qy = stmt->Query;
    int i, n;

	/** Only if finish not already called for this one **/
	if (!(stmt->Flags & MQ_TF_FINISHED))
	    {
	    stmt->Flags |= MQ_TF_FINISHED;

	    /** Make sure the cur objlist is correct, otherwise the Finish
	     ** routines in the drivers might close up incorrect objects.
	     **/
	    /*if (qy->CurSerial != qy->CntSerial)
		{
		qy->CurSerial = qy->CntSerial;
		memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
		}*/

	    /** Shutdown the mq drivers.  This can implicitly modify the object list,
	     ** so we update the serial# and save the new object list
	     **/
	    if (stmt->Tree)
		{
		stmt->Tree->Driver->Finish(stmt->Tree,stmt);

		/*memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));*/
		qy->CntSerial = ++qy->CurSerial;
		}

	    /** Trim the object list back to the # of provided objects **/
	    n = stmt->Query->ObjList->nObjects;
	    for(i=stmt->Query->nProvidedObjects; i<n; i++)
		{
		expRemoveParamFromListById(stmt->Query->ObjList, i);
		}
	    }

    return 0;
    }



/*** mq_internal_FreeStatement() - release memory/resources for a statement.
 ***/
int
mq_internal_FreeStatement(pQueryStatement stmt)
    {

	/** Free the expressions **/
	if (stmt->WhereClause && !(stmt->WhereClause->Flags & EXPR_F_CVNODE)) 
	    expFreeExpression(stmt->WhereClause);
	stmt->WhereClause = NULL;
	if (stmt->HavingClause) 
	    expFreeExpression(stmt->HavingClause);
	stmt->HavingClause = NULL;

	/** Free the query-exec tree. **/
	if (stmt->Tree)
	    mq_internal_FreeQE(stmt->Tree);
	stmt->Tree = NULL;

	/** Free the query-structure tree. **/
	if (stmt->QTree)
	    mq_internal_FreeQS(stmt->QTree);
	stmt->QTree = NULL;

	/** Release the objlist for the having clause **/
	if (stmt->OneObjList)
	    expFreeParamList(stmt->OneObjList);
	stmt->OneObjList = NULL;

	/** List of query execution nodes **/
	xaDeInit(&stmt->Trees);

	/** Multistatement mode disabled? **/
	if (!(stmt->Query->Flags & MQ_F_MULTISTATEMENT))
	    stmt->Query->Flags |= MQ_F_ENDOFSQL;

	/** Free the statement itself **/
	nmFree(stmt, sizeof(QueryStatement));

    return 0;
    }



/*** mq_internal_CloseStatement() - clean up after running one statement.
 ***/
int
mq_internal_CloseStatement(pQueryStatement stmt)
    {

	/** Check link cnt - don't free the structure out from under someone... **/
	stmt->LinkCnt--;
	if (stmt->LinkCnt > 0) return 0;

	/** Issue Finish() if last unlink **/
	mq_internal_FinishStatement(stmt);

	/** Release it **/
	mq_internal_FreeStatement(stmt);

    return 0;
    }



/*** mq_internal_NextStatement() - read the next SQL statement in from the
 *** query text, parse it, and start it.  Returns 1 on success, 0 if no
 *** more queries in the sql, and < 0 on an error condition.
 ***/
int
mq_internal_NextStatement(pMultiQuery this)
    {
    pQueryStructure qs, select_qs, sub_qs, update_qs;
    char* exp;
    int i;
    int t;
    pQueryDriver qdrv;
    pQueryStatement stmt = NULL;

	/** End of statement? **/
	if (this->Flags & MQ_F_ENDOFSQL)
	    return 0;

	/** Create new statement structure **/
	stmt = (pQueryStatement)nmMalloc(sizeof(QueryStatement));
	if (!stmt) goto error;
	memset(stmt, 0, sizeof(QueryStatement));
	stmt->LinkCnt = 1;
	stmt->Query = this;
	this->CurStmt = stmt;
	xaInit(&stmt->Trees, 16);
	stmt->IterCnt = -1;
	stmt->UserIterCnt = 0;
	stmt->Flags = MQ_TF_FINISHED;

	this->QueryCnt++;

	/** Build the one-object list for evaluating the Having clause, etc. **/
	stmt->OneObjList = expCreateParamList();
	expCopyList(this->ObjList, stmt->OneObjList, this->nProvidedObjects);
	stmt->OneObjList->Session = stmt->Query->SessionID;
	expAddParamToList(stmt->OneObjList, "this", NULL, EXPR_O_CURRENT | EXPR_O_REPLACE);
	expSetParamFunctions(stmt->OneObjList, "this", mqGetAttrType, mqGetAttrValue, mqSetAttrValue);

	/** Parse the query **/
	stmt->QTree = mq_internal_SyntaxParse(this->LexerSession, stmt);
	if (!stmt->QTree)
	    {
	    mssError(0,"MQ","Could not analyze query text");
	    goto error;
	    }
	/*mq_internal_DumpQS(this->QTree,0);*/

	/** Got a semicolon or end-of-text?  Remember it for the next call. **/
	t = mlxNextToken(this->LexerSession);
	if (t == MLX_TOK_EOF || t == MLX_TOK_ERROR) 
	    this->Flags |= MQ_F_ENDOFSQL;
	else if (t != MLX_TOK_SEMICOLON)
	    {
	    mssError(1, "MQ", "Unexpected tokens after end of SQL statement");
	    goto error;
	    }

	/** Are we doing this "for update"? **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	if (qs && qs->Flags & MQ_SF_FORUPDATE)
	    stmt->Flags |= MQ_TF_ALLOWUPDATE;

	/** Ok, got syntax.  Find the where clause. **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_WHERECLAUSE, NULL);
	if (!qs)
	    exp = "1";
	else
	    exp = qs->RawData.String;
	stmt->WhereClause = expCompileExpression(exp,this->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_OUTERJOIN);
	if (!stmt->WhereClause)
	    {
	    mssError(0,"MQ","Error in query's WHERE clause");
	    goto error;
	    }
	for(i=0;i<this->nProvidedObjects;i++)
	    {
	    if (expFreezeOne(stmt->WhereClause, this->ObjList, i) < 0)
		{
		mssError(0, "MQ", "Error evaluating query's WHERE clause");
		goto error;
		}
	    }

	if (qs)
	    {
	    /** Convert the where clause OR elements to UNION elements **/
	    mq_internal_ProcessWhere(stmt->WhereClause, stmt->WhereClause, &(stmt->QTree));

	    /** Break the where clause into tiny little chunks **/
	    select_qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	    update_qs = mq_internal_FindItem(stmt->QTree, MQ_T_UPDATECLAUSE, NULL);
	    mq_internal_DetermineCoverage(stmt, stmt->WhereClause, qs, select_qs, update_qs, 0);
	    if (stmt->WhereClause->Flags & EXPR_F_CVNODE) stmt->WhereClause = NULL;

	    /** Optimize the "little chunks" **/
	    for(i=0;i<qs->Children.nItems;i++)
	        {
		sub_qs = (pQueryStructure)(qs->Children.Items[i]);
		mq_internal_OptimizeExpr(&(sub_qs->Expr));
		}
	    }

	/** Compile the having expression, if one. **/
	stmt->HavingClause = NULL;
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_HAVINGCLAUSE, NULL);
	if (qs)
	    {
	    qs->Expr = expCompileExpression(qs->RawData.String, this->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_LATEBIND);
	    if (!qs->Expr)
	        {
		mssError(0,"MQ","Error in query's HAVING clause");
		goto error;
		}
	    stmt->HavingClause = qs->Expr;
	    qs->Expr = NULL;
	    }

	/** Limit clause? **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_LIMITCLAUSE, NULL);
	if (qs)
	    {
	    stmt->LimitStart = qs->IntVals[0];
	    stmt->LimitCnt = qs->IntVals[1];
	    }
	else
	    {
	    stmt->LimitStart = 0;
	    stmt->LimitCnt = 0x7FFFFFFF;
	    }

	/** Ok, got the from, select, and where built.  Now call the mq-drivers **/
	for(i=0;i<MQINF.Drivers.nItems;i++)
	    {
	    qdrv = (pQueryDriver)(MQINF.Drivers.Items[i]);
	    if (qdrv->Analyze(stmt) < 0)
	        {
		goto error;
		}
	    }
	//mq_internal_DumpQEWithExpr(stmt->Tree, 0);

	/** Just did a declaration, print, or log? **/
	if (!stmt->Tree && (stmt->Flags & MQ_TF_IMMEDIATE))
	    {
	    mq_internal_CloseStatement(stmt);
	    this->CurStmt = NULL;
	    if (thExcessiveRecursion())
		goto error;
	    else
		return mq_internal_NextStatement(this);
	    }

	/** General failure to parse... **/
	if (!stmt->Tree)
	    {
	    mssError(0,"MQ","Error in SQL statement syntax");
	    goto error;
	    }

	/** Build dependency mask **/
	mq_internal_SetCoverage(stmt, stmt->Tree);
	mq_internal_SetDependencies(stmt, stmt->Tree, NULL);

	/** Query plan diagnostics? **/
	if (stmt->Query->Flags & MQ_F_SHOWPLAN)
	    {
	    mq_internal_DumpQE(stmt->Tree, 0);
	    }

	/** Have the MQ drivers start the query. **/
	if (stmt->Tree->Driver->Start(stmt->Tree, stmt, NULL) < 0)
	    {
	    mssError(0,"MQ","Could not start the query");
	    goto error;
	    }

	/** Query is now "running" **/
	stmt->Flags &= ~MQ_TF_FINISHED;

	/** Issuing a Start can implicitly modify the object list, so
	 ** we save the new copy
	 **/
	/*memcpy(&this->CurObjList, this->ObjList, sizeof(ParamObjects));*/
	this->CntSerial = ++this->CurSerial;

	return 1;

    error:
	if (stmt) mq_internal_FreeStatement(stmt);
	this->CurStmt = NULL;
	return -1;
    }


/*** mq_internal_FinalizeAppData - this is called when the application context
 *** closes and we need to release any application-scope objects or collections.
 ***/
int
mq_internal_FinalizeAppData(void* appdata_v)
    {
    pQueryAppData appdata = (pQueryAppData)appdata_v;
    int i;
    pQueryDeclaredObject qdo;
    pQueryDeclaredCollection qdc;

	/** Free the objects **/
	for(i=0; i<appdata->DeclaredObjects.nItems; i++)
	    {
	    qdo = (pQueryDeclaredObject)appdata->DeclaredObjects.Items[i];
	    stFreeInf(qdo->Data);
	    nmFree(qdo, sizeof(QueryDeclaredObject));
	    }
	xaDeInit(&appdata->DeclaredObjects);

	/** Free collections **/
	for(i=0; i<appdata->DeclaredCollections.nItems; i++)
	    {
	    qdc = (pQueryDeclaredCollection)appdata->DeclaredCollections.Items[i];
	    objDeleteTempObject(qdc->Collection);
	    nmFree(qdc, sizeof(QueryDeclaredCollection));
	    }
	xaDeInit(&appdata->DeclaredCollections);

	/** Free the appdata structure itself **/
	nmFree(appdata, sizeof(QueryAppData));

    return 0;
    }


/*** mqStartQuery - starts a new MultiQuery.  The function of this routine
 *** is similar to the objOpenQuery; but this is called from objMultiQuery,
 *** and is not associated with any given object when it is opened.  Returns
 *** a pointer to a MultiQuery structure.
 ***/
void*
mqStartQuery(pObjSession session, char* query_text, pParamObjects objlist, int flags)
    {
    pMultiQuery this;
    pQueryAppData appdata;
    int i;

	/** Ensure the application-scope data is initialized **/
	appdata = appLookupAppData("MQ:appdata");
	if (!appdata)
	    {
	    appdata = (pQueryAppData)nmMalloc(sizeof(QueryAppData));
	    if (!appdata)
		goto error;
	    xaInit(&appdata->DeclaredObjects, 8);
	    xaInit(&appdata->DeclaredCollections, 8);
	    appRegisterAppData("MQ:appdata", appdata, mq_internal_FinalizeAppData);
	    }

    	/** Allocate the multiquery structure itself. **/
	this = (pMultiQuery)nmMalloc(sizeof(MultiQuery));
	/*printf("ALLOC %s\n", query_text);*/
	if (!this) return NULL;
	memset(this,0,sizeof(MultiQuery));
	this->CntSerial = 0;
	this->CurSerial = 0;
	this->SessionID = session;
	this->LinkCnt = 1;
	this->ObjList = NULL;
	this->nProvidedObjects = 0;
	this->QueryText = nmSysStrdup(query_text);
	this->RowCnt = 0;
	this->QueryCnt = 0;
	this->YieldMsec = 0;
	this->StartMsec = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	xaInit(&this->DeclaredObjects, 8);
	xaInit(&this->DeclaredCollections, 8);
	if (flags & OBJ_MQ_F_ONESTATEMENT)
	    {
	    this->Flags = MQ_F_ONESTATEMENT; /* multi statements disabled, cannot be enabled */
	    }
	else
	    {
	    this->Flags = MQ_F_MULTISTATEMENT; /* on by default, can be turned on/off */
	    }
	if (flags & OBJ_MQ_F_NOUPDATE)
	    this->Flags |= MQ_F_NOUPDATE;

	/** Parse the text of the query, building the syntax structure **/
	this->LexerSession = mlxStringSession(this->QueryText, 
		MLX_F_CCOMM | MLX_F_DASHCOMM | MLX_F_ICASER | MLX_F_FILENAMES | MLX_F_EOF);
	if (!this->LexerSession)
	    {
	    mssError(0,"MQ","Could not begin analysis of query text");
	    goto error;
	    }

	/** Import any externally provided data sources **/
	this->ObjList = expCreateParamList();
	if (objlist)
	    {
	    expCopyList(objlist, this->ObjList, -1);
	    /*expLinkParams(this->ObjList, 0, -1);*/
	    this->nProvidedObjects = this->ObjList->nObjects;
	    }
	this->ObjList->Session = this->SessionID;

	/** Import any declared objects at the application scope **/
	for(i=0; i<appdata->DeclaredObjects.nItems; i++)
	    {
	    /** Ignore failure to import (return == -1) due to scope shadowing **/
	    mq_internal_AddDeclaredObject(this, (pQueryDeclaredObject)appdata->DeclaredObjects.Items[i]);
	    }

	/** Add the __inserted object **/
	if (expLookupParam(this->ObjList, "__inserted", 0) < 0 && expAddParamToList(this->ObjList, "__inserted", NULL, 0) >= 0)
	    this->nProvidedObjects++;
	else
	    this->Flags |= MQ_F_NOINSERTED;

	this->ProvidedObjMask = (1<<(this->nProvidedObjects)) - 1;

	/** Parse one SQL statement **/
	if (mq_internal_NextStatement(this) != 1)
	    goto error;

	return (void*)this;

    error:
	mq_internal_QueryClose(this, NULL);
	return NULL;
    }



/*** The following are OSDriver functions.  The MQ Module implements a part
 *** of the ObjectSystem Driver interface.
 ***/

/*** mqClose - closes a pseudo-object that was open as a result of a fetch
 *** operation on the multiquery object or query.
 ***/
int
mqClose(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject inf = (pPseudoObject)inf_v;
    int n;
    pMultiQuery mq;

    	/** Close the query **/
	n = inf->Stmt->Query->nProvidedObjects;
	mq = inf->Stmt->Query;
	mq_internal_CloseStatement(inf->Stmt);
	mq_internal_QueryClose(mq, oxt);

	/** Release all objects in our param list **/
	/*for(i=n;i<inf->ObjList.nObjects;i++) if (inf->ObjList.Objects[i])
	    {
	    objRequestNotify(obj, mq_internal_UpdateNotify, inf, 0);
	    }*/
	expUnlinkParams(inf->ObjList, n, -1);
	expFreeParamList(inf->ObjList);

	/** Release the pseudo-object **/
	nmFree(inf, sizeof(PseudoObject));

    return 0;
    }


/*** Return the object id of the IDENTITY object or assumed IDENTITY object
 *** Or, return -1 if not found.
 ***/
int
mq_internal_GetIdentObjId(pPseudoObject p)
    {
    int objid = 0;
    pQueryStructure from_qs;
    int n;

	objid = -1;
	n = p->ObjList->nObjects - p->Stmt->Query->nProvidedObjects;

	/** No objects?  Then no identity object. **/
	if (n == 0)
	    {
	    return -1;
	    }
	else if (n > 1)
	    {
	    from_qs = NULL;
	    while((from_qs = mq_internal_FindItem(p->Stmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
		{
		if (from_qs->QELinkage && from_qs->QELinkage->SrcIndex >= 0 && (from_qs->Flags & MQ_SF_IDENTITY) && p->ObjList->Objects[from_qs->QELinkage->SrcIndex])
		    {
		    objid = from_qs->QELinkage->SrcIndex;
		    break;
		    }
		}
	    }
	else if (n == 1)
	    {
	    objid = p->Stmt->Query->nProvidedObjects;
	    }

    return objid;
    }


/*** mqDeleteObj - delete an open query result item.  This only
 *** works on queries off of a single source.
 ***/
int
mqDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject inf = (pPseudoObject)inf_v;
    int rval;
    int objid;

	/** Do a 'delete obj' operation on the source object **/
	objid = mq_internal_GetIdentObjId(inf);
	if (objid < 0)
	    {
	    mssError(1,"MQ","Could not delete - the query must have one valid FROM source labled as the query IDENTITY source");
	    return -1;
	    }
	rval = objDeleteObj((pObject)(inf->ObjList->Objects[objid]));
	inf->ObjList->Objects[objid] = NULL;
	if (rval < 0) return rval;
	
    return mqClose(inf_v, oxt);
    }


pPseudoObject
mq_internal_CreatePseudoObject(pMultiQuery qy, pObject hl_obj)
    {
    pPseudoObject p;

	/** Allocate the pseudo-object. **/
	p = (pPseudoObject)nmMalloc(sizeof(PseudoObject));
	if (!p) return NULL;
	p->Stmt = qy->CurStmt;
	p->Obj = hl_obj;
	qy->LinkCnt++;
	qy->CurStmt->LinkCnt++;
	p->ObjList = expCreateParamList();
	p->Offset = 0;

	/** Record the Counters **/
	p->QueryID = qy->QueryCnt - 1;
	p->RowIDAllQuery = qy->RowCnt;
	p->RowIDThisQuery = qy->CurStmt->UserIterCnt;
	p->RowIDBeforeLimit = qy->CurStmt->IterCnt + 1;

	/** Copy the object list and link to the objects.
	 ** We don't link to objects for the qy->CurObjList since that
	 ** objlist normally shadows QTree->ObjList, except when the
	 ** user skips back to previous objects temporarily and then
	 ** goes and does another fetch.
	 **/
	/*memcpy(&p->ObjList, qy->ObjList, sizeof(ParamObjects));
	memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));
	for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList.nObjects;i++) 
	    {
	    obj = (pObject)(p->ObjList.Objects[i]);
	    if (obj) objLinkTo(obj);
	    }*/
	expCopyList(qy->ObjList, p->ObjList, -1);
	expLinkParams(p->ObjList, p->Stmt->Query->nProvidedObjects, -1);

	p->Serial = qy->CurSerial;

    return p;
    }


int
mq_internal_FreePseudoObject(pPseudoObject p)
    {
    pMultiQuery qy = p->Stmt->Query;

	mq_internal_CloseStatement(p->Stmt); /* unlink */
	/*qy->LinkCnt--;*/
	mq_internal_QueryClose(qy, NULL); /* unlink */

	expUnlinkParams(p->ObjList, qy->nProvidedObjects, -1);
	expFreeParamList(p->ObjList);

	nmFree(p, sizeof(PseudoObject));

    return 0;
    }


/*** return 1 if LIMIT matches, 0 if LIMIT does not match, or -1 if error.
 ***/
int
mq_internal_EvalLimitClause(pQueryStatement stmt, pPseudoObject p)
    {
    return stmt->IterCnt >= stmt->LimitStart && stmt->UserIterCnt < stmt->LimitCnt;
    }


/*** return 1 if HAVING matches, 0 if HAVING does not match, or -1 if error.
 ***/
int
mq_internal_EvalHavingClause(pQueryStatement stmt, pPseudoObject p)
    {
    pPseudoObject our_p = p;
    int rval;
    pParamObjects having_objlist;

	/** No having clause? **/
	if (!stmt->HavingClause)
	    return 1;

	/** Caller does not have a pseudo-object context?  If not,
	 ** just use the current values.
	 **/
	if (!our_p)
	    our_p = mq_internal_CreatePseudoObject(stmt->Query, NULL);

	/** Create our param list for having clause evaluation **/
	having_objlist = expCreateParamList();
	expCopyList(stmt->Query->ObjList, having_objlist, -1);
	having_objlist->Session = stmt->Query->SessionID;
	expAddParamToList(having_objlist, "this", (void*)our_p, EXPR_O_CURRENT | EXPR_O_REPLACE);
	expSetParamFunctions(having_objlist, "this", mqGetAttrType, mqGetAttrValue, mqSetAttrValue);

	/** Do late binding, and evaluate it **/
	/*expModifyParam(having_objlist, "this", (void*)our_p);*/
	expBindExpression(stmt->HavingClause, having_objlist, 0);
	rval = expEvalTree(stmt->HavingClause, having_objlist);

	/** Release the parameter list **/
	expFreeParamList(having_objlist);

	/** Determine our results **/
	if (p != our_p)
	    mq_internal_FreePseudoObject(our_p);
	if (rval < 0)
	    {
	    mssError(1,"MQ","Could not evaluate HAVING clause");
	    return -1;
	    }
	if (stmt->HavingClause->DataType != DATA_T_INTEGER)
	    {
	    mssError(1,"MQ","HAVING clause must evaluate to boolean or integer");
	    return -1;
	    }
	if (!(stmt->HavingClause->Flags & EXPR_F_NULL) && stmt->HavingClause->Integer != 0) 
	    {
	    return 1;
	    }

    return 0;
    }


/*** mq_internal_HandleAssignments - perform SELECT item assignments.
 ***/
int
mq_internal_HandleAssignments(pMultiQuery qy, pPseudoObject p)
    {
    pQueryStructure select_qs, item_qs;
    int i,j;
    int t;
    ObjData val;
    pQueryDeclaredObject qdo;
    pQueryDeclaredObject found_qdo;
    pStructInf attr;
    int rval;
    pQueryAppData appdata;

	/** Find the select items **/
	select_qs = mq_internal_FindItem(qy->CurStmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	if (select_qs)
	    {
	    for(i=0;i<select_qs->Children.nItems;i++)
		{
		item_qs = (pQueryStructure)select_qs->Children.Items[i];
		if (item_qs->Flags & MQ_SF_ASSIGNMENT)
		    {
		    /** Get the type and value **/
		    t = mqGetAttrType(p, item_qs->Presentation, NULL);
		    if (t > 0)
			{
			rval = mqGetAttrValue(p, item_qs->Presentation, t, &val, NULL);
			if (rval >= 0)
			    {
			    /** Got type and value, now find it in the StructInf **/
			    found_qdo = NULL;
			    for(j=0;j<qy->DeclaredObjects.nItems;j++)
				{
				qdo = (pQueryDeclaredObject)qy->DeclaredObjects.Items[j];
				if (!strcmp(item_qs->Name, qdo->Name))
				    {
				    found_qdo = qdo;
				    break;
				    }
				}
			    if (!found_qdo)
				{
				appdata = appLookupAppData("MQ:appdata");
				if (appdata)
				    {
				    for(j=0;j<appdata->DeclaredObjects.nItems;j++)
					{
					qdo = (pQueryDeclaredObject)appdata->DeclaredObjects.Items[j];
					if (!strcmp(item_qs->Name, qdo->Name))
					    {
					    found_qdo = qdo;
					    break;
					    }
					}
				    }
				}

			    if (found_qdo)
				{
				/** Existing or created attribute? **/
				attr = stLookup(found_qdo->Data, item_qs->Presentation);
				if (!attr && rval == 0)
				    attr = stAddAttr(found_qdo->Data, item_qs->Presentation);

				/** set it or delete it **/
				if (attr && rval == 0)
				    stSetAttrValue(attr, t, &val, 0);
				else if (attr && rval == 1)
				    stRemoveInf(attr);
				}
			    else
				{
				mssError(1, "MQ", "Undeclared object '%s'", item_qs->Name);
				return -1;
				}
			    }
			}
		    }
		}
	    }

    return 0;
    }


/*** mqQueryFetch - retrieves the next item in the query result set.
 ***/
void*
mqQueryFetch(void* qy_v, pObject highlevel_obj, int mode, pObjTrxTree* oxt)
    {
    pPseudoObject p;
    pMultiQuery qy = (pMultiQuery)qy_v;
    int i, rval;
    //pObject obj;

	/** Make sure the cur objlist is correct **/
	/*if (qy->CurSerial != qy->CntSerial)
	    {
	    qy->CurSerial = qy->CntSerial;
	    memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
	    }*/

	if (!qy->CurStmt) return NULL;

	/** Search for results matching the HAVING clause **/
	while(1)
	    {
	    /** We're about to modify the objlist implicitly - bump the
	     ** counter so it doesn't match any existing pseudo-objects.
	     **/
	    qy->CurSerial++;

    	    /** Try to fetch the next record. **/
	    if (qy->CurStmt->UserIterCnt < qy->CurStmt->LimitCnt)
		{
		rval = qy->CurStmt->Tree->Driver->NextItem(qy->CurStmt->Tree, qy->CurStmt);
		/*memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));*/
		}
	    else
		{
		rval = 0; /* limit-end of results, don't fetch */
		}
	    qy->CntSerial = qy->CurSerial;

	    if (rval != 1)
	        {
		if (rval == 0)
		    {
		    /** End of results from that statement - go to next statement **/
		    mq_internal_FinishStatement(qy->CurStmt);
		    mq_internal_CloseStatement(qy->CurStmt);
		    qy->CurStmt = NULL;
		    rval = mq_internal_NextStatement(qy);
		    if (rval != 1)
			{
			return NULL;
			}
		    continue;
		    /*rval = qy->CurStmt->Tree->Driver->NextItem(qy->CurStmt->Tree, qy->CurStmt);*/
		    }

		if (rval == -1)
		    {
		    return NULL;
		    }
		/*if (rval != 1)
		    {
		    memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));
		    qy->CntSerial = qy->CurSerial;
		    return NULL;
		    }*/
	        }

	    /** Create the pseudo object **/
	    p = mq_internal_CreatePseudoObject(qy, highlevel_obj);

	    /** Update row serial # **/
	    qy->CntSerial = qy->CurSerial;

	    /** Verify HAVING clause **/
	    rval = mq_internal_EvalHavingClause(qy->CurStmt, p);
	    if (rval == 1)
		{
		/** matched **/
		}
	    else if (rval == -1)
		{
		/** error **/
		mq_internal_FreePseudoObject(p);
		mssError(1,"MQ","Could not evaluate HAVING clause");
		return NULL;
		}
	    else
	        {
		/** no match, go get another row **/
		mq_internal_FreePseudoObject(p);
		p = NULL;
		continue;
		}

	    qy->CurStmt->IterCnt++;

	    /** Apply LIMIT clause **/
	    rval = mq_internal_EvalLimitClause(qy->CurStmt, p);
	    if (rval == 1)
		{
		/** matched **/
		}
	    else if (rval == -1)
		{
		/** error **/
		mq_internal_FreePseudoObject(p);
		mssError(1,"MQ","Could not evaluate LIMIT clause");
		return NULL;
		}
	    else
		{
		/** no match, go get another row **/
		mq_internal_FreePseudoObject(p);
		p = NULL;
		continue;
		}

	    /** Check assignments **/
	    if (qy->CurStmt->Flags & MQ_TF_ONEASSIGN)
		{
		/** Do any needed assignments **/
		if (mq_internal_HandleAssignments(qy, p) < 0)
		    {
		    mq_internal_FreePseudoObject(p);
		    p = NULL;
		    return NULL;
		    }

		/** If all select items are assignments, don't return the row at all,
		 ** but still include it in the LIMIT iteration count.
		 **/
		if (qy->CurStmt->Flags & MQ_TF_ALLASSIGN)
		    {
		    qy->CurStmt->UserIterCnt++;
		    mq_internal_FreePseudoObject(p);
		    p = NULL;
		    continue;
		    }
		}

	    break;
	    }

	/** If we have a pseudo-object, request update-notifies on its parts **/
	if (p)
	    {
	    for(i=qy->nProvidedObjects;i<p->ObjList->nObjects;i++) 
	        {
	        //obj = (pObject)(p->ObjList->Objects[i]);
		/*if (obj) 
		    objRequestNotify(obj, mq_internal_UpdateNotify, p, OBJ_RN_F_ATTRIB);*/
		}
	    }

	qy->RowCnt++;
	qy->CurStmt->UserIterCnt++;

    return (void*)p;
    }


/*** mqQueryCreate - create a new object in the context of a running query;
 *** this requires object creation at one or more underlying levels depending
 *** on the nature of the query's joins and so forth.
 ***/
void*
mqQueryCreate(void* qy_v, pObject new_obj, char* name, int mode, int permission_mask, pObjTrxTree *oxt)
    {
    pPseudoObject p = NULL;
    pMultiQuery qy = (pMultiQuery)qy_v;

	/** For now, we just fail if this involves a join operation. **/
	if (qy->ObjList->nObjects - qy->nProvidedObjects > 1)
	    {
	    mssError(1,"MQ","Bark! QueryCreate() on a multi-source query is not yet supported.");
	    return NULL;
	    }

    // TODO Implement the real return value!
    return (void*)p;
    }


/*** mqQueryClose - closes an open query and dismantles the projection and
 *** join structures in the multiquery query tree.
 ***/
int
mqQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pMultiQuery qy = (pMultiQuery)qy_v;

	/** Make sure the cur objlist is correct, otherwise the Finish
	 ** routines in the drivers might close up incorrect objects.
	 **/
	/*if (qy->CurSerial != qy->CntSerial)
	    {
	    qy->CurSerial = qy->CntSerial;
	    memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
	    }*/
	qy->CurSerial = (++qy->CntSerial);

	/** No more results possible after QueryClose(), so we call
	 ** Finish() on the statement.
	 **/
	if (qy->CurStmt)
	    mq_internal_FinishStatement(qy->CurStmt);

	/** Shutdown the query structure **/
	mq_internal_QueryClose(qy, oxt);

    return 0;
    }


/*** mq_internal_QueryClose() - close the query; may be called by
 *** mqQueryClose or by mqClose.
 ***/
int
mq_internal_QueryClose(pMultiQuery qy, pObjTrxTree* oxt)
    {
    int i, id;
    pQueryDeclaredObject qdo;
    pQueryDeclaredCollection qdc;

    	/** Check the link cnt **/
	if ((--qy->LinkCnt) > 0) return 0;

	/** Release resources used by the one SQL statement **/
	if (qy->CurStmt)
	    mq_internal_CloseStatement(qy->CurStmt);

	/** Close an __inserted object **/
	if (qy->ObjList && !(qy->Flags & MQ_F_NOINSERTED) && (id = expLookupParam(qy->ObjList, "__inserted", 0)) >= 0)
	    {
	    if (qy->ObjList->Objects[id])
		{
		objClose(qy->ObjList->Objects[id]);
		qy->ObjList->Objects[id] = NULL;
		}
	    }

	/** Release the object list for the main query **/
	if (qy->ObjList)
	    {
	    /*expUnlinkParams(qy->ObjList, 0, -1);*/
	    expFreeParamList(qy->ObjList);
	    }

	/*printf("CLOSE %s\n", qy->QueryText);*/
	if (qy->QueryText)
	    nmSysFree(qy->QueryText);

	if (qy->LexerSession)
	    mlxCloseSession(qy->LexerSession);

	/** Declared objects and collections **/
	for(i=0;i<qy->DeclaredObjects.nItems;i++)
	    {
	    qdo = (pQueryDeclaredObject)(qy->DeclaredObjects.Items[i]);
	    stFreeInf(qdo->Data);
	    nmFree(qdo, sizeof(QueryDeclaredObject));
	    }
	xaDeInit(&qy->DeclaredObjects);
	for(i=0;i<qy->DeclaredCollections.nItems;i++)
	    {
	    qdc = (pQueryDeclaredCollection)(qy->DeclaredCollections.Items[i]);
	    objDeleteTempObject(qdc->Collection);
	    nmFree(qdc, sizeof(QueryDeclaredCollection));
	    }
	xaDeInit(&qy->DeclaredCollections);

	/** Free the qy itself **/
	nmFree(qy,sizeof(MultiQuery));

    return 0;
    }


/*** mqRead - reads from the object.  We do this simply by trying the 
 *** objRead functionality in the objects comprising the query, in order of
 *** specificity (childmost object first; object on many side of a one-to
 *** many relationship first, as determined by the structure of the join
 *** clause).
 ***/
int
mqRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int objid;
    char* content;
    int n;

	/** If an "objcontent" attribute is explicitly SELECTed... **/
	if (p->Stmt->Flags & MQ_TF_OBJCONTENT)
	    {
	    /** "Read" the content from an attribute **/
	    if (mqGetAttrValue(inf_v, "objcontent", DATA_T_STRING, POD(&content), NULL) == 0)
		{
		if (flags & OBJ_U_SEEK)
		    p->Offset = offset;
		if (strlen(content) <= p->Offset)
		    {
		    n = 0;
		    }
		else
		    {
		    n = strlen(content) - p->Offset;
		    }
		if (n > maxcnt)
		    n = maxcnt;
		memcpy(buffer, content + p->Offset, n);
		p->Offset += n;
		return n;
		}
	    else
		{
		return -1;
		}
	    }
	
	/** Read the content from a FROM source **/
	objid = mq_internal_GetIdentObjId(p);
	if (objid < 0)
	    {
	    mssError(1,"MQ","Could not read - the query must have one valid FROM source labled as the query IDENTITY source");
	    return -1;
	    }
	if (!p->ObjList->Objects[objid])
	    {
	    /** Underlying object isn't currently valid **/
	    mssError(1,"MQ","Could not read - underlying data source not available");
	    return -1;
	    }

    return objRead((pObject)(p->ObjList->Objects[objid]), buffer, maxcnt, offset, flags);
    }


/*** mqWrite - writes to the pseudo-object.  Works like the above, except that
 *** the query must explicitly allow FOR UPDATE.
 ***/
int
mqWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int objid;

	objid = mq_internal_GetIdentObjId(p);
	if (objid < 0)
	    {
	    mssError(1,"MQ","Could not write - the query must have one valid FROM source labled as the query IDENTITY source");
	    return -1;
	    }
	if (!p->ObjList->Objects[objid])
	    {
	    /** Object isn't currently valid - return empty. **/
	    mssError(1,"MQ","Could not write - underlying data source not available");
	    return -1;
	    }

    return objWrite((pObject)(p->ObjList->Objects[objid]), buffer, cnt, offset, flags);
    }


/*** mqGetAttrType - retrieves the data type of an attribute.  Returns DATA_T_xxx
 *** values (from obj.h).
 ***/
int
mqGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int id=-1,i,dt;
    int any_dt;

    	/** Request for ls__rowid? **/
	if (!strcmp(attrname,"ls__rowid") || !strcmp(attrname,"cx__rowid") ||
	    !strcmp(attrname,"cx__queryid") || !strcmp(attrname,"cx__rowid_one_query") ||
	    !strcmp(attrname,"cx__rowid_before_limit")) return DATA_T_INTEGER;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	any_dt = 0;
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    dt = objGetAttrType(p->ObjList->Objects[i], attrname);
		    if (dt > 0)
			return dt;

		    /** source doesn't know the attribute, but might accept
		     ** a setattr on it?
		     **/
		    if (dt == DATA_T_UNAVAILABLE)
			any_dt = 1;
		    }
		}
	    }

	/** Didn't find it? **/
	if (id == -1) 
	    {
	    /** Return 'unavailable' if a source indicated that it might
	     ** accept a new/non-null attribute.
	     **/
	    if (any_dt)
		return DATA_T_UNAVAILABLE;

	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
	        return -1;
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expression to get the data type **/
	if (expEvalTree((pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id],p->ObjList) < 0)
	    return -1;
	dt = ((pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id])->DataType;

    return dt;
    }


/*** mqGetAttrValue - retrieves the value of an attribute.  See the object system
 *** documentation on the appropriate pointer types for *value.
 ***
 *** Returns: -1 on error, 0 on success, and 1 if successful but value was NULL.
 ***/
int
mqGetAttrValue(void* inf_v, char* attrname, int datatype, void* value, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int id=-1,i, rval;
    pExpression exp;
    pQueryStructure from_qs;

    	/** Request for row id? **/
	if (!strcmp(attrname,"ls__rowid") || !strcmp(attrname,"cx__rowid") ||
	    !strcmp(attrname,"cx__queryid") || !strcmp(attrname,"cx__rowid_one_query") ||
	    !strcmp(attrname,"cx__rowid_before_limit"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"MQ","Type mismatch getting attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    if (!strcmp(attrname,"ls__rowid"))
		*((int*)value) = p->Serial;
	    else if (!strcmp(attrname,"cx__rowid"))
		*((int*)value) = p->RowIDAllQuery;
	    else if (!strcmp(attrname,"cx__queryid"))
		*((int*)value) = p->QueryID;
	    else if (!strcmp(attrname,"cx__rowid_one_query"))
		*((int*)value) = p->RowIDThisQuery;
	    else if (!strcmp(attrname,"cx__rowid_before_limit"))
		*((int*)value) = p->RowIDBeforeLimit;
	    return 0;
	    }

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    rval = objGetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
		    if (rval >= 0)
			return rval;
		    }
		}
	    }
	    
	if (id == -1)
	    {
	    /** Suppress the error message on certain attrs **/
	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
		{
		/** for 'system' attrs, try to find 'identity' source **/
		from_qs = NULL;
		i = -1;
		while((from_qs = mq_internal_FindItem(p->Stmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
		    {
		    if (from_qs->QELinkage && from_qs->QELinkage->SrcIndex >= 0 && (from_qs->Flags & MQ_SF_IDENTITY) && p->ObjList->Objects[from_qs->QELinkage->SrcIndex])
			{
			i = from_qs->QELinkage->SrcIndex;
			break;
			}
		    }
	        if (i == -1) return -1;
		rval = objGetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
		return rval;
		}
	    else
		{
		mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
		return -1;
		}
	    }

	/** Evaluate the expression to get the value **/
	exp = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (expEvalTree(exp,p->ObjList) < 0) return 1;
	if (exp->Flags & EXPR_F_NULL) return 1;
	if (exp->DataType != datatype)
	    {
	    mssError(1,"MQ","Type mismatch getting attribute '%s' [requested=%s, actual=%s]", 
		    attrname,obj_type_names[datatype],obj_type_names[exp->DataType]);
	    return -1;
	    }
	switch(exp->DataType)
	    {
	    case DATA_T_INTEGER: *(int*)value = exp->Integer; break;
	    case DATA_T_STRING: *(char**)value = exp->String; break;
	    case DATA_T_DOUBLE: *(double*)value = exp->Types.Double; break;
	    case DATA_T_MONEY: *(pMoneyType*)value = &(exp->Types.Money); break;
	    case DATA_T_DATETIME: *(pDateTime*)value = &(exp->Types.Date); break;
	    case DATA_T_BINARY:
		((pBinary)value)->Data = (unsigned char*)exp->String;
		((pBinary)value)->Size = exp->Size;
		break;
	    default:
		mssError(1,"MQ","Unsupported data type for attribute '%s' [%s]", 
			attrname, obj_type_names[datatype]);
		return -1;
	    }

    return 0;
    }


/*** mq_internal_QEGetNextAttr - get the next attribute available from the
 *** query exec tree element.  Uses two externals - attrid and astobjid - to
 *** track iteration state.  If astobjid == -1 on return, then the attribute
 *** is internal, otherwise it is via "select *" and iterated from the given
 *** astobjid object.
 ***/
char*
mq_internal_QEGetNextAttr(pMultiQuery mq, pQueryElement qe, pParamObjects objlist, int* attrid, int* astobjid)
    {
    char* attrname = NULL;

    	/** Check overflow... **/
	while(!attrname)
	    {
	    if (*attrid >= qe->AttrNames.nItems) return NULL;

	    /** Asterisk? **/
	    attrname = qe->AttrNames.Items[*attrid];
	    if (!strcmp(attrname,"*"))
		{
		attrname = NULL;
		while(!attrname)
		    {
		    if (*astobjid == -1)
			{
			/** First non-external object **/
			if (mq->nProvidedObjects < objlist->nObjects && objlist->Objects[mq->nProvidedObjects])
			    attrname = objGetFirstAttr(objlist->Objects[mq->nProvidedObjects]);
			*astobjid = mq->nProvidedObjects;
			}
		    else	
			{
			if (objlist->Objects[*astobjid])
			    attrname = objGetNextAttr(objlist->Objects[*astobjid]);
			}
		    if (attrname == NULL)
			{
			(*astobjid)++;
			if (*astobjid >= objlist->nObjects)
			    {
			    *astobjid = -1;
			    (*attrid)++;
			    break;
			    }
			if (objlist->Objects[*astobjid])
			    attrname = objGetFirstAttr(objlist->Objects[*astobjid]);
			}
		    }
		}
	    else
		{
		(*attrid)++;
		}
	    }

    return attrname;
    }


/*** mqGetNextAttr - returns the name of the _next_ attribute.  This function, and
 *** the previous one, both return NULL if there are no (more) attributes.
 ***/
char*
mqGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

    return mq_internal_QEGetNextAttr(p->Stmt->Query, p->Stmt->Tree, p->ObjList, &p->AttrID, &p->AstObjID);
    }


/*** mqGetFirstAttr - returns the name of the first attribute in the multiquery
 *** result set, and preparse mqGetNextAttr to return subsequent attributes.
 ***/
char*
mqGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Set attr id, and return next attr **/
	p->AstObjID = -1;
	p->AttrID = 0;

    return mqGetNextAttr(inf_v, oxt);
    }


/*** mqSetAttrValue - sets the value of an attribute.  In order for this to work,
 *** the query must have been opened FOR UPDATE.
 ***/
int
mqSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData value, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int i, id, rval, dt;
    pExpression exp;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Updates not permitted? **/
	if (p->Stmt->Query->Flags & MQ_F_NOUPDATE)
	    {
	    mssError(1,"MQ","setattr '%s' disallowed because data changes are forbidden", attrname);
	    return -1;
	    }

	/** Figure out which attribute needs updating... **/
	id = -1;
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    dt = objGetAttrType(p->ObjList->Objects[i], attrname);
		    if (dt >= 0)
			{
			rval = objSetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
			return rval;
			}
		    }
		}
	    }
	    
	if (id == -1)
	    {
	    mssError(1,"MQ","setattr: unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expr first to find the data type.  This isn't
	 ** ideal, but it'll work for now.
	 **/
	exp = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (expEvalTree(exp,p->ObjList) < 0) return -1;
	if (exp->DataType == DATA_T_UNAVAILABLE) return -1;

	/** Verify that the types match. **/
	if (exp->DataType != datatype)
	    {
	    mssError(1,"MQ","Type mismatch setting attribute '%s' [requested=%s, actual=%s]", 
		    attrname,obj_type_names[datatype],obj_type_names[exp->DataType]);
	    return -1;
	    }

	/** Set the expression result to the given value. **/
	if (value)
	    {
	    exp->Flags &= ~(EXPR_F_NULL);
	    switch(exp->DataType)
		{
		case DATA_T_INTEGER: 
		    exp->Integer = value->Integer; 
		    break;
		case DATA_T_STRING: 
		    if (exp->Alloc && exp->String)
			{
			nmSysFree(exp->String);
			exp->Alloc = 0;
			}
		    if (strlen(value->String) > 63)
			{
			exp->Alloc = 1;
			exp->String = nmSysMalloc(strlen(value->String)+1);
			}
		    else
			{
			exp->String = exp->Types.StringBuf;
			}
		    strcpy(exp->String, value->String);
		    break;
		case DATA_T_DOUBLE: 
		    exp->Types.Double = value->Double; 
		    break;
		case DATA_T_MONEY: 
		    memcpy(&(exp->Types.Money), value->Money, sizeof(MoneyType));
		    break;
		case DATA_T_DATETIME: 
		    memcpy(&(exp->Types.Date), value->DateTime, sizeof(DateTime));
		    break;
		}
	    }
	else
	    {
	    exp->Flags |= EXPR_F_NULL;
	    }

	/** Evaluate the expression in reverse to set the value!! **/
	if (expReverseEvalTree(exp, p->ObjList) < 0) return -1;

    return 0;
    }


/*** mqAddAttr - does NOT work for multiqueries.  Return 'bad user'.  Sigh.
 ***/
int
mqAddAttr(void* inf_v, char* attrname, int type, void* value, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* not yet impl. */
    }


/*** mqOpenAttr - does NOT work for multiqueries.  Maybe someday.  Return failed.
 ***/
void*
mqOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqGetFirstMethod - returns the first method associated with this object.
 *** This function will attempt to try to access methods available on the underlying
 *** objects, and will return the first one it finds.
 ***/
char*
mqGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqGetNextMethod - returns the _next_ method associated with this object.
 *** This function will do like the above one, and look at underlying objects.
 ***/
char*
mqGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqExecuteMethod - executes a given method.  This function will attempt to
 *** locate the method in the underlying objects, and execute it there.  The
 *** query must be opened FOR UPDATE for this to work.
 ***/
int
mqExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* not yet impl. */
    }


/*** mqCommit - commit changes.  Basically, run 'commit' on all underlying
 *** objects for the query.
 ***/
int
mqCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int i;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Commit each underlying object **/
	//objCommit(p->Stmt->Query->SessionID);
	for(i=p->Stmt->Query->nProvidedObjects; i<p->ObjList->nObjects; i++)
	    {
	    if (p->ObjList->Objects[i])
		objCommitObject(p->ObjList->Objects[i]);
	    }

    return 0;
    }


/*** The following are administrative functions -- that is, they are used to
 *** setup/initialize/maintain the multiquery system.
 ***/


/*** mqRegisterQueryDriver - registers a query driver with this system.  A
 *** query driver may handle such things as a join, union, or projection, and
 *** has a precedence to control the order in which the drivers get chances to
 *** evaluate the SELECT query and arrange themselves.
 ***/
int
mqRegisterQueryDriver(pQueryDriver drv)
    {

    	/** Add the thing to the global XArray. **/
	xaAddItemSortedInt32(&MQINF.Drivers, (void*)drv, offsetof(QueryDriver, Precedence));

    return 0;
    }

/*** mqPresentationHints
 ***/
pObjPresentationHints
mqPresentationHints(void* inf_v, char* attrname, pObjTrxTree* otx)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    pObjPresentationHints ph;
    pObject obj;
    int i, id, objid;
    pExpression e;

    	/** Request for ls__rowid? **/
	if (!strcmp(attrname,"ls__rowid")) return NULL;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	id = -1;
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    ph = objPresentationHints(p->ObjList->Objects[i], attrname);
		    if (ph)
			return ph;
		    }
		}
	    }
	    
	if (id == -1) 
	    {
	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
	        return NULL;
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return NULL;
	    }

	/** Can we figure out what object this expression is referencing? **/
	p->ObjList->Session = p->Stmt->Query->SessionID;
	e = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (e->NodeType == EXPR_N_PROPERTY || e->NodeType == EXPR_N_OBJECT)
	    {
	    objid = expObjID(e,p->ObjList);
	    if (objid >= 0)
		{
		obj = p->ObjList->Objects[objid];
		if (obj && p->ObjList->GetTypeFn[objid] == objGetAttrType)
		    {
		    ph = objPresentationHints(obj, attrname);
		    return ph;
		    }
		}
	    }

    return NULL;
    }


/*** mqGetQueryIdentityPath() - return the pathname to the identity source
 *** in the currently active statement.
 ***/
int
mqGetQueryIdentityPath(void* qy_v, char* pathbuf, int maxlen)
    {
    pMultiQuery qy = (pMultiQuery)qy_v;
    pQueryStructure from_qs, identity_qs;
    int n_sources;

	/** No current statement? **/
	if (!qy->CurStmt)
	    {
	    mssError(1,"MQ","Cannot determine identity path: no statement active");
	    return -1;
	    }

	/** Search for the IDENTITY source **/
	identity_qs = from_qs = NULL;
	n_sources = 0;
	while((from_qs = mq_internal_FindItem(qy->CurStmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
	    {
	    n_sources++;
	    identity_qs = from_qs;
	    if (from_qs->Flags & MQ_SF_IDENTITY)
		break;
	    }

	/** Not found? **/
	if (!identity_qs || (n_sources != 1 && !(identity_qs->Flags & MQ_SF_IDENTITY)))
	    {
	    mssError(1,"MQ","Cannot determine identity path: no identity FROM source");
	    return -1;
	    }

	/** Copy it **/
	strtcpy(pathbuf, identity_qs->Source, maxlen);

    return 0;
    }


/*** mq_internal_FindCollection() - search both the query scope and app scope
 *** collections lists to find a collection by name.  Returns the handle to the
 *** OSML temporary object containing the collection.
 ***/
handle_t
mq_internal_FindCollection(pMultiQuery mq, char* collection)
    {
    pQueryAppData appdata;
    pQueryDeclaredCollection qdc;
    int i;

	/** First, search the query scope collection list **/
	for(i=0; i<mq->DeclaredCollections.nItems; i++)
	    {
	    qdc = (pQueryDeclaredCollection)mq->DeclaredCollections.Items[i];
	    if (!strcmp(qdc->Name, collection))
		return qdc->Collection;
	    }

	/** Lookup the application scope data **/
	appdata = appLookupAppData("MQ:appdata");

	/** Next, search the query scope collection list **/
	if (appdata)
	    {
	    for(i=0; i<appdata->DeclaredCollections.nItems; i++)
		{
		qdc = (pQueryDeclaredCollection)appdata->DeclaredCollections.Items[i];
		if (!strcmp(qdc->Name, collection))
		    return qdc->Collection;
		}
	    }

    return XHN_INVALID_HANDLE;
    }


/*** mqInitialize - initialize the multiquery module and link in with the
 *** objectsystem management layer, registering as the multiquery module.
 ***/
int
mqInitialize()
    {
    pObjDriver drv;

    	/** Initialize globals **/
	xaInit(&MQINF.Drivers,16);

    	/** Allocate the driver structure **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(ObjDriver));

	/** Fill in the driver structure, as best we can. **/
	drv->Capabilities = OBJDRV_C_ISMULTIQUERY;
	xaInit(&drv->RootContentTypes,16);
	strcpy(drv->Name, "MQ - ObjectSystem MultiQuery Module");
	drv->Open = NULL;
	drv->Close = mqClose;
	drv->Create = NULL;
	drv->Delete = NULL;
	drv->DeleteObj = mqDeleteObj;
	drv->OpenQuery = mqStartQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = mqQueryFetch;
	drv->QueryCreate = mqQueryCreate;
	drv->QueryClose = mqQueryClose;
	drv->Read = mqRead;
	drv->Write = mqWrite;
	drv->GetAttrType = mqGetAttrType;
	drv->GetAttrValue = mqGetAttrValue;
	drv->GetFirstAttr = mqGetFirstAttr;
	drv->GetNextAttr = mqGetNextAttr;
	drv->SetAttrValue = mqSetAttrValue;
	drv->AddAttr = mqAddAttr;
	drv->OpenAttr = mqOpenAttr;
	drv->GetFirstMethod = mqGetFirstMethod;
	drv->GetNextMethod = mqGetNextMethod;
	drv->ExecuteMethod = mqExecuteMethod;
	drv->PresentationHints = mqPresentationHints;
	drv->Commit = mqCommit;
	drv->GetQueryIdentityPath = mqGetQueryIdentityPath;

	nmRegister(sizeof(QueryElement),"QueryElement");
	nmRegister(sizeof(QueryStructure),"QueryStructure");
	nmRegister(sizeof(MultiQuery),"MultiQuery");
	nmRegister(sizeof(QueryDriver),"QueryDriver");
	nmRegister(sizeof(PseudoObject),"PseudoObject");
	nmRegister(sizeof(Expression),"Expression");
	nmRegister(sizeof(ExpControl),"ExpControl");
	nmRegister(sizeof(ParamObjects),"ParamObjects");

	/** Register the module with the OSML. **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }


