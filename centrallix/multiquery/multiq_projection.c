#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
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
/* Module: 	multiq_projection.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 25, 1999					*/
/* Description:	Provides support for a Projection SQL operation, or 	*/
/*		pulling data items from an ObjectSystem query.		*/
/************************************************************************/


struct
    {
    pQueryDriver        ThisDriver;
    }
    MQPINF;

pFile mqp_log;

/*** used by subtree projections to remember what depth objects were at, etc. ***/
typedef struct _MSI
    {
    int		Depth;
    char	Path[OBJSYS_MAX_PATH+1];
    int		LinkCnt;
    struct _MSI* Parent;
    }
    MqpSbtInf, *pMqpSbtInf;

#define MQP_MAX_SUBTREE	    64
    
/*** used by subtree projections to store the open query and object stack ***/
typedef struct
    {
    pObject	ObjStack[MQP_MAX_SUBTREE];
    pObjQuery	QueryStack[MQP_MAX_SUBTREE];
    int		nStacked;
    pMqpSbtInf	CtxStack[MQP_MAX_SUBTREE];
    pMqpSbtInf	CtxCurrent;
    }
    MqpSubtrees, *pMqpSubtrees;


/*** used by the single-row result set caching mechanism ***/
typedef struct
    {
    pObject	Obj;		/* the cached object */
    char*	Criteria;	/* serialized 'where' clause (cx sql) */
    int		nMatches;	/* total number of matches against the criteria */
    int		LastMatch;
    }
    MqpOneRow, *pMqpOneRow;

#define MQP_MAX_ROWCACHE	128

typedef struct
    {
    XArray	Cache;		/* of type pMqpOneRow */
    int		LastMatch;	/* serial counter for MqpOneRow.LastMatch */
    XString	CriteriaBuf;
    int		CurCached;	/* set to N if current Start() is using the cache, -1 otherwise */
    }
    MqpRowCache, *pMqpRowCache;


/*** Private data used by this driver ***/
typedef struct _MPI
    {
    pMqpSubtrees    Subtrees;
    pMqpRowCache    RowCache;
    int		    ObjMode;	/* O_xxx mode to open objects with */
    char	    CurrentSource[OBJSYS_MAX_PATH];
    XArray	    SourceList;
    int		    SourceIndex;
    pExpression	    AddlExp;
    int		    Flags;
    pQueryStatement Statement;
    }
    MqpInf, *pMqpInf;

#define MQP_MI_F_SOURCELIST	1	/* source list is valid */
#define MQP_MI_F_USINGCACHE	2	/* using row cache */
#define MQP_MI_F_SOURCEOPEN	4	/* source has been opened */
#define MQP_MI_F_LASTMATCHED	8	/* last item returned matched (for pruning) */


int mqp_internal_SetupSubtreeAttrs(pQueryElement qe, pObject obj);


/*** mqp_internal_GetSbtAttrType() - get the data type of one of the
 *** "special" subtree select attributes:
 ***
 ***    __cx_path (string)
 ***	__cx_parentpath (string)
 ***	__cx_parentname (string)
 ***	__cx_depth (integer)
 ***/
int
mqp_internal_GetSbtAttrType(pObjSession s, pObject obj, char* attrname, void* ctx)
    {

	/** Choose it **/
	if (!strcmp(attrname,"__cx_path"))
	    return DATA_T_STRING;
	else if (!strcmp(attrname,"__cx_parentpath"))
	    return DATA_T_STRING;
	else if (!strcmp(attrname,"__cx_parentname"))
	    return DATA_T_STRING;
	else if (!strcmp(attrname,"__cx_depth"))
	    return DATA_T_INTEGER;

    return -1;
    }


/*** mqp_internal_GetSbtAttrValue() - get the value of one of the special
 *** subtree select attrs.
 ***/
int
mqp_internal_GetSbtAttrValue(pObjSession s, pObject obj, char* attrname, void* ctx_v, int type, pObjData val)
    {
    pMqpSbtInf ctx = (pMqpSbtInf)ctx_v;
    char* ptr;

	/** Path... **/
	if (!strcmp(attrname, "__cx_path"))
	    {
	    val->String = ctx->Path;
	    return 0;
	    }

	/** Parent's Path... **/
	if (!strcmp(attrname, "__cx_parentpath"))
	    {
	    if (!ctx->Parent) return 1;
	    val->String = ctx->Parent->Path;
	    return 0;
	    }

	/** Parent's Name... **/
	if (!strcmp(attrname, "__cx_parentname"))
	    {
	    if (!ctx->Parent) return 1;
	    ptr = strrchr(ctx->Parent->Path, '/');
	    if (ptr)
		val->String = ptr+1;
	    else
		val->String = ctx->Parent->Path;
	    return 0;
	    }

	/** Depth **/
	if (!strcmp(attrname, "__cx_depth"))
	    {
	    val->Integer = ctx->Depth;
	    return 0;
	    }

    return -1;
    }


/*** mqp_internal_SetSbtAttrValue() - set the value of one of the special
 *** subtree select attributes.  Not currently supported.
 ***/
int
mqp_internal_SetSbtAttrValue(pObjSession s, pObject obj, char* attrname, void* ctx, int type, pObjData val)
    {
    return -1;
    }


/*** mqp_internal_UnlinkSbt() - unlink a subtree context data structure, and free
 *** if ready to do so.
 ***/
int
mqp_internal_UnlinkSbt(pMqpSbtInf* sbtctx)
    {

	if (--(*sbtctx)->LinkCnt <= 0)
	    {
	    if ((*sbtctx)->Parent) mqp_internal_UnlinkSbt(&((*sbtctx)->Parent));
	    nmFree((*sbtctx), sizeof(MqpSbtInf));
	    *sbtctx = NULL;
	    }

    return 0;
    }


/*** mqp_internal_FinalizeSbt() - when object with vattrs is finally closed, this is
 *** called so that the context structure can be cleaned up.
 ***/
int
mqp_internal_FinalizeSbt(pObjSession s, pObject obj, char* attrname, void* ctx_v)
    {
    pMqpSbtInf ctx = (pMqpSbtInf)ctx_v;

	mqp_internal_UnlinkSbt(&ctx);

    return 0;
    }


