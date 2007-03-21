#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/magic.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_query.c: handles directory interfacing and 	*/
/*		objectsystem query support.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_query.c,v 1.14 2007/03/21 04:48:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_query.c,v $

    $Log: obj_query.c,v $
    Revision 1.14  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.13  2005/09/24 20:15:43  gbeeley
    - Adding objAddVirtualAttr() to the OSML API, which can be used to add
      an attribute to an object which invokes callback functions to get the
      attribute values, etc.
    - Changing objLinkTo() to return the linked-to object (same thing that
      got passed in, but good for style in reference counting).
    - Cleanup of some memory leak issues in objOpenQuery()

    Revision 1.12  2005/09/17 01:35:10  gbeeley
    - preset default values for SubPtr and SubCnt for child objects
      returned from objQueryFetch()

    Revision 1.11  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.10  2004/08/27 01:28:32  jorupp
     * cleaning up some compile warnings

    Revision 1.9  2004/07/30 04:25:35  gbeeley
    - need to recognize asc/desc in order by text

    Revision 1.8  2004/06/12 04:02:28  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.7  2003/07/10 19:20:57  gbeeley
    objOpenQuery now links to the parent object of the query to prevent it
    from being pulled out from under the osdriver if the user calls objClose
    on the parent object before calling objQueryClose on the query.

    Revision 1.6  2003/07/09 18:05:59  gbeeley
    Interim fix for 'order by' causing objects to be fetched differently
    because of the query-sort-reopen logic.  Now uses open-as to reopen
    objects in a way that won't cause other drivers to be invoked.

    Revision 1.5  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.4  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

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

    Revision 1.2  2002/04/25 17:59:59  gbeeley
    Added better magic number support in the OSML API.  ObjQuery and
    ObjSession structures are now protected with magic numbers, and
    support for magic numbers in Object structures has been improved
    a bit.

    Revision 1.1.1.1  2001/08/13 18:00:59  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:01  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** obj_internal_SortCompare - compares two items in the sorting 
 *** criteria.
 ***/
int
obj_internal_SortCompare(pObjQuerySort sortinf, int bufferid1, int bufferid2, int id1, int id2)
    {
    int len1,len2,n;
    int r;
    char *ptr1, *ptr2;
    	
	len1 = (int)(sortinf->SortPtrLen[bufferid1].Items[id1]);
	len2 = (int)(sortinf->SortPtrLen[bufferid2].Items[id2]);
	ptr1 = sortinf->SortPtr[bufferid1].Items[id1];
	ptr2 = sortinf->SortPtr[bufferid2].Items[id2];
	n = (len1>len2)?len2:len1;
	r = memcmp(ptr1,ptr2,n);
	if (r == 0) r = len1-len2;

    return r;
    }


/*** obj_internal_MergeSort - mergesorts a result set given the sort inf
 *** structure.  This merge algorithm switches to a selection sort when the
 *** number of items is less than about 16.
 ***/
