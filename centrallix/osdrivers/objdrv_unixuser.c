#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
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
/*		Replace all occurrenced of uxu, UXU, and Uxu with your	*/
/*		driver's prefix in the same capitalization.  In vi,	*/
/*									*/
/*			:1,$s/Uxu/Pop/g					*/
/*			:1,$s/UXU/POP/g					*/
/*			:1,$s/uxu/pop/g					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_unixuser.c,v 1.5 2004/06/23 21:33:56 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_unixuser.c,v $

    $Log: objdrv_unixuser.c,v $
    Revision 1.5  2004/06/23 21:33:56  mmcgill
    Implemented the ObjInfo interface for all the drivers that are currently
    a part of the project (in the Makefile, in other words). Authors of the
    various drivers might want to check to be sure that I didn't botch any-
    thing, and where applicable see if there's a neat way to keep track of
    whether or not an object actually has subobjects (I did not set this flag
    unless it was immediately obvious how to test for the condition).

    Revision 1.4  2004/06/12 00:10:15  mmcgill
    Chalk one up under 'didn't understand the build process'. The remaining
    os drivers have been updated, and the prototype for objExecuteMethod
    in obj.h has been changed to match the changes made everywhere it's
    called - param is now of type pObjData, not void*.

    Revision 1.3  2002/08/10 02:09:45  gbeeley
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

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:11  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:10  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure for a password file entry. **/
typedef struct
    {
    char	Name[16];
    char	Passwd[20];
    int		Uid;
    int		Gid;
    char	Description[128];
    char	Dir[128];
    char	Shell[128];
    }
    UxuPwEnt, *pUxuPwEnt;


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
    int		Type;
    pUxuPwEnt	PasswdData;
    }
    UxuData, *pUxuData;

#define UXU_T_USERLIST	0
#define UXU_T_USER	1


#define UXU(x) ((pUxuData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pUxuData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    XArray	PasswdEntries;
    }
    UxuQuery, *pUxuQuery;


/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    UXU_INF;


/*** List of standard user attributes ***/
char* uxu_attrs[] =
    {
    "username",
    "password",
    "uid",
    "description",
    "shell",
    "gid",
    "directory",
    NULL
    };


/*** Types of those attributes ***/
int uxu_attr_types[] =
    {
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_INTEGER,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_INTEGER,
    DATA_T_STRING,
    -1
    };


/*** uxu_internal_CopyPwEnt - copies from a getpwent style struct passwd
 *** to our structure and returns a new one.
 ***/
pUxuPwEnt
uxu_internal_CopyPwEnt(struct passwd* pwent)
    {
    pUxuPwEnt this;

    	/** Allocate the structure **/
	this = (pUxuPwEnt)nmMalloc(sizeof(UxuPwEnt));
	if (!this) return NULL;

	/** Fill it in **/
	memccpy(this->Name, pwent->pw_name, '\0', 15);
	this->Name[15]=0;
	memccpy(this->Passwd, pwent->pw_passwd, '\0', 19);
	this->Passwd[19] = 0;
	this->Uid = pwent->pw_uid;
	this->Gid = pwent->pw_gid;
	memccpy(this->Description, pwent->pw_gecos, '\0', 127);
	this->Description[127] = 0;
	memccpy(this->Dir, pwent->pw_dir, '\0', 127);
	this->Dir[127] = 0;
	memccpy(this->Shell, pwent->pw_shell, '\0', 127);
	this->Shell[127] = 0;

    return this;
    }


/*** uxuOpen - open an object.
 ***/
void*
uxuOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pUxuData inf;
    char* node_path;
    pSnNode node = NULL;
    char* name;
    struct passwd* pwent;

	/** Allocate the structure **/
	inf = (pUxuData)nmMalloc(sizeof(UxuData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(UxuData));
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
		nmFree(inf,sizeof(UxuData));
		mssError(0,"UXU","Could not create new node object");
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
	    nmFree(inf,sizeof(UxuData));
	    mssError(0,"UXU","Could not open node object");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** Determine type of object **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    inf->Type = UXU_T_USERLIST;
	    obj->SubCnt = 1;
	    }
	else if (obj->SubPtr == obj->Pathname->nElements - 1)
	    {
	    inf->Type = UXU_T_USER;
	    obj->SubCnt = 2;

	    /** Load info for this user. **/
	    name = obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1);
	    pwent = getpwnam(name);
	    if (!pwent)
	        {
		mssError(1,"UXU","User '%s' does not exist", name);
		nmFree(inf, sizeof(UxuData));
		return NULL;
		}
	    inf->PasswdData = uxu_internal_CopyPwEnt(pwent);
	    if (!inf->PasswdData)
	        {
		nmFree(inf, sizeof(UxuData));
		return NULL;
		}
	    obj_internal_PathPart(obj->Pathname, 0,0);
	    }
	else
	    {
	    inf->Node->OpenCnt--;
	    nmFree(inf,sizeof(UxuData));
	    return NULL;
	    }

    return (void*)inf;
    }