/*** mqp_internal_Recurse() - attempt to recurse down into subobjects for a
 *** subtree type projection.  Returns a subobject if one is found, NULL
 *** otherwise.
 ***/
pObject
mqp_internal_Recurse(pQueryElement qe, pQueryStatement stmt, pObject obj)
    {
    pObjectInfo oi;
    pObjQuery newqy;
    pObject newobj;
    pMqpSubtrees ms = ((pMqpInf)(qe->PrivateData))->Subtrees;

	/** Too many levels of recursion? **/
	if (ms->nStacked >= MQP_MAX_SUBTREE) return NULL;

	/** First, can we discern subobjs w/o running a query? **/
	oi = objInfo(obj);
	if (oi && (oi->Flags & OBJ_INFO_F_NO_SUBOBJ)) return NULL;

	/** Try running the query. **/
	newqy = objOpenQuery(obj, NULL, NULL, (qe->Flags & MQ_EF_FROMSUBTREE)?NULL:qe->Constraint, (void**)(qe->OrderBy[0]?qe->OrderBy:NULL));
	if (!newqy) return NULL;
	objUnmanageQuery(stmt->Query->SessionID, newqy);
	newobj = objQueryFetch(newqy, ((pMqpInf)(qe->PrivateData))->ObjMode);
	if (!newobj)
	    {
	    objQueryClose(newqy);
	    return NULL;
	    }
	objUnmanageObject(stmt->Query->SessionID, newobj);

	/** Stack em. **/
	ms->ObjStack[ms->nStacked] = qe->LLSource;
	ms->QueryStack[ms->nStacked] = qe->LLQuery;
	ms->CtxStack[ms->nStacked] = ms->CtxCurrent;
	ms->nStacked++;
	qe->LLSource = obj;
	qe->LLQuery = newqy;

	mqp_internal_SetupSubtreeAttrs(qe, newobj);

    return newobj;
    }


/*** mqp_internal_Return() - reverse of the above.
 ***/
pObject
mqp_internal_Return(pQueryElement qe, pQueryStatement stmt, pObject obj)
    {
    pMqpSubtrees ms = ((pMqpInf)(qe->PrivateData))->Subtrees;
    pObject oldobj;

	/** Nothing to return from? **/
	if (ms->nStacked == 0) return NULL;

	/** Close the qy and obj **/
	objQueryClose(qe->LLQuery);
	if (obj) objClose(obj);
	oldobj = qe->LLSource;

	/** Remove original data from stack **/
	ms->nStacked--;
	qe->LLSource = ms->ObjStack[ms->nStacked];
	qe->LLQuery = ms->QueryStack[ms->nStacked];
	ms->CtxCurrent = ms->CtxStack[ms->nStacked];

    return oldobj;
    }


/*** mqp_internal_SetupSubtreeAttrs() - set up the four special subtree select
 *** attributes on a newly opened object.
 ***/
int
mqp_internal_SetupSubtreeAttrs(pQueryElement qe, pObject obj)
    {
    pMqpSubtrees ms = ((pMqpInf)(qe->PrivateData))->Subtrees;
    pMqpSbtInf ctx;

	/** Build the context structure **/
	ctx = (pMqpSbtInf)nmMalloc(sizeof(MqpSbtInf));
	if (!ctx) return -1;
	ctx->Depth = ms->nStacked+1;
	strcpy(ctx->Path, obj_internal_PathPart(obj->Pathname, obj->Pathname->nElements - ctx->Depth, ctx->Depth));
	if (ms->nStacked > 0)
	    {
	    ctx->Parent = ms->CtxStack[ms->nStacked-1];
	    ctx->Parent->LinkCnt++;
	    }
	else
	    {
	    ctx->Parent = NULL;
	    }
	ctx->LinkCnt = 4; /* four attributes will be set up. */
	ms->CtxCurrent = ctx;

	/** Set up the attributes **/
	objAddVirtualAttr(obj, "__cx_path", ctx, mqp_internal_GetSbtAttrType, mqp_internal_GetSbtAttrValue,
		mqp_internal_SetSbtAttrValue, mqp_internal_FinalizeSbt);
	objAddVirtualAttr(obj, "__cx_parentpath", ctx, mqp_internal_GetSbtAttrType, mqp_internal_GetSbtAttrValue,
		mqp_internal_SetSbtAttrValue, mqp_internal_FinalizeSbt);
	objAddVirtualAttr(obj, "__cx_parentname", ctx, mqp_internal_GetSbtAttrType, mqp_internal_GetSbtAttrValue,
		mqp_internal_SetSbtAttrValue, mqp_internal_FinalizeSbt);
	objAddVirtualAttr(obj, "__cx_depth", ctx, mqp_internal_GetSbtAttrType, mqp_internal_GetSbtAttrValue,
		mqp_internal_SetSbtAttrValue, mqp_internal_FinalizeSbt);

    return 0;
    }


/*** mqpAnalyze - take a given query syntax structure (qs) and scan it
 *** for projection operations.  If found, add entries in the query element
 *** execution (qe) tree.  This routine should silently exit with 0 returned
 *** if it finds no projections, and return -1 if it finds error(s).
 ***/
