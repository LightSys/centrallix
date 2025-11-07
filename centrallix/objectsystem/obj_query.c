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
#include "mergesort.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2025 LightSys Technology Services, Inc.		*/
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
 *** criteria.  This function is passed to mergesort() to sort a query
 *** result set.
 ***/
int
obj_internal_SortCompare(pObjQuerySortItem a, pObjQuerySortItem b)
    {
    size_t n, len_a, len_b;
    int r;
    
	/** Length that we will memcmp is the minimum of the two lengths **/
	len_a = a->SortDataLen;
	len_b = b->SortDataLen;
	n = (len_a > len_b) ? len_b : len_a;

	/** Compare the binary sortable data **/
	r = memcmp(a->SortInf->SortDataBuf.String + a->SortDataOffset, b->SortInf->SortDataBuf.String + b->SortDataOffset, n);

	/** Same?  Longer one comes after shorter one.  We use a conditional
	 ** here instead of just len_a - len_b because of the likely data
	 ** type conversion from size_t to int.
	 **/
	if (r == 0 && len_a != len_b)
	    {
	    if (len_a > len_b)
		r = 1;
	    else
		r = -1;
	    }

    return r;
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
objOpenQuery(pObject obj, char* query, char* order_by, void* tree_v, void** orderby_exp_v, int flags)
    {
    pObjQuery this = NULL;
    pExpression tree = (pExpression)tree_v;
    pExpression *orderbyexp = (pExpression*)orderby_exp_v;
    int i,len,t;
    int n_sortby;
    pLxSession lxs = NULL;
    pObject tmp_obj;
    char* ptr;
    pObject linked_obj = NULL;
    pObjectInfo info;
    pObjQuerySortItem sort_item;
    pXString reopen_path = NULL;

    	ASSERTMAGIC(obj,MGK_OBJECT);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objOpenQuery(%8.8lX:%3.3s:%s,'%s','%s')... ", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf, query, order_by);

	/** Allocate a query object **/
	this = (pObjQuery)nmMalloc(sizeof(ObjQuery));
	if (!this) 
	    goto error;
	memset(this, 0, sizeof(ObjQuery));
	this->QyText = query;
	this->Magic = MGK_OBJQUERY;
	linked_obj = objLinkTo(obj);
	this->Obj = linked_obj;
        this->ObjList = (void*)expCreateParamList();
	expAddParamToList((pParamObjects)(this->ObjList), NULL, NULL, EXPR_O_CURRENT);

	/** Ok, first parse the query. **/
	if (obj_internal_ParseCriteria(this, query, tree) < 0)
	    {
	    mssError(0,"OSML","Query search criteria is invalid");
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
	    goto error;
	    }

	/** Now, parse the order-by clause **/
	n_sortby = 0;
	if (orderbyexp)
	    {
	    /** A list of expressions is provided **/
	    for(i=0; i < OBJSYS_SORT_MAX; i++)
	        {
		this->SortBy[i] = orderbyexp[i];
		if (!orderbyexp[i]) break;
		n_sortby++;
		}
	    }
	else if (order_by && order_by[0])
	    {
	    /** A list of text strings is provided **/
	    lxs = mlxStringSession(order_by, MLX_F_EOF | MLX_F_FILENAMES | MLX_F_ICASER);
	    for(i=0; i < OBJSYS_SORT_MAX; i++)
	        {
		this->SortBy[i] = exp_internal_CompileExpression_r(lxs, 0, this->ObjList, EXPR_CMP_ASCDESC);
		if (!this->SortBy[i])
		    {
		    mssError(0, "OSML", "Invalid sort criteria '%s'", order_by);
		    goto error;
		    }
		n_sortby++;
		t = mlxNextToken(lxs);
		if (t == MLX_TOK_EOF)
		    {
		    break;
		    }
		else if (t != MLX_TOK_COMMA)
		    {
		    mssError(1, "OSML", "Invalid sort criteria '%s'", order_by);
		    goto error;
		    }
		}
	    mlxCloseSession(lxs);
	    lxs = NULL;
	    }

	/** Issue the open query operation to driver **/
	this->Data = linked_obj->Driver->OpenQuery(linked_obj->Data,this,&(linked_obj->Session->Trx));
	if (!(this->Data))
	    {
	    mssError(0,"OSML","Either queries not supported on this object or query failed");
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
	    goto error;
	    }

	/** Add to session open queries... **/
	xaAddItem(&(linked_obj->Session->OpenQueries),(void*)this);
	
	/** If sort requested and driver no support, set from sort flag and start the sort **/
	if (this->SortBy[0] && !(this->Flags & OBJ_QY_F_FULLSORT))
	    {
	    /** Ok, first item of business is to read entire result set.  Init the sort structure **/
	    this->SortInf = (pObjQuerySort)nmMalloc(sizeof(ObjQuerySort));
	    if (!this->SortInf)
		goto error;
	    xaInit(&this->SortInf->SortItems, OBJSYS_SORT_XASIZE);
	    xsInit(&this->SortInf->SortDataBuf);
	    this->SortInf->Reopen = OBJSYS_SORT_REOPEN;

	    /** Temp object or caller requested no-reopen behavior? **/
	    info = objInfo(obj);
	    if ((flags & OBJ_QY_F_NOREOPEN) || (info && (info->Flags & OBJ_INFO_F_TEMPORARY)))
		this->SortInf->Reopen = 0;

	    /** Generate path for doing reopens **/
	    if (this->SortInf->Reopen)
		{
		reopen_path = xsNew();
		if (!reopen_path)
		    goto error;
		if (obj_internal_PathToText(obj->Pathname, obj->SubPtr + obj->SubCnt, reopen_path) < 0)
		    goto error;
		if (strlen(xsString(reopen_path)) >= OBJSYS_MAX_PATH)
		    {
		    mssError(1, "OSML", "objOpenQuery: pathname too long");
		    goto error;
		    }
		strtcpy(this->SortInf->ReopenPath, xsString(reopen_path), sizeof(this->SortInf->ReopenPath));
		xsFree(reopen_path);
		reopen_path = NULL;
		}

	    /** Read result set **/
	    while((tmp_obj = objQueryFetch(this, 0400)))
	        {
		/** Record our sortable item **/
		sort_item = (pObjQuerySortItem)nmMalloc(sizeof(ObjQuerySortItem));
		if (!sort_item)
		    goto error;
		memset(sort_item, 0, sizeof(ObjQuerySortItem));
		if (!this->SortInf->Reopen)
		    {
		    sort_item->Obj = tmp_obj;
		    }
		else
		    {
		    if (objGetAttrValue(tmp_obj,"name",DATA_T_STRING,POD(&ptr)) != 0)
			{
			mssError(1, "OSML", "objOpenQuery: could not get 'name' of result set object");
			goto error;
			}
		    sort_item->Name = nmSysStrdup(ptr);
		    if (!sort_item->Name)
			goto error;
		    }
		    
		/** Build the sortable binary string representing the sort criteria values **/
		expModifyParam(this->ObjList, NULL, tmp_obj);
		sort_item->SortDataOffset = xsLength(&this->SortInf->SortDataBuf);
		len = objBuildBinaryImageXString(&this->SortInf->SortDataBuf, this->SortBy, n_sortby, this->ObjList, 0);
		if (len < 0)
		    {
		    mssError(1, "OSML", "objOpenQuery: could build binary comparison image for sort");
		    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "null\n");
		    goto error;
		    }
		sort_item->SortDataLen = len;

		xaAddItem(&this->SortInf->SortItems, (void*)sort_item);
		sort_item->SortInf = this->SortInf;

		/** Reopening later?  If so close the open object. **/
		if (this->SortInf->Reopen)
		    objClose(tmp_obj);
		}

	    /** Mergesort the result set. **/
	    mergesort(this->SortInf->SortItems.Items, this->SortInf->SortItems.nItems, obj_internal_SortCompare);
	    this->Flags |= OBJ_QY_F_FROMSORT;
	    }

	this->RowID = 0;

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "%8.8lX\n", (long)this);

	/** Order by expressions? **/
	if (this && order_by && !orderbyexp)
	    {
	    for(i=0; this->SortBy[i] && i < OBJSYS_SORT_MAX; i++)
		{
		expFreeExpression(this->SortBy[i]);
		this->SortBy[i] = NULL;
		}
	    }

	return this;

    error:
	if (reopen_path)
	    xsFree(reopen_path);

	/** Lexer session for textual order by items **/
	if (lxs)
	    mlxCloseSession(lxs);

	/** Order by expressions? **/
	if (this && order_by && !orderbyexp)
	    {
	    for(i=0; this->SortBy[i] && i < OBJSYS_SORT_MAX; i++)
		{
		expFreeExpression(this->SortBy[i]);
		this->SortBy[i] = NULL;
		}
	    }

	/** Clean up the query structure **/
	if (this)
	    objQueryClose(this);

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
    pObject obj = NULL;
    void* obj_data;
    char* name;
    char buf[OBJSYS_MAX_PATH + 32];
    pObjQuerySortItem sort_item;
    int rval;

    	ASSERTMAGIC(this,MGK_OBJQUERY);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryFetch(%8.8lX:%3.3s)...", (long)this, this->Drv->Name);

    	/** Multiquery? **/
	if (this->Drv) 
	    {
	    obj = obj_internal_AllocObj();
	    if (!obj)
		goto error;
	    if ((obj->Data = this->Drv->QueryFetch(this->Data, obj, mode, NULL)) == NULL)
	        {
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		goto error;
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
	    /** End of results? **/
	    if (this->RowID >= this->SortInf->SortItems.nItems)
		goto error;

	    /** Get next item **/
	    sort_item = (pObjQuerySortItem)this->SortInf->SortItems.Items[this->RowID++];

	    /** Temp objects we kept open; others we reopen by name **/
	    if (!this->SortInf->Reopen)
		{
		/** Not reopening - just grab the object **/
		obj = sort_item->Obj;
		sort_item->Obj = NULL;
		}
	    else
		{
		/** Reopening - build the path to use **/
		obj_internal_PathPart(this->Obj->Pathname, 0, 0);
		if (strlen(sort_item->Name) + strlen(this->SortInf->ReopenPath) + 27 > OBJSYS_MAX_PATH) 
		    {
		    mssError(1, "OSML", "Filename in query result exceeded internal limits");
		    goto error;
		    }
		rval = snprintf(buf, sizeof(buf), "%s/%s?ls__type=system%%2fobject", this->SortInf->ReopenPath, sort_item->Name);
		if (rval < 0 || rval >= sizeof(buf))
		    {
		    mssError(1, "OSML", "Filename in query result exceeded internal limits");
		    goto error;
		    }
		obj = objOpen(this->Obj->Session, buf, mode, 0400, "");
		if (!obj)
		    mssError(1, "OSML", "Could not re-open sorted query item '%s'", sort_item->Name);
		nmSysFree(sort_item->Name);
		sort_item->Name = NULL;
		}
	    if (obj)
		obj->RowID = this->RowID;
	    OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " %8.8lX:%3.3s:%s\n", (long)obj, obj->Driver->Name, obj->Pathname->Pathbuf);
	    return obj;
	    }

	/** Open up the object descriptor **/
	obj = obj_internal_AllocObj();
	if (!obj)
	    goto error;
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
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
		goto error;
		}
            obj->Data = obj_data;
    
            this->Obj->Driver->GetAttrValue(obj_data, "name", DATA_T_STRING, &name, NULL);
            if (strlen(name) + strlen(this->Obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH) 
                {
		mssError(1,"OSML","Filename in query result exceeded internal limits");
		OSMLDEBUG(OBJ_DEBUG_F_APITRACE, " null\n");
                goto error;
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

    error: /* error or end of results - return NULL */
	if (obj)
	    obj_internal_FreeObj(obj);

	return NULL;
    }


/*** objQueryCreate - creates a new object in the context of a running
 *** query.  Returns the newly open object.
 ***/
pObject
objQueryCreate(pObjQuery this, char* name, int mode, int permission_mask, char* type)
    {
    pObject new_obj = NULL;
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
		goto error;
		}
	    /** This is inherent with autoname **/
	    mode |= OBJ_O_EXCL;
	    }

	/** Open up the new object descriptor **/
	new_obj = obj_internal_AllocObj();
	if (!new_obj)
	    goto error;
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
		goto error;
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
	    goto error;
	    }

	/** Set the name **/
	new_obj->Driver->GetAttrValue(new_obj->Data, "name", DATA_T_STRING, &newname, NULL);
	if (strlen(newname) + strlen(new_obj->Pathname->Pathbuf) + 2 > OBJSYS_MAX_PATH || new_obj->Pathname->nElements + 1 > OBJSYS_MAX_ELEMENTS)
	    {
	    mssError(1,"OSML","Filename in query result exceeded internal limits");
	    goto error;
	    }
	bufptr = strchr(new_obj->Pathname->Pathbuf,0);
	*(bufptr++) = '/';
	new_obj->Pathname->Elements[new_obj->Pathname->nElements++] = bufptr;
	strcpy(bufptr, newname);

	return new_obj;

    error:
	if (new_obj)
	    {
	    if (new_obj->Data)
		new_obj->Driver->Close(new_obj->Data, &(new_obj->Session->Trx));
	    obj_internal_FreeObj(new_obj);
	    }
	return NULL;
    }


