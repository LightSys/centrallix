#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "expression.h"
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
/* Module: 	objdrv_qytree.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 17, 1998					*/
/* Description:	Query Tree object driver.  This driver uses a structure	*/
/*		file (see stparse.[ch]) for its node, which contains	*/
/*		a set of queries which rearrange the appearance of data	*/
/*		contained in other directory structures.  A powerful 	*/
/*		analogy to a database's VIEW.				*/
/************************************************************************/



/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    pStructInf	NodeData;
    pSnNode	BaseNode;
    pObject	LLObj;
    int		Offset;
    pParamObjects ObjList;	/* contains other objects in the tree that could be referenced */
    }
    QytData, *pQytData;

#define QYT_F_ISTEXT	1
#define QYT_F_FLEAF	2
#define QYT_F_USEHAVING	4	/* use HAVING clause when appending a lookup clause */

#define QYT(x) ((pQytData)(x))

#define QYT_MAX_CACHE	32


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pQytData	ObjInf;
    pObjQuery	Query;
    char	NameBuf[256];
    char*	QyText;
    pObject	LLQueryObj;
    pObjQuery	LLQuery;
    int		NextSubInfID;
    char*	ItemText;
    char*	ItemSrc;
    char*	ItemWhere;
    char*	ItemSql;
    pExpression	ItemSqlExpr;
    XHashTable	StructTable;
    pExpression	Constraint;
    }
    QytQuery, *pQytQuery;


/*** Structure used for attr/obj cache ***/
typedef struct
    {
    char	ObjPathname[256];
    XString	AttrNameBuf;
    XString	AttrValueBuf;
    XArray	AttrNames;
    XArray	AttrValues;
    time_t	CacheTime;
    int		LinkCnt;
    }
    QytObjAttr, *pQytObjAttr;


/*** Globals - including object attribute cache. ***/
struct
    {
    XHashTable	ObjAttrCache;
    XArray	ObjAttrCacheList;
    XHashTable	NodeCache;
    }
    QYT_INF;


/*** qyt_internal_ReadObject - reads an object, with its attributes (blobs
 *** limited to 255 bytes), and stores the data in the object-attr cache,
 *** or looks the object up there if it has already been cached.
 ***/
pQytObjAttr
qyt_internal_ReadObject(char* path, pObjSession s)
    {
    pQytObjAttr objattr;
    pObject obj;
    char* attrname;
    int type;
    int ival;
    char* sval;

    	/** Check the cache first. **/
	objattr = (pQytObjAttr)xhLookup(&QYT_INF.ObjAttrCache, path);
	if (objattr) 
	    {
	    objattr->CacheTime = time(NULL);
	    objattr->LinkCnt++;
	    return objattr;
	    }

	/** Otherwise, open the object & read its attrs. **/
	obj = objOpen(s, path, O_RDONLY, 0600, "system/file");
	if (!obj) return NULL;

	/** Allocate... **/
	objattr = (pQytObjAttr)nmMalloc(sizeof(QytObjAttr));
	if (!objattr)
	    {
	    objClose(obj);
	    return NULL;
	    }
	memccpy(objattr->ObjPathname,path,0,255);
	objattr->ObjPathname[255]=0;
	xsInit(&objattr->AttrNameBuf);
	xsInit(&objattr->AttrValueBuf);
	xaInit(&objattr->AttrNames,32);
	xaInit(&objattr->AttrValues,32);

	/** Loop through attributes. **/
	for(attrname=objGetFirstAttr(obj);attrname;attrname=objGetNextAttr(obj))
	    {
	    /** Copy attrname to names listing.  Be sure to copy the '\0'. **/
	    xaAddItem(&objattr->AttrNames, xsStringEnd(&objattr->AttrNameBuf));
	    xsConcatenate(&objattr->AttrNameBuf, attrname, strlen(attrname)+1);

	    /** Get type and copy to attrvalues. **/
	    type = objGetAttrType(obj,attrname);
	    switch(type)
	        {
		case DATA_T_INTEGER:
		    if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&ival)) == 0)
		        {
		        xaAddItem(&objattr->AttrValues, xsStringEnd(&objattr->AttrValueBuf));
			xsConcatenate(&objattr->AttrValueBuf, (char*)&ival, 4);
			}
		    else
		        {
		        xaAddItem(&objattr->AttrValues, NULL);
			}
		    break;

		case DATA_T_STRING:
		    if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&sval)) == 0)
		        {
		        xaAddItem(&objattr->AttrValues, xsStringEnd(&objattr->AttrValueBuf));
		        xsConcatenate(&objattr->AttrValueBuf,sval, strlen(sval)+1);
			}
		    else
		        {
		        xaAddItem(&objattr->AttrValues, NULL);
			}
		    break;

		default:
		    break;
		}
	    }

	/** Close up the object **/
	objClose(obj);

	/** Cache the objattr structure **/
	xhAdd(&QYT_INF.ObjAttrCache, objattr->ObjPathname, (void*)objattr);
	xaAddItem(&QYT_INF.ObjAttrCacheList, (void*)objattr);

    return objattr;
    }


/*** qyt_internal_AttrValue - retrieves an attribute value from the objattr
 *** structure, and returns a pointer to that value, whether a 4-byte int or 
 *** an array of char.
 ***/
void*
qyt_internal_AttrValue(pQytObjAttr objattr, char* attrname)
    {
    int i;
    char* name;

    	/** Scan through the attrnames. **/
	for(i=0;i<objattr->AttrNames.nItems;i++)
	    {
	    name = (char*)(objattr->AttrNames.Items[i]);
	    if (!strcmp(name,attrname)) return (void*)(objattr->AttrValues.Items[i]);
	    }

    return NULL;
    }