int
mqpAnalyze(pQueryStatement stmt)
    {
    pQueryElement qe;
    pQueryStructure from_qs = NULL;
    pQueryStructure select_qs = NULL;
    pQueryStructure where_qs = NULL;
    pQueryStructure orderby_qs = NULL;
    pQueryStructure groupby_qs = NULL;
    pQueryStructure item;
    pQueryStructure select_item;
    pQueryStructure where_item;
    int src_idx,i,j;
    pExpression new_exp;
    pMqpInf mi;
    int found;

    	/** Search for FROM clauses for this driver... **/
	while((from_qs = mq_internal_FindItem(stmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
	    {
	    if (from_qs->Flags & MQ_SF_USED) continue;

	    /** allocate one query element for each from clause. **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQPINF.ThisDriver;
	    qe->QSLinkage = (void*)from_qs;
	    qe->OrderPrio = 999;

	    /** Find the index of the object for this FROM clause.
	     ** If this is an expression, Presentation has a forced value
	     ** if not otherwise supplied by the SQL coder.
	     **/
	    src_idx = expLookupParam(stmt->Query->ObjList, from_qs->Presentation[0]?(from_qs->Presentation):(from_qs->Source));
	    if (src_idx == -1)
	        {
		mq_internal_FreeQE(qe);
		continue;
		}

	    /** Find the SELECT clause associated with this FROM clause. **/
	    select_qs = mq_internal_FindItem(from_qs->Parent->Parent, MQ_T_SELECTCLAUSE, NULL);
	    if (select_qs)
	        {
		/** Loop through the select items looking for ones that match this FROM source **/
		for(i=0;i<select_qs->Children.nItems;i++)
		    {
		    select_item = (pQueryStructure)(select_qs->Children.Items[i]);
		    if (select_item->QELinkage != NULL) continue;
		    if (select_item->ObjCnt == 1 && (select_item->ObjFlags[src_idx] & EXPR_O_REFERENCED))
			{
			if (select_item->Flags & MQ_SF_ASTERISK)
			    stmt->Flags |= MQ_TF_ASTERISK;
			xaAddItem(&qe->AttrNames, (void*)select_item->Presentation);
			xaAddItem(&qe->AttrExprPtr, (void*)select_item->RawData.String);
			xaAddItem(&qe->AttrCompiledExpr, (void*)select_item->Expr);
			select_item->QELinkage = qe;
			xaAddItem(&qe->AttrDeriv, (void*)NULL);
			}
		    }
		}

	    /** Find the WHERE items that match this FROM object... **/
	    qe->Constraint = NULL;
	    where_qs = mq_internal_FindItem(from_qs->Parent->Parent, MQ_T_WHERECLAUSE, NULL);
	    if (where_qs)
	        {
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		    if (((where_item->Expr->ObjCoverageMask & ~EXPR_MASK_EXTREF) & (~stmt->Query->ProvidedObjMask)) == (1<<src_idx))
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

			/** Increase this source's specificity rating. **/
			if (where_item->Expr->NodeType == EXPR_N_AND)
			    from_qs->Specificity += 3;
			else if (where_item->Expr->NodeType == EXPR_N_COMPARE && where_item->Expr->CompareType == MLX_CMP_EQUALS)
			    from_qs->Specificity += 2;
			else if (where_item->Expr->NodeType == EXPR_N_ISNULL)
			    from_qs->Specificity += 2;
			else
			    from_qs->Specificity += 1;

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

	    /** Add any group by to the order-by list first. **/
	    qe->OrderBy[0] = NULL;
	    groupby_qs = mq_internal_FindItem(from_qs->Parent->Parent, MQ_T_GROUPBYCLAUSE, NULL);
	    if (groupby_qs)
	        {
		for(i=0;i<groupby_qs->Children.nItems;i++)
		    {
		    item = (pQueryStructure)(groupby_qs->Children.Items[i]);
		    if (item->ObjCnt == 1 && (item->ObjFlags[src_idx] & EXPR_O_REFERENCED))
		        {
			j=0;
			while(qe->OrderBy[j]) j++;
			if ((j+1) >= sizeof(qe->OrderBy) / sizeof(*(qe->OrderBy))) break;
			if (qe->OrderPrio == 999 || qe->OrderPrio > i) qe->OrderPrio = i;
			qe->OrderBy[j] = exp_internal_CopyTree(item->Expr);
			qe->OrderBy[j+1] = NULL;
			expRemapID(qe->OrderBy[j], src_idx, 0);
			}
		    }
		}
	    
	    /** Find ORDER BY expressions that match this object. **/
	    orderby_qs = mq_internal_FindItem(from_qs->Parent->Parent, MQ_T_ORDERBYCLAUSE, NULL);
	    if (orderby_qs)
	        {
		for(i=0;i<orderby_qs->Children.nItems;i++)
		    {
		    item = (pQueryStructure)(orderby_qs->Children.Items[i]);
		    if (item->ObjCnt == 1 && (item->ObjFlags[src_idx] & EXPR_O_REFERENCED) && item->Expr && item->Expr->AggLevel == 0)
		        {
			new_exp = exp_internal_CopyTree(item->Expr);
			expRemapID(new_exp, src_idx, 0);

			/** Check to see if it is already in the list **/
			found = 0;
			for(j=0;qe->OrderBy[j];j++)
			    {
			    if (expCompareExpressions(new_exp, qe->OrderBy[j]))
				{
				found = 1;
				break;
				}
			    }
			if (found)
			    {
			    expFreeExpression(new_exp);
			    continue;
			    }

			/** Add it **/
			j=0;
			while(qe->OrderBy[j]) j++;
			if ((j+1) >= sizeof(qe->OrderBy) / sizeof(*(qe->OrderBy)))
			    {
			    expFreeExpression(new_exp);
			    break;
			    }
			if (qe->OrderPrio == 999 || qe->OrderPrio > i) qe->OrderPrio = i;
			qe->OrderBy[j] = new_exp;
			qe->OrderBy[j+1] = NULL;
			}
		    }
		}

	    /** If we got a constraint expression, replace the obj id's with id #0. **/
	    /** This is so the obj driver can evaluate it against a single-object objlist **/
	    /*if (qe->Constraint) expRemapID(qe->Constraint, src_idx, 0);*/

	    /** Link into the multiquery **/
	    xaAddItem(&stmt->Trees, qe);
	    stmt->Tree = qe;

	    /** Setup the object that this will use. **/
	    qe->SrcIndex = src_idx;
	    if (from_qs->Flags & MQ_SF_FROMSUBTREE) qe->Flags |= MQ_EF_FROMSUBTREE;
	    if (from_qs->Flags & MQ_SF_PRUNESUBTREE) qe->Flags |= MQ_EF_PRUNESUBTREE;
	    if (from_qs->Flags & MQ_SF_INCLSUBTREE) qe->Flags |= MQ_EF_INCLSUBTREE;
	    if (from_qs->Flags & MQ_SF_WILDCARD) qe->Flags |= MQ_EF_WILDCARD;
	    if (from_qs->Flags & MQ_SF_FROMOBJECT) qe->Flags |= MQ_EF_FROMOBJECT;
	    from_qs->Flags |= MQ_SF_USED;
	    from_qs->QELinkage = qe;

	    /** Setup private data, as needed **/
	    qe->PrivateData = mi = (pMqpInf)nmMalloc(sizeof(MqpInf));
	    memset(mi, 0, sizeof(MqpInf));
	    if (qe->Flags & MQ_EF_FROMSUBTREE)
		{
		mi->Subtrees = (pMqpSubtrees)nmMalloc(sizeof(MqpSubtrees));
		memset(mi->Subtrees, 0, sizeof(MqpSubtrees));
		}
	    xaInit(&mi->SourceList, 16);
	    mi->SourceIndex = 0;
	    mi->Statement = stmt;

	    /** Mode to open new objects with **/
	    if (stmt->Flags & MQ_TF_ALLOWUPDATE)
		mi->ObjMode = O_RDWR;
	    else
		mi->ObjMode = O_RDONLY;
	    }

    return 0;
    }


/*** mqp_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqp_internal_CheckConstraint(pQueryElement qe, pQueryStatement stmt)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, stmt->Query->ObjList);
	    if (qe->Constraint->DataType != DATA_T_INTEGER)
	        {
	        mssError(1,"MQP","WHERE clause item must have a boolean/integer value.");
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


/*** mqp_internal_CacheRow - add a row to the single-row result set cache
 ***/
int
mqp_internal_CacheRow(pQueryStatement stmt, pQueryElement qe, pExpression criteria, pObject obj)
    {
    pMqpRowCache rc;
    pMqpOneRow row;
    pMqpOneRow fewest_matches;
    int fewest_matches_id;
    int i;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);

	/** Set up the cache itself? **/
	if (!mi->RowCache)
	    {
	    rc = mi->RowCache = (pMqpRowCache)nmMalloc(sizeof(MqpRowCache));
	    xaInit(&rc->Cache, MQP_MAX_ROWCACHE);
	    xsInit(&rc->CriteriaBuf);
	    rc->LastMatch = 0;
	    }
	else
	    rc = mi->RowCache;

	/** Need to prune the cache of one item? **/
	if (rc->Cache.nItems == MQP_MAX_ROWCACHE)
	    {
	    fewest_matches = xaGetItem(&rc->Cache, 0);
	    fewest_matches_id = 0;
	    for(i=1;i<MQP_MAX_ROWCACHE;i++)
		{
		row = xaGetItem(&rc->Cache, i);
		if (row->LastMatch < rc->LastMatch - (MQP_MAX_ROWCACHE * 2))
		    {
		    /** stale item **/
		    fewest_matches = row;
		    fewest_matches_id = i;
		    break;
		    }
		if (row->nMatches < fewest_matches->nMatches || (row->nMatches == fewest_matches->nMatches && row->LastMatch < fewest_matches->LastMatch))
		    {
		    /** least used item **/
		    fewest_matches = row;
		    fewest_matches_id = i;
		    }
		}
	    /*fdQPrintf(mqp_log, "DISC:  %STR (%POS)\n", fewest_matches->Criteria, fewest_matches_id);*/
	    xaRemoveItem(&rc->Cache, fewest_matches_id);
	    nmSysFree(fewest_matches->Criteria);
	    if (fewest_matches->Obj) objClose(fewest_matches->Obj);
	    row = fewest_matches;
	    }
	else
	    {
	    row = (pMqpOneRow)nmMalloc(sizeof(MqpOneRow));
	    }

	/** Create a new item **/
	row->LastMatch = rc->LastMatch++;
	row->nMatches = 0;
	row->Obj = obj;
	xsCopy(&rc->CriteriaBuf, mi->CurrentSource, -1);
	xsConcatenate(&rc->CriteriaBuf, " :: ", 4);
	if (criteria) expGenerateText(criteria, stmt->Query->ObjList, xsWrite, &rc->CriteriaBuf, '\0', "cxsql", 0);
	row->Criteria = nmSysStrdup(rc->CriteriaBuf.String);
	xaAddItem(&rc->Cache, row);
	/*fdQPrintf(mqp_log, "ADD:   %STR (%POS)\n", row->Criteria, xaCount(&rc->Cache)-1);*/

    return 0;
    }


/*** mqp_internal_CheckCache() - look in the single row result set cache
 *** for something that matches the current Constraint.  Returns -1 if not
 *** found in the cache, and the cache id otherwise.
 ***/
int
mqp_internal_CheckCache(pQueryStatement stmt, pMqpRowCache rc, char* source, pExpression criteria)
    {
    int i;
    pMqpOneRow row;

	xsCopy(&rc->CriteriaBuf, source, -1);
	xsConcatenate(&rc->CriteriaBuf, " :: ", 4);
	if (criteria) expGenerateText(criteria, stmt->Query->ObjList, xsWrite, &rc->CriteriaBuf, '\0', "cxsql", 0);
	for(i=0;i<rc->Cache.nItems;i++)
	    {
	    row = xaGetItem(&rc->Cache, i);
	    if (!strcmp(row->Criteria, rc->CriteriaBuf.String))
		{
		/** Found it **/
		row->nMatches++;
		row->LastMatch = rc->LastMatch++;
		/*fdQPrintf(mqp_log, "HIT:   %STR (%POS)\n", rc->CriteriaBuf.String, i);*/
		return i;
		}
	    }

	/*fdQPrintf(mqp_log, "MISS:  %STR\n", rc->CriteriaBuf.String);*/

    return -1;
    }


/*** mqp_internal_CloseSource() - clean up after *one* source object has been used
 ***/
int
mqp_internal_CloseSource(pQueryElement qe)
    {
    pMqpInf mi = (pMqpInf)(qe->PrivateData);
    pMqpSubtrees ms = ((pMqpInf)(qe->PrivateData))->Subtrees;
    pObject obj;

	/** Return from any subtrees **/
	while (ms && ms->nStacked > 0)
	    {
	    obj = mqp_internal_Return(qe, mi->Statement, NULL);
	    if (obj)
		objClose(obj);
	    else
		break;
	    }

    	/** Close the source object and the query. **/
	if (qe->LLQuery) objQueryClose(qe->LLQuery);
	if (qe->LLSource) objClose(qe->LLSource);
	qe->LLQuery = NULL;
	qe->LLSource = NULL;

	mi->Flags &= ~MQP_MI_F_SOURCEOPEN;

    return 0;
    }


/*** mqp_internal_OpenNextSource() - take the next source off of the
 *** source list compiled by wildcard processing, and open it.
 ***/
int
mqp_internal_OpenNextSource(pQueryElement qe, pQueryStatement stmt)
    {
    pMqpRowCache rc;
    char* src;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);

	mi->Flags &= ~MQP_MI_F_USINGCACHE;

	/** Get next source **/
	while(1)
	    {
	    if (mi->SourceIndex >= mi->SourceList.nItems) return 0;
	    src = xaGetItem(&(mi->SourceList), mi->SourceIndex++);
	    strtcpy(mi->CurrentSource, src, sizeof(mi->CurrentSource));

	    /** Check for cached single row result set **/
	    if (mi->RowCache)
		{
		rc = mi->RowCache;
		rc->CurCached = mqp_internal_CheckCache(stmt, rc, mi->CurrentSource, qe->Constraint);
		if (rc->CurCached >= 0)
		    {
		    /** We're going from the cache this time **/
		    /*objClose(qe->LLSource);*/
		    qe->LLSource = NULL;
		    qe->LLQuery = NULL;
		    mi->Flags |= MQP_MI_F_USINGCACHE;
		    break;
		    }
		}

	    /** Open the data source in the objectsystem **/
	    qe->LLSource = objOpen(stmt->Query->SessionID, mi->CurrentSource, mi->ObjMode, 0600, (qe->Flags & MQ_EF_FROMOBJECT)?"system/object":"system/directory");
	    if (!qe->LLSource) 
		{
		if ((qe->Flags & MQ_EF_WILDCARD) || (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_EXPRESSION))
		    {
		    /** with wildcarding and/or expressions, it is ok for a source to not exist, we just ignore it. **/
		    continue;
		    }
		else
		    {
		    mssError(0,"MQP","Could not open source object for SQL projection");
		    return -1;
		    }
		}
	    objUnmanageObject(stmt->Query->SessionID, qe->LLSource);
	    break;
	    }

    	/** Open the query with the objectsystem. **/
	if (((qe->Flags & MQ_EF_FROMSUBTREE) && (qe->Flags & MQ_EF_INCLSUBTREE)) || (qe->Flags & MQ_EF_FROMOBJECT))
	    {
	    qe->LLQuery = NULL;
	    }
	else if (!(mi->Flags & MQP_MI_F_USINGCACHE))
	    {
	    qe->LLQuery = objOpenQuery(qe->LLSource, NULL, NULL, (qe->Flags & MQ_EF_FROMSUBTREE)?NULL:qe->Constraint, (void**)(qe->OrderBy[0]?qe->OrderBy:NULL));
	    if (!qe->LLQuery) 
		{
		mqpFinish(qe,stmt);
		mssError(0,"MQP","Could not query source object for SQL projection");
		return -1;
		}
	    objUnmanageQuery(stmt->Query->SessionID, qe->LLQuery);
	    }
	qe->IterCnt = 0;

	mi->Flags |= MQP_MI_F_SOURCEOPEN;

    return 1;
    }


/*** mqp_internal_WildcardMatch() - compare a wildcard string with a constant string
 *** to see if there is a match.  Returns 1 on a match, 0 on no match.
 ***/
int
mqp_internal_WildcardMatch(char* pattern, int pattern_len, char* str, int str_len)
    {
    char* astptr;
    char* chptr;
    int leading_len;

	/** Find first wildcard **/
	astptr = memchr(pattern, '*', pattern_len);

	/** special cases - * pattern, or no wildcard at all (straight compare) **/
	if (astptr && pattern_len == 1)
	    return 1;
	if (!astptr)
	    return ((pattern_len == str_len) && memcmp(pattern, str, str_len) == 0);

	/** leading pattern doesn't match? **/
	leading_len = astptr - pattern;
	if (leading_len > 0 && (str_len < leading_len || memcmp(pattern, str, leading_len) != 0)) return 0;

	/** Asterisk is at the end of the pattern? **/
	if (leading_len + 1 == pattern_len) return 1;

	/** Now deal with trailing pattern - look for occurences of 1st char after the asterisk, and go from there **/
	chptr = str + leading_len - 1;
	while((chptr = memchr(chptr+1, astptr[1], str_len - (chptr+1 - str))) != NULL)
	    {
	    return mqp_internal_WildcardMatch(astptr+1, pattern_len - leading_len - 1, chptr, str_len - (chptr - str));
	    }

    return 0;
    }


/* leading_static_elements + wildcarded_name + ?query + static_elements + wildcarded_name + ?query + trailing_static_elements
 * prev_path + static_elements + wildcarded_name + ?query + trailing_static_elements
 * prev_path + trailing_static_elements -> add to list
 *
 */

/*** mqp_internal_SetupWildcard_r() - recursive function which does the hard
 *** work of building the list of wildcard matches.
 ***
 *** orig_path = original pathname with wildcards in it
 *** pathbuf = buffer used for building the new (expanded) pathnames
 *** last_ending = pointer to next element in orig_path after one just expanded
 *** n_elements = number of wildcarded path elements
 *** element_list[] = pointers to wildcarded path elements
 ***/
int
mqp_internal_SetupWildcard_r(pQueryElement qe, pQueryStatement stmt, char* orig_path, char* pathbuf, char* last_ending, int n_elements, char* element_list[])
    {
    int cur_len = strlen(pathbuf);
    int orig_len;
    char* ptr;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);
    pObjQuery qy = NULL;
    pObject obj = NULL, subobj = NULL;
    pObjectInfo info;
    char* slashptr;
    char* qptr;

	/** Add static element(s) if any **/
	if (n_elements)
	    ptr = element_list[0];
	else
	    ptr = strchr(orig_path, '\0');
	if (ptr != last_ending)
	    {
	    if ((ptr - last_ending) + cur_len >= OBJSYS_MAX_PATH)
		{
		mssError(1,"MQP","Wildcard pathname expansion exceeds internal limits");
		goto error;
		}
	    strncat(pathbuf, last_ending, ptr - last_ending);
	    cur_len += (ptr - last_ending);
	    }
	
	if (n_elements == 0)
	    {
	    /** End of our work?  Add to path list if so **/
	    xaAddItem(&mi->SourceList, nmSysStrdup(pathbuf));
	    /*printf("expands to: %s\n", pathbuf);*/
	    }
	else
	    {
	    /** Otherwise, do a pathname expansion on the current element **/
	    slashptr = strchr(element_list[0]+1, '/');
	    if (!slashptr) slashptr = strchr(element_list[0], '\0');
	    qptr = strchr(element_list[0], '?');
	    orig_len = cur_len;

	    /** Open path so far and query for matching objects **/
	    obj = objOpen(stmt->Query->SessionID, pathbuf, O_RDONLY, 0600, "system/directory");
	    if (!obj)
		goto finished;
	    info = objInfo(obj);
	    if (info && (info->Flags & (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ)))
		goto finished;
	    qy = objOpenQuery(obj, NULL, NULL, NULL, NULL);
	    if (!qy)
		goto finished;
	    while ((subobj = objQueryFetch(qy, O_RDONLY)) != NULL)
		{
		objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&ptr));
		if (mqp_internal_WildcardMatch(element_list[0]+1, (qptr?qptr:slashptr) - (element_list[0]+1), ptr, strlen(ptr)))
		    {
		    if (strlen(ptr) + 1 + cur_len >= OBJSYS_MAX_PATH)
			{
			mssError(1,"MQP","Wildcard pathname expansion exceeds internal limits");
			goto error;
			}
		    strcat(pathbuf, "/");
		    strcat(pathbuf, ptr);
		    if (qptr)
			{
			if (strlen(pathbuf) + (slashptr - qptr) >= OBJSYS_MAX_PATH)
			    {
			    mssError(1,"MQP","Wildcard pathname expansion exceeds internal limits");
			    goto error;
			    }
			strncat(pathbuf, qptr, slashptr - qptr);
			}
		    if (mqp_internal_SetupWildcard_r(qe, stmt, orig_path, pathbuf, slashptr, n_elements-1, element_list+1) < 0)
			goto error;
		    pathbuf[orig_len] = '\0';
		    }
		objClose(subobj);
		subobj = NULL;
		}
	    objQueryClose(qy);
	    qy = NULL;
	    objClose(obj);
	    obj = NULL;
	    }

	return 0;

    finished:
	if (subobj) objClose(subobj);
	if (qy) objQueryClose(qy);
	if (obj) objClose(obj);
	return 0;

    error:
	if (subobj) objClose(subobj);
	if (qy) objQueryClose(qy);
	if (obj) objClose(obj);
	return -1;
    }