int
obj_internal_MergeSort(pObjQuerySort sortinf, int bufferid, int startid, int endid)
    {
    int i,j,k,found,n = (endid-startid+1);

    	/** Fewer than 16 items?  If so, selection sort. **/
	if (n < 16)
	    {
	    for(i=startid;i<startid+n;i++)
	        {
		found = -1;
		for(j=startid;j<startid+n;j++)
		    {
		    if (sortinf->SortPtr[bufferid].Items[j])
		        {
			if (found == -1)
			    {
			    found = j;
			    }
			else
			    {
			    if (obj_internal_SortCompare(sortinf,bufferid,bufferid, found, j) > 0) found=j;
			    }
			}
		    }
		sortinf->SortPtr[1-bufferid].Items[i] = sortinf->SortPtr[bufferid].Items[found];
		sortinf->SortPtrLen[1-bufferid].Items[i] = sortinf->SortPtrLen[bufferid].Items[found];
		sortinf->SortNames[1-bufferid].Items[i] = sortinf->SortNames[bufferid].Items[found];
		sortinf->SortPtr[bufferid].Items[found] = NULL;
		}
	    return 0;
	    }

	/** Otherwise, mergesort.  Sort the halves first. **/
	obj_internal_MergeSort(sortinf, 1-bufferid, startid, startid+(n/2));
	obj_internal_MergeSort(sortinf, 1-bufferid, startid+(n/2)+1, endid);

	/** Now, merge the halves together into the other buffer **/
	i=startid;
	j=startid+(n/2)+1;
	for(k=startid;k<=endid;k++)
	    {
	    /** Pick which side to choose from. **/
	    if (i > startid+(n/2))
	        found = 1;
	    else if (j > endid)
	        found = 0;
	    else if (obj_internal_SortCompare(sortinf, bufferid, bufferid, i,j) > 0)
	        found = 1;
	    else
	        found = 0;

	    /** Ok. Found the lowest one.  Now transfer to the other buffer. **/
	    if (found == 0)
	        {
                sortinf->SortPtr[1-bufferid].Items[k] = sortinf->SortPtr[bufferid].Items[i];
                sortinf->SortPtrLen[1-bufferid].Items[k] = sortinf->SortPtrLen[bufferid].Items[i];
                sortinf->SortNames[1-bufferid].Items[k] = sortinf->SortNames[bufferid].Items[i];
		i++;
		}
	    else
	        {
                sortinf->SortPtr[1-bufferid].Items[k] = sortinf->SortPtr[bufferid].Items[j];
                sortinf->SortPtrLen[1-bufferid].Items[k] = sortinf->SortPtrLen[bufferid].Items[j];
                sortinf->SortNames[1-bufferid].Items[k] = sortinf->SortNames[bufferid].Items[j];
		j++;
		}
	    }

    return 0;
    }


/*** objMultiQuery - issue a query against one or more directories in
 *** the objectsystem, potentially performing joins.
 ***/
pObjQuery 
objMultiQuery(pObjSession session, char* query)
    {
    pObjQuery this;

	ASSERTMAGIC(session, MGK_OBJSESSION);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objMultiQuery(%8.8X,%s)... ", (int)session, query);

    	/** Allocate a query structure **/
	this = (pObjQuery)nmMalloc(sizeof(ObjQuery));
	if (!this) return NULL;
	memset(this,0,sizeof(ObjQuery));
	this->QyText = query;
	this->Drv = OSYS.MultiQueryLayer;
	this->QySession = session;
	this->Magic = MGK_OBJQUERY;
	this->Obj = NULL;

	/** Start the query. **/
	this->Data = this->Drv->OpenQuery(session, query);
	if (!this->Data)
	    {
	    nmFree(this,sizeof(ObjQuery));
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE," null\n");
	    return NULL;
	    }

	/** Add to session open queries... **/
	xaAddItem(&(session->OpenQueries),(void*)this);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE," %8.8X\n", (int)this);

    return this;
    }


/*** objOpenQuery - issue a query against just one object, with no joins.
 ***/