/*** qyt_internal_ProcessPath - handles the lookup of the snNode from the given
 *** path in the querytree, and the optional creation of a new entity, as needed,
 *** manipulating the WHERE sql in the snNode to assign parent foreign key links
 *** and so forth.
 ***/
pQytData
qyt_internal_ProcessPath(pObjSession s, pPathname path, pSnNode node, int subref, pStructInf dptr, int openflags, int no_open)
    {
    pQytData inf;
    pStructInf lookup_inf, find_inf, next_inf;
    char* strval;
    char* exprval;
    char* ptr;
    int i,v,t;
    pExpression expr;
    pParamObjects objlist;
    pObject test_obj;
    XHashTable struct_table;
    XString sql;
    pObjQuery test_qy;

    	/** Setup the pathname into its subparts **/
	for(i=1;i<path->nElements;i++) path->Elements[i][-1] = 0;

    	/** Allocate the data info first **/
	inf = (pQytData)nmMalloc(sizeof(QytData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(QytData));
	inf->BaseNode = node;
	inf->LLObj = NULL;
	inf->Offset = 0;

	/** Create the object parameter list... **/
        objlist = expCreateParamList();

	/** Initialize the listing of query tree elements **/
	xhInit(&struct_table, 17, 0);

	/** Look for the part of the structinf which this path references. **/
	while(subref < path->nElements)
	    {
	    /** Add a slot to the param object list for this tree level **/
	    objlist->ParentID = objlist->CurrentID;

	    /** Look for text and source items in the querytree definition. **/
	    next_inf = NULL;
	    for(i=0;i<dptr->nSubInf;i++) 
	        {
		find_inf = dptr->SubInf[i];
		if (stStructType(find_inf) == ST_T_SUBGROUP)
                    {
	    PROCESS_SUBGROUP:
                    inf->Pathname[0] = '\0';
		    inf->Flags = 0;
                    if ((lookup_inf = stLookup(find_inf, "text")))
                        {
                        strval = "";
                        stAttrValue(lookup_inf,NULL,&strval,0);
                        if (!strcmp(path->Elements[subref],strval))
                            {
			    expAddParamToList(objlist,strval,NULL,EXPR_O_CURRENT | EXPR_O_ALLOWDUPS);
                            next_inf = find_inf;
                            break;
                            }
                        if (subref == path->nElements - 1)
                            inf->LLObj = NULL;
                        }
                    else if ((lookup_inf = stLookup(find_inf, "sql")))
			{
			xhAdd(&struct_table, find_inf->Name, (void*)find_inf);

			/** not opening objects? skip sql query if so **/
			if (no_open)
			    {
			    next_inf = find_inf;
			    break;
			    }

			/** forced leaf flag and having clause flag **/
			strval = NULL;
			stAttrValue(stLookup(find_inf, "force_leaf"), NULL, &strval, 0);
			if (strval && !strcasecmp(strval,"yes"))
			    inf->Flags |= QYT_F_FLEAF;
			strval = NULL;
			stAttrValue(stLookup(find_inf, "use_having"), NULL, &strval, 0);
			if (strval && !strcasecmp(strval,"yes"))
			    inf->Flags |= QYT_F_USEHAVING;

			/** build and run the sql query to find the object **/
			expAddParamToList(objlist,find_inf->Name,NULL,EXPR_O_CURRENT | EXPR_O_ALLOWDUPS);
			xsInit(&sql);
			strval = "";
			expr = NULL;
			t = stGetAttrType(lookup_inf, 0);
			if (t == DATA_T_STRING)
			    {
			    stAttrValue(lookup_inf, NULL, &strval, 0);
			    }
			else if (t == DATA_T_CODE)
			    {
			    if (stGetAttrValue(lookup_inf, DATA_T_CODE, POD(&expr), 0) == 0)
				{
				expr = expDuplicateExpression(expr);
				if (expr)
				    {
				    objlist->Session = s;
				    expBindExpression(expr, objlist, EXPR_F_RUNSERVER);
				    if (expEvalTree(expr, objlist) == 0 && expr->DataType == DATA_T_STRING && !(expr->Flags & EXPR_F_NULL))
					strval = expr->String;
				    }
				}
			    }

			if (!strcmp(strval, ""))
			    {
			    if (expr) expFreeExpression(expr);
			    mssError(0,"QYT","Invalid SQL for '%s'",find_inf->Name);
			    goto error;
			    }

			xsQPrintf(&sql, "%STR %STR :name = %STR&DQUOT", strval, (inf->Flags & QYT_F_USEHAVING)?"HAVING":"WHERE", path->Elements[subref]);
			if (expr) expFreeExpression(expr);
			expr = NULL;

			test_obj = NULL;
			test_qy = objMultiQuery(s, sql.String, objlist, 0);
			if (test_qy)
			    {
			    /** query open succeeded, try to fetch a result **/
			    test_obj = objQueryFetch(test_qy, O_RDONLY);
			    objQueryClose(test_qy);
			    }
			xsDeInit(&sql);
			if (!test_obj)
			    break;

			/** ok, found the object **/
			expModifyParam(objlist,NULL,test_obj);
			objUnmanageObject(test_obj->Session, test_obj);
                        if (subref == path->nElements - 1) 
			    {
                            inf->LLObj = objLinkTo(test_obj);
			    }
			next_inf = find_inf;
			break;
			}
                    else if ((lookup_inf = stLookup(find_inf, "source")))
                        {
                        strval = "";
                        stAttrValue(lookup_inf,NULL,&strval,0);
                        expr = NULL;
                        exprval = NULL;
                        stAttrValue(stLookup(find_inf, "where"),NULL,&exprval,0);
                        if (exprval && !no_open) 
			    {
			    //objlist->Names[(signed char)(objlist->CurrentID)] = find_inf->Name;
			    expr = (pExpression)expCompileExpression(exprval, objlist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
			    }
                        snprintf(inf->Pathname,sizeof(inf->Pathname),"%s/%s?ls__type=application%%2foctet-stream",strval,path->Elements[subref]);

			/** forced leaf flag **/
			strval = NULL;
			stAttrValue(stLookup(find_inf, "force_leaf"), NULL, &strval, 0);
			if (strval && !strcasecmp(strval,"yes"))
			    inf->Flags |= QYT_F_FLEAF;
			strval = NULL;
			stAttrValue(stLookup(find_inf, "use_having"), NULL, &strval, 0);
			if (strval && !strcasecmp(strval,"yes"))
			    inf->Flags |= QYT_F_USEHAVING;

			/** Setup this querytree struct entry for lookups **/
			xhAdd(&struct_table, find_inf->Name, (void*)find_inf);
			expAddParamToList(objlist,find_inf->Name,NULL,EXPR_O_CURRENT | EXPR_O_ALLOWDUPS);

			/** Not opening any objects?  Skip open attempt if so. **/
			if (no_open)
			    {
			    next_inf = find_inf;
			    break;
			    }
    
                        /** Open the object, or create if need be.  If create, convert any WHERE to setattrs **/
                        test_obj = objOpen(s, inf->Pathname, O_RDONLY, 0600, "system/file");
			if (test_obj) 
			    {
			    expModifyParam(objlist, NULL, test_obj);
			    objUnmanageObject(test_obj->Session, test_obj);
			    }
                        if (!test_obj && subref == path->nElements - 1 && (openflags & O_CREAT))
                            {
                            test_obj = objOpen(s, inf->Pathname, O_RDWR | O_TRUNC | (openflags & (O_CREAT | OBJ_O_AUTONAME)), 0600, "system/file");
                            if (!test_obj) break;
			    expModifyParam(objlist, NULL, test_obj);
			    objUnmanageObject(test_obj->Session, test_obj);
			    objlist->Flags[(signed char)(objlist->CurrentID)] |= EXPR_O_UPDATE;
                            if (expr)
                                {
				/** Reverse eval the expression to set the object attributes **/
				expr->Integer = 1;
				v = expReverseEvalTree(expr, objlist);
				expFreeExpression(expr);
				expr = NULL;
				if (v < 0)
				    {
				    objClose(test_obj);
				    expModifyParam(objlist, NULL, NULL);
				    break;
				    }
                                }
                            }
                        else if (!test_obj) break;
    
                        /** Validate the where clause expression if need be. **/
			objlist->Session = s;
                        v = !expr || (expEvalTree(expr,objlist) >= 0 && expr->Integer != 0);
                        if (expr) expFreeExpression(expr);
			expr = NULL;
                        if (subref == path->nElements - 1) 
			    {
                            inf->LLObj = objLinkTo(test_obj);
			    }
                        /*else 
			    {
                            if (!v) objClose(test_obj);
			    }*/
			if (v)
			    {
			    next_inf = find_inf;
			    break;
			    }
                        }
                    }
		else if (!strcmp(find_inf->Name,"recurse"))
		    {
		    expr = NULL;
		    exprval = NULL;
		    stAttrValue(stLookup(dptr, "known_leaf"),NULL,&exprval,0);
		    expAddParamToList(objlist,find_inf->Name,NULL,EXPR_O_CURRENT | EXPR_O_ALLOWDUPS);
		    if (exprval) 
			{
			expr = (pExpression)expCompileExpression(exprval, objlist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
			if (!expr)
			    {
			    mssError(0,"QYT","Error in known_leaf expression");
			    break;
			    }
			else
			    {
			    v = (expEvalTree(expr,objlist) >= 0 && expr->Integer != 0);
			    expFreeExpression(expr);
			    expr = NULL;
			    if (v) break;
			    }
			}
		    expRemoveParamFromList(objlist, find_inf->Name);
		    stGetAttrValue(find_inf, DATA_T_STRING, POD(&ptr), 0);
		    find_inf = (pStructInf)xhLookup(&struct_table, ptr);
		    if (find_inf) goto PROCESS_SUBGROUP;
		    }
		}

	    /** Didn't find? **/
	    if (!next_inf) 
	        {
		inf->LLObj = NULL;
		mssError(0,"QYT","Could not find object via access through querytree");
		goto error;
		}
	    dptr = next_inf;
	    subref++;
	    }

	/** Ok, close up the structure table. **/
	xhClear(&struct_table, NULL, NULL);
	xhDeInit(&struct_table);

	inf->NodeData = dptr;
	inf->ObjList = objlist;

	return inf;

    error:
	/** Ok, close up the structure table. **/
	xhClear(&struct_table, NULL, NULL);
	xhDeInit(&struct_table);
	for(i=0;i<objlist->nObjects-1;i++)
	    if (objlist->Objects[i])
		objClose(objlist->Objects[i]);
	nmFree(inf,sizeof(QytData));

	return NULL;
    }


/*** qytOpen - open a file or directory.
 ***/
void*
qytOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pQytData inf;
    char* node_path;
    pSnNode node = NULL;
    char buf[1];

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		mssError(0,"QYT","Could not create new querytree node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    objRead(obj->Prev, buf, 0, 0, OBJ_U_SEEK);
	    node = snReadNode(obj->Prev);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    mssError(0,"QYT","Could not open querytree node object");
	    return NULL;
	    }

	/** Allocate the structure **/
	inf = qyt_internal_ProcessPath(obj->Session, obj->Pathname, node, obj->SubPtr, node->Data, obj->Mode & (O_CREAT | OBJ_O_AUTONAME), 0);
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!inf) return NULL;
	inf->Obj = obj;
	inf->Mask = mask;
	obj->SubCnt = obj->Pathname->nElements - obj->SubPtr + 1;

	/** Set object params. **/
	inf->BaseNode = node;
	inf->BaseNode->OpenCnt++;
	inf->Offset = 0;

    return (void*)inf;
    }


/*** qytClose - close an open file or directory.
 ***/
int
qytClose(void* inf_v, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    int i;

    	/** Write the node first **/
	snWriteNode(inf->Obj->Prev, inf->BaseNode);

	/** Close the lowlevel-object **/
	if (inf->LLObj) objClose(inf->LLObj);
	
	/** Release the memory **/
	inf->BaseNode->OpenCnt --;
	for(i=0;i<inf->ObjList->nObjects-1;i++)
	    if (inf->ObjList->Objects[i])
		objClose(inf->ObjList->Objects[i]);
	nmFree(inf,sizeof(QytData));

    return 0;
    }


/*** qytCreate - create a new file without actually opening that 
 *** file.
 ***/
int
qytCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = qytOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	qytClose(inf, oxt);

    return 0;
    }


