#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
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
/* Module: 	<driver filename goes here>				*/
/* Author:	<author name goes here>					*/
/* Creation:	<create data goes here>					*/
/* Description:	<replace this> This file is a prototype objectsystem	*/
/*		driver skeleton.  It is used for 'getting started' on	*/
/*		a new objectsystem driver.				*/
/*									*/
/*		To use, some global search/replace must be done.	*/
/*		Replace all occurrenced of pop, POP, and Pop with your	*/
/*		driver's prefix in the same capitalization.  In vi,	*/
/*									*/
/*			:1,$s/Pop/Pop/g					*/
/*			:1,$s/POP/POP/g					*/
/*			:1,$s/pop/pop/g					*/
/************************************************************************/



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
    }
    PopData, *pPopData;

#define POP_T_SERVER	0
#define POP_T_USERMBOX	1
#define POP_T_MESSAGE	2


#define POP(x) ((pPopData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pPopData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    PopQuery, *pPopQuery;


/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    POP_INF;


/*** List of standard user attributes ***/
char* pop_attrs[] =
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
int pop_attr_types[] =
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


/*** pop_internal_CopyPwEnt - copies from a getpwent style struct passwd
 *** to our structure and returns a new one.
 ***/
pPopPwEnt
pop_internal_CopyPwEnt(struct passwd* pwent)
    {
    pPopPwEnt this;

    	/** Allocate the structure **/
	this = (pPopPwEnt)nmMalloc(sizeof(PopPwEnt));
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


/*** popOpen - open an object.
 ***/
void*
popOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pPopData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
    char* name;
    struct passwd* pwent;

	/** Allocate the structure **/
	inf = (pPopData)nmMalloc(sizeof(PopData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(PopData));
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
		nmFree(inf,sizeof(PopData));
		mssError(0,"POP","Could not create new node object");
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
	    nmFree(inf,sizeof(PopData));
	    mssError(0,"POP","Could not node object");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** Determine type of object **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    inf->Type = POP_T_USERLIST;
	    }
	else if (obj->SubPtr == obj->Pathname->nElements - 1)
	    {
	    inf->Type = POP_T_USER;

	    /** Load info for this user. **/
	    name = obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1);
	    pwent = getpwnam(name);
	    if (!pwent)
	        {
		mssError(1,"POP","User '%s' does not exist", name);
		nmFree(inf, sizeof(PopData));
		return NULL;
		}
	    inf->PasswdData = pop_internal_CopyPwEnt(pwent);
	    if (!inf->PasswdData)
	        {
		nmFree(inf, sizeof(PopData));
		return NULL;
		}
	    obj_internal_PathPart(obj->Pathname, 0,0);
	    }
	else
	    {
	    inf->Node->OpenCnt--;
	    nmFree(inf,sizeof(PopData));
	    return NULL;
	    }

    return (void*)inf;
    }


/*** popClose - close an open object.
 ***/
int
popClose(void* inf_v, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Node);

	/** Release the user information **/
	if (inf->PasswdData) nmFree(inf->PasswdData, sizeof(PopPwEnt));
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(PopData));

    return 0;
    }


/*** popCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
popCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = popOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	popClose(inf, oxt);

    return 0;
    }


/*** popDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
popDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pPopData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pPopData)popOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		popClose(inf, oxt);
		mssError(1,"POP","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 1;
	    if (!is_empty)
	        {
		popClose(inf, oxt);
		mssError(1,"POP","Cannot delete: object not empty");
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
	    nmFree(inf,sizeof(PopData));
	    return -1;
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(PopData));

    return 0;
    }


/*** popRead - Structure elements have no content.  Fails.
 ***/
int
popRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    return -1;
    }


/*** popWrite - Again, no content.  This fails.
 ***/
int
popWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    return -1;
    }