/*** objQueryClose - close an open query.  This does _not_ close any of
 *** the objects in the result set that may still be open.
 ***/
int 
objQueryClose(pObjQuery this)
    {
    int i;
    pObjQuerySortItem sort_item;
    pObjSession sess;

    	ASSERTMAGIC(this,MGK_OBJQUERY);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objQueryClose(%p:%3.3s)\n", this, this->Drv->Name);

    	/** Release sort information? **/
	if (this->SortInf)
	    {
	    /** Close temp objects? **/
	    if (!this->SortInf->Reopen)
		{
		for(i = this->SortInf->SortItems.nItems - 1; i >= this->RowID; i--)
		    {
		    sort_item = (pObjQuerySortItem)this->SortInf->SortItems.Items[i];
		    if (sort_item->Obj) objClose(sort_item->Obj);
		    if (sort_item->Name) nmSysFree(sort_item->Name);
		    }
		}
	    xaDeInit(&this->SortInf->SortItems);
	    xsDeInit(&this->SortInf->SortDataBuf);
	    nmFree(this->SortInf,sizeof(ObjQuerySort));
	    }

	/** Shut down query with the driver. **/
	if (this->Data)
	    {
	    if (this->Drv) 
		{
		/** Using multiquery **/
		this->Drv->QueryClose(this->Data);
		sess = this->QySession;
		}
	    else
		{
		/** Standard query **/
		this->Obj->Driver->QueryClose(this->Data, &(this->Obj->Session->Trx));
		sess = this->Obj->Session;
		}

	    /** Remove from session open queries... **/
	    xaRemoveItem(&(sess->OpenQueries), xaFindItem(&(sess->OpenQueries), (void*)this));
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