/*** qytDelete - delete an existing file or directory.
 ***/
int
qytDelete(pObject obj, pObjTrxTree* oxt)
    {
    pQytData inf = NULL;
    char* node_path;
    pSnNode node;
    int rval = 0;
    int i;

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);
	node = snReadNode(obj->Prev);
	if (!node) 
	    {
	    mssError(0,"QYT","Could not access node object for querytree");
	    rval = -1;
	    goto error;
	    }

	/** Process path to determine actual path of object. **/
	inf = qyt_internal_ProcessPath(obj->Session, obj->Pathname, node, obj->SubPtr, node->Data, 0, 0);
	if (inf->LLObj) objClose(inf->LLObj);
	obj_internal_PathPart(obj->Pathname, 0,0);

	/** Call delete on it, using the actual path determined by process_path. **/
	if (inf->Pathname[0]) 
	    {
	    if (objDelete(obj->Session, inf->Pathname) < 0)
	        {
		mssError(0,"QYT","Could not delete object referenced by querytree");
		rval = -1;
		goto error;
		}
	    }
	else
	    {
	    mssError(0,"QYT","Could not determine referenced querytree object");
	    rval = -1;
	    goto error;
	    }

    error:

	/** Release, don't call close because that might write data to a deleted object **/
	if (inf)
	    {
	    for(i=0;i<inf->ObjList->nObjects-1;i++)
		if (inf->ObjList->Objects[i])
		    objClose(inf->ObjList->Objects[i]);
	    nmFree(inf,sizeof(QytData));
	    }

    return rval;
    }