/*** uxuClose - close an open object.
 ***/
int
uxuClose(void* inf_v, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	/** Release the user information **/
	if (inf->PasswdData) nmFree(inf->PasswdData, sizeof(UxuPwEnt));
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(UxuData));

    return 0;
    }


/*** uxuCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
uxuCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = uxuOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	uxuClose(inf, oxt);

    return 0;
    }


/*** uxuDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
uxuDelete(pObject obj, pObjTrxTree* oxt)
    {
    pUxuData inf;
    int is_empty = 1;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pUxuData)uxuOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		uxuClose(inf, oxt);
		mssError(1,"UXU","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 1;
	    if (!is_empty)
	        {
		uxuClose(inf, oxt);
		mssError(1,"UXU","Cannot delete: object not empty");
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
	    nmFree(inf,sizeof(UxuData));
	    return -1;
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(UxuData));

    return 0;
    }


/*** uxuRead - Structure elements have no content.  Fails.
 ***/
int
uxuRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pUxuData inf = UXU(inf_v);*/
    mssError(1,"UXU","Users or user-lists do not have content.");
    return -1;
    }


/*** uxuWrite - Again, no content.  This fails.
 ***/
int
uxuWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pUxuData inf = UXU(inf_v);*/
    mssError(1,"UXU","Users or user-lists do not have content.");
    return -1;
    }


/*** uxuOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
uxuOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);
    pUxuQuery qy;
    struct passwd *pwent;
    pUxuPwEnt pwd_data;

    	/** This DOES NOT work on an individual user, just the list **/
	if (inf->Obj->SubPtr != inf->Obj->Pathname->nElements)
	    {
	    mssError(1,"UXU","User objects have no subobjects");
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pUxuQuery)nmMalloc(sizeof(UxuQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(UxuQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	xaInit(&(qy->PasswdEntries), 32);

	/** Load the password table **/
	setpwent();
	while((pwent = getpwent()))
	    {
	    pwd_data = uxu_internal_CopyPwEnt(pwent);
	    if (!pwd_data) break;
	    xaAddItem(&(qy->PasswdEntries), (void*)pwd_data);
	    }
	setpwent();
    
    return (void*)qy;
    }


/*** uxuQueryFetch - get the next directory entry as an open object.
 ***/
void*
uxuQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pUxuQuery qy = ((pUxuQuery)(qy_v));
    pUxuData inf;
    char* new_obj_name = "newobj";
    pUxuPwEnt orig_ent;
    char* ptr;

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	if (qy->ItemCnt >= qy->PasswdEntries.nItems) return NULL;
	orig_ent = (pUxuPwEnt)(qy->PasswdEntries.Items[qy->ItemCnt]);
	new_obj_name = orig_ent->Name;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"UXU","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	/*sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);*/
	obj->SubPtr = qy->Data->Obj->SubPtr;
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
        *(ptr++) = '/';
        strcpy(ptr,new_obj_name);
        obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;

	/** Alloc the structure **/
	inf = (pUxuData)nmMalloc(sizeof(UxuData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	inf->Type = UXU_T_USER;
	inf->PasswdData = (pUxuPwEnt)nmMalloc(sizeof(UxuPwEnt));
	memcpy(inf->PasswdData, orig_ent, sizeof(UxuPwEnt));
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** uxuQueryClose - close the query.
 ***/
int
uxuQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    int i;
    pUxuQuery qy = (pUxuQuery)qy_v;
    pUxuPwEnt pw_data;

	/** Free the structure **/
	for(i=0;i<qy->PasswdEntries.nItems;i++)
	    {
	    pw_data = (pUxuPwEnt)(qy->PasswdEntries.Items[i]);
	    nmFree(pw_data, sizeof(UxuPwEnt));
	    }
	xaDeInit(&(qy->PasswdEntries));
	nmFree(qy_v,sizeof(UxuQuery));

    return 0;
    }


/*** uxuGetAttrType - get the type (DATA_T_uxu) of an attribute by name.
 ***/
int
uxuGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);
    int i;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    return -1;
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/
	for(i=0;uxu_attrs[i];i++)
	    {
	    if (!strcmp(uxu_attrs[i],attrname)) return uxu_attr_types[i];
	    }

    return -1;
    }


/*** uxuGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
uxuGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);
    int t;

	/** Check the type **/
	t = uxuGetAttrType(inf_v, attrname, oxt);
	if (datatype != t)
	    {
	    mssError(1,"UXU","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]",
		    attrname, obj_type_names[datatype], obj_type_names[t]);
	    return -1;
	    }

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
	    if (inf->Type == UXU_T_USERLIST) val->String = "system/void";
	    else if (inf->Type == UXU_T_USER) val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (inf->Type == UXU_T_USERLIST) val->String = "system/uxuserlist";
	    else if (inf->Type == UXU_T_USER) val->String = "system/user";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    return -1;
	    }
	else if (!strcmp(attrname,"username"))
	    {
	    val->String = inf->PasswdData->Name;
	    return 0;
	    }
	else if (!strcmp(attrname,"password"))
	    {
	    val->String = inf->PasswdData->Passwd;
	    return 0;
	    }
	else if (!strcmp(attrname,"description"))
	    {
	    val->String = inf->PasswdData->Description;
	    return 0;
	    }
	else if (!strcmp(attrname,"shell"))
	    {
	    val->String = inf->PasswdData->Shell;
	    return 0;
	    }
	else if (!strcmp(attrname,"directory"))
	    {
	    val->String = inf->PasswdData->Dir;
	    return 0;
	    }
	else if (!strcmp(attrname,"uid"))
	    {
	    val->Integer = inf->PasswdData->Uid;
	    return 0;
	    }
	else if (!strcmp(attrname,"gid"))
	    {
	    val->Integer = inf->PasswdData->Gid;
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = inf->PasswdData->Description;
	    return 0;
	    }

	mssError(1,"UXU","Could not locate requested attribute");

    return -1;
    }