/*** mqp_internal_SetupWildcard() - take the given pathname with wildcards
 *** in it, and generate a list of objects that match the wildcard pathname.
 ***/
int
mqp_internal_SetupWildcard(pQueryElement qe, pQueryStatement stmt)
    {
    char* path = nmSysStrdup(((pQueryStructure)qe->QSLinkage)->Source);
    char* pathbuf = nmSysMalloc(OBJSYS_MAX_PATH + 1);
    char* element_list[OBJSYS_MAX_ELEMENTS];
    int n_elements;
    char* slashptr;
    char* astptr;
    char* qptr;
    char* element;

	/** Find where pathname elements with wildcards are **/
	n_elements = 0;
	element = path;
	while(element)
	    {
	    /** / is next path element.  * is wildcard.  ? is begin of open params. **/
	    astptr = strchr(element, '*');
	    slashptr = strchr(element+1, '/');
	    qptr = strchr(element, '?');

	    if (astptr && (!slashptr || astptr < slashptr) && (!qptr || astptr < qptr))
		{
		/** too many? **/
		if (n_elements >= OBJSYS_MAX_ELEMENTS)
		    {
		    mssError(1, "MQP", "Pathname has too many elements in it.");
		    goto error;
		    }

		element_list[n_elements++] = element;
		}
	    element = slashptr;
	    }

	/** Build the wildcard expansion path list **/
	strcpy(pathbuf, "");
	if (mqp_internal_SetupWildcard_r(qe, stmt, path, pathbuf, path, n_elements, element_list) < 0)
	    goto error;

	nmSysFree(pathbuf);
	nmSysFree(path);
	return 0;

    error:
	nmSysFree(pathbuf);
	nmSysFree(path);
	return -1;
    }