pObjQuery 
objOpenQuery(pObject obj, char* query, char* order_by, void* tree_v, void** orderby_exp_v)
    {
    pObjQuery this = NULL;
    pExpression tree = (pExpression)tree_v;
    pExpression *orderbyexp = (pExpression*)orderby_exp_v;
    int i,n,len,j;
    short sn;
    pExpression sort_item;
    pLxSession lxs;
    pObject tmp_obj;
    char* ptr;
    char* start_ptr;
    char dbuf[8];
    pObject linked_obj = NULL;

    	ASSERTMAGIC(obj,MGK_OBJECT);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objOpenQuery(%8.8X:%3.3s:%s,'%s','%s')... ", (int)obj, obj->Driver->Name, obj->Pathname->Pathbuf, query, order_by);

	/** Allocate a query object **/
	this = (pObjQuery)nmMalloc(sizeof(ObjQuery));
	if (!this) 
	    goto error_return;
	this->QyText = query;
	this->Drv = NULL;
	this->Flags = 0;
	this->SortInf = NULL;
	this->Magic = MGK_OBJQUERY;
	this->ObjList = NULL;
	this->Tree = NULL;

	/** Ok, first parse the query. **/
	linked_obj = objLinkTo(obj);
	this->Obj = linked_obj;
        this->ObjList = (void*)expCreateParamList();
	expAddParamToList((pParamObjects)(this->ObjList), NULL, NULL, 0);
	if (query && *query)
	    {
	    this->Tree = (void*)expCompileExpression(query, (pParamObjects)(this->ObjList), MLX_F_ICASE | MLX_F_FILENAMES, 0);
	    if (!(this->Tree))
	        {
		mssError(0,"OSML","Query search criteria is invalid");
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
		goto error_return;
		}
	    this->Flags |= OBJ_QY_F_ALLOCTREE;
	    }
	else
	    {
	    if (tree)
	        {
	        this->Tree = tree;
		}
	    else
	        {
		this->Tree = NULL;
		}
	    }

	/** Now, parse the order-by clause **/
	if (orderbyexp)
	    {
	    for(i=0;orderbyexp[i];i++)
	        {
		this->SortBy[i] = orderbyexp[i];
		}
	    this->SortBy[i] = orderbyexp[i];
	    }
	else if (order_by)
	    {
	    lxs = mlxStringSession(order_by, MLX_F_EOF | MLX_F_FILENAMES | MLX_F_ICASER);
	    for(i=0;(sort_item=exp_internal_CompileExpression_r(lxs, 0, this->ObjList, EXPR_CMP_ASCDESC));i++)
	        {
		this->SortBy[i] = sort_item;
		}
	    this->SortBy[i] = NULL;
	    }
	else
	    {
	    this->SortBy[0] = NULL;
	    }

	/** Issue to driver **/
	this->Data = linked_obj->Driver->OpenQuery(linked_obj->Data,this,&(linked_obj->Session->Trx));

	if (!(this->Data))
	    {
	    mssError(0,"OSML","Either queries not supported on this object or query failed");
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
	    goto error_return;
	    }

	/** Add to session open queries... **/
	xaAddItem(&(linked_obj->Session->OpenQueries),(void*)this);
	
	/** If sort requested and driver no support, set from sort flag and start the sort **/
	if (this->SortBy[0] && !(this->Flags & OBJ_QY_F_FULLSORT))
	    {
	    /** Ok, first item of business is to read entire result set.  Init the sort structure **/
	    this->SortInf = (pObjQuerySort)nmMalloc(sizeof(ObjQuerySort));
	    xaInit(this->SortInf->SortPtr+0,4096);
	    xaInit(this->SortInf->SortPtrLen+0,4096);
	    xaInit(this->SortInf->SortNames+0,4096);
	    xsInit(&this->SortInf->SortDataBuf);
	    xsInit(&this->SortInf->SortNamesBuf);

	    /** Read result set **/
	    while((tmp_obj = objQueryFetch(this, 0400)))
	        {
		xaAddItem(this->SortInf->SortNames+0, (void*)(xsStringEnd(&this->SortInf->SortNamesBuf) - this->SortInf->SortNamesBuf.String));
		objGetAttrValue(tmp_obj,"name",DATA_T_STRING,POD(&ptr));
		xsConcatenate(&this->SortInf->SortNamesBuf, ptr, strlen(ptr)+1);
		expModifyParam(this->ObjList, NULL, tmp_obj);
		start_ptr = xsStringEnd(&this->SortInf->SortDataBuf);
		xaAddItem(this->SortInf->SortPtr+0, (void*)(start_ptr - this->SortInf->SortDataBuf.String));
		len = 0;
		for(i=0;this->SortBy[i];i++)
		    {
		    sort_item = this->SortBy[i];
		    if (expEvalTree(sort_item, this->ObjList) < 0)
		        {
			len++;
			xsConcatenate(&this->SortInf->SortDataBuf, "0", 1);
			}
		    else if (sort_item->Flags & EXPR_F_NULL)
		        {
			len++;
			xsConcatenate(&this->SortInf->SortDataBuf, "0", 1);
			}
		    else
		        {
			len++;
			xsConcatenate(&this->SortInf->SortDataBuf, "1", 1);
			switch(sort_item->DataType)
			    {
			    case DATA_T_INTEGER:
			        n = htonl(sort_item->Integer + 0x80000000);
			        if (sort_item->Flags & EXPR_F_DESC) n = ~n;
			        xsConcatenate(&this->SortInf->SortDataBuf, (char*)&n, 4);
			        len+=4;
				break;

			    case DATA_T_DOUBLE:
			        if (sort_item->Flags & EXPR_F_DESC) sort_item->Types.Double = -sort_item->Types.Double;
				xsConcatenate(&this->SortInf->SortDataBuf, (char*)&(sort_item->Types.Double), 8);
			        if (sort_item->Flags & EXPR_F_DESC) sort_item->Types.Double = -sort_item->Types.Double;
				len+=8;
				break;

			    case DATA_T_MONEY:
				n = htonl(sort_item->Types.Money.WholePart + 0x80000000);
				sn = htons(sort_item->Types.Money.FractionPart);
			        if (sort_item->Flags & EXPR_F_DESC)
				    {
				    n = ~n;
				    sn = ~sn;
				    }
			        xsConcatenate(&this->SortInf->SortDataBuf, (char*)&n, 4);
			        xsConcatenate(&this->SortInf->SortDataBuf, (char*)&sn, 2);
				len+=6;
				break;

			    case DATA_T_DATETIME:
			        if (sort_item->Flags & EXPR_F_DESC) 
				    {
				    sort_item->Types.Date.Value = ~sort_item->Types.Date.Value;
				    sort_item->Types.Date.Part.Second = ~sort_item->Types.Date.Part.Second;
				    }
				dbuf[0] = sort_item->Types.Date.Part.Year>>8;
				dbuf[1] = sort_item->Types.Date.Part.Year & 0xFF;
				dbuf[2] = sort_item->Types.Date.Part.Month;
				dbuf[3] = sort_item->Types.Date.Part.Day;
				dbuf[4] = sort_item->Types.Date.Part.Hour;
				dbuf[5] = sort_item->Types.Date.Part.Minute;
				dbuf[6] = sort_item->Types.Date.Part.Second;
				xsConcatenate(&this->SortInf->SortDataBuf, dbuf, 7);
			        if (sort_item->Flags & EXPR_F_DESC) 
				    {
				    sort_item->Types.Date.Value = ~sort_item->Types.Date.Value;
				    sort_item->Types.Date.Part.Second = ~sort_item->Types.Date.Part.Second;
				    }
				len+=4;
				break;
			    
			    case DATA_T_STRING:
			        n = strlen(sort_item->String);
			        len += (n+1);
			        if (sort_item->Flags & EXPR_F_DESC)
			            {
				    for(j=0;j<n;j++) sort_item->String[j] = ~(sort_item->String[j]);
				    }
			        xsConcatenate(&this->SortInf->SortDataBuf, sort_item->String, n+1);
				break;
			    }
			}
		    }
		xaAddItem(this->SortInf->SortPtrLen+0, (void*)len);
		objClose(tmp_obj);
		}

	    /** Absolute-reference the string offsets. **/
	    n = this->SortInf->SortPtr[0].nItems;
	    for(i=0;i<n;i++)
	        {
		this->SortInf->SortNames[0].Items[i] = this->SortInf->SortNamesBuf.String + (int)(this->SortInf->SortNames[0].Items[i]);
		this->SortInf->SortPtr[0].Items[i] = this->SortInf->SortDataBuf.String + (int)(this->SortInf->SortPtr[0].Items[i]);
		}

	    /** Mergesort the result set. **/
	    xaInit(this->SortInf->SortPtr+1, n);
	    memcpy(this->SortInf->SortPtr[1].Items, this->SortInf->SortPtr[0].Items, n*4);
	    xaInit(this->SortInf->SortPtrLen+1, n);
	    memcpy(this->SortInf->SortPtrLen[1].Items, this->SortInf->SortPtrLen[0].Items, n*4);
	    xaInit(this->SortInf->SortNames+1, n);
	    memcpy(this->SortInf->SortNames[1].Items, this->SortInf->SortNames[0].Items, n*4);
	    obj_internal_MergeSort(this->SortInf, 1, 0, n - 1);
	    this->RowID = 0;
	    this->Flags |= OBJ_QY_F_FROMSORT;
	    }

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "%8.8X\n", (int)this);

    return this;

    error_return:

	if (this && order_by && !orderbyexp) for(i=0;this->SortBy[i];i++) expFreeExpression(this->SortBy[i]);
	if (linked_obj) objClose(linked_obj); /* unlink */
	if (this && this->Flags & OBJ_QY_F_ALLOCTREE) expFreeExpression((pExpression)(this->Tree));
	if (this && this->ObjList) expFreeParamList((pParamObjects)(this->ObjList));
	if (this) nmFree(this,sizeof(ObjQuery));

    return NULL;
    }