/*** qytDeleteObj - delete an existing object that is already open.
 ***/
int
qytDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    int rval = 0;
    int i;

    	/** Write the node first **/
	snWriteNode(inf->Obj->Prev, inf->BaseNode);

	/** Delete the lowlevel-object **/
	if (inf->LLObj) 
	    rval = objDeleteObj(inf->LLObj);
	else
	    {
	    rval = -1;
	    mssError(1,"QYT","Could not delete object not having an underlying data source");
	    }
	
	/** Release the memory **/
	inf->BaseNode->OpenCnt --;
	for(i=0;i<inf->ObjList->nObjects-1;i++)
	    if (inf->ObjList->Objects[i])
		objClose(inf->ObjList->Objects[i]);
	nmFree(inf,sizeof(QytData));

    return rval;
    }


/*** qyt_internal_ReadText() - attempt to read data from a 'text' type object
 *** that might have a 'content' property giving the object's content.
 ***/
int
qyt_internal_ReadText(pQytData inf, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pStructInf content_inf;
    int len;
    char* ptr;

	/** do we have content? **/
	content_inf = stLookup(inf->NodeData, "content");
	if (!content_inf)
	    {
	    mssError(1,"QYT","Text querytree object has no content");
	    return -1;
	    }

	/** ok, get our bearings... **/
	if (stGetAttrValue(content_inf, DATA_T_STRING, POD(&ptr), 0) != 0)
	    {
	    mssError(1,"QYT","Could not read content from text querytree object");
	    return -1;
	    }
	len = strlen(ptr);
	if (flags & OBJ_U_SEEK)
	    inf->Offset = offset;
	if (inf->Offset < 0) inf->Offset = 0;
	if (inf->Offset > len) inf->Offset = len;
	if (inf->Offset + maxcnt > len) maxcnt = len - inf->Offset;
	if (maxcnt <= 0) return 0;
	memcpy(buffer, ptr+inf->Offset, maxcnt);
	inf->Offset += maxcnt;

    return maxcnt;
    }


/*** qytRead - Attempt to read from the underlying object.
 ***/
int
qytRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    int rcnt;

    	/** If no actual pathname (presented path was a text="") ... **/
	if (inf->LLObj == NULL) 
	    {
	    return qyt_internal_ReadText(inf, buffer, maxcnt, offset, flags, oxt);
	    }

	/** Otherwise, attempt the read operation. **/
	rcnt = objRead(inf->LLObj, buffer, maxcnt, offset, flags);

    return rcnt;
    }


/*** qytWrite - As above, attempt to read if we found an actual object.
 ***/
int
qytWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    int wcnt;

    	/** No write if the object didn't map to an actual one. **/
	if (inf->LLObj == NULL) 
	    {
	    mssError(1,"QYT","Cannot write content to a text='' querytree object");
	    return -1;
	    }

	/** Ok, attempt the write operation **/
	wcnt = objWrite(inf->LLObj, buffer, cnt, offset, flags);

    return wcnt;
    }


