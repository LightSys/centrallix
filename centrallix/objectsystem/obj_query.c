#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "expression.h"
#include "magic.h"
#include "mtsession.h"

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

    $Id: obj_query.c,v 1.1 2001/08/13 18:00:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_query.c,v $

    $Log: obj_query.c,v $
    Revision 1.1  2001/08/13 18:00:59  gbeeley
    Initial revision

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

    	/** Allocate a query structure **/
	this = (pObjQuery)nmMalloc(sizeof(ObjQuery));
	if (!this) return NULL;
	memset(this,0,sizeof(ObjQuery));
	this->QyText = query;
	this->Drv = OSYS.MultiQueryLayer;
	this->QySession = session;

	/** Start the query. **/
	this->Data = this->Drv->OpenQuery(session, query);
	if (!this->Data)
	    {
	    nmFree(this,sizeof(ObjQuery));
	    return NULL;
	    }

    return this;
    }


/*** objOpenQuery - issue a query against just one object, with no joins.
 ***/
pObjQuery 
objOpenQuery(pObject obj, char* query, char* order_by, void* tree_v, void** orderby_exp_v)
    {
    pObjQuery this;
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

    	ASSERTMAGIC(obj,MGK_OBJECT);

	/** Allocate a query object **/
	this = (pObjQuery)nmMalloc(sizeof(ObjQuery));
	if (!this) return NULL;
	this->QyText = query;
	this->Drv = NULL;
	this->Flags = 0;
	this->SortInf = NULL;

	/** Ok, first parse the query. **/
	this->Obj = obj;
        this->ObjList = (void*)expCreateParamList();
	expAddParamToList((pParamObjects)(this->ObjList), NULL, NULL, 0);
	if (query && *query)
	    {
	    this->Tree = (void*)expCompileExpression(query, (pParamObjects)(this->ObjList), MLX_F_ICASE | MLX_F_FILENAMES, 0);
	    if (!(this->Tree))
	        {
	        nmFree(this,sizeof(ObjQuery));
		mssError(0,"OSML","Query search criteria is invalid");
	        return NULL;
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
	    for(i=0;(sort_item=exp_internal_CompileExpression_r(lxs, 0, this->ObjList, 0));i++)
	        {
		this->SortBy[i] = sort_item;
		}
	    this->SortBy[i] = NULL;
	    }
	else
	    {
	    this->SortBy[0] = NULL;
	    }

	/** Issue to driver if capable; otherwise do query ourselves **/
	if ((obj->Driver->Capabilities & OBJDRV_C_FULLQUERY) || 
	    ((obj->Driver->Capabilities & OBJDRV_C_LLQUERY) &&
	     (obj->LowLevelDriver->Capabilities & OBJDRV_C_FULLQUERY)))
	    {
	    this->Data = obj->Driver->OpenQuery(obj->Data,this,&(obj->Session->Trx));
	    }
	else
	    {
	    this->Data = obj->Driver->OpenQuery(obj->Data,this,&(obj->Session->Trx));
	    }

	if (!(this->Data))
	    {
	    if (this->Flags & OBJ_QY_F_ALLOCTREE) expFreeExpression((pExpression)(this->Tree));
	    if (this->ObjList) expFreeParamList((pParamObjects)(this->ObjList));
	    nmFree(this,sizeof(ObjQuery));
	    mssError(0,"OSML","Either queries not supported on this object or query failed");
	    return NULL;
	    }

	/** Add to session open queries... **/
	xaAddItem(&(obj->Session->OpenQueries),(void*)this);
	
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
		objGetAttrValue(tmp_obj,"name",POD(&ptr));
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

    return this;
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

    	ASSERTMAGIC(this->Obj,MGK_OBJECT);

    	/** Multiquery? **/
	if (this->Drv) return this->Drv->QueryDelete(this->Data);

	/** Intelligent query support in driver? **/
	if ((this->Obj->Driver->Capabilities & OBJDRV_C_FULLQUERY) ||
	    ((this->Obj->Driver->Capabilities & OBJDRV_C_LLQUERY) &&
	     (this->Obj->LowLevelDriver->Capabilities & OBJDRV_C_FULLQUERY)))
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
    char buf[256];

    	ASSERTMAGIC(this->Obj,MGK_OBJECT);

    	/** Multiquery? **/
	if (this->Drv) 
	    {
	    obj = (pObject)nmMalloc(sizeof(Object));
	    if (!obj) return NULL;
	    if ((obj->Data = this->Drv->QueryFetch(this->Data, mode)) == NULL)
	        {
		nmFree(obj,sizeof(Object));
		return NULL;
		}
	    obj->Driver = this->Drv;
	    obj->LowLevelDriver = NULL;
	    obj->Session = this->QySession;
	    obj->LinkCnt = 1;
	    obj->Pathname = NULL;
	    obj->Obj = NULL;
	    obj->ContentPtr = NULL;
	    obj->Magic = MGK_OBJECT;
	    obj->Flags = 0;
	    obj->Prev = NULL;
	    obj->Next = NULL;
	    obj->Type = NULL;
	    xaInit(&obj->Attrs,4);
	    return obj;
	    }

	/** Retrieving from a sort result set? **/
	if (this->Flags & OBJ_QY_F_FROMSORT)
	    {
	    if (this->RowID >= this->SortInf->SortNames[0].nItems) return NULL;
	    sprintf(buf,"%s/%s",this->Obj->Pathname->Pathbuf+1,(char*)(this->SortInf->SortNames[0].Items[this->RowID++]));
	    obj = objOpen(this->Obj->Session, buf, mode, 0400, "");
	    return obj;
	    }

	/** Open up the object descriptor **/
	obj = (pObject)nmMalloc(sizeof(Object));
	if (!obj) return NULL;
	obj->Driver = this->Obj->Driver;
	obj->LowLevelDriver = this->Obj->LowLevelDriver;
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

	/** Scan objects til we find one matching the query. **/
	while(1)
	    {
	    /** setup the new pathname. **/
	    obj_internal_CopyPath(obj->Pathname,this->Obj->Pathname);
	    obj->Pathname->LinkCnt = 1;

	    /** Fetch next from driver. **/
            obj_data = this->Obj->Driver->QueryFetch(this->Data, obj, mode, &(obj->Session->Trx));
            if (!obj_data) 
	        {
		objClose(obj->Prev); /* unlink */
		xaDeInit(&(obj->Attrs));
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));
		return NULL;
		}
            obj->Data = obj_data;
    
            this->Obj->Driver->GetAttrValue(obj_data, "name", &name);
            if (strlen(name) + strlen(this->Obj->Pathname->Pathbuf) + 1 > 255) 
                {
		this->Obj->Driver->Close(obj_data, &(obj->Session->Trx));
		objClose(obj->Prev); /* unlink */
		xaDeInit(&(obj->Attrs));
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));
		mssError(1,"OSML","Filename in query result exceeded internal limits");
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

    return obj;
    }


/*** objQueryClose - close an open query.  This does _not_ close any of
 *** the objects in the result set that may still be open.
 ***/
int 
objQueryClose(pObjQuery this)
    {

    	ASSERTMAGIC(this->Obj,MGK_OBJECT);

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
	    }
	else
	    {
	    this->Obj->Driver->QueryClose(this->Data, &(this->Obj->Session->Trx));

	    /** Remove from session open queries... **/
	    xaRemoveItem(&(this->Obj->Session->OpenQueries),
	        xaFindItem(&(this->Obj->Session->OpenQueries),(void*)this));
	    }
	
	/** Free up the expression tree and the qy object itself **/
	if (this->Flags & OBJ_QY_F_ALLOCTREE) expFreeExpression((pExpression)(this->Tree));
	if (this->ObjList) expFreeParamList((pParamObjects)(this->ObjList));
	nmFree(this,sizeof(ObjQuery));

    return 0;
    }