/*** popOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
popOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    pPopQuery qy;
    struct passwd *pwent;
    pPopPwEnt pwd_data;

    	/** This DOES NOT work on an individual user, just the list **/
	if (inf->Obj->SubPtr != inf->Obj->Pathname->nElements)
	    {
	    mssError(1,"POP","User objects have no subobjects");
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pPopQuery)nmMalloc(sizeof(PopQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(PopQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	xaInit(&(qy->PasswdEntries), 32);

	/** Load the password table **/
	setpwent();
	while(pwent = getpwent())
	    {
	    pwd_data = pop_internal_CopyPwEnt(pwent);
	    if (!pwd_data) break;
	    xaAddItem(&(qy->PasswdEntries), (void*)pwd_data);
	    }
	setpwent();
    
    return (void*)qy;
    }


/*** popQueryFetch - get the next directory entry as an open object.
 ***/
void*
popQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pPopQuery qy = ((pPopQuery)(qy_v));
    pPopData inf;
    char* new_obj_name = "newobj";
    pPopPwEnt orig_ent, new_ent;
    char* ptr;

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	if (qy->ItemCnt >= qy->PasswdEntries.nItems) return NULL;
	orig_ent = (pPopPwEnt)(qy->PasswdEntries.Items[qy->ItemCnt]);
	new_obj_name = orig_ent->Name;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"POP","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	/*sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);*/
	obj->SubPtr = qy->Data->Obj->SubPtr;
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
        *(ptr++) = '/';
        strcpy(ptr,new_obj_name);
        obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;

	/** Alloc the structure **/
	inf = (pPopData)nmMalloc(sizeof(PopData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	inf->Type = POP_T_USER;
	inf->PasswdData = (pPopPwEnt)nmMalloc(sizeof(PopPwEnt));
	memcpy(inf->PasswdData, orig_ent, sizeof(PopPwEnt));
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** popQueryClose - close the query.
 ***/
int
popQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    int i;
    pPopQuery qy = (pPopQuery)qy_v;
    pPopPwEnt pw_data;

	/** Free the structure **/
	for(i=0;i<qy->PasswdEntries.nItems;i++)
	    {
	    pw_data = (pPopPwEnt)(qy->PasswdEntries.Items[i]);
	    nmFree(pw_data, sizeof(PopPwEnt));
	    }
	xaDeInit(&(qy->PasswdEntries));
	nmFree(qy_v,sizeof(PopQuery));

    return 0;
    }


/*** popGetAttrType - get the type (DATA_T_pop) of an attribute by name.
 ***/
int
popGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
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
	    return -1;
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/
	for(i=0;pop_attrs[i];i++)
	    {
	    if (!strcmp(pop_attrs[i],attrname)) return pop_attr_types[i];
	    }

    return -1;
    }


/*** popGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
popGetAttrValue(void* inf_v, char* attrname, pObjData val, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
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
	    if (inf->Type == POP_T_USERLIST) *((char**)val) = "system/popserlist";
	    else if (inf->Type == POP_T_USER) *((char**)val) = "system/user";
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
	    *(char**)val = inf->PasswdData->Description;
	    return 0;
	    }

	mssError(1,"POP","Could not locate requested attribute");

    return -1;
    }


/*** popGetNextAttr - get the next attribute name for this object.
 ***/
char*
popGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (inf->CurAttr < 7)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    return pop_attrs[inf->CurAttr++];
	    }

    return NULL;
    }


/*** popGetFirstAttr - get the first attribute name for this object.
 ***/
char*
popGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = popGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** popSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
popSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
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
		    mssError(1,"POP","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"POP","SetAttr 'name': could not rename structure file node object");
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


/*** popAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
popAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** popOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
popOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
popGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popGetNextMethod -- same as above.  Always fails. 
 ***/
char*
popGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popExecuteMethod - No methods to execute, so this fails.
 ***/
int
popExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** popInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
popInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&POP_INF,0,sizeof(POP_INF));
	POP_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"POP - POP-3 Mailbox driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"network/pop3");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = popOpen;
	drv->Close = popClose;
	drv->Create = popCreate;
	drv->Delete = popDelete;
	drv->OpenQuery = popOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = popQueryFetch;
	drv->QueryClose = popQueryClose;
	drv->Read = popRead;
	drv->Write = popWrite;
	drv->GetAttrType = popGetAttrType;
	drv->GetAttrValue = popGetAttrValue;
	drv->GetFirstAttr = popGetFirstAttr;
	drv->GetNextAttr = popGetNextAttr;
	drv->SetAttrValue = popSetAttrValue;
	drv->AddAttr = popAddAttr;
	drv->OpenAttr = popOpenAttr;
	drv->GetFirstMethod = popGetFirstMethod;
	drv->GetNextMethod = popGetNextMethod;
	drv->ExecuteMethod = popExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(PopData),"PopData");
	nmRegister(sizeof(PopQuery),"PopQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

