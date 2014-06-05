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
#include <dirent.h>
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
    char*		Name;
    int			Type;
    pObject		Obj;
    int			Mask;
    pSnNode		Node;
    pXArray		AttributeNames; /* XArray of char*. */
    pXHashTable		Attributes; /* Hash of attribute name to SmtpAttribute. */
    int			CurAttr;

    /** Root node specific attributes. **/

    /** Email node specific attributes. **/
    pFile		Content;
    }
    SmtpData, *pSmtpData;

#define SMTP(x) ((pSmtpData)(x))

/*** Structure used by queries in this driver. ***/
typedef struct
    {
    pSmtpData	Data;
    DIR*	Directory;
    }
    SmtpQueryData, *pSmtpQueryData;

#define SMTP_QY(x) ((pSmtpQueryData)(x))

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

/*** smtp_internal_GetRootAttributes - Loads the attributes from the node into
 *** the SMTP object.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_GetRootAttributes(pSmtpData inf)
    {
    pSmtpAttribute attr = NULL;
    pStructInf currentAttr = NULL;
    int i;

	for (i = 0; i < inf->Node->Data->nSubInf; i++)
	    {
	    currentAttr = inf->Node->Data->SubInf[i];

	    attr = nmMalloc(sizeof(SmtpAttribute));
	    if (!attr)
		{
		mssError(1,"SMTP","Could not create new attribute object.");
		return -1;
		}

	    attr->Name = currentAttr->Name;
	    attr->Type = currentAttr->Value->DataType;

	    xaAddItem(inf->AttributeNames, attr->Name);

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

    return 0;
    }

/*** smtp_internal_OpenGeneral - Loads attributes common to all SMTP objects.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenGeneral(pSmtpData inf, pObject obj, char* usrtype)
    {
    pSnNode node = NULL;

	inf->Obj = obj;

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
		{
		mssError(0,"SMTP", "Could not create new node object");
		return -1;
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
	    return -1;
	    }

	/** Store the node object. **/
	inf->Node = node;
	inf->Node->OpenCnt++;

	inf->Name = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-1, 0);
	inf->Name = nmSysStrdup(inf->Name);

	inf->AttributeNames = (pXArray)nmMalloc(sizeof(XArray));
	if (!inf->AttributeNames)
	    {
	    mssError(1,"SMTP","Could not create attribute names array.");
	    return -1;
	    }
	memset(inf->AttributeNames, 0, sizeof(XArray));
	xaInit(inf->AttributeNames, 16);

	inf->Attributes = (pXHashTable)nmMalloc(sizeof(XHashTable));
	if (!inf->Attributes)
	    {
	    mssError(1,"SMTP","Could not create attributes hash table.");
	    return -1;
	    }
	memset(inf->Attributes, 0, sizeof(XHashTable));
	xhInit(inf->Attributes, 16, 0);

	inf->CurAttr = 0;

	if (smtp_internal_GetRootAttributes(inf))
	    {
	    mssError(0, "SMTP", "Could not load root attributes.");
	    return -1;
	    }

    return 0;
    }

/*** smtp_internal_OpenRoot - Open the root node of the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenRoot(pSmtpData inf)
    {
	inf->Type = SMTP_T_ROOT;
	inf->Obj->SubCnt = 1;

    return 0;
    }

/*** smtp_internal_OpenEml - Open an email file in the smtp structure.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenEml(pSmtpData inf)
    {
    pSmtpAttribute spoolDir = NULL;
    pXString emailPath = NULL;

	inf->Type = SMTP_T_EML;
	inf->Obj->SubCnt = 2;

	emailPath = (pXString)nmMalloc(sizeof(XString));
	if (!emailPath)
	    {
	    mssError(1, "SMTP", "Unable to allocate space for email path.");
	    goto error;
	    }
	memset(emailPath, 0, sizeof(XString));
	xsInit(emailPath);

	/** Calculate the real path of the email file. **/
	spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	if (!spoolDir)
	    {
	    mssError(1, "SMTP", "Unable to get the spool directory path.");
	    goto error;
	    }

	if (xsCopy(emailPath, spoolDir->Value.String, strlen(spoolDir->Value.String)))
	    {
	    mssError(1, "SMTP", "Unable to copy spool directory path into the email path.");
	    goto error;
	    }

	if (xsConcatenate(emailPath, "/", 1) || xsConcatenate(emailPath, inf->Name, strlen(inf->Name)))
	    {
	    mssError(1, "SMTP", "Unable to append email name to email path.");
	    goto error;
	    }

	/** Open the email file. **/
	inf->Content = fdOpen(emailPath->String, inf->Obj->Mode, inf->Mask);
	if (!inf->Content)
	    {
	    mssErrorErrno(1, "SMTP", "Could not open email file (%s).", emailPath->String);
	    goto error;
	    }

    return 0;

    error:
	if (emailPath)
	    {
	    nmFree(emailPath, sizeof(XString));
	    }

	return -1;
    }

/*** smtpOpen - open an object.
 ***/
void*
smtpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSmtpData inf = NULL;

	inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(SmtpData));
	inf->Mask = mask;

	if (smtp_internal_OpenGeneral(inf, obj, usrtype))
	    {
	    goto error;
	    }

	/** Determine the type of the object. **/
	if (inf->Obj->SubPtr == inf->Obj->Pathname->nElements)
	    {
	    if (smtp_internal_OpenRoot(inf))
		{
		goto error;
		}
	    }
	else if (inf->Obj->SubPtr+1 == inf->Obj->Pathname->nElements &&
		!strcmp(inf->Obj->Pathname->Pathbuf + strlen(inf->Obj->Pathname->Pathbuf) - 4, ".eml"))
	    {
	    if (smtp_internal_OpenEml(inf))
		{
		goto error;
		}
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
	    smtpClose(inf, NULL);
	    }

	return NULL;
    }