/*** qyt_internal_GetQueryItem - finds the first(next) inf item in the .qyt
 *** file to include in this query.  Items to include are system/querytree
 *** type subitems that either have a source= or text= property.  Source=
 *** type subitems will result in zero or more items for the query.  Text=
 *** subitems will result in exactly one returned object for the query.
 *** Text= types cannot be modified/etc by the enduser.  The .qyt must be
 *** opened as a structure file (via objdrv_struct) to modify Text=items.
 *** modifications/addidions/deletions of query results from a Source=
 *** item will be passed on to the source driver (such as objdrv_sybase).
 ***/
int
qyt_internal_GetQueryItem(pQytQuery qy)
    {
    pStructInf main_inf;
    pStructInf find_inf;
    char* val;
    char* ptr;
    int t;

    	/** Already hit end of query? **/
	if (qy->NextSubInfID == -1) return -1;

	/** Need to 'prime' the struct table? **/
	if (qy->StructTable.nItems == 0)
	    {
	    find_inf = qy->ObjInf->NodeData;
	    while(find_inf)
	        {
		if (stStructType(find_inf) == ST_T_SUBGROUP) 
		    xhAdd(&qy->StructTable, find_inf->Name, (void*)find_inf);
		find_inf = find_inf->Parent;
		}
	    }

    	/** Search for a SOURCE= or TEXT= subgroup inf **/
	main_inf = qy->ObjInf->NodeData;

	/** Is main_inf already a 'recurse' node? **/
	/*if (stStructType(main_inf) == ST_T_ATTRIB && !strcmp(main_inf->Name, "recurse"))
	    {
	    stGetAttrValue(main_inf, DATA_T_STRING, POD(&ptr), 0);
	    find_inf = (pStructInf)xhLookup(&qy->StructTable, ptr);
	    if (find_inf) main_inf = find_inf;
	    }*/

	/** Do the search **/
	while(qy->NextSubInfID < main_inf->nSubInf)
	    {
	    find_inf = main_inf->SubInf[qy->NextSubInfID++];
	    if (stStructType(find_inf) == ST_T_ATTRIB && !strcmp(find_inf->Name, "recurse"))
	        {
		stGetAttrValue(find_inf, DATA_T_STRING, POD(&ptr), 0);
		find_inf = (pStructInf)xhLookup(&qy->StructTable, ptr);
		if (!find_inf) continue;
		}
	    qy->ItemText = NULL;
	    qy->ItemSrc = NULL;
	    qy->ItemWhere = NULL;
	    qy->ItemSql = NULL;
	    qy->ItemSqlExpr = NULL;
	    if (stStructType(find_inf) == ST_T_SUBGROUP)
	        {
		xhAdd(&qy->StructTable, find_inf->Name, (void*)find_inf);
		val = NULL;
		stAttrValue(stLookup(find_inf,"text"),NULL,&val,0);
		if (val)
		    {
		    qy->ItemText = val;
		    return qy->NextSubInfID - 1;
		    }
		stAttrValue(stLookup(find_inf,"source"),NULL,&val,0);
		if (val)
		    {
		    qy->ItemSrc = val;
		    stAttrValue(stLookup(find_inf,"where"),NULL,&(qy->ItemWhere),0);
		    return qy->NextSubInfID - 1;
		    }
		t = stGetAttrType(stLookup(find_inf,"sql"), 0);
		if (t == DATA_T_STRING)
		    {
		    stAttrValue(stLookup(find_inf,"sql"),NULL,&val,0);
		    if (val)
			{
			qy->ItemSql = val;
			return qy->NextSubInfID - 1;
			}
		    }
		else if (t == DATA_T_CODE)
		    {
		    stGetAttrValue(stLookup(find_inf,"sql"), DATA_T_CODE, POD(&qy->ItemSqlExpr), 0);
		    if (qy->ItemSqlExpr)
			{
			qy->ItemSqlExpr = expDuplicateExpression(qy->ItemSqlExpr);
			qy->ItemSql = "";
			return qy->NextSubInfID - 1;
			}
		    }
		}
	    }

	/** End of query. **/
	qy->NextSubInfID = -1;

    return -1;
    }


/*** qyt_internal_StartQuery - starts a query on the low-level object on which the
 *** querytree translation is being performed.  Returns a query object from 
 *** objOpenQuery, which can be used for objQueryFetch()ing.
 ***/
