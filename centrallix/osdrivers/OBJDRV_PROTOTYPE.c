#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "st_node.h"
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
/* Module: 	<driver filename goes here>				*/
/* Author:	<author name goes here>					*/
/* Creation:	<create data goes here>					*/
/* Description:	<replace this> This file is a prototype objectsystem	*/
/*		driver skeleton.  It is used for 'getting started' on	*/
/*		a new objectsystem driver.				*/
/*									*/
/*		To use, some global search/replace must be done.	*/
/*		Replace all occurrenced of xxx, XXX, and Xxx with your	*/
/*		driver's prefix in the same capitalization.  In vi,	*/
/*									*/
/*			:1,$s/Xxx/Pop/g					*/
/*			:1,$s/XXX/POP/g					*/
/*			:1,$s/xxx/pop/g					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: OBJDRV_PROTOTYPE.c,v 1.1 2001/08/13 18:01:00 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/OBJDRV_PROTOTYPE.c,v $

    $Log: OBJDRV_PROTOTYPE.c,v $
    Revision 1.1  2001/08/13 18:01:00  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:02  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
    }
    XxxData, *pXxxData;


#define XXX(x) ((pXxxData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pXxxData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    XxxQuery, *pXxxQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    XXX_INF;


/*** xxxOpen - open an object.
 ***/
void*
xxxOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pXxxData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;

	/** Allocate the structure **/
	inf = (pXxxData)nmMalloc(sizeof(XxxData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(XxxData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(node_path, usrtype);
	    if (!node)
	        {
		nmFree(inf,sizeof(XxxData));
		mssError(0,"XXX","Could not create new node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(node_path);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(node_path, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    nmFree(inf,sizeof(XxxData));
	    mssError(0,"XXX","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

    return (void*)inf;
    }


/*** xxxClose - close an open object.
 ***/
int
xxxClose(void* inf_v, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(XxxData));

    return 0;
    }


/*** xxxCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
xxxCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = xxxOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	xxxClose(inf, oxt);

    return 0;
    }


/*** xxxDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
xxxDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pXxxData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pXxxData)xxxOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		xxxClose(inf, oxt);
		mssError(1,"XXX","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		xxxClose(inf, oxt);
		mssError(1,"XXX","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(XxxData));

    return 0;
    }


/*** xxxRead - Structure elements have no content.  Fails.
 ***/
int
xxxRead(void* inf_v, char* buffer, int maxcnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);
    return -1;
    }


/*** xxxWrite - Again, no content.  This fails.
 ***/
int
xxxWrite(void* inf_v, char* buffer, int cnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);
    return -1;
    }


/*** xxxOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
xxxOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);
    pXxxQuery qy;

	/** Allocate the query structure **/
	qy = (pXxxQuery)nmMalloc(sizeof(XxxQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(XxxQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
    
    return (void*)qy;
    }


/*** xxxQueryFetch - get the next directory entry as an open object.
 ***/
void*
xxxQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pXxxQuery qy = ((pXxxQuery)(qy_v));
    pXxxData inf;
    char* new_obj_name = "newobj";

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	return NULL;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"XXX","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);

	/** Alloc the structure **/
	inf = (pXxxData)nmMalloc(sizeof(XxxData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** xxxQueryClose - close the query.
 ***/
int
xxxQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(XxxQuery));

    return 0;
    }


/*** xxxGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
xxxGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/

    return -1;
    }


/*** xxxGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
xxxGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pXxxData inf = XXX(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    *((char**)val) = "myobject/type";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	mssError(1,"XXX","Could not locate requested attribute");

    return -1;
    }


/*** xxxGetNextAttr - get the next attribute name for this object.
 ***/
char*
xxxGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pXxxData inf = XXX(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** xxxGetFirstAttr - get the first attribute name for this object.
 ***/
char*
xxxGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pXxxData inf = XXX(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = xxxGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** xxxSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
xxxSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pXxxData inf = XXX(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"XXX","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"XXX","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** xxxAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
xxxAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pXxxData inf = XXX(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** xxxOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
xxxOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xxxGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
xxxGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xxxGetNextMethod -- same as above.  Always fails. 
 ***/
char*
xxxGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xxxExecuteMethod - No methods to execute, so this fails.
 ***/
int
xxxExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** xxxInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
xxxInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&XXX_INF,0,sizeof(XXX_INF));
	XXX_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"XXX - <description here>");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"<obj-type-here>");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = xxxOpen;
	drv->Close = xxxClose;
	drv->Create = xxxCreate;
	drv->Delete = xxxDelete;
	drv->OpenQuery = xxxOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = xxxQueryFetch;
	drv->QueryClose = xxxQueryClose;
	drv->Read = xxxRead;
	drv->Write = xxxWrite;
	drv->GetAttrType = xxxGetAttrType;
	drv->GetAttrValue = xxxGetAttrValue;
	drv->GetFirstAttr = xxxGetFirstAttr;
	drv->GetNextAttr = xxxGetNextAttr;
	drv->SetAttrValue = xxxSetAttrValue;
	drv->AddAttr = xxxAddAttr;
	drv->OpenAttr = xxxOpenAttr;
	drv->GetFirstMethod = xxxGetFirstMethod;
	drv->GetNextMethod = xxxGetNextMethod;
	drv->ExecuteMethod = xxxExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(XxxData),"XxxData");
	nmRegister(sizeof(XxxQuery),"XxxQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

