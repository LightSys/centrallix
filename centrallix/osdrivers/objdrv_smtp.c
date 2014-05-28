/**
#include <unistd.h>
#include <fcntl.h>
#include "cxlib/mtask.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h" **/
/** module definintions **/
/**#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
**/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "obj.h"
#include "st_node.h"
#include "cxlib/xarray.h"
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

/** Define types of SMTP objects. **/
#define SMTP_T_ROOT	0
#define SMTP_T_EML	1

/*** Structure to store attribute information. ***/
typedef struct
    {
    char*	Name;
    int		Type; /* DATA_T_xxx */
    ObjData	Value;
    }
    SmtpAttribute, *pSmtpAttribute;

#define SMTP_ATTR(x) ((pSmtpAttribute)(x))

/*** Structure used by this driver internally. ***/
typedef struct
    {
    int			Type;
    char*		Name;
    pObject		Obj;
    int			Mask;
    pSnNode		Node;
    pXHashTable		Attributes; /* Hash of attribute name to SmtpAttribute. */
    }
    SmtpData, *pSmtpData;

#define SMTP(x) ((pSmtpData)(x))

/*** Structure used by queries in this driver. ***/
typedef struct
    {
    pSmtpData	Data;
    int		ItemCnt;
    }
    SmtpQueryData, *pSmtpQueryData;

/*** smtp_internal_ClearAttributes - Clears all the elements of the attributes
 *** hash table.
 ***/
int
smtp_internal_ClearAttribute(char* inf_c, void* customParams)
    {
    pSmtpAttribute inf = SMTP_ATTR(inf_c);

	nmFree(inf, sizeof(SmtpAttribute));
	
    return 0;
    }

/*** smtpOpen - open an object.
 ***/
void*
smtpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;
    pSnNode node = NULL;
    pSmtpAttribute attr = NULL;
    int i;
    pStructInf currentAttr = NULL;

	inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(SmtpData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
		{
		mssError(0,"SMTP", "Could not create new node object");
		goto error;
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
	    mssError(0,"SMTP","Could not open structure file");
	    goto error;
	    }
	
	/** Store the node object. **/
	inf->Node = node;
	inf->Node->OpenCnt++;

	inf->Name = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-1, 0);
	inf->Name = nmSysStrdup(inf->Name);

	inf->Attributes = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (!inf->Attributes)
	    {
	    mssError(1,"SMTP","Could not create attributes hash table.");
	    goto error;
	    }
	memset(inf->Attributes, 0, sizeof(XHashTable));
	xhInit(inf->Attributes, 16, 0);

	/** Determine the type of the object. **/
	if (inf->Obj->SubPtr == inf->Obj->Pathname->nElements)
	    {
	    inf->Type = SMTP_T_ROOT;

	    for (i = 0; i < inf->Node->Data->nSubInf; i++)
		{
		currentAttr = inf->Node->Data->SubInf[i];

		attr = nmMalloc(sizeof(SmtpAttribute));
		if (!attr)
		    {
		    mssError(1,"SMTP","Could not create new attribute object.");
		    goto error;
		    }
		attr->Name = currentAttr->Name;
		attr->Type = currentAttr->Value->DataType;

		if (currentAttr->Value->DataType == DATA_T_STRING)
		    {
		    if (stAttrValue(currentAttr, NULL, &attr->Value.String, 0) < 0)
			{
			attr->Value.String = NULL;
			}
		    }
		if (currentAttr->Value->DataType == DATA_T_INTEGER)
		    {
		    if (stAttrValue(currentAttr, &attr->Value.Integer, NULL, 0) < 0)
			{
			attr->Value.Integer = 0;
			}
		    }
		xhAdd(inf->Attributes, currentAttr->Name, (char*)attr);
		}
	    }
	else if (!strcmp(inf->Name+strlen(inf->Name)-4,".eml"))
	    {
	    inf->Type = SMTP_T_EML;
	    }
	else
	    {
	    mssError(1,"SMTP","Could not open file");
	    goto error;
	    }

	return inf;

    error:
	if (inf)
	    {
	    if (inf->Attributes)
		{
		xhClear(inf->Attributes, smtp_internal_ClearAttribute, NULL);
		xhDeInit(inf->Attributes);
		}
	    nmFree(inf, sizeof(SmtpData));
	    }

	return NULL;
    }