/*** mqpStart - starts the query operation for a given projection element
 *** in the query.  Does not fetch the first row -- that is what NextItem
 *** is there for.
 ***/
int
mqpStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    pMqpInf mi = (pMqpInf)(qe->PrivateData);
    pExpression new_exp;
    pExpression source_exp;

	if (additional_expr)
	    expFreezeEval(additional_expr, stmt->Query->ObjList, qe->SrcIndex);

	mi->AddlExp = additional_expr;
	mi->Flags &= ~(MQP_MI_F_USINGCACHE | MQP_MI_F_SOURCEOPEN);
	mi->SourceIndex = 0;

	/** Additional expression supplied?? **/
	if (mi->AddlExp) qe->Flags |= MQ_EF_ADDTLEXP;
	if (qe->Constraint) qe->Flags |= MQ_EF_CONSTEXP;
	if (mi->AddlExp && qe->Constraint)
	    {
	    new_exp = expAllocExpression();
	    new_exp->NodeType = EXPR_N_AND;
	    expAddNode(new_exp, qe->Constraint);
	    expAddNode(new_exp, mi->AddlExp);
	    qe->Constraint = new_exp;
	    }
	if (!qe->Constraint) qe->Constraint = mi->AddlExp;
        if (qe->Constraint && !(qe->Flags & MQ_EF_FROMSUBTREE) && !(qe->Flags & MQ_EF_FROMOBJECT))
	    expRemapID(qe->Constraint, qe->SrcIndex, 0);

	qe->LLSource = NULL;
	qe->LLQuery = NULL;

	/** Evaluate source expression? **/
	if (((pQueryStructure)qe->QSLinkage)->Flags & MQ_SF_EXPRESSION)
	    {
	    /** Remove previous source list if necessary, since it will likely
	     ** be different this time.
	     **/
	    if (mi->Flags & MQP_MI_F_SOURCELIST)
		{
		while(mi->SourceList.nItems)
		    {
		    nmSysFree(mi->SourceList.Items[0]);
		    xaRemoveItem(&(mi->SourceList), 0);
		    }
		mi->Flags &= ~MQP_MI_F_SOURCELIST;
		}

	    /** Evalute **/
	    source_exp = ((pQueryStructure)qe->QSLinkage)->Expr;
	    if (expEvalTree(source_exp, stmt->Query->ObjList) < 0)
		{
		mssError(0, "MQP", "Error in expression for FROM clause item");
		return -1;
		}

	    /** If NULL, this source is valid but returns no rows **/
	    if (source_exp->Flags & EXPR_F_NULL)
		{
		mi->Flags |= MQP_MI_F_SOURCELIST;
		return 0;
		}

	    /** If non-string, error. **/
	    if (source_exp->DataType != DATA_T_STRING)
		{
		mssError(1, "MQP", "Expression for FROM clause item must be a string");
		return -1;
		}

	    /** If too long, error **/
	    if (strlen(source_exp->String) >= sizeof(((pQueryStructure)qe->QSLinkage)->Source))
		{
		mssError(1, "MQP", "Expression for FROM clause item resulted in an over-long string");
		return -1;
		}

	    strtcpy(((pQueryStructure)qe->QSLinkage)->Source, source_exp->String, sizeof(((pQueryStructure)qe->QSLinkage)->Source));
	    }

	/** Wildcard processing needed? **/
	if (!(mi->Flags & MQP_MI_F_SOURCELIST))
	    {
	    if (qe->Flags & MQ_EF_WILDCARD)
		{
		if (mqp_internal_SetupWildcard(qe, stmt) < 0)
		    return -1;
		}
	    else
		{
		xaAddItem(&(mi->SourceList), nmSysStrdup(((pQueryStructure)qe->QSLinkage)->Source));
		}
	    mi->Flags |= MQP_MI_F_SOURCELIST;
	    }

    return 0;
    }


