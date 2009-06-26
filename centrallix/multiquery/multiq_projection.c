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
/* Module: 	multiq_projection.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 25, 1999					*/
/* Description:	Provides support for a Projection SQL operation, or 	*/
/*		pulling data items from an ObjectSystem query.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: multiq_projection.c,v 1.12 2009/06/26 16:04:26 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_projection.c,v $

    $Log: multiq_projection.c,v $
    Revision 1.12  2009/06/26 16:04:26  gbeeley
    - (feature) adding DELETE support
    - (change) HAVING clause now honored in INSERT ... SELECT
    - (bugfix) some join order issues resolved
    - (performance) cache 0 or 1 row result sets during a join
    - (feature) adding INCLUSIVE option to SUBTREE selects
    - (bugfix) switch to qprintf for building RawData sql data
    - (change) some minor refactoring

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

    Revision 1.6  2005/10/18 22:51:06  gbeeley
    - (bugfix) request writability on objects when query open for update.

    Revision 1.5  2005/09/24 20:19:18  gbeeley
    - Adding "select ... from subtree /path" support to the SQL engine,
      allowing the retrieval of an entire subtree with one query.  Uses
      the new virtual attr support to supply the relative path of each
      retrieved object.  Much the reverse of what a querytree object can
      do.
    - Memory leak fixes in multiquery.c
    - Fix for objdrv_ux regarding fetched objects and the obj->Pathname.

    Revision 1.4  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.3  2002/05/03 03:51:21  gbeeley
    Added objUnmanageObject() and objUnmanageQuery() which cause an object
    or query to not be closed automatically on session close.  This should
    NEVER be used with the intent of keeping an object or query open after
    session close, but rather it is used when the object or query would be
    closed in some other way, such as 'hidden' objects and queries that the
    multiquery layer opens behind the scenes (closing the multiquery objects
    and queries will cause the underlying ones to be closed).
    Also fixed some problems in the OSML where some objects and queries
    were not properly being added to the session's open objects and open
    queries lists.

    Revision 1.2  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.1.1.1  2001/08/13 18:00:54  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:57  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

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
mqp_internal_Recurse(pQueryElement qe, pMultiQuery mq, pObject obj)
    {
    pObjectInfo oi;
    pObjQuery newqy;
    pObject newobj;
    pMqpSubtrees ms = (pMqpSubtrees)(qe->PrivateData);

	/** Too many levels of recursion? **/
	if (ms->nStacked >= MQP_MAX_SUBTREE) return NULL;

	/** First, can we discern subobjs w/o running a query? **/
	oi = objInfo(obj);
	if (oi && (oi->Flags & OBJ_INFO_F_NO_SUBOBJ)) return NULL;

	/** Try running the query. **/
	newqy = objOpenQuery(obj, NULL, NULL, qe->Constraint, (void**)(qe->OrderBy[0]?qe->OrderBy:NULL));
	if (!newqy) return NULL;
	objUnmanageQuery(mq->SessionID, newqy);
	newobj = objQueryFetch(newqy, (mq->Flags & MQ_F_ALLOWUPDATE)?O_RDWR:O_RDONLY);
	if (!newobj)
	    {
	    objQueryClose(newqy);
	    return NULL;
	    }
	objUnmanageObject(mq->SessionID, newobj);

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
mqp_internal_Return(pQueryElement qe, pMultiQuery mq, pObject obj)
    {
    pMqpSubtrees ms = (pMqpSubtrees)(qe->PrivateData);
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
    pMqpSubtrees ms = (pMqpSubtrees)(qe->PrivateData);
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
mqpAnalyze(pMultiQuery mq)
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

    	/** Search for FROM clauses for this driver... **/
	while((from_qs = mq_internal_FindItem(mq->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
	    {
	    if (from_qs->Flags & MQ_SF_USED) continue;

	    /** allocate one query element for each from clause. **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQPINF.ThisDriver;
	    qe->QSLinkage = (void*)from_qs;
	    qe->OrderPrio = 999;

	    /** Find the index of the object for this FROM clause **/
	    src_idx = expLookupParam(mq->ObjList, from_qs->Presentation[0]?(from_qs->Presentation):(from_qs->Source));
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
			    mq->Flags |= MQ_F_ASTERISK;
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
		    if ((where_item->Expr->ObjCoverageMask & (~mq->ProvidedObjMask)) == (1<<src_idx))
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
		    if (item->ObjCnt == 1 && (item->ObjFlags[src_idx] & EXPR_O_REFERENCED) && item->Expr)
		        {
			j=0;
			while(qe->OrderBy[j]) j++;
			if ((j+1) >= sizeof(qe->OrderBy) / sizeof(*(qe->OrderBy))) break;
			if (qe->OrderPrio == 999 || qe->OrderPrio > i) qe->OrderPrio = i;
			qe->OrderBy[j] = item->Expr;
			item->Expr = NULL;
			qe->OrderBy[j+1] = NULL;
			expRemapID(qe->OrderBy[j], src_idx, 0);
			}
		    }
		}

	    /** If we got a constraint expression, replace the obj id's with id #0. **/
	    /** This is so the obj driver can evaluate it against a single-object objlist **/
	    /*if (qe->Constraint) expRemapID(qe->Constraint, src_idx, 0);*/

	    /** Link into the multiquery **/
	    xaAddItem(&mq->Trees, qe);
	    mq->Tree = qe;

	    /** Setup the object that this will use. **/
	    qe->SrcIndex = src_idx;
	    if (from_qs->Flags & MQ_SF_FROMSUBTREE) qe->Flags |= MQ_EF_FROMSUBTREE;
	    if (from_qs->Flags & MQ_SF_INCLSUBTREE) qe->Flags |= MQ_EF_INCLSUBTREE;
	    from_qs->Flags |= MQ_SF_USED;
	    from_qs->QELinkage = qe;

	    /** Setup private data if needed **/
	    if (qe->Flags & MQ_EF_FROMSUBTREE)
		{
		qe->PrivateData = (pMqpSubtrees)nmMalloc(sizeof(MqpSubtrees));
		memset(qe->PrivateData, 0, sizeof(MqpSubtrees));
		}
	    }

    return 0;
    }


/*** mqp_internal_CheckConstraint - validate the constraint, if any, on the
 *** current qe and mq.  Return -1 on error, 0 if fail, 1 if pass.
 ***/
int
mqp_internal_CheckConstraint(pQueryElement qe, pMultiQuery mq)
    {

	/** Validate the constraint expression, otherwise succeed by default **/
        if (qe->Constraint)
            {
            expEvalTree(qe->Constraint, mq->ObjList);
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
mqp_internal_CacheRow(pMultiQuery mq, pQueryElement qe, pExpression criteria, pObject obj)
    {
    pMqpRowCache rc;
    pMqpOneRow row;
    pMqpOneRow fewest_matches;
    int fewest_matches_id;
    int i;

	/** Set up the cache itself? **/
	if (!qe->PrivateData)
	    {
	    qe->PrivateData = nmMalloc(sizeof(MqpRowCache));
	    rc = (pMqpRowCache)(qe->PrivateData);
	    xaInit(&rc->Cache, MQP_MAX_ROWCACHE);
	    xsInit(&rc->CriteriaBuf);
	    rc->LastMatch = 0;
	    }
	else
	    rc = (pMqpRowCache)(qe->PrivateData);

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
	xsCopy(&rc->CriteriaBuf, "", 0);
	if (criteria) expGenerateText(criteria, mq->ObjList, xsWrite, &rc->CriteriaBuf, '\0', "cxsql", 0);
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
mqp_internal_CheckCache(pMultiQuery mq, pMqpRowCache rc, pExpression criteria)
    {
    int i;
    pMqpOneRow row;

	xsCopy(&rc->CriteriaBuf, "", 0);
	if (criteria) expGenerateText(criteria, mq->ObjList, xsWrite, &rc->CriteriaBuf, '\0', "cxsql", 0);
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


/*** mqpStart - starts the query operation for a given projection element
 *** in the query.  Does not fetch the first row -- that is what NextItem
 *** is there for.
 ***/
int
mqpStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    pExpression new_exp;
    pMqpRowCache rc;
    int using_cache = 0;

    	/** Open the data source in the objectsystem **/
	if (mq->Flags & MQ_F_ALLOWUPDATE)
	    qe->LLSource = objOpen(mq->SessionID, ((pQueryStructure)qe->QSLinkage)->Source, O_RDWR, 0600, "system/directory");
	else
	    qe->LLSource = objOpen(mq->SessionID, ((pQueryStructure)qe->QSLinkage)->Source, O_RDONLY, 0600, "system/directory");
	if (!qe->LLSource) 
	    {
	    mssError(0,"MQP","Could not open source object for SQL projection");
	    return -1;
	    }
	objUnmanageObject(mq->SessionID, qe->LLSource);

	/** Additional expression supplied?? **/
	if (additional_expr) qe->Flags |= MQ_EF_ADDTLEXP;
	if (qe->Constraint) qe->Flags |= MQ_EF_CONSTEXP;
	if (additional_expr && qe->Constraint)
	    {
	    new_exp = expAllocExpression();
	    new_exp->NodeType = EXPR_N_AND;
	    expAddNode(new_exp, qe->Constraint);
	    expAddNode(new_exp, additional_expr);
	    qe->Constraint = new_exp;
	    }
	if (!qe->Constraint) qe->Constraint = additional_expr;

        if (qe->Constraint) expRemapID(qe->Constraint, qe->SrcIndex, 0);

	/** Check for cached single row result set **/
	if (!(qe->Flags & MQ_EF_FROMSUBTREE))
	    {
	    rc = (pMqpRowCache)(qe->PrivateData);
	    if (rc)
		{
		rc->CurCached = mqp_internal_CheckCache(mq, rc, qe->Constraint);
		if (rc->CurCached >= 0)
		    {
		    /** We're going from the cache this time **/
		    objClose(qe->LLSource);
		    qe->LLSource = NULL;
		    using_cache = 1;
		    }
		}
	    }

    	/** Open the query with the objectsystem. **/
	if ((qe->Flags & MQ_EF_FROMSUBTREE) && (qe->Flags & MQ_EF_INCLSUBTREE))
	    {
	    qe->LLQuery = NULL;
	    }
	else if (!using_cache)
	    {
	    qe->LLQuery = objOpenQuery(qe->LLSource, NULL, NULL, qe->Constraint, (void**)(qe->OrderBy[0]?qe->OrderBy:NULL));
	    if (!qe->LLQuery) 
		{
		mqpFinish(qe,mq);
		mssError(0,"MQP","Could not query source object for SQL projection");
		return -1;
		}
	    objUnmanageQuery(mq->SessionID, qe->LLQuery);
	    }
	qe->IterCnt = 0;

    return 0;
    }


/*** mqpNextItem - retrieves the first/next item in the result set for the
 *** projection.  Returns 1 if valid row obtained, 0 if no more rows are
 *** available, and -1 on error.
 ***/
int
mqpNextItem(pQueryElement qe, pMultiQuery mq)
    {
    pObject obj;
    pObject saved_obj = NULL;
    pMqpRowCache rc;
    pMqpOneRow row;

	qe->IterCnt++;

	/** Using cache? **/
	if (qe->PrivateData && !(qe->Flags & MQ_EF_FROMSUBTREE))
	    {
	    rc = (pMqpRowCache)(qe->PrivateData);
	    if (rc->CurCached >= 0)
		{
		row = xaGetItem(&rc->Cache, rc->CurCached);
		if (qe->IterCnt == 1)
		    {
		    if (row->Obj) objLinkTo(row->Obj);
		    expModifyParam(mq->ObjList, mq->ObjList->Names[qe->SrcIndex], row->Obj);
		    return row->Obj?1:0;
		    }
		else if (qe->IterCnt == 2)
		    {
		    if (mq->ObjList->Objects[qe->SrcIndex])
			objClose(mq->ObjList->Objects[qe->SrcIndex]);
		    mq->ObjList->Objects[qe->SrcIndex] = NULL;
		    }
		return 0;
		}
	    }

	/** Inclusive subtree?  If so return the top-level src object too **/
	if ((qe->Flags & MQ_EF_FROMSUBTREE) && (qe->Flags & MQ_EF_INCLSUBTREE) && !mq->ObjList->Objects[qe->SrcIndex] && qe->IterCnt == 1)
	    {
	    obj = objLinkTo(qe->LLSource);
	    expModifyParam(mq->ObjList, mq->ObjList->Names[qe->SrcIndex], obj);

	    if (mqp_internal_CheckConstraint(qe, mq) > 0)
		{
		/** the top level one matches.  use it. **/
		mqp_internal_SetupSubtreeAttrs(qe, obj);
		return 1;
		}
	    else
		{
		objClose(mq->ObjList->Objects[qe->SrcIndex]);
		mq->ObjList->Objects[qe->SrcIndex] = NULL;
		}
	    }

	/** Attempt to recurse a subtree? **/
	if (qe->Flags & MQ_EF_FROMSUBTREE && mq->ObjList->Objects[qe->SrcIndex])
	    {
	    obj = mqp_internal_Recurse(qe, mq, mq->ObjList->Objects[qe->SrcIndex]);
	    if (obj)
		{
		expModifyParam(mq->ObjList, mq->ObjList->Names[qe->SrcIndex], obj);
		return 1;
		}
	    }

	/** Loop is for when we are returning from deep recursion **/
	while(1)
	    {
	    /** Close the previous fetched object? **/
	    if (mq->ObjList->Objects[qe->SrcIndex])
		{
		/** if this is after the user has processed row #1, create a tmp copy of row 1 **/
		if (qe->LLQuery && qe->IterCnt == 2 && !(qe->Flags & MQ_EF_FROMSUBTREE))
		    saved_obj = objLinkTo(mq->ObjList->Objects[qe->SrcIndex]);

		/** "close" it **/
		objClose(mq->ObjList->Objects[qe->SrcIndex]);
		mq->ObjList->Objects[qe->SrcIndex] = NULL;
		}

	    /** Fetch the next item and set the object... **/
	    if (!qe->LLQuery)
		return 0;
	    obj = objQueryFetch(qe->LLQuery, (mq->Flags & MQ_F_ALLOWUPDATE)?O_RDWR:O_RDONLY);
	    if (obj)
		{
		/** Got one. **/
		if (saved_obj) objClose(saved_obj);
		objUnmanageObject(mq->SessionID, obj);
		expModifyParam(mq->ObjList, mq->ObjList->Names[qe->SrcIndex], obj);
		if (qe->Flags & MQ_EF_FROMSUBTREE) mqp_internal_SetupSubtreeAttrs(qe, obj);
		return 1;
		}

	    /** No more objects - return from a subtree? **/
	    if (qe->Flags & MQ_EF_FROMSUBTREE)
		{
		obj = mqp_internal_Return(qe, mq, NULL);
		if (!obj || obj == qe->LLSource) return 0;
		expModifyParam(mq->ObjList, mq->ObjList->Names[qe->SrcIndex], obj);
		}
	    else
		{
		if (qe->IterCnt == 1 || (qe->IterCnt == 2 && saved_obj))
		    {
		    /** empty or single-row result set.  Save the saved_obj in the cache **/
		    mqp_internal_CacheRow(mq, qe, qe->Constraint, saved_obj);
		    }
		return 0;
		}
	    }

    return 0;
    }


/*** mqpFinish - ends the projection operation and frees up any memory used
 *** in the process of running the query.
 ***/
int
mqpFinish(pQueryElement qe, pMultiQuery mq)
    {
    pExpression del_exp;

    	/** Close the previous fetched object? **/
	if (mq->ObjList->Objects[qe->SrcIndex])
	    {
	    objClose(mq->ObjList->Objects[qe->SrcIndex]);
	    mq->ObjList->Objects[qe->SrcIndex] = NULL;
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

    	/** Close the source object and the query. **/
	if (qe->LLQuery) objQueryClose(qe->LLQuery);
	if (qe->LLSource) objClose(qe->LLSource);
	qe->LLQuery = NULL;
	qe->LLSource = NULL;

    return 0;
    }


/*** mqpRelease - release any resources allocated (other than QueryElements)
 *** during the Analyze phase.
 ***/
int
mqpRelease(pQueryElement qe, pMultiQuery mq)
    {
    int i;
    pMqpRowCache rc;
    pMqpOneRow row;

    	/** Release the order-by expressions **/
	for(i=0;qe->OrderBy[i];i++)
	    {
	    expFreeExpression(qe->OrderBy[i]);
	    qe->OrderBy[i] = NULL;
	    }

	/** Release private data, if needed, from subtree data **/
	if (qe->Flags & MQ_EF_FROMSUBTREE)
	    nmFree(qe->PrivateData, sizeof(MqpSubtrees));

	/** Release private data from row caching **/
	if (!(qe->Flags & MQ_EF_FROMSUBTREE) && qe->PrivateData)
	    {
	    rc = (pMqpRowCache)(qe->PrivateData);

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
	    }

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
