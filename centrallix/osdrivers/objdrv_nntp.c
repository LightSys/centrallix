#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
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
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_nntp.c              				*/
/* Author:	Nathan Ehresman        					*/
/* Creation:	Nov. 7, 1999           					*/
/* Description:	This object system driver allows access to an NNTP  	*/
/*		server.                                              	*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_nntp.c,v 1.2 2001/09/27 19:26:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_nntp.c,v $

    $Log: objdrv_nntp.c,v $
    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:03  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:05  gbeeley
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
    }
    NtpData, *pNtpData;

#define NTP_SERVER	0
#define NTP_GROUP	1
#define NTP_MESSAGE	2

#define NNTP_PORT	119
#define NNTP_SERVER	"10.4.1.1"

#define NTP(x) ((pNtpData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pNtpData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    NtpQuery, *pNtpQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    NTP_INF;


/*** ntpOpen - open an object.
 ***/
    {
    pNtpData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
    int nntpSocketFD;
    struct sockaddr_in nntpAddr;

	/** Allocate the structure **/
	inf = (pNtpData)nmMalloc(sizeof(NtpData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(NtpData));
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
		nmFree(inf,sizeof(NtpData));
		mssError(0,"NTP","Could not create new node object");
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
	    nmFree(inf,sizeof(NtpData));
	    mssError(0,"NTP","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	if (obj->SubPtr == obj->Pathname->nElements) {
	    inf->Type = NTP_SERVER;
	    nntpSocketFD = socket(AF_INET, SOCK_STREAM, 0);

	    /* set up the struct for the address to connect to */
	    nntpAddr.sin_family = AF_INET;
	    nntpAddr.sin_port = htons(NNTP_PORT);
	    nntpAddr.sin_addr.s_addr = inet_addr(NNTP_SERVER);
	    bzero(&(nntpAddr.sin_zero), 8);
	    
	    /* lets bind the socket and address together */
	    if (connect(nntpSocketFD, (struct sockaddr *)&nntpAddr, sizeof(struct sockaddr)) == -1) {
	        /* error out */
	    }

	    
	} else if (obj->SubPtr == obj->Pathname->nElements - 1) {
	    inf->Type = NTP_GROUP;
	} else if (obj->SubPtr == obj->Pathname->nElements - 2) {
	    inf->Type = NTP_MESSAGE;
	}
	

    return (void*)inf;
    }


/*** ntpClose - close an open object.
 ***/
int
ntpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(NtpData));

    return 0;
    }


/*** ntpCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
ntpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = ntpOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	ntpClose(inf, oxt);

    return 0;
    }


/*** ntpDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
ntpDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pNtpData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pNtpData)ntpOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		ntpClose(inf, oxt);
		mssError(1,"NTP","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		ntpClose(inf, oxt);
		mssError(1,"NTP","Cannot delete: object not empty");
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
	nmFree(inf,sizeof(NtpData));

    return 0;
    }


/*** ntpRead - Structure elements have no content.  Fails.
 ***/
int
ntpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);
    return -1;
    }


/*** ntpWrite - Again, no content.  This fails.
 ***/
int
ntpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);
    return -1;
    }


/*** ntpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
ntpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);
    pNtpQuery qy;

	/** Allocate the query structure **/
	qy = (pNtpQuery)nmMalloc(sizeof(NtpQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(NtpQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
    
    return (void*)qy;
    }


/*** ntpQueryFetch - get the next directory entry as an open object.
 ***/
void*
ntpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pNtpQuery qy = ((pNtpQuery)(qy_v));
    pNtpData inf;
    char* new_obj_name = "newobj";

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	return NULL;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"NTP","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);

	/** Alloc the structure **/
	inf = (pNtpData)nmMalloc(sizeof(NtpData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** ntpQueryClose - close the query.
 ***/
int
ntpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(NtpQuery));

    return 0;
    }


/*** ntpGetAttrType - get the type (DATA_T_ntp) of an attribute by name.
 ***/
int
ntpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);
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


/*** ntpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
ntpGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pNtpData inf = NTP(inf_v);
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

	mssError(1,"NTP","Could not locate requested attribute");

    return -1;
    }


/*** ntpGetNextAttr - get the next attribute name for this object.
 ***/
char*
ntpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pNtpData inf = NTP(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** ntpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
ntpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pNtpData inf = NTP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = ntpGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** ntpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
ntpSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pNtpData inf = NTP(inf_v);
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
		    mssError(1,"NTP","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"NTP","SetAttr 'name': could not rename structure file node object");
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


/*** ntpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
ntpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pNtpData inf = NTP(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** ntpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
ntpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** ntpGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
ntpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** ntpGetNextMethod -- same as above.  Always fails. 
 ***/
char*
ntpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** ntpExecuteMethod - No methods to execute, so this fails.
 ***/
int
ntpExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** ntpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
ntpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&NTP_INF,0,sizeof(NTP_INF));
	NTP_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"NTP - <description here>");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"<obj-type-here>");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = ntpOpen;
	drv->Close = ntpClose;
	drv->Create = ntpCreate;
	drv->Delete = ntpDelete;
	drv->OpenQuery = ntpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = ntpQueryFetch;
	drv->QueryClose = ntpQueryClose;
	drv->Read = ntpRead;
	drv->Write = ntpWrite;
	drv->GetAttrType = ntpGetAttrType;
	drv->GetAttrValue = ntpGetAttrValue;
	drv->GetFirstAttr = ntpGetFirstAttr;
	drv->GetNextAttr = ntpGetNextAttr;
	drv->SetAttrValue = ntpSetAttrValue;
	drv->AddAttr = ntpAddAttr;
	drv->OpenAttr = ntpOpenAttr;
	drv->GetFirstMethod = ntpGetFirstMethod;
	drv->GetNextMethod = ntpGetNextMethod;
	drv->ExecuteMethod = ntpExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(NtpData),"NtpData");
	nmRegister(sizeof(NtpQuery),"NtpQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