pObjQuery
qyt_internal_StartQuery(pQytQuery qy)
    {
    pObjQuery qyinf = NULL;
    pExpression usr_query = NULL;
    pExpression item_query = NULL;
    pExpression expr = NULL;
    pParamObjects objlist = NULL;
    char* where_clause;
    XString sql;

	/** Doing a full MultiQuery? **/
	if (qy->ItemSql || qy->ItemSqlExpr)
	    {
	    xsInit(&sql);
	    objlist = expCreateParamList();
	    expCopyList(qy->ObjInf->ObjList, objlist, -1);
	    expAddParamToList(objlist,"this",NULL,EXPR_O_CURRENT);

	    /** Expression-based sql? **/
	    if (qy->ItemSqlExpr)
		{
		objlist->Session = qy->ObjInf->Obj->Session;
		expBindExpression(qy->ItemSqlExpr, objlist, EXPR_F_RUNSERVER);
		if (expEvalTree(qy->ItemSqlExpr, objlist) == 0 && qy->ItemSqlExpr->DataType == DATA_T_STRING && !(qy->ItemSqlExpr->Flags & EXPR_F_NULL))
		    qy->ItemSql = qy->ItemSqlExpr->String;
		}

	    if (qy->Query->Tree)
		{
		/** Merging in a WHERE clause from the user... **/
		xsPrintf(&sql, (qy->ObjInf->Flags & QYT_F_USEHAVING)?"%s HAVING ":"%s WHERE ", qy->ItemSql);
		expGenerateText(qy->Query->Tree, objlist, xsWrite, &sql, '\0', "cxsql", 0);
		}
	    else
		{
		xsCopy(&sql, qy->ItemSql, -1);
		}
	    qyinf = objMultiQuery(qy->ObjInf->Obj->Session, sql.String, objlist, 0);
	    if (qyinf)
		objUnmanageQuery(qy->ObjInf->Obj->Session, qyinf);
	    else
		mssError(0,"QYT","Could not open querytree sql query");
	    expFreeParamList(objlist);
	    objlist = NULL;
	    xsDeInit(&sql);
	    return qyinf;
	    }

    	/** Get the where clause restrictions **/
	where_clause = qy->ItemWhere;
	if (qy->Query->Tree)
	    {
	    usr_query = expDuplicateExpression(qy->Query->Tree);
	    }
	if (where_clause)
	    {
	    objlist = expCreateParamList();
	    expCopyList(qy->ObjInf->ObjList, objlist, -1);
	    expAddParamToList(objlist,"this",NULL,EXPR_O_CURRENT);
	    item_query = expCompileExpression(where_clause, objlist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
	    if (!item_query)
		goto error;
	    expRemapID(item_query, objlist->nObjects-1, 0);
	    expFreezeEval(item_query, objlist, EXPR_OBJID_CURRENT);
	    /*expFreezeEval(item_query, objlist, objlist->nObjects-1);*/
	    }

	/** Combine the expressions as needed **/
	if (usr_query && !item_query)
	    {
	    expr = usr_query;
	    usr_query = NULL;
	    }
	else if (!usr_query && item_query)
	    {
	    expr = item_query;
	    item_query = NULL;
	    }
	else if (usr_query && item_query)
	    {
	    expr = expAllocExpression();
	    expr->NodeType = EXPR_N_AND;
	    expAddNode(expr, usr_query);
	    expAddNode(expr, item_query);
	    item_query = NULL;
	    usr_query = NULL;
	    }

	/** Issue the query. **/
	qy->LLQueryObj = objOpen(qy->ObjInf->Obj->Session, qy->ItemSrc, O_RDONLY, 0600, "system/directory");
	if (qy->LLQueryObj) 
	    {
	    objUnmanageObject(qy->LLQueryObj->Session, qy->LLQueryObj);
	    qyinf = objOpenQuery(qy->LLQueryObj, NULL, NULL, expr, NULL);
	    if (!qyinf)
		{
		objClose(qy->LLQueryObj);
		qy->LLQueryObj = NULL;
		}
	    else
		{
		objUnmanageQuery(qy->LLQueryObj->Session, qyinf);
		qy->Constraint = expr;
		expr = NULL;
		}
	    }

	/** Failed to open source object or issue query? **/
	if (!(qyinf)) 
	    {
	    mssError(0,"QYT","Could not open querytree source object/query");
	    goto error;
	    }
    
    error:
	if (usr_query) expFreeExpression(usr_query);
	if (expr) expFreeExpression(expr);
	if (item_query) expFreeExpression(item_query);
	if (objlist) expFreeParamList(objlist);

	return qyinf;
    }


/*** qytOpenQuery - open a directory query.
 ***/
void*
qytOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    pQytQuery qy;

	/** Allocate the query structure **/
	qy = (pQytQuery)nmMalloc(sizeof(QytQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(QytQuery));
	qy->ObjInf = inf;
	qy->NextSubInfID = 0;
	qy->Query = query;
	qy->ObjInf = inf;
	qy->Constraint = NULL;
	xhInit(&qy->StructTable,17,0);

	/** Get the next subinf ready for retrieval. **/
	if (qyt_internal_GetQueryItem(qy) < 0)
	    {
	    xhClear(&qy->StructTable, NULL, NULL);
	    xhDeInit(&qy->StructTable);
	    nmFree(qy, sizeof(QytQuery));
	    return NULL;
	    }

	/** If a source= query, start it now. **/
	if (qy->ItemSrc != NULL || qy->ItemSql != NULL)
	    qy->LLQuery = qyt_internal_StartQuery(qy);

    return (void*)qy;
    }


/*** qytQueryFetch - get the next directory entry as an open object.
 ***/
void*
qytQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pQytQuery qy = ((pQytQuery)(qy_v));
    pQytData inf;
    pObject llobj = NULL;
    pStructInf find_inf;
    char* ptr;
    char* objname = NULL;
    int cur_id = -1;

	/** If text item, return it and go to next item, else look at sub query **/
	while(!objname && qy->NextSubInfID != -1)
	    {
	    cur_id = qy->NextSubInfID - 1;
	    if (qy->ItemText != NULL)
	        {
		qy->LLQuery = NULL;
	        objname = qy->ItemText;
	        qyt_internal_GetQueryItem(qy);
	        if (qy->NextSubInfID != -1 && (qy->ItemSrc != NULL || qy->ItemSql != NULL))
		    qy->LLQuery = qyt_internal_StartQuery(qy);
	        }
	    else
	        {
	        if (qy->LLQuery) 
		    {
		    llobj = objQueryFetch(qy->LLQuery, mode);
		    if (!llobj) 
		        {
			objQueryClose(qy->LLQuery);
			qy->LLQuery = NULL;
			if (qy->LLQueryObj)
			    {
			    objClose(qy->LLQueryObj);
			    qy->LLQueryObj = NULL;
			    }
			}
		    }
		if (!llobj)
		    {
	            qyt_internal_GetQueryItem(qy);
	            if (qy->NextSubInfID != -1 && (qy->ItemSrc != NULL || qy->ItemSql != NULL))
			qy->LLQuery = qyt_internal_StartQuery(qy);
		    }
		else
		    {
		    objGetAttrValue(llobj, "name", DATA_T_STRING,POD(&objname));
		    objUnmanageObject(llobj->Session, llobj);
		    }
	        }
	    }

    	/** If end of query, return NULL. **/
	if (cur_id == -1 || !objname) return NULL;

	/** Build the filename. **/
	if (obj_internal_AddToPath(obj->Pathname, objname) < 0)
	    {
	    mssError(1,"QYT","Could not fetch object from querytree");
	    return NULL;
	    }

	/** Alloc the structure **/
	inf = qyt_internal_ProcessPath(obj->Session, obj->Pathname, qy->ObjInf->BaseNode, 
		qy->ObjInf->Obj->SubPtr, qy->ObjInf->BaseNode->Data, 0, 1);
	if (!inf)
	    return NULL;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname, 0, 0));
	if (inf->LLObj)
	    objClose(inf->LLObj);
	inf->LLObj = llobj;
	inf->BaseNode = qy->ObjInf->BaseNode;

	/** Point to the correct structure file subgroup **/
	inf->NodeData = qy->ObjInf->NodeData->SubInf[cur_id];
	if (stStructType(inf->NodeData) == ST_T_ATTRIB && !strcmp(inf->NodeData->Name, "recurse"))
	    {
	    stGetAttrValue(inf->NodeData, DATA_T_STRING, POD(&ptr), 0);
	    find_inf = (pStructInf)xhLookup(&qy->StructTable, ptr);
	    if (find_inf) inf->NodeData = find_inf;
	    }

	inf->BaseNode->OpenCnt++;
	inf->Obj = obj;
	inf->Offset = 0;
	obj_internal_PathPart(obj->Pathname,0,0);

	/** Set up the param objects list for this fetched object. **/
	inf->ObjList = expCreateParamList();
	expCopyList(qy->ObjInf->ObjList, inf->ObjList, -1);
	expAddParamToList(inf->ObjList, objname, obj, EXPR_O_CURRENT);
	expLinkParams(inf->ObjList, 0, -1);

    return (void*)inf;
    }


