/**#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h"
/** module definintions *
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
**/
#include "centrallix.h"
#include <sys/types.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module: 	Email Objectsystem driver				*/
/* Authors:	Hazen Johnson, Justin Southworth			*/
/* Creation:	May 29, 2014						*/
/* Description:	Provides an email interface for Centrallix through the	*/
/*		ObjectSystem.						*/
/*									*/
/*		Current Shortcomings:					*/
/*		  - All functionality is perfect... There is no		*/
/*		  functionality.*/
/*									*/
/************************************************************************/

/*** emlOpen - open an object.
 ***/
void*
emlOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
	return NULL;
    }


/*** emlClose - close an open object.
 ***/
int
emlClose(void* inf_v, pObjTrxTree* oxt)
    {
    return 0;
    }


/*** emlCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
emlCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** emlDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
emlDelete(pObject obj, pObjTrxTree* oxt)
    {
    /** Unimplemented **/
    return -1;
    }


/*** emlRead - Read from the JSON element
 ***/
int
emlRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    error:
	return -1;
    }


/*** emlWrite - Write to the JSON element
 ***/
int
emlWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** emlOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
emlOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
        return NULL;
    }


/*** emlQueryFetch - get the next directory entry as an open object.
 ***/
void*
emlQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
   return NULL;
    }


/*** emlQueryClose - close the query.
 ***/
int
emlQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return 0;
    }


/*** emlGetAttrType - get the type (DATA_T_json) of an attribute by name.
 ***/
int
emlGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** emlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
emlGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    return 1; /* null if not there presently */
    }


/*** emlGetNextAttr - get the next attribute name for this object.
 ***/
char*
emlGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
        return NULL;
    }


/*** emlGetFirstAttr - get the first attribute name for this object.
 ***/
char*
emlGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
        return ptr;
    }


/*** emlSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
emlSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** emlAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
emlAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** emlOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
emlOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** emlGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
emlGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** emlGetNextMethod -- same as above.  Always fails. 
 ***/
char*
emlGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** emlExecuteMethod - No methods to execute, so this fails.
 ***/
int
emlExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** emlInfo - Return the capabilities of the object
 ***/
int
emlInfo(void* inf_v, pObjectInfo info)
    {
    return 0;
    }


/*** emlInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
emlInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/

	/** Setup the structure **/
	strcpy(drv->Name,"E-mail - Electronic Mail OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	xaAddItem(&(drv->RootContentTypes),"message/rfc822");

	/** Setup the function references. **/
	drv->Open = emlOpen;
	drv->Close = emlClose;
	drv->Create = emlCreate;
	drv->Delete = emlDelete;
	drv->OpenQuery = emlOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = emlQueryFetch;
	drv->QueryClose = emlQueryClose;
	drv->Read = emlRead;
	drv->Write = emlWrite;
	drv->GetAttrType = emlGetAttrType;
	drv->GetAttrValue = emlGetAttrValue;
	drv->GetFirstAttr = emlGetFirstAttr;
	drv->GetNextAttr = emlGetNextAttr;
	drv->SetAttrValue = emlSetAttrValue;
	drv->AddAttr = emlAddAttr;
	drv->OpenAttr = emlOpenAttr;
	drv->GetFirstMethod = emlGetFirstMethod;
	drv->GetNextMethod = emlGetNextMethod;
	drv->ExecuteMethod = emlExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = emlInfo;

	/** nmRegister(sizeof(JsonData),"JsonData");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;


    return 0;
    }