/*** mqpNextItem - retrieves the first/next item in the result set for the
 *** projection.  Returns 1 if valid row obtained, 0 if no more rows are
 *** available, and -1 on error.
 ***/
int
mqp_internal_NextItemFromSource(pQueryElement qe, pQueryStatement stmt)
    {
    pObject obj;
    pObject saved_obj = NULL;
    pMqpRowCache rc;
    pMqpOneRow row;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);

	qe->IterCnt++;

	/** Using cache? **/
	if (mi->RowCache)
	    {
	    rc = mi->RowCache;
	    if (rc->CurCached >= 0)
		{
		row = xaGetItem(&rc->Cache, rc->CurCached);
		if (qe->IterCnt == 1)
		    {
		    if (row->Obj) objLinkTo(row->Obj);
		    expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, row->Obj);
		    return row->Obj?1:0;
		    }
		else if (qe->IterCnt == 2)
		    {
		    if (stmt->Query->ObjList->Objects[qe->SrcIndex])
			objClose(stmt->Query->ObjList->Objects[qe->SrcIndex]);
		    expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, NULL);
		    }
		return 0;
		}
	    }

	/** Inclusive subtree or FROM OBJECT?  If so return the top-level src object too **/
	if ((qe->Flags & MQ_EF_FROMOBJECT || ((qe->Flags & MQ_EF_FROMSUBTREE) && (qe->Flags & MQ_EF_INCLSUBTREE)))
		&& !stmt->Query->ObjList->Objects[qe->SrcIndex] && qe->IterCnt == 1)
	    {
	    obj = objLinkTo(qe->LLSource);
	    expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, obj);

	    if (qe->Flags & MQ_EF_FROMSUBTREE)
		{
		mqp_internal_SetupSubtreeAttrs(qe, obj);
		return 1;
		}

	    if (mqp_internal_CheckConstraint(qe, stmt) > 0)
		{
		/** the top level one matches.  use it. **/
		if (qe->Flags & MQ_EF_FROMSUBTREE)
		    mqp_internal_SetupSubtreeAttrs(qe, obj);
		return 1;
		}
	    else
		{
		objClose(stmt->Query->ObjList->Objects[qe->SrcIndex]);
		expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, NULL);
		}
	    }

	/** Attempt to recurse a subtree? **/
	if ((qe->Flags & MQ_EF_FROMSUBTREE) && (!(mi->Flags & MQP_MI_F_LASTMATCHED) || !(qe->Flags & MQ_EF_PRUNESUBTREE)) && stmt->Query->ObjList->Objects[qe->SrcIndex])
	    {
	    obj = mqp_internal_Recurse(qe, stmt, stmt->Query->ObjList->Objects[qe->SrcIndex]);
	    if (obj)
		{
		expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, obj);
		return 1;
		}
	    }

	/** Loop is for when we are returning from deep recursion **/
	while(1)
	    {
	    /** Close the previous fetched object? **/
	    if (stmt->Query->ObjList->Objects[qe->SrcIndex])
		{
		/** if this is after the user has processed row #1, create a tmp copy of row 1 **/
		if (qe->LLQuery && qe->IterCnt == 2 && !(qe->Flags & MQ_EF_FROMSUBTREE) && !(qe->Flags & MQ_EF_WILDCARD))
		    saved_obj = objLinkTo(stmt->Query->ObjList->Objects[qe->SrcIndex]);

		/** "close" it **/
		objClose(stmt->Query->ObjList->Objects[qe->SrcIndex]);
		expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, NULL);
		}

	    /** FROM OBJECT, or end of a subtree, won't have a LLQuery **/
	    if (!qe->LLQuery)
		return 0;

	    /** Fetch the next item and set the object... **/
	    obj = objQueryFetch(qe->LLQuery, mi->ObjMode);
	    if (obj)
		{
		/** Got one. **/
		if (saved_obj) objClose(saved_obj);
		objUnmanageObject(stmt->Query->SessionID, obj);
		expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, obj);
		if (qe->Flags & MQ_EF_FROMSUBTREE) mqp_internal_SetupSubtreeAttrs(qe, obj);
		return 1;
		}

	    /** No more objects - return from a subtree? **/
	    if (qe->Flags & MQ_EF_FROMSUBTREE)
		{
		obj = mqp_internal_Return(qe, stmt, NULL);
		if (!obj || obj == qe->LLSource) return 0;
		expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, obj);
		}
	    else
		{
		if ((qe->IterCnt == 1 || (qe->IterCnt == 2 && saved_obj)) && !(qe->Flags & MQ_EF_WILDCARD))
		    {
		    /** empty or single-row result set.  Save the saved_obj in the cache **/
		    mqp_internal_CacheRow(stmt, qe, qe->Constraint, saved_obj);
		    }
		return 0;
		}
	    }

    return 0;
    }