/*** qytQueryClose - close the query.
 ***/
int
qytQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pQytQuery qy = ((pQytQuery)(qy_v));

    	/** Close any pending low-level query **/
	if (qy->LLQuery) objQueryClose(qy->LLQuery);
	if (qy->LLQueryObj) objClose(qy->LLQueryObj);

	/** Free the structure **/
	if (qy->Constraint) expFreeExpression(qy->Constraint);
	xhClear(&qy->StructTable, NULL, NULL);
	xhDeInit(&qy->StructTable);
	nmFree(qy_v,sizeof(QytQuery));

    return 0;
    }


/*** qytGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
qytGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** If there is a low-level object, lookup within it **/
	if (inf->LLObj) return objGetAttrType(inf->LLObj, attrname);

	mssError(1,"QYT","Invalid attribute '%s' for querytree object", attrname);

    return -1;
    }


/*** qytGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
qytGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    pStructInf content_inf;
    int rval;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Obj->Mode & OBJ_O_AUTONAME && inf->LLObj)
		{
		return objGetAttrValue(inf->LLObj, "name", DATA_T_STRING, val);
		}
	    else
		{
		val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements - 1, 0);
		obj_internal_PathPart(inf->Obj->Pathname,0,0);
		}
	    return 0;
	    }

	/** annotation? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}

	    /** If object associated, get annot from it **/
	    if (inf->LLObj) return objGetAttrValue(inf->LLObj, attrname, datatype, val);

	    /** Otherwise, get annot from node if it has it **/
	    if (stGetAttrValue(stLookup(inf->NodeData,attrname), DATA_T_STRING, val, 0) == 0)
		{
		return 0;
		}
	    else
		{
		val->String = "";
		return 0;
		}
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"outer_type") && !(inf->LLObj))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->NodeData->UsrType;
	    return 0;
	    }
	else if ((!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type")) && !(inf->LLObj))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYT","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (stLookup(inf->NodeData, "content"))
		{
		if ((content_inf = stLookup(inf->NodeData, "content_type")) != NULL)
		    stGetAttrValue(content_inf, DATA_T_STRING, val, 0);
		else
		    val->String = "text/plain";
		}
	    else
		{
		val->String = "system/void";
		}
	    return 0;
	    }

	/** Low-level object?  Lookup the attribute in it **/
	if (inf->LLObj)
	    {
	    rval = objGetAttrValue(inf->LLObj, attrname, datatype, val);
	    if (rval != 0 && (!strcmp(attrname,"inner_type") || !strcmp(attrname,"content_type")) && datatype == DATA_T_STRING)
		{
		val->String = "application/octet-stream";
		return 0;
		}
	    if (rval != 0 && !strcmp(attrname,"outer_type") && datatype == DATA_T_STRING)
		{
		val->String = "system/object";
		return 0;
		}
	    return rval;
	    }

	/** last_modification?  Lookup from node **/
	if (!strcmp(attrname,"last_modification"))
	    {
	    val->DateTime = snGetLastModification(inf->BaseNode);
	    return 0;
	    }

	mssError(1,"QYT","Invalid attribute '%s' for querytree object", attrname);

    return -1;
    }


/*** qytGetNextAttr - get the next attribute name for this object.
 ***/
char*
qytGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

    	/** If low-level object, pass this through to it. **/
	if (inf->LLObj) return objGetNextAttr(inf->LLObj);

    return NULL;
    }


/*** qytGetFirstAttr - get the first attribute name for this object.
 ***/
char*
qytGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

    	/** If low-level object, pass this through to it. **/
	if (inf->LLObj) return objGetFirstAttr(inf->LLObj);

    return NULL;
    }