/*** uxuGetNextAttr - get the next attribute name for this object.
 ***/
char*
uxuGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (inf->CurAttr < 7)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    return uxu_attrs[inf->CurAttr++];
	    }

    return NULL;
    }


/*** uxuGetFirstAttr - get the first attribute name for this object.
 ***/
char*
uxuGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = uxuGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** uxuSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
uxuSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pUxuData inf = UXU(inf_v);
    int t;

	/** Check the type **/
	t = uxuGetAttrType(inf_v, attrname, oxt);
	if (datatype != t)
	    {
	    mssError(1,"UXU","Type mismatch accessing attribute '%s' [requested=%s, actual=%s]",
		    attrname, obj_type_names[datatype], obj_type_names[t]);
	    return -1;
	    }

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
		    mssError(1,"UXU","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"UXU","SetAttr 'name': could not rename structure file node object");
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
	return -1;

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** uxuAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
uxuAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    /*pUxuData inf = UXU(inf_v);*/

    return -1;
    }


/*** uxuOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
uxuOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxuGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
uxuGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxuGetNextMethod -- same as above.  Always fails.  
***/
char*
uxuGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxuExecuteMethod - No methods to execute, so this fails.
 ***/
int
uxuExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** uxuInfo - Return the capabilities of the object
 ***/
int
uxuInfo(void* inf_v, pObjectInfo info)
    {
    pUxuData inf = UXU(inf_v);

	info->Flags |= ( OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
	    OBJ_INFO_F_NO_CONTENT );
	if (inf->Type == UXU_T_USERLIST) info->Flags |= OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	else info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ );
	return 0;
    }


/*** uxuInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
uxuInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&UXU_INF,0,sizeof(UXU_INF));
	UXU_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"UXU - UNIX Users Driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/uxuserlist");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = uxuOpen;
	drv->Close = uxuClose;
	drv->Create = uxuCreate;
	drv->Delete = uxuDelete;
	drv->OpenQuery = uxuOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = uxuQueryFetch;
	drv->QueryClose = uxuQueryClose;
	drv->Read = uxuRead;
	drv->Write = uxuWrite;
	drv->GetAttrType = uxuGetAttrType;
	drv->GetAttrValue = uxuGetAttrValue;
	drv->GetFirstAttr = uxuGetFirstAttr;
	drv->GetNextAttr = uxuGetNextAttr;
	drv->SetAttrValue = uxuSetAttrValue;
	drv->AddAttr = uxuAddAttr;
	drv->OpenAttr = uxuOpenAttr;
	drv->GetFirstMethod = uxuGetFirstMethod;
	drv->GetNextMethod = uxuGetNextMethod;
	drv->ExecuteMethod = uxuExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = uxuInfo;

	nmRegister(sizeof(UxuData),"UxuData");
	nmRegister(sizeof(UxuQuery),"UxuQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

