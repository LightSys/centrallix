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

/**CVSDATA***************************************************************

    $Id: multiq_insertselect.c,v 1.2 2008/03/19 07:30:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiq_insertselect.c,v $

    $Log: multiq_insertselect.c,v $
    Revision 1.2  2008/03/19 07:30:53  gbeeley
    - (feature) adding UPDATE statement capability to the multiquery module.
      Note that updating was of course done previously, but not via SQL
      statements - it was programmatic via objSetAttrValue.
    - (bugfix) fixes for two bugs in the expression module, one a memory leak
      and the other relating to null values when copying expression values.
    - (bugfix) the Trees array in the main multiquery structure could
      overflow; changed to an xarray.

    Revision 1.1  2008/03/14 18:25:44  gbeeley
    - (feature) adding INSERT INTO ... SELECT support, for creating new data
      using SQL as well as using SQL to copy rows around between different
      objects.


 **END-CVSDATA***********************************************************/


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
mqisAnalyze(pMultiQuery mq)
    {
    pQueryStructure select_qs, insert_qs;
    pQueryElement qe;
    int n;

    	/** Search for an INSERT and a SELECT statement... **/
	select_qs = mq_internal_FindItem(mq->QTree, MQ_T_SELECTCLAUSE, NULL);
	insert_qs = mq_internal_FindItem(mq->QTree, MQ_T_INSERTCLAUSE, NULL);

	/** We get to sit on the bench this time? **/
	if (!select_qs || !insert_qs)
	    return 0;

	/** Bad news if other drivers haven't found a SELECT tree **/
	if (!mq->Tree)
	    return -1;

	/** Alloc a queryexec node **/
	qe = mq_internal_AllocQE();
	qe->Driver = MQISINF.ThisDriver;

	/** link in with insert clause **/
	insert_qs->QELinkage = qe;
	qe->QSLinkage = insert_qs;

	/** Link the qe into the multiquery **/
	n=0;
	xaAddItem(&mq->Trees, qe);
	xaAddItem(&qe->Children, mq->Tree);
	mq->Tree = qe;

    return 0;
    }


/*** mqisStart - starts the query operation for the insert.  This actually
 *** does all of the work of iterating the selected rows and doing the
 *** resulting inserts.  Fetch/NextItem always just returns NULL.
 ***/
int
mqisStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    pQueryElement sel;
    int rval = -1;
    int sel_rval = 0;
    char pathname[OBJSYS_MAX_PATH];
    pObject new_obj = NULL;
    int is_started = 0;
    int attrid, astobjid;
    char* attrname;
    ObjData od;
    int t;
    int use_attrid;

	/** Prepare for the inserts **/
	if (strlen(((pQueryStructure)qe->QSLinkage)->Source) + 2 >= sizeof(pathname))
	    {
	    mssError(1, "MQIS", "Pathname too long for INSERT destination");
	    goto error;
	    }
	snprintf(pathname, sizeof(pathname), "%s/*", ((pQueryStructure)qe->QSLinkage)->Source);
	    
	/** Start the SELECT query **/
	sel = (pQueryElement)(qe->Children.Items[0]);
	if (sel->Driver->Start(sel, mq, NULL) < 0)
	    goto error;
	is_started = 1;

	/** Select all items in the result set **/
	while((sel_rval = sel->Driver->NextItem(sel, mq)) == 1)
	    {
	    /** open a new object **/
	    new_obj = objOpen(mq->SessionID, pathname, OBJ_O_RDWR | OBJ_O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");
	    if (!new_obj)
		goto error;
	   
	    /** Set all of the SELECTed attributes **/
	    attrid = 0;
	    astobjid = -1;
	    while(1)
		{
		use_attrid = attrid;
		attrname = mq_internal_QEGetNextAttr(mq, sel, mq->ObjList, &attrid, &astobjid);
		if (!attrname) break;
		if (astobjid >= 0)
		    {
		    /** attr available direct from object via SELECT * **/
		    t = objGetAttrType(mq->ObjList->Objects[astobjid], attrname);
		    if (t <= 0)
			continue;
		    if (objGetAttrValue(mq->ObjList->Objects[astobjid], attrname, t, &od) != 0)
			continue;
		    }
		else
		    {
		    /** attr available through SELECT item list **/
		    if (expEvalTree((pExpression)sel->AttrCompiledExpr.Items[use_attrid], mq->ObjList) < 0)
			goto error;
		    t = ((pExpression)sel->AttrCompiledExpr.Items[use_attrid])->DataType;
		    if (t <= 0)
			continue;
		    if (expExpressionToPod((pExpression)(sel->AttrCompiledExpr.Items[use_attrid]), t, &od) < 0)
			goto error;
		    }

		/** Set the attribute on the newly inserted object **/
		if (objSetAttrValue(new_obj, attrname, t, &od) < 0)
		    goto error;
		}
	    objClose(new_obj);
	    new_obj = NULL;
	    }

	if (sel_rval < 0)
	    goto error;

	rval = 0;

    error:
	/** Close the SELECT **/
	if (is_started)
	    if (sel->Driver->Finish(sel, mq) < 0)
		return -1;
	if (new_obj)
	    objClose(new_obj);

    return rval;
    }


/*** mqisNextItem - retrieves the first/next item.  This always returns
 *** 0 (end of results) because an insert does not generate a result set.
 ***/
int
mqisNextItem(pQueryElement qe, pMultiQuery mq)
    {
    return 0;
    }


/*** mqisFinish - ends the operation.
 ***/
int
mqisFinish(pQueryElement qe, pMultiQuery mq)
    {
    return 0;
    }


/*** mqisRelease - release any private data allocated (none)
 ***/
int
mqisRelease(pQueryElement qe, pMultiQuery mq)
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
