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



/*** obj_internal_SortCompare - compares two items in the sorting 
 *** criteria.
 ***/
int
obj_internal_SortCompare(pObjQuerySort sortinf, int bufferid1, int bufferid2, int id1, int id2)
    {
    int len1,len2,n;
    int r;
    char *ptr1, *ptr2;
    	
	len1 = (intptr_t)(sortinf->SortPtrLen[bufferid1].Items[id1]);
	len2 = (intptr_t)(sortinf->SortPtrLen[bufferid2].Items[id2]);
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
objMultiQuery(pObjSession session, char* query, void* objlist_v, int flags)
    {
    pObjQuery this;
    pParamObjects objlist = (pParamObjects)objlist_v;

	ASSERTMAGIC(session, MGK_OBJSESSION);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objMultiQuery(%8.8lX,%s)... ", (long)session, query);

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
	this->Data = this->Drv->OpenQuery(session, query, objlist, flags);
	if (!this->Data)
	    {
	    nmFree(this,sizeof(ObjQuery));
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE," null\n");
	    return NULL;
	    }

	/** Add to session open queries... **/
	xaAddItem(&(session->OpenQueries),(void*)this);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE," %8.8lX\n", (long)this);

    return this;
    }


/*** obj_internal_ParseCriteria() - set up query criteria when opening a
 *** new query.
 ***/
int
obj_internal_ParseCriteria(pObjQuery this, char* query, pExpression tree)
    {

	if (query && *query)
	    {
	    /** Query text supplied **/
	    this->Tree = (void*)expCompileExpression(query, (pParamObjects)(this->ObjList), MLX_F_ICASE | MLX_F_FILENAMES, 0);
	    if (!(this->Tree))
	        {
		return -1;
		}
	    this->Flags |= OBJ_QY_F_ALLOCTREE;
	    }
	else
	    {
	    /** Compiled expression tree, or no criteria at all, supplied **/
	    this->Tree = tree;
	    }

    return 0;
    }


/*** objOpenQuery - issue a query against just one object, with no joins.
 ***/