/*** objQueryDelete - delete all rows in the current query result set.
 *** if driver doesn't understand queries, we'll have to delete the
 *** dumb things one at a time.
 ***/
int 
objQueryDelete(pObjQuery this)
    {
    int rval = 0;
    pObject obj;

	ASSERTMAGIC(this, MGK_OBJQUERY);

    	/** Multiquery? **/
	if (this->Drv) return this->Drv->QueryDelete(this->Data);

	/** Intelligent query support in driver? **/
	if ((this->Obj->Driver->Capabilities & OBJDRV_C_FULLQUERY) ||
	    ((this->Obj->Driver->Capabilities & OBJDRV_C_LLQUERY) &&
	     (this->Obj->TLowLevelDriver->Capabilities & OBJDRV_C_FULLQUERY)))
	    {
	    rval = this->Obj->Driver->QueryDelete(this->Data, &(this->Obj->Session->Trx));
	    }
	else
	    {
	    while((obj = objQueryFetch(this,O_WRONLY)))
		{
		objDelete(this->Obj->Session, this->Obj->Pathname->Pathbuf);
		objClose(obj);
		}
	    }

    return rval;
    }


/*** objQueryFetch - open the next object in the result set (or first
 *** object, if the query was just issued).
 ***/
pObject 
objQueryFetch(pObjQuery this, int mode)
    {
    pObject obj;
    void* obj_data;
    char* name;
    char buf[OBJSYS_MAX_PATH + 32];

    	ASSERTMAGIC(this,MGK_OBJQUERY);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryFetch(%8.8X:%3.3s)...", (int)this, this->Drv->Name);

    	/** Multiquery? **/
	if (this->Drv) 
	    {
	    obj = (pObject)nmMalloc(sizeof(Object));
	    if (!obj) return NULL;
	    if ((obj->Data = this->Drv->QueryFetch(this->Data, obj, mode, NULL)) == NULL)
	        {
		nmFree(obj,sizeof(Object));
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		return NULL;
		}
	    obj->EvalContext = NULL;
	    obj->Driver = this->Drv;
	    obj->TLowLevelDriver = NULL;
	    obj->ILowLevelDriver = NULL;
	    obj->Session = this->QySession;
	    obj->LinkCnt = 1;
	    obj->Obj = NULL;
	    obj->ContentPtr = NULL;
	    obj->Magic = MGK_OBJECT;
	    obj->Flags = 0;
	    obj->Prev = NULL;
	    obj->Next = NULL;
	    obj->Type = NULL;
	    obj->NotifyItem = NULL;
	    obj->VAttrs = NULL;
	    xaInit(&obj->Attrs,4);
            xaAddItem(&(this->QySession->OpenObjects),(void*)obj);
	    obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	    memset(obj->Pathname, 0, sizeof(Pathname));
	    obj->Pathname->OpenCtlBuf = NULL;
	    sprintf(obj->Pathname->Pathbuf, "./INTERNAL/MQ.%16.16llx", (long long)(OSYS.PathID++));
	    obj->Pathname->nElements = 3;
	    obj->Pathname->Elements[0] = obj->Pathname->Pathbuf;
	    obj->Pathname->Elements[1] = obj->Pathname->Pathbuf+2;
	    obj->Pathname->Elements[2] = obj->Pathname->Pathbuf+11;
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8X:%3.3s:%s\n", (int)obj, obj->Driver->Name, obj->Pathname->Pathbuf);
	    return obj;
	    }

	/** Retrieving from a sort result set? **/
	if (this->Flags & OBJ_QY_F_FROMSORT)
	    {
	    if (this->RowID >= this->SortInf->SortNames[0].nItems) return NULL;
	    snprintf(buf,sizeof(buf),"%s/%s?ls__type=system%%2fobject",this->Obj->Pathname->Pathbuf+1,(char*)(this->SortInf->SortNames[0].Items[this->RowID++]));
	    obj = objOpen(this->Obj->Session, buf, mode, 0400, "");
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8X:%3.3s:%s\n", (int)obj, obj->Driver->Name, obj->Pathname->Pathbuf);
	    return obj;
	    }

	/** Open up the object descriptor **/
	obj = (pObject)nmMalloc(sizeof(Object));
	if (!obj) return NULL;
	obj->EvalContext = this->Obj->EvalContext;	/* inherit from parent */
	obj->Driver = this->Obj->Driver;
	obj->ILowLevelDriver = this->Obj->ILowLevelDriver;
	obj->TLowLevelDriver = this->Obj->TLowLevelDriver;
	obj->Obj = NULL;
	xaInit(&(obj->Attrs),4);
	obj->Mode = mode;
	obj->Session = this->Obj->Session;
	obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	obj->Pathname->LinkCnt = 1;
	obj->Pathname->OpenCtlBuf = NULL;
	obj->LinkCnt = 1;
	obj->ContentPtr = NULL;
	objLinkTo(this->Obj->Prev);
	obj->Prev = this->Obj->Prev;
	obj->Magic = MGK_OBJECT;
	obj->Flags = 0;
	obj->Type = NULL;
	obj->NotifyItem = NULL;
	obj->VAttrs = NULL;

	/** Scan objects til we find one matching the query. **/
	while(1)
	    {
	    /** setup the new pathname. **/
	    obj_internal_CopyPath(obj->Pathname,this->Obj->Pathname);
	    obj->Pathname->LinkCnt = 1;
	    obj->SubPtr = this->Obj->SubPtr;
	    obj->SubCnt = this->Obj->SubCnt+1;

	    /** Fetch next from driver. **/
            obj_data = this->Obj->Driver->QueryFetch(this->Data, obj, mode, &(obj->Session->Trx));
            if (!obj_data) 
	        {
		objClose(obj->Prev); /* unlink */
		xaDeInit(&(obj->Attrs));
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		return NULL;
		}
            obj->Data = obj_data;
    
            this->Obj->Driver->GetAttrValue(obj_data, "name", DATA_T_STRING, &name, NULL);
            if (strlen(name) + strlen(this->Obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH) 
                {
		this->Obj->Driver->Close(obj_data, &(obj->Session->Trx));
		objClose(obj->Prev); /* unlink */
		xaDeInit(&(obj->Attrs));
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));
		mssError(1,"OSML","Filename in query result exceeded internal limits");
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
                return NULL;
                }

	    /** If we need to check it, do so now. **/
	    if (!(this->Flags & OBJ_QY_F_FULLQUERY) && this->Tree)
		{
		expModifyParam((pParamObjects)(this->ObjList), NULL, obj);
		((pParamObjects)(this->ObjList))->Session = obj->Session;
		if (expEvalTree((pExpression)(this->Tree), (pParamObjects)(this->ObjList)) < 0 || 
		    ((pExpression)(this->Tree))->DataType != DATA_T_INTEGER || 
		    !(((pExpression)(this->Tree))->Integer))
		    {
		    this->Obj->Driver->Close(obj_data, &(obj->Session->Trx));
		    continue;
		    }
		}

	    /** Ok, add item to opens and return to caller **/
            xaAddItem(&(this->Obj->Session->OpenObjects),(void*)obj);
	    break;
	    }

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8X:%3.3s:%s\n", (int)obj, obj->Driver->Name, obj->Pathname->Pathbuf);

    return obj;
    }