int
mqpNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    int rval;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);
   
	/** Loop through sources and items in sources **/
	while(1)
	    {
	    if (!(mi->Flags & MQP_MI_F_SOURCEOPEN))
		{
		rval = mqp_internal_OpenNextSource(qe, stmt);
		if (rval != 1) break; /* return if no more sources (0) or error (-1) */
		}

	    /** Loop looking for a valid item from this source.  For SUBTREE selects
	     ** we have to do this here because the WHERE can't be passed to the
	     ** data source due to the need to search subobjects of objects (whether
	     ** they match or not) too.
	     **/
	    while(1)
		{
		rval = mqp_internal_NextItemFromSource(qe, stmt);
		if (rval == 1 && (qe->Flags & MQ_EF_FROMSUBTREE))
		    {
		    /** Subtree select, valid object -- handle WHERE condition here **/
		    rval = mqp_internal_CheckConstraint(qe, stmt);
		    mi->Flags &= ~MQP_MI_F_LASTMATCHED;
		    if (rval == 1) mi->Flags |= MQP_MI_F_LASTMATCHED;
		    if (rval != 0) break;
		    }
		else
		    {
		    break;
		    }
		}

	    /** Return if valid row (1) or if error (-1) **/
	    if (rval != 0) break;

	    /** No more items in that source.  Try another one. **/
	    mqp_internal_CloseSource(qe);
	    }

    return rval;
    }