/*** smtpClose - close an open object.
 ***/
int
smtpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSmtpData inf = SMTP(inf_v);

    if (inf->AttributeNames)
	{
	xaDeInit(inf->AttributeNames);
	}

    if (inf->Attributes)
	{
	xhClear(inf->Attributes, smtp_internal_ClearAttribute, NULL);
	xhDeInit(inf->Attributes);
	}

    if (inf->Content)
	{
	if (fdClose(inf->Content, 0))
	    {
	    mssError(0, "SMTP", "Unable to close email file.");
	    return -1;
	    }
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
    pSmtpData inf = SMTP(inf_v);
    int rval = -1;

	/** Read the contents of emails directly. **/
	if (inf->Type == SMTP_T_EML)
	    {
	    rval = fdRead(inf->Content, buffer, maxcnt, offset, flags);
	    }

    return rval;
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
    pSmtpData inf = SMTP(inf_v);
    pSmtpQueryData qy = NULL;
    pSmtpAttribute attr = NULL;
    char* spoolPath = NULL;

	/** Allocate the query object. **/
	qy = (pSmtpQueryData)nmMalloc(sizeof(SmtpQueryData));
	if (!qy)
	    {
	    mssError(1,"SMTP","Unable to allocate query object");
	    goto error;
	    }
	memset(qy, 0, sizeof(SmtpQueryData));

	qy->Data = inf;

	/** Construct the query for the root node. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Find and open the spool directory path. **/
	    attr = (pSmtpAttribute)xhLookup(inf->Attributes, "spool_dir");
	    if (!attr)
		{
		mssError(1,"SMTP","Unable to locate spool directory");
		goto error;
		}
	    spoolPath = attr->Value.String;

	    qy->Directory = opendir(spoolPath);
	    if (!qy->Directory)
		{
		mssErrorErrno(1,"SMTP","Could not open spool directory for query");
		goto error;
		}

	    return qy;
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query on system/smtp-message type objects");
	    goto error;
	    }
	
    error:
	if (qy)
	    {
	    smtpQueryClose(qy, NULL);
	    }

        return NULL;
    }


/*** smtpQueryFetch - get the next directory entry as an open object.
 ***/
void*
smtpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pSmtpQueryData qy = SMTP_QY(qy_v);
    pSmtpData inf = NULL;
    struct dirent *mailEntry = NULL;

	if (qy->Data->Type == SMTP_T_ROOT)
	    {
	    /** Infinite while loops are better than GOTOs... probably. **/
	    while (1)
		{
		mailEntry = readdir(qy->Directory);
		if (!mailEntry || !strcmp(mailEntry->d_name + strlen(mailEntry->d_name) - 4, ".eml"))
		    {
			break;
		    }
		}

	    if (!mailEntry)
		{
		return NULL;
		}

	    if (obj_internal_AddToPath(obj->Pathname, mailEntry->d_name) < 0)
		{
		mssError(1, "SMTP", "Query result pathname exceeds internal limits");
		return NULL;
		}
	     obj->Mode = mode;

	    inf = (pSmtpData)nmMalloc(sizeof(SmtpData));
	    if (!inf)
		{
		mssError(1, "SMTP", "Unable to create smtp data object");
		return NULL;
		}
	    memset(inf, 0, sizeof(SmtpData));

	    if (smtp_internal_OpenGeneral(inf, obj, "system/smtp-message") || smtp_internal_OpenEml(inf))
		{
		return NULL;
		}

	    }
	else if (qy->Data->Type == SMTP_T_EML)
	    {
	    mssError(1, "SMTP", "Unable to query smtp-message data objects");
	    return 0;
	    }

	return inf;
    }


/*** smtpQueryClose - close the query.
 ***/
int
smtpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pSmtpQueryData qy = SMTP_QY(qy_v);

	if (qy->Directory)
	    {
	    if (closedir(qy->Directory))
		{
		mssErrorErrno(1,"SMTP","Unable to close directory");
		}
	    }
	nmFree(qy, sizeof(SmtpQueryData));
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

	    if (inf->Type == SMTP_T_ROOT)
		{
		val->String = "system/void";
		}
	    else if (inf->Type == SMTP_T_EML)
		{
		val->String = "message/rfc822";
		}

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
	    if (inf->Type == SMTP_T_ROOT)
		{
		val->String = "system/smtp";
		}
	    else if (inf->Type == SMTP_T_EML)
		{
		val->String = "system/smtp-message";
		}

	    return 0;
	    }
	
	if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1, "SMTP", "Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
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
	    val->String = attr->Value.String;
	    return 0;
	    }

    return 1; /* null if not there presently */
    }


/*** smtpGetNextAttr - get the next attribute name for this object.
 ***/
char*
smtpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);

	if (inf->CurAttr < inf->AttributeNames->nItems)
	    {
	    return (char*)inf->AttributeNames->Items[inf->CurAttr++];
	    }

    return NULL;
    }


/*** smtpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
smtpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
	
	inf->CurAttr = 0;

        return smtpGetNextAttr(inf_v, oxt);
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
	xaAddItem(&(drv->RootContentTypes),"system/smtp");
	xaAddItem(&(drv->RootContentTypes),"system/smtp-message");

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