/*** smtpClose - close an open object.
 ***/
int
smtpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);

    if (inf->Attributes)
	{
	xhClear(inf->Attributes, smtp_internal_ClearAttribute, NULL);
	xhDeInit(inf->Attributes);
	}

    nmFree(inf, sizeof(SmtpData));
    return 0;
    }


/*** smtpCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
smtpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** smtpDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
smtpDelete(pObject obj, pObjTrxTree* oxt)
    {
    /** Unimplemented **/
    return -1;
    }


/*** smtpRead - Read from the JSON element
 ***/
int
smtpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    error:
	return -1;
    }


/*** smtpWrite - Write to the JSON element
 ***/
int
smtpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** smtpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
smtpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
        return NULL;
    }


/*** smtpQueryFetch - get the next directory entry as an open object.
 ***/
void*
smtpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
   return NULL;
    }


/*** smtpQueryClose - close the query.
 ***/
int
smtpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return 0;
    }


/*** smtpGetAttrType - get the type (DATA_T_json) of an attribute by name.
 ***/
int
smtpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;

	/** Default values all happen to be strings. **/
	if (!strcmp(attrname, "name")) return DATA_T_STRING;
	if (!strcmp(attrname, "content_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname, "annotation")) return DATA_T_STRING;
	
	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (attr)
	    {
	    return attr->Type;
	    }

    return -1;
    }


/*** smtpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
smtpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;

	if (!strcmp(attrname, "name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		    mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be a string)", attrname);
		    return -1;
		}
	    val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-1, 0);
	    return 0;
	    }

	/** inner_type is an alias for content_type **/
	if (!strcmp(attrname,"inner_type") || !strcmp(attrname, "content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "message/rfc822";
	    return 0;
	    }

	/** If outer type, and it wasn't specified in the JSON **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "network/smtp";
	    return 0;
	    }
	
	/** Get the type of the stored attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (attr)
	    {
	    if (datatype != attr->Type)
		{
		mssError(1,"SMTP","Type mismatch getting attribute '%s' (should be %s)", attrname, obj_type_names[attr->Type]);
		return -1;
		}
	    val = &attr->Value;
	    return 0;
	    }

    return 1; /* null if not there presently */
    }


/*** smtpGetNextAttr - get the next attribute name for this object.
 ***/
char*
smtpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
        return NULL;
    }


/*** smtpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
smtpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
        return NULL;
    }


/*** smtpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
smtpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** smtpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
smtpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** smtpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
smtpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
smtpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpGetNextMethod -- same as above.  Always fails. 
 ***/
char*
smtpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** smtpExecuteMethod - No methods to execute, so this fails.
 ***/
int
smtpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** smtpInfo - Return the capabilities of the object
 ***/
int
smtpInfo(void* inf_v, pObjectInfo info)
    {
    return 0;
    }


/*** smtpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
smtpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/

	/** Setup the structure **/
	strcpy(drv->Name,"SMTP - Simple Mail Transfer Protocol OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	/** xaAddItem(&(drv->RootContentTypes),"message/rfc822"); **/
	xaAddItem(&(drv->RootContentTypes),"network/smtp");

	/** Setup the function references. **/
	drv->Open = smtpOpen;
	drv->Close = smtpClose;
	drv->Create = smtpCreate;
	drv->Delete = smtpDelete;
	drv->OpenQuery = smtpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = smtpQueryFetch;
	drv->QueryClose = smtpQueryClose;
	drv->Read = smtpRead;
	drv->Write = smtpWrite;
	drv->GetAttrType = smtpGetAttrType;
	drv->GetAttrValue = smtpGetAttrValue;
	drv->GetFirstAttr = smtpGetFirstAttr;
	drv->GetNextAttr = smtpGetNextAttr;
	drv->SetAttrValue = smtpSetAttrValue;
	drv->AddAttr = smtpAddAttr;
	drv->OpenAttr = smtpOpenAttr;
	drv->GetFirstMethod = smtpGetFirstMethod;
	drv->GetNextMethod = smtpGetNextMethod;
	drv->ExecuteMethod = smtpExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = smtpInfo;

	/** nmRegister(sizeof(JsonData),"JsonData"); **/

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;


    return 0;
    }

MODULE_INIT(smtpInitialize);
MODULE_PREFIX("smtp");
MODULE_DESC("SMTP ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);