/*** mqpFinish - ends the projection operation and frees up any memory used
 *** in the process of running the query.
 ***/
int
mqpFinish(pQueryElement qe, pQueryStatement stmt)
    {
    pExpression del_exp;

    	/** Close the previous fetched object? **/
	if (stmt->Query->ObjList->Objects[qe->SrcIndex])
	    {
	    objClose(stmt->Query->ObjList->Objects[qe->SrcIndex]);
	    expModifyParamByID(stmt->Query->ObjList, qe->SrcIndex, NULL);
	    }

	if (qe->Constraint) expRemapID(qe->Constraint, qe->SrcIndex, qe->SrcIndex);

	/** Remove additional expression?? **/
	if ((qe->Flags & MQ_EF_ADDTLEXP) && (qe->Flags & MQ_EF_CONSTEXP))
	    {
	    del_exp = qe->Constraint;
	    qe->Constraint = (pExpression)(qe->Constraint->Children.Items[0]);
	    xaRemoveItem(&del_exp->Children,0);
	    xaRemoveItem(&del_exp->Children,0);
	    expFreeExpression(del_exp);
	    }
	else if (qe->Flags & MQ_EF_ADDTLEXP)
	    {
	    qe->Constraint = NULL;
	    }

	/** Close the source **/
	mqp_internal_CloseSource(qe);

    return 0;
    }


/*** mqpRelease - release any resources allocated (other than QueryElements)
 *** during the Analyze phase.
 ***/
int
mqpRelease(pQueryElement qe, pQueryStatement stmt)
    {
    int i;
    pMqpRowCache rc;
    pMqpOneRow row;
    pMqpInf mi = (pMqpInf)(qe->PrivateData);

    	/** Release the order-by expressions **/
	for(i=0;qe->OrderBy[i];i++)
	    {
	    expFreeExpression(qe->OrderBy[i]);
	    qe->OrderBy[i] = NULL;
	    }

	/** Clean up the list of sources **/
	for (i=0; i<mi->SourceList.nItems; i++)
	    nmSysFree(mi->SourceList.Items[i]);
	xaDeInit(&mi->SourceList);

	/** Release private data, if needed, from subtree data **/
	if (mi->Subtrees)
	    nmFree(mi->Subtrees, sizeof(MqpSubtrees));

	/** Release private data from row caching **/
	if (mi->RowCache)
	    {
	    rc = mi->RowCache;

	    /** Release any cached rows **/
	    while(rc->Cache.nItems)
		{
		row = (pMqpOneRow)xaGetItem(&rc->Cache, 0);
		if (row->Obj) objClose(row->Obj);
		nmSysFree(row->Criteria);
		nmFree(row, sizeof(MqpOneRow));
		xaRemoveItem(&rc->Cache, 0);
		}

	    /** Release the cache data itself **/
	    xaDeInit(&rc->Cache);
	    xsDeInit(&rc->CriteriaBuf);
	    nmFree(rc, sizeof(MqpRowCache));

	    mi->RowCache = NULL;
	    }

	/** Now the main private data structure **/
	nmFree(mi, sizeof(MqpInf));

    return 0;
    }


/*** mqpInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqpInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQP - MultiQuery Projection Module");
	drv->Precedence = 2000;
	drv->Flags = 0;
	drv->Analyze = mqpAnalyze;
	drv->Start = mqpStart;
	drv->NextItem = mqpNextItem;
	drv->Finish = mqpFinish;
	drv->Release = mqpRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQPINF.ThisDriver = drv;

	/*mqp_log = fdOpen("/tmp/optimlog.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);*/

    return 0;
    }
