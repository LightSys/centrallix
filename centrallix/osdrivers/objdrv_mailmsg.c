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
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_mailmsg.c        				*/
/* Author:	Greg Beeley (GRB)      					*/
/* Creation:	January 18th, 2000     					*/
/* Description:	Driver to handle an RFC822 / MIME encoded email message.*/
/*		This driver breaks the message in to appropriate parts	*/
/*		according to the structured encoding.  Headers become	*/
/*		attributes, and parts within a part become subobjects.	*/
/*		This driver provides no discrimination between the 	*/
/*		various types of multipart messages (mixed, alternative,*/
/*		related, etc).  That is up to the application-level.	*/
/*		The TO:, CC:, Received:, etc. attributes become sub-	*/
/*		objects under the "fixed" TO, CC, RECEIVED objects at	*/
/*		the next-to-top level.  The TO, CC, etc can be		*/
/*		differentiated from normal subparts via the non-	*/
/*		iterated integer attribute "is_subpart".		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_mailmsg.c,v 1.1 2001/08/13 18:01:03 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/Attic/objdrv_mailmsg.c,v $

    $Log: objdrv_mailmsg.c,v $
    Revision 1.1  2001/08/13 18:01:03  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:04  gbeeley
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
    int		Type;
    pMsgPwEnt	PasswdData;
    }
    MsgData, *pMsgData;

#define MSG_T_USERLIST	0
#define MSG_T_USER	1


#define MSG(x) ((pMsgData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pMsgData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    XArray	PasswdEntries;
    }
    MsgQuery, *pMsgQuery;


/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    MSG_INF;


/*** List of standard message attributes ***/
char* msg_attrs[] =
    {
    "received_cnt",
    "to_cnt",
    "cc_cnt",
    "newsgroups_cnt",

    "from",
    "sender",
    "reply_to",
    "return_path",

    "date",
    "subject",
    "in_reply_to",
    "x_mailer",

    "message_id",
    "mime_version",

    "content_id",
    "content_length",
    "content_type",
    "content_type_name",
    "content_type_charset",
    "content_type_boundary",
    "content_transfer_encoding",
    "content_description",
    "content_disposition",
    "content_disposition_filename",
    "content_base",
    "content_location",
    NULL
    };


/*** Types of those attributes ***/
int msg_attr_types[] =
    {
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_INTEGER,
    DATA_T_INTEGER,

    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,

    DATA_T_DATETIME,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,

    DATA_T_STRING,
    DATA_T_STRING,

    DATA_T_STRING,
    DATA_T_INTEGER,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    DATA_T_STRING,
    -1
    };


/*** msg_internal_CopyPwEnt - copies from a getpwent style struct passwd
 *** to our structure and returns a new one.
 ***/
pMsgPwEnt
msg_internal_CopyPwEnt(struct passwd* pwent)
    {
    pMsgPwEnt this;

    	/** Allocate the structure **/
	this = (pMsgPwEnt)nmMalloc(sizeof(MsgPwEnt));
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


/*** msgOpen - open an object.
 ***/
void*
msgOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pMsgData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
    char* name;
    struct passwd* pwent;

	/** Allocate the structure **/
	inf = (pMsgData)nmMalloc(sizeof(MsgData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(MsgData));
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
		nmFree(inf,sizeof(MsgData));
		mssError(0,"MSG","Could not create new node object");
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
	    nmFree(inf,sizeof(MsgData));
	    mssError(0,"MSG","Could not open node object");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** Determine type of object **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    inf->Type = MSG_T_USERLIST;
	    obj->SubCnt = 1;
	    }
	else if (obj->SubPtr == obj->Pathname->nElements - 1)
	    {
	    inf->Type = MSG_T_USER;
	    obj->SubCnt = 2;

	    /** Load info for this user. **/
	    name = obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1);
	    pwent = getpwnam(name);
	    if (!pwent)
	        {
		mssError(1,"MSG","User '%s' does not exist", name);
		nmFree(inf, sizeof(MsgData));
		return NULL;
		}
	    inf->PasswdData = msg_internal_CopyPwEnt(pwent);
	    if (!inf->PasswdData)
	        {
		nmFree(inf, sizeof(MsgData));
		return NULL;
		}
	    obj_internal_PathPart(obj->Pathname, 0,0);
	    }
	else
	    {
	    inf->Node->OpenCnt--;
	    nmFree(inf,sizeof(MsgData));
	    return NULL;
	    }

    return (void*)inf;
    }


/*** msgClose - close an open object.
 ***/
int
msgClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	/** Release the user information **/
	if (inf->PasswdData) nmFree(inf->PasswdData, sizeof(MsgPwEnt));
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(MsgData));

    return 0;
    }


/*** msgCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
msgCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = msgOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	msgClose(inf, oxt);

    return 0;
    }


/*** msgDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
msgDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pMsgData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pMsgData)msgOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		msgClose(inf, oxt);
		mssError(1,"MSG","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 1;
	    if (!is_empty)
	        {
		msgClose(inf, oxt);
		mssError(1,"MSG","Cannot delete: object not empty");
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
	    nmFree(inf,sizeof(MsgData));
	    return -1;
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(MsgData));

    return 0;
    }


/*** msgRead - Structure elements have no content.  Fails.
 ***/
int
msgRead(void* inf_v, char* buffer, int maxcnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);
    mssError(1,"MSG","Users or user-lists do not have content.");
    return -1;
    }


/*** msgWrite - Again, no content.  This fails.
 ***/
