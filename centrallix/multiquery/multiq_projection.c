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

    $Id: multiq_projection.c,v 1.4 2005/02/26 06:42:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_projection.c,v $

    $Log: multiq_projection.c,v $
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
    int src_idx,i,n=0,j;
    pExpression new_exp;

    	/** Search for FROM clauses for this driver... **/
	while((from_qs = mq_internal_FindItem(mq->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
	    {
	    if (from_qs->Flags & MQ_SF_USED) continue;

	    /** allocate one query element for each from clause. **/
	    qe = mq_internal_AllocQE();
	    qe->Driver = MQPINF.ThisDriver;
	    qe->QSLinkage = (void*)from_qs;

	    /** Find the index of the object for this FROM clause **/
	    src_idx = -1;
	    for(i=0;i<mq->QTree->ObjList->nObjects;i++)
	        {
		if (!strcmp(mq->QTree->ObjList->Names[i],from_qs->Presentation[0]?(from_qs->Presentation):(from_qs->Source)))
		    {
		    src_idx = i;
		    break;
		    }
		}
	    if (src_idx == -1)
	        {
		mq_internal_FreeQE(qe);
		continue;
		}

	    /** Find the SELECT clause associated with this FROM clause. **/
	    select_qs = mq_internal_FindItem(from_qs->Parent->Parent, MQ_T_SELECTCLAUSE, NULL);
	    if (!select_qs)
	        {
		mq_internal_FreeQE(qe);
		continue;
		}
	    
	    /** Loop through the select items looking for ones that match this FROM source **/
	    for(i=0;i<select_qs->Children.nItems;i++)
	        {
		select_item = (pQueryStructure)(select_qs->Children.Items[i]);
		if (select_item->QELinkage != NULL) continue;
		if (select_item->ObjCnt == 1 && (select_item->ObjFlags[src_idx] & EXPR_O_REFERENCED))
		    {
		    xaAddItem(&qe->AttrNames, (void*)select_item->Presentation);
		    xaAddItem(&qe->AttrExprPtr, (void*)select_item->RawData.String);
		    xaAddItem(&qe->AttrCompiledExpr, (void*)select_item->Expr);
		    select_item->QELinkage = qe;
		    xaAddItem(&qe->AttrDeriv, (void*)NULL);
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
		    if (where_item->Expr->ObjCoverageMask == (1<<src_idx))
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
			    xaAddItem(&new_exp->Children, (void*)qe->Constraint);
			    xaAddItem(&new_exp->Children, (void*)where_item->Expr);
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
	    while(mq->Trees[n]) n++;
	    mq->Trees[n] = qe;
	    mq->Tree = qe;

	    /** Setup the object that this will use. **/
	    qe->SrcIndex = src_idx;
	    from_qs->Flags |= MQ_SF_USED;
	    }

    return 0;
    }


/*** mqpStart - starts the query operation for a given projection element
 *** in the query.  Does not fetch the first row -- that is what NextItem
 *** is there for.
 ***/
int
mqpStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    pExpression new_exp;

    	/** Open the data source in the objectsystem **/
	if (mq->Flags & MQ_F_ALLOWUPDATE)
	    qe->LLSource = objOpen(mq->SessionID, ((pQueryStructure)qe->QSLinkage)->Source, O_RDWR, 0600, "system/directory");
	else
	    qe->LLSource = objOpen(mq->SessionID, ((pQueryStructure)qe->QSLinkage)->Source, O_RDONLY, 0600, "system/directory");
	objUnmanageObject(mq->SessionID, qe->LLSource);
	if (!qe->LLSource) 
	    {
	    mssError(0,"MQP","Could not open source object for SQL projection");
	    return -1;
	    }

	/** Additional expression supplied?? **/
	if (additional_expr) qe->Flags |= MQ_EF_ADDTLEXP;
	if (qe->Constraint) qe->Flags |= MQ_EF_CONSTEXP;
	if (additional_expr && qe->Constraint)
	    {
	    new_exp = expAllocExpression();
	    new_exp->NodeType = EXPR_N_AND;
	    xaAddItem(&new_exp->Children,(void*)qe->Constraint);
	    xaAddItem(&new_exp->Children,(void*)additional_expr);
	    qe->Constraint = new_exp;
	    }
	if (!qe->Constraint) qe->Constraint = additional_expr;

        if (qe->Constraint) expRemapID(qe->Constraint, qe->SrcIndex, 0);

    	/** Open the query with the objectsystem. **/
	qe->LLQuery = objOpenQuery(qe->LLSource, NULL, NULL, qe->Constraint, (void**)(qe->OrderBy[0]?qe->OrderBy:NULL));
	if (!qe->LLQuery) 
	    {
	    objClose(qe->LLSource);
	    mssError(0,"MQP","Could not query source object for SQL projection");
	    return -1;
	    }
	objUnmanageQuery(mq->SessionID, qe->LLQuery);
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

    	/** Close the previous fetched object? **/
	if (mq->QTree->ObjList->Objects[qe->SrcIndex])
	    {
	    objClose(mq->QTree->ObjList->Objects[qe->SrcIndex]);
	    mq->QTree->ObjList->Objects[qe->SrcIndex] = NULL;
	    }

    	/** Fetch the next item and set the object... **/
	obj = objQueryFetch(qe->LLQuery, O_RDONLY);
	if (!obj) return 0;
	objUnmanageObject(mq->SessionID, obj);
	expModifyParam(mq->QTree->ObjList, mq->QTree->ObjList->Names[qe->SrcIndex], obj);

    return 1;
    }


/*** mqpFinish - ends the projection operation and frees up any memory used
 *** in the process of running the query.
 ***/
int
mqpFinish(pQueryElement qe, pMultiQuery mq)
    {
    pExpression del_exp;


    	/** Close the previous fetched object? **/
	if (mq->QTree->ObjList->Objects[qe->SrcIndex])
	    {
	    objClose(mq->QTree->ObjList->Objects[qe->SrcIndex]);
	    mq->QTree->ObjList->Objects[qe->SrcIndex] = NULL;
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
	objQueryClose(qe->LLQuery);
	objClose(qe->LLSource);

    return 0;
    }


/*** mqpRelease - release any resources allocated (other than QueryElements)
 *** during the Analyze phase.
 ***/
int
mqpRelease(pQueryElement qe, pMultiQuery mq)
    {
    int i;

    	/** Release the order-by expressions **/
	for(i=0;qe->OrderBy[i];i++)
	    {
	    expFreeExpression(qe->OrderBy[i]);
	    qe->OrderBy[i] = NULL;
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

    return 0;
    }