/*** objQueryCreate - creates a new object in the context of a running
 *** query.  Returns the newly open object.
 ***/
pObject
objQueryCreate(pObjQuery this, char* name, int mode, int permission_mask, char* type)
    {
    pObject new_obj;
    void* rval;
    char* bufptr;
    char* newname;

	ASSERTMAGIC(this,MGK_OBJQUERY);

	/** Duh. **/
	mode |= OBJ_O_CREAT;

	/** Make sure supplied name is "*" if using autokeying **/
	if (mode & OBJ_O_AUTONAME)
	    {
	    if (strcmp(name,"*"))
		{
		mssError(1,"OSML","When creating an object using autokeying, name must be '*'");
		return NULL;
		}
	    /** This is inherent with autoname **/
	    mode |= OBJ_O_EXCL;
	    }

	/** Open up the new object descriptor **/
	new_obj = (pObject)nmMalloc(sizeof(Object));
	if (!new_obj) return NULL;
	new_obj->EvalContext = NULL;
	new_obj->Driver = this->Obj->Driver;
	new_obj->ILowLevelDriver = this->Obj->ILowLevelDriver;
	new_obj->TLowLevelDriver = this->Obj->TLowLevelDriver;
	new_obj->Obj = NULL;
	xaInit(&(new_obj->Attrs),4);
	new_obj->Mode = mode;
	new_obj->Session = this->Obj->Session;
	new_obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	new_obj->Pathname->LinkCnt = 1;
	new_obj->Pathname->OpenCtlBuf = NULL;
	new_obj->LinkCnt = 1;
	new_obj->ContentPtr = NULL;
	objLinkTo(this->Obj->Prev);
	new_obj->Prev = this->Obj->Prev;
	new_obj->Magic = MGK_OBJECT;
	new_obj->Flags = 0;
	new_obj->Type = NULL;
	new_obj->NotifyItem = NULL;
	new_obj->VAttrs = NULL;

	/** Does driver support QueryCreate? **/
	if (new_obj->Driver->QueryCreate && 
	    (!new_obj->ILowLevelDriver || new_obj->ILowLevelDriver->QueryCreate) &&
	    (!new_obj->TLowLevelDriver || new_obj->TLowLevelDriver->QueryCreate))
	    {
	    /** Supported.  Call querycreate **/
	    rval = new_obj->Driver->QueryCreate(this->Data, new_obj, name, mode, permission_mask, &(new_obj->Session->Trx));
	    if (!rval)
		{
		/** Call failed.  Free up the obj structure **/
		objClose(new_obj->Prev); /* unlink */
		xaDeInit(&(new_obj->Attrs));
		obj_internal_FreePath(new_obj->Pathname);
                nmFree(new_obj,sizeof(Object));
		return NULL;
		}

	    /** querycreate succeeded. **/
	    new_obj->Data = rval;
	    }
	else
	    {
	    /** not supported by underlying driver - we have to do it ourselves, but
	     ** that condition is not yet handled.  Sorry... (FIXME)
	     **/
	    mssError(1,"OSML","Bark!  Unimplemented functionality in objQueryCreate()");
	    objClose(new_obj->Prev); /* unlink */
	    xaDeInit(&(new_obj->Attrs));
	    obj_internal_FreePath(new_obj->Pathname);
	    nmFree(new_obj,sizeof(Object));
	    return NULL;
	    }

	/** Set the name **/
	new_obj->Driver->GetAttrValue(new_obj->Data, "name", DATA_T_STRING, &newname, NULL);
	if (strlen(newname) + strlen(new_obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH || new_obj->Pathname->nElements + 1 > OBJSYS_MAX_ELEMENTS)
	    {
	    new_obj->Driver->Close(new_obj->Data, &(new_obj->Session->Trx));
	    objClose(new_obj->Prev); /* unlink */
	    xaDeInit(&(new_obj->Attrs));
	    /*nmFree(obj->Pathname,sizeof(Pathname));*/
	    obj_internal_FreePath(new_obj->Pathname);
	    nmFree(new_obj,sizeof(Object));
	    mssError(1,"OSML","Filename in query result exceeded internal limits");
	    return NULL;
	    }
	bufptr = strchr(new_obj->Pathname->Pathbuf,0);
	*(bufptr++) = '/';
	new_obj->Pathname->Elements[new_obj->Pathname->nElements++] = bufptr;
	strcpy(bufptr, newname);

    return new_obj;
    }


/*** objQueryClose - close an open query.  This does _not_ close any of
 *** the objects in the result set that may still be open.
 ***/
int 
objQueryClose(pObjQuery this)
    {

    	ASSERTMAGIC(this,MGK_OBJQUERY);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryClose(%p:%3.3s)\n", this, this->Drv->Name);

    	/** Release sort information? **/
	if (this->SortInf)
	    {
	    xaDeInit(this->SortInf->SortPtr+0);
	    xaDeInit(this->SortInf->SortPtrLen+0);
	    xaDeInit(this->SortInf->SortNames+0);
	    xaDeInit(this->SortInf->SortPtr+1);
	    xaDeInit(this->SortInf->SortPtrLen+1);
	    xaDeInit(this->SortInf->SortNames+1);
	    xsDeInit(&this->SortInf->SortDataBuf);
	    xsDeInit(&this->SortInf->SortNamesBuf);
	    nmFree(this->SortInf,sizeof(ObjQuerySort));
	    }

	/** Shut down query with the driver. **/
	if (this->Drv) 
	    {
	    this->Drv->QueryClose(this->Data);

	    /** Remove from session open queries... **/
	    xaRemoveItem(&(this->QySession->OpenQueries),
	        xaFindItem(&(this->QySession->OpenQueries),(void*)this));
	    }
	else
	    {
	    this->Obj->Driver->QueryClose(this->Data, &(this->Obj->Session->Trx));

	    /** Remove from session open queries... **/
	    xaRemoveItem(&(this->Obj->Session->OpenQueries),
	        xaFindItem(&(this->Obj->Session->OpenQueries),(void*)this));
	    }

	/** Close the parent object (or, just unlink from it), if applicable **/
	if (this->Obj) objClose(this->Obj);
	
	/** Free up the expression tree and the qy object itself **/
	if (this->Flags & OBJ_QY_F_ALLOCTREE) expFreeExpression((pExpression)(this->Tree));
	if (this->ObjList) expFreeParamList((pParamObjects)(this->ObjList));
	nmFree(this,sizeof(ObjQuery));

    return 0;
    }