int
msgWrite(void* inf_v, char* buffer, int cnt, int flags, int offset, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);
    mssError(1,"MSG","Users or user-lists do not have content.");
    return -1;
    }


/*** msgOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
msgOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);
    pMsgQuery qy;
    struct passwd *pwent;
    pMsgPwEnt pwd_data;

    	/** This DOES NOT work on an individual user, just the list **/
	if (inf->Obj->SubPtr != inf->Obj->Pathname->nElements)
	    {
	    mssError(1,"MSG","User objects have no subobjects");
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pMsgQuery)nmMalloc(sizeof(MsgQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(MsgQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	xaInit(&(qy->PasswdEntries), 32);

	/** Load the password table **/
	setpwent();
	while(pwent = getpwent())
	    {
	    pwd_data = msg_internal_CopyPwEnt(pwent);
	    if (!pwd_data) break;
	    xaAddItem(&(qy->PasswdEntries), (void*)pwd_data);
	    }
	setpwent();
    
    return (void*)qy;
    }


/*** msgQueryFetch - get the next directory entry as an open object.
 ***/
void*
msgQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pMsgQuery qy = ((pMsgQuery)(qy_v));
    pMsgData inf;
    char* new_obj_name = "newobj";
    pMsgPwEnt orig_ent, new_ent;
    char* ptr;

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	if (qy->ItemCnt >= qy->PasswdEntries.nItems) return NULL;
	orig_ent = (pMsgPwEnt)(qy->PasswdEntries.Items[qy->ItemCnt]);
	new_obj_name = orig_ent->Name;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"MSG","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	/*sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);*/
	obj->SubPtr = qy->Data->Obj->SubPtr;
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
        *(ptr++) = '/';
        strcpy(ptr,new_obj_name);
        obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;

	/** Alloc the structure **/
	inf = (pMsgData)nmMalloc(sizeof(MsgData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	inf->Type = MSG_T_USER;
	inf->PasswdData = (pMsgPwEnt)nmMalloc(sizeof(MsgPwEnt));
	memcpy(inf->PasswdData, orig_ent, sizeof(MsgPwEnt));
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** msgQueryClose - close the query.
 ***/
int
msgQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    int i;
    pMsgQuery qy = (pMsgQuery)qy_v;
    pMsgPwEnt pw_data;

	/** Free the structure **/
	for(i=0;i<qy->PasswdEntries.nItems;i++)
	    {
	    pw_data = (pMsgPwEnt)(qy->PasswdEntries.Items[i]);
	    nmFree(pw_data, sizeof(MsgPwEnt));
	    }
	xaDeInit(&(qy->PasswdEntries));
	nmFree(qy_v,sizeof(MsgQuery));

    return 0;
    }


/*** msgGetAttrType - get the type (DATA_T_msg) of an attribute by name.
 ***/
int
msgGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);
    int i;
    pStructInf find_inf;

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
	for(i=0;msg_attrs[i];i++)
	    {
	    if (!strcmp(msg_attrs[i],attrname)) return msg_attr_types[i];
	    }

    return -1;
    }


/*** msgGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
msgGetAttrValue(void* inf_v, char* attrname, pObjData val, pObjTrxTree* oxt)
    {
    pMsgData inf = MSG(inf_v);
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
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (inf->Type == MSG_T_USERLIST) *((char**)val) = "system/void";
	    else if (inf->Type == MSG_T_USER) *((char**)val) = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (inf->Type == MSG_T_USERLIST) *((char**)val) = "system/msgserlist";
	    else if (inf->Type == MSG_T_USER) *((char**)val) = "system/user";
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

	mssError(1,"MSG","Could not locate requested attribute");

    return -1;
    }


/*** msgGetNextAttr - get the next attribute name for this object.
 ***/
char*
msgGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMsgData inf = MSG(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (inf->CurAttr < 7)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    return msg_attrs[inf->CurAttr++];
	    }

    return NULL;
    }


/*** msgGetFirstAttr - get the first attribute name for this object.
 ***/
char*
msgGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMsgData inf = MSG(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = msgGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** msgSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
msgSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pMsgData inf = MSG(inf_v);
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
		    mssError(1,"MSG","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"MSG","SetAttr 'name': could not rename structure file node object");
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


/*** msgAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
msgAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pMsgData inf = MSG(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** msgOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
msgOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** msgGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
msgGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** msgGetNextMethod -- same as above.  Always fails. 
 ***/
char*
msgGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** msgExecuteMethod - No methods to execute, so this fails.
 ***/
int
msgExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** msgInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
msgInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&MSG_INF,0,sizeof(MSG_INF));
	MSG_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"MSG - RFC822/MIME e-mail message driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"message/rfc822");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = msgOpen;
	drv->Close = msgClose;
	drv->Create = msgCreate;
	drv->Delete = msgDelete;
	drv->OpenQuery = msgOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = msgQueryFetch;
	drv->QueryClose = msgQueryClose;
	drv->Read = msgRead;
	drv->Write = msgWrite;
	drv->GetAttrType = msgGetAttrType;
	drv->GetAttrValue = msgGetAttrValue;
	drv->GetFirstAttr = msgGetFirstAttr;
	drv->GetNextAttr = msgGetNextAttr;
	drv->SetAttrValue = msgSetAttrValue;
	drv->AddAttr = msgAddAttr;
	drv->OpenAttr = msgOpenAttr;
	drv->GetFirstMethod = msgGetFirstMethod;
	drv->GetNextMethod = msgGetNextMethod;
	drv->ExecuteMethod = msgExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(MsgData),"MsgData");
	nmRegister(sizeof(MsgQuery),"MsgQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