pObjQuery 
objOpenQuery(pObject obj, char* query, char* order_by, void* tree_v, void** orderby_exp_v)
    {
    pObjQuery this = NULL;
    pExpression tree = (pExpression)tree_v;
    pExpression *orderbyexp = (pExpression*)orderby_exp_v;
    int i,n,len,t;
    int n_sortby;
    pExpression sort_item;
    pLxSession lxs = NULL;
    pObject tmp_obj;
    char* ptr;
    char* start_ptr;
    pObject linked_obj = NULL;
    pObjectInfo info;

    	ASSERTMAGIC(obj,MGK_OBJECT);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objOpenQuery(%8.8lX:%3.3s:%s,'%s','%s')... ", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf, query, order_by);

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
	linked_obj = objLinkTo(obj);
	this->Obj = linked_obj;
        this->ObjList = (void*)expCreateParamList();
	expAddParamToList((pParamObjects)(this->ObjList), NULL, NULL, EXPR_O_CURRENT);

	/** Ok, first parse the query. **/
	if (obj_internal_ParseCriteria(this, query, tree) < 0)
	    {
	    mssError(0,"OSML","Query search criteria is invalid");
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
	    goto error_return;
	    }

	/** Now, parse the order-by clause **/
	n_sortby = 0;
	if (orderbyexp)
	    {
	    for(i=0;i < sizeof(this->SortBy)/sizeof(void*);i++)
	        {
		this->SortBy[i] = orderbyexp[i];
		if (!orderbyexp[i]) break;
		n_sortby++;
		}
	    }
	else if (order_by)
	    {
	    lxs = mlxStringSession(order_by, MLX_F_EOF | MLX_F_FILENAMES | MLX_F_ICASER);
	    for(i=0;i < sizeof(this->SortBy)/sizeof(void*);i++)
	        {
		sort_item = exp_internal_CompileExpression_r(lxs, 0, this->ObjList, EXPR_CMP_ASCDESC);
		this->SortBy[i] = sort_item;
		if (!sort_item) break;
		n_sortby++;
		t = mlxNextToken(lxs);
		if (t == MLX_TOK_EOF)
		    {
		    i++;
		    if (i < sizeof(this->SortBy)/sizeof(void*))
			this->SortBy[i] = NULL;
		    break;
		    }
		else if (t != MLX_TOK_COMMA)
		    {
		    mssError(1,"OSML","Invalid sort criteria '%s'", order_by);
		    goto error_return;
		    }
		}
	    mlxCloseSession(lxs);
	    lxs = NULL;
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
	    if (!this->SortInf)
		goto error_return;
	    xaInit(this->SortInf->SortPtr+0,4096);
	    xaInit(this->SortInf->SortPtrLen+0,4096);
	    xaInit(this->SortInf->SortNames+0,4096);
	    xsInit(&this->SortInf->SortDataBuf);
	    xsInit(&this->SortInf->SortNamesBuf);
	    this->SortInf->IsTemp = 0;

	    /** Temp object? **/
	    info = objInfo(obj);
	    if (info && (info->Flags & OBJ_INFO_F_TEMPORARY))
		this->SortInf->IsTemp = 1;

	    /** Read result set **/
	    while((tmp_obj = objQueryFetch(this, 0400)))
	        {
		/** We keep temp objects open, for others we squirrel away the name instead and re-open later **/
		if (this->SortInf->IsTemp)
		    xaAddItem(this->SortInf->SortNames+0, (void*)tmp_obj);
		else
		    xaAddItem(this->SortInf->SortNames+0, (void*)(xsStringEnd(&this->SortInf->SortNamesBuf) - this->SortInf->SortNamesBuf.String));
		objGetAttrValue(tmp_obj,"name",DATA_T_STRING,POD(&ptr));
		xsConcatenate(&this->SortInf->SortNamesBuf, ptr, strlen(ptr)+1);
		expModifyParam(this->ObjList, NULL, tmp_obj);
		start_ptr = xsStringEnd(&this->SortInf->SortDataBuf);
		xaAddItem(this->SortInf->SortPtr+0, (void*)(start_ptr - this->SortInf->SortDataBuf.String));

		len = objBuildBinaryImageXString(&this->SortInf->SortDataBuf, this->SortBy, n_sortby, this->ObjList, 0);
		if (len < 0)
		    {
		    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
		    goto error_return;
		    }

		xaAddItem(this->SortInf->SortPtrLen+0, (void*)(intptr_t)len);
		if (!this->SortInf->IsTemp)
		    objClose(tmp_obj);
		}

	    /** Absolute-reference the string offsets. **/
	    n = this->SortInf->SortPtr[0].nItems;
	    for(i=0;i<n;i++)
	        {
		if (!this->SortInf->IsTemp)
		    this->SortInf->SortNames[0].Items[i] = this->SortInf->SortNamesBuf.String + (intptr_t)(this->SortInf->SortNames[0].Items[i]);
		this->SortInf->SortPtr[0].Items[i] = this->SortInf->SortDataBuf.String + (intptr_t)(this->SortInf->SortPtr[0].Items[i]);
		}

	    /** Mergesort the result set. **/
	    xaInit(this->SortInf->SortPtr+1, n);
	    memcpy(this->SortInf->SortPtr[1].Items, this->SortInf->SortPtr[0].Items, n*sizeof(void *));
	    xaInit(this->SortInf->SortPtrLen+1, n);
	    memcpy(this->SortInf->SortPtrLen[1].Items, this->SortInf->SortPtrLen[0].Items, n*sizeof(void *));
	    xaInit(this->SortInf->SortNames+1, n);
	    memcpy(this->SortInf->SortNames[1].Items, this->SortInf->SortNames[0].Items, n*sizeof(void *));
	    obj_internal_MergeSort(this->SortInf, 1, 0, n - 1);
	    this->Flags |= OBJ_QY_F_FROMSORT;
	    }

	this->RowID = 0;

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "%8.8lX\n", (long)this);

    return this;

    error_return:

	if (lxs) mlxCloseSession(lxs);
	if (this && order_by && !orderbyexp) for(i=0;this->SortBy[i];i++) expFreeExpression(this->SortBy[i]);
	if (linked_obj) objClose(linked_obj); /* unlink */
	if (this)
	    {
	    xaRemoveItem(&(linked_obj->Session->OpenQueries), xaFindItem(&(linked_obj->Session->OpenQueries), (void*)this));
	    if (this->Flags & OBJ_QY_F_ALLOCTREE) expFreeExpression((pExpression)(this->Tree));
	    if (this->ObjList) expFreeParamList((pParamObjects)(this->ObjList));
	    nmFree(this,sizeof(ObjQuery));
	    }

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
	if (((this->Obj->Driver->Capabilities & OBJDRV_C_FULLQUERY) ||
	    ((this->Obj->Driver->Capabilities & OBJDRV_C_LLQUERY) &&
	     (this->Obj->TLowLevelDriver->Capabilities & OBJDRV_C_FULLQUERY))) &&
	    this->Obj->Driver->QueryDelete != NULL && 
	    (!this->Obj->TLowLevelDriver || this->Obj->TLowLevelDriver->QueryDelete != NULL))
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

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryFetch(%8.8lX:%3.3s)...", (long)this, this->Drv->Name);

    	/** Multiquery? **/
	if (this->Drv) 
	    {
	    /*obj = (pObject)nmMalloc(sizeof(Object));*/
	    obj = obj_internal_AllocObj();
	    if (!obj) return NULL;
	    if ((obj->Data = this->Drv->QueryFetch(this->Data, obj, mode, NULL)) == NULL)
	        {
		obj_internal_FreeObj(obj);
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		return NULL;
		}
	    obj->Driver = this->Drv;
	    obj->Session = this->QySession;
            xaAddItem(&(this->QySession->OpenObjects),(void*)obj);
	    obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	    memset(obj->Pathname, 0, sizeof(Pathname));
	    obj->Pathname->OpenCtlBuf = NULL;
	    sprintf(obj->Pathname->Pathbuf, "./INTERNAL/MQ.%16.16llx", (long long)(OSYS.PathID++));
	    obj->Pathname->nElements = 3;
	    obj->Pathname->Elements[0] = obj->Pathname->Pathbuf;
	    obj->Pathname->Elements[1] = obj->Pathname->Pathbuf+2;
	    obj->Pathname->Elements[2] = obj->Pathname->Pathbuf+11;
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8lX:%3.3s:%s\n", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf);
	    return obj;
	    }

	/** Retrieving from a sort result set? **/
	if (this->Flags & OBJ_QY_F_FROMSORT)
	    {
	    if (this->RowID >= this->SortInf->SortNames[0].nItems) return NULL;

	    /** Temp objects we kept open; others we reopen by name **/
	    if (this->SortInf->IsTemp)
		{
		obj = (pObject)this->SortInf->SortNames[0].Items[this->RowID++];
		this->SortInf->SortNames[0].Items[this->RowID - 1] = NULL;
		}
	    else
		{
		obj_internal_PathPart(this->Obj->Pathname, 0, 0);
		snprintf(buf,sizeof(buf),"%s/%s?ls__type=system%%2fobject",this->Obj->Pathname->Pathbuf+1,(char*)(this->SortInf->SortNames[0].Items[this->RowID++]));
		obj = objOpen(this->Obj->Session, buf, mode, 0400, "");
		}
	    if (obj)
		obj->RowID = this->RowID;
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8lX:%3.3s:%s\n", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf);
	    return obj;
	    }

	/** Open up the object descriptor **/
	/*obj = (pObject)nmMalloc(sizeof(Object));*/
	obj = obj_internal_AllocObj();
	if (!obj) return NULL;
	obj->EvalContext = this->Obj->EvalContext;	/* inherit from parent */
	obj->Driver = this->Obj->Driver;
	obj->ILowLevelDriver = this->Obj->ILowLevelDriver;
	obj->TLowLevelDriver = this->Obj->TLowLevelDriver;
	obj->Mode = mode;
	obj->Session = this->Obj->Session;
	obj->Pathname = NULL;
	if (this->Obj->Prev)
	    objLinkTo(this->Obj->Prev);
	obj->Prev = this->Obj->Prev;

	/** Scan objects til we find one matching the query. **/
	while(1)
	    {
	    /** setup the new pathname. **/
	    if (obj->Pathname)
		obj_internal_FreePath(obj->Pathname);
	    obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	    obj->Pathname->OpenCtlBuf = NULL;
	    obj->Pathname->LinkCnt = 1;
	    obj_internal_CopyPath(obj->Pathname, this->Obj->Pathname);
	    obj->SubCnt = this->Obj->SubCnt+1;
	    obj->SubPtr = this->Obj->SubPtr;

	    /** Fetch next from driver. **/
            obj_data = this->Obj->Driver->QueryFetch(this->Data, obj, mode, &(obj->Session->Trx));
            if (!obj_data) 
	        {
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		/*objClose(obj->Prev);
		xaDeInit(&(obj->Attrs));
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));*/
		obj_internal_FreeObj(obj);
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		return NULL;
		}
            obj->Data = obj_data;
    
            this->Obj->Driver->GetAttrValue(obj_data, "name", DATA_T_STRING, &name, NULL);
            if (strlen(name) + strlen(this->Obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH) 
                {
		/*this->Obj->Driver->Close(obj_data, &(obj->Session->Trx));*/
		/*nmFree(obj->Pathname,sizeof(Pathname));*/
		/*objClose(obj->Prev);
		xaDeInit(&(obj->Attrs));
		obj_internal_FreePath(obj->Pathname);
                nmFree(obj,sizeof(Object));*/
		obj_internal_FreeObj(obj);
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
		    obj->Data = NULL;
		    continue;
		    }
		}

	    /** Ok, add item to opens and return to caller **/
            xaAddItem(&(this->Obj->Session->OpenObjects),(void*)obj);
	    break;
	    }
    
	if (obj)
	    {
	    this->RowID++;
	    obj->RowID = this->RowID;
	    }

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8lX:%3.3s:%s\n", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf);

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
	/*new_obj = (pObject)nmMalloc(sizeof(Object));*/
	new_obj = obj_internal_AllocObj();
	if (!new_obj) return NULL;
	new_obj->Driver = this->Obj->Driver;
	new_obj->ILowLevelDriver = this->Obj->ILowLevelDriver;
	new_obj->TLowLevelDriver = this->Obj->TLowLevelDriver;
	new_obj->Mode = mode;
	new_obj->Session = this->Obj->Session;
	new_obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	new_obj->Pathname->LinkCnt = 1;
	new_obj->Pathname->OpenCtlBuf = NULL;
	objLinkTo(this->Obj->Prev);
	new_obj->Prev = this->Obj->Prev;

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
		obj_internal_FreeObj(new_obj);
		/*objClose(new_obj->Prev);
		xaDeInit(&(new_obj->Attrs));
		obj_internal_FreePath(new_obj->Pathname);
                nmFree(new_obj,sizeof(Object));*/
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
	    obj_internal_FreeObj(new_obj);
	    /*objClose(new_obj->Prev);
	    xaDeInit(&(new_obj->Attrs));
	    obj_internal_FreePath(new_obj->Pathname);
	    nmFree(new_obj,sizeof(Object));*/
	    return NULL;
	    }

	/** Set the name **/
	new_obj->Driver->GetAttrValue(new_obj->Data, "name", DATA_T_STRING, &newname, NULL);
	if (strlen(newname) + strlen(new_obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH || new_obj->Pathname->nElements + 1 > OBJSYS_MAX_ELEMENTS)
	    {
	    new_obj->Driver->Close(new_obj->Data, &(new_obj->Session->Trx));
	    obj_internal_FreeObj(new_obj);
	    /*nmFree(obj->Pathname,sizeof(Pathname));*/
	    /*objClose(new_obj->Prev);
	    xaDeInit(&(new_obj->Attrs));
	    obj_internal_FreePath(new_obj->Pathname);
	    nmFree(new_obj,sizeof(Object));*/
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
    int i;
    pObject obj;

    	ASSERTMAGIC(this,MGK_OBJQUERY);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryClose(%p:%3.3s)\n", this, this->Drv->Name);

    	/** Release sort information? **/
	if (this->SortInf)
	    {
	    /** Close temp objects? **/
	    if (this->SortInf->IsTemp)
		{
		for(i = this->SortInf->SortNames[0].nItems - 1; i >= this->RowID; i--)
		    {
		    obj = (pObject)this->SortInf->SortNames[0].Items[i];
		    if (obj) objClose(obj);
		    }
		}
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


/*** objGetQueryCoverageMask() - determine what external objects are in use
 *** by the given query.  This is only valid on multiqueries (query handles
 *** returned by objMultiQuery()), and only makes sense when an external
 *** ParamObject list is used with the multiquery.
 ***/
int
objGetQueryCoverageMask(pObjQuery this)
    {
    if (this->Obj->Driver->GetQueryCoverageMask)
	return this->Obj->Driver->GetQueryCoverageMask(this->Data);
    else
	return -1;
    }


/*** objGetQueryIdentityPath() - get the pathname to the "identity" object
 *** underlying a given query.  If the query is a normal query (from the
 *** objOpenQuery() call), this returns the path to the object that was
 *** passed to objOpenQuery().  Otherwise, for a multiquery, this returns
 *** the path to the "identity" query source (or to the only source, if the
 *** query only has one source).
 ***/
int
objGetQueryIdentityPath(pObjQuery this, char* pathbuf, int maxlen)
    {
    char* ptr;

	/** Driver can handle this?  (i.e., MultiQuery module) **/
	if (this->Drv->GetQueryIdentityPath)
	    return this->Drv->GetQueryIdentityPath(this->Data, pathbuf, maxlen);

	/** Get the pathname **/
	if (!this->Obj)
	    return -1;
	ptr = obj_internal_PathPart(this->Obj->Pathname, 0, 0);
	if (!ptr)
	    return -1;
	if (strlen(ptr) >= maxlen-1)
	    return -1;

	/** Copy it **/
	strtcpy(pathbuf, ptr, maxlen);

    return 0;
    }