/*** qytSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
qytSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYT","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->NodeData == inf->BaseNode->Data)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"QYT","SetAttr 'name': name too long for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) return -1;
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    strcpy(inf->NodeData->Name,val->String);
	    return 0;
	    }

	/** Otherwise, attempt to pass setattr through to lowlevel obj **/
	if (inf->LLObj) return objSetAttrValue(inf->LLObj, attrname, datatype, val);

	mssError(1,"QYT","Invalid attribute for querytree object");

    return -1;
    }


/*** qytAddAttr - add an attribute to an object.  Passthrough to lowlevel.
 ***/
int
qytAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Attempt to pass this through... **/
	if (inf->LLObj) return objAddAttr(inf->LLObj, attrname, type, val);

    return -1;
    }


/*** qytOpenAttr - open an attribute as an object.  Passthrough.
 ***/
void*
qytOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Attempt to pass this through... **/
	if (inf->LLObj) return objOpenAttr(inf->LLObj, attrname, mode);

    return NULL;
    }


/*** qytGetFirstMethod -- passthrough.
 ***/
char*
qytGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Attempt to pass this through... **/
	if (inf->LLObj) return objGetFirstMethod(inf->LLObj);

    return NULL;
    }


/*** qytGetNextMethod -- passthrough.
 ***/
char*
qytGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Attempt to pass this through... **/
	if (inf->LLObj) return objGetNextMethod(inf->LLObj);

    return NULL;
    }


/*** qytExecuteMethod - passthrough.
 ***/
int
qytExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    pQytData inf = QYT(inf_v);

	/** Attempt to pass this through... **/
	if (inf->LLObj) return objExecuteMethod(inf->LLObj, methodname, param);

    return -1;
    }

/*** qytInfo - Return the capabilities of the object.
 ***/
int
qytInfo(void* inf_v, pObjectInfo info)
    {
    pQytData inf = QYT(inf_v);
    pObjectInfo ll_info;
    pStructInf find_inf;
    int n_groups = 0;
    int is_recurse = 0;
    int is_leaf = 0;
    int i;
    pExpression exp;
    pParamObjects objlist;
    char* expstr;

    if (inf->LLObj) 
	{
	/** Get basic character of object from underlying object **/
	ll_info = objInfo(inf->LLObj);
	if (ll_info)
	    memcpy(info, ll_info, sizeof(ObjectInfo));

	/** Now add what we know, structurally, from the querytree **/
	for(i=0;i<inf->NodeData->nSubInf;i++)
	    {
	    find_inf = inf->NodeData->SubInf[i];
	    if (stStructType(find_inf) == ST_T_SUBGROUP)
		n_groups++;
	    else if (!strcmp(find_inf->Name,"recurse"))
		is_recurse = 1;
	    else if (!strcmp(find_inf->Name,"known_leaf"))
		{
		objlist = expCreateParamList();
		if (objlist)
		    {
		    expstr = NULL;
		    stAttrValue(find_inf, NULL, &expstr, 0);
		    if (expstr)
			{
			exp = (pExpression)expCompileExpression(expstr, objlist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
			if (exp)
			    {
			    expAddParamToList(objlist,"",inf->LLObj,EXPR_O_CURRENT);
			    if (expEvalTree(exp, objlist) >= 0 && exp->Integer != 0 && !(exp->Flags & EXPR_F_NULL))
				is_leaf = 1;
			    expFreeExpression(exp);
			    }
			}
		    expFreeParamList(objlist);
		    }
		}
	    }
	if ((is_recurse || n_groups > 0) && !is_leaf)
	    {
	    info->Flags &= ~( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ );
	    info->Flags |= ( OBJ_INFO_F_CAN_HAVE_SUBOBJ );
	    }

	if (inf->Flags & QYT_F_FLEAF)
	    info->Flags |= OBJ_INFO_F_FORCED_LEAF;

	return 0;
	}
    return -1;
    }


/*** qytPresentationHints - return the hints associated with the
 *** underlying object.
 ***/
pObjPresentationHints
qytPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQytData inf = QYT(inf_v);
    pObjPresentationHints hints;

	if (inf->LLObj)
	    hints = objPresentationHints(inf->LLObj, attrname);
	else
	    hints = NULL;

    return hints;
    }

/*** qytInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
qytInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&QYT_INF,0,sizeof(QYT_INF));
	xhInit(&QYT_INF.ObjAttrCache,255,0);
	xaInit(&QYT_INF.ObjAttrCacheList,QYT_MAX_CACHE);

	/** Setup the structure **/
	strcpy(drv->Name,"QYT - QueryTree Translation Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/querytree");

	/** Setup the function references. **/
	drv->Open = qytOpen;
	drv->Close = qytClose;
	drv->Create = qytCreate;
	drv->Delete = qytDelete;
	drv->DeleteObj = qytDeleteObj;
	drv->OpenQuery = qytOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = qytQueryFetch;
	drv->QueryClose = qytQueryClose;
	drv->Read = qytRead;
	drv->Write = qytWrite;
	drv->GetAttrType = qytGetAttrType;
	drv->GetAttrValue = qytGetAttrValue;
	drv->GetFirstAttr = qytGetFirstAttr;
	drv->GetNextAttr = qytGetNextAttr;
	drv->SetAttrValue = qytSetAttrValue;
	drv->AddAttr = qytAddAttr;
	drv->OpenAttr = qytOpenAttr;
	drv->GetFirstMethod = qytGetFirstMethod;
	drv->GetNextMethod = qytGetNextMethod;
	drv->ExecuteMethod = qytExecuteMethod;
	drv->Info = qytInfo;
	drv->PresentationHints = qytPresentationHints;

	/** Register some structures **/
	nmRegister(sizeof(QytData),"QytData");
	nmRegister(sizeof(QytQuery),"QytQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

