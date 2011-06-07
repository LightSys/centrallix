#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
/*		Replace all occurrenced of cpp, CPP, and Cpp with your	*/
/*		driver's prefix in the same capitalization.  In vi,	*/
/*									*/
/*			:1,$s/Cpp/Pop/g					*/
/*			:1,$s/CPP/POP/g					*/
/*			:1,$s/cpp/pop/g					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: OBJDRV_PROTOTYPE.c,v 1.7 2005/02/26 06:42:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/OBJDRV_PROTOTYPE.c,v $

    $Log: OBJDRV_PROTOTYPE.c,v $
    Revision 1.7  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.6  2004/12/31 04:21:49  gbeeley
    - bring prototype osdriver source file more up to date

    Revision 1.5  2004/06/11 21:06:57  mmcgill
    Did some code tree scrubbing.

    Changed cppGetAttrValue(), cppSetAttrValue(), cppAddAttr(), and
    cppExecuteMethod() to use pObjData as the type for val (or param in
    the case of cppExecuteMethod) instead of void* for the audio, BerkeleyDB,
    GZip, HTTP, MBox, MIME, and Shell drivers, and found/fixed a 2-byte buffer
    overflow in objdrv_shell.c (line 1046).

    Also, the Berkeley API changed in v4 in a few spots, so objdrv_berk.c is
    broken as of right now.

    It should be noted that I haven't actually built the audio or Berkeley
    drivers, so I *could* have messed up, but they look ok. The others
    compiled, and passed a cursory testing.

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

    Revision 1.3  2002/07/29 01:18:07  jorupp
     * added the include and calls to build as a module

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:00  gbeeley
    Centrallix Core initial import

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
    CppData, *pCppData;


#define CPP(x) ((pCppData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pCppData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    CppQuery, *pCppQuery;

/*** GLOBALS ***/
typedef struct
    {
    int		dmy_global_variable;
    }
    CPP_INF_t;
    
CPP_INF_t CPP_INF;

/*** cppOpen - open an object.
 ***/
void*
cppOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pCppData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;

	/** Allocate the structure **/
	inf = (pCppData)nmMalloc(sizeof(CppData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(CppData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		nmFree(inf,sizeof(CppData));
		mssError(0,"CPP","Could not create new node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
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
	    nmFree(inf,sizeof(CppData));
	    mssError(0,"CPP","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

    return (void*)inf;
    }


/*** cppClose - close an open object.
 ***/
int
cppClose(void* inf_v, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Obj->Prev, inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(CppData));

    return 0;
    }


/*** cppCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
cppCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = cppOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	cppClose(inf, oxt);

    return 0;
    }


/*** cppDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
cppDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pCppData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pCppData)cppOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		cppClose(inf, oxt);
		mssError(1,"CPP","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		cppClose(inf, oxt);
		mssError(1,"CPP","Cannot delete: object not empty");
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
	nmFree(inf,sizeof(CppData));

    return 0;
    }


/*** cppRead - Structure elements have no content.  Fails.
 ***/
int
cppRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);
    return -1;
    }


/*** cppWrite - Again, no content.  This fails.
 ***/
int
cppWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);
    return -1;
    }


/*** cppOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
cppOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);
    pCppQuery qy;

	/** Allocate the query structure **/
	qy = (pCppQuery)nmMalloc(sizeof(CppQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(CppQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
    
    return (void*)qy;
    }


/*** cppQueryFetch - get the next directory entry as an open object.
 ***/
void*
cppQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pCppQuery qy = ((pCppQuery)(qy_v));
    pCppData inf;
    char* new_obj_name = "newobj";

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	return NULL;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"CPP","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);

	/** Alloc the structure **/
	inf = (pCppData)nmMalloc(sizeof(CppData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** cppQueryClose - close the query.
 ***/
int
cppQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(CppQuery));

    return 0;
    }


/*** cppGetAttrType - get the type (DATA_T_cpp) of an attribute by name.
 ***/
int
cppGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
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


/*** cppGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
cppGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pCppData inf = CPP(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    /*val->String = "application/octet-stream";*/
	    val->String = "system/void";
	    return 0;
	    }

	/** Object type. **/
	/** REPLACE WITH SOMETHING MORE COHERENT **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "system/object";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	mssError(1,"CPP","Could not locate requested attribute");

    return -1;
    }


/*** cppGetNextAttr - get the next attribute name for this object.
 ***/
char*
cppGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pCppData inf = CPP(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** cppGetFirstAttr - get the first attribute name for this object.
 ***/
char*
cppGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pCppData inf = CPP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = cppGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** cppSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
cppSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pCppData inf = CPP(inf_v);
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
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"CPP","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"CPP","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }
	if (!strcmp(attrname,"outer_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** cppAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
cppAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pCppData inf = CPP(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** cppOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
cppOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppGetFirstMethod -- return name of First method available on the object.
 ***/
char*
cppGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
cppGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppExecuteMethod - Execute a method, by name.
 ***/
int
cppExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** cppPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
cppPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    /** No hints yet on this **/
    return NULL;
    }


/*** cppInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
cppInfo(void* inf_v, pObjectInfo info_struct)
    {
    memset(info_struct, sizeof(ObjectInfo), 0);
    return 0;
    }


/*** cppCommit - commit any changes made to the underlying data source.
 ***/
int
cppCommit(void* inf_v, pObjTrxTree* oxt)
    {
    /** no uncommitted changes yet **/
    return 0;
    }


/*** cppInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
cppInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&CPP_INF,0,sizeof(CPP_INF));
	CPP_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"CPP - fake module, will be cpp wrapper ");/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),(void*)"cpp");/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = (void* (*)())cppOpen;
	drv->Close = (int (*)())cppClose;
	drv->Create = (int (*)())cppCreate;
	drv->Delete = (int (*)())cppDelete;
	drv->OpenQuery = (void* (*)())cppOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = (void* (*)())cppQueryFetch;
	drv->QueryClose = (int (*)())cppQueryClose;
	drv->Read = (int (*)())cppRead;
	drv->Write = (int (*)())cppWrite;
	drv->GetAttrType = (int (*)())cppGetAttrType;
	drv->GetAttrValue = (int (*)())cppGetAttrValue;
	drv->GetFirstAttr = (char* (*)())cppGetFirstAttr;
	drv->GetNextAttr = (char* (*)())cppGetNextAttr;
	drv->SetAttrValue = (int (*)())cppSetAttrValue;
	drv->AddAttr = (int (*)())cppAddAttr;
	drv->OpenAttr = (void* (*)())cppOpenAttr;
	drv->GetFirstMethod = (char* (*)())cppGetFirstMethod;
	drv->GetNextMethod = (char* (*)())cppGetNextMethod;
	drv->ExecuteMethod = (int (*)())cppExecuteMethod;
	drv->PresentationHints = (_PH* (*)())cppPresentationHints;
	drv->Info = (int (*)())cppInfo;
	drv->Commit = (int (*)())cppCommit;

	nmRegister(sizeof(CppData),"CppData");
	nmRegister(sizeof(CppQuery),"CppQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(cppInitialize);
MODULE_PREFIX("cpp");
MODULE_DESC("CPP ObjectSystem Driver");
MODULE_VERSION(0,0,0);
MODULE_IFACE(CX_CURRENT_IFACE);

