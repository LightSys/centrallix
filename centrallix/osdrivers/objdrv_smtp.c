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

/*** Global data structure for the SMTP module. ***/
struct
    {
    XArray		DefaultRootAttributes;		/* XArray of pSmtpAttibute */
    XArray		DefaultEmailAttributes;		/* XArray of pSmtpAttibute */
    }
    SMTP_INF;

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

/*** smtp_internal_CreateAttribute - Creates an attribute with the given values.
 ***/
pSmtpAttribute
smtp_internal_CreateAttribute(char* name, int type, int intVal, char* strVal, pObjData value)
    {
    pSmtpAttribute inf = NULL;

	inf = (pSmtpAttribute)nmMalloc(sizeof(SmtpAttribute));

	inf->Name = name;
	inf->Type = type;

	if (type == DATA_T_INTEGER)
	    {
	    inf->Value.Integer = intVal;
	    }
	else if (type == DATA_T_STRING && strVal)
	    {
	    inf->Value.String = strVal;
	    }
	else
	    {
	    inf->Value = *value;
	    }
    
    return inf;
    }

/*** smtp_internal_InitGlobals - Initializes global information for the SMTP
 *** driver.
 ***/
int
smtp_internal_InitGlobals()
    {
	/** Initialize the global attributes. **/
	xaInit(&SMTP_INF.DefaultRootAttributes, 8);
	xaInit(&SMTP_INF.DefaultEmailAttributes, 8);

	/** Add all the required attributes. Yay hardcoding! **/
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("send_method", DATA_T_STRING, 0, "sendmail", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("server", DATA_T_STRING, 0, "127.0.0.1", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("port", DATA_T_INTEGER, 23, NULL, NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("spool_dir", DATA_T_STRING, 0, "/var/spool/mail/_centrallix", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_dir", DATA_T_STRING, 0, "/var/log", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_date_attr", DATA_T_STRING, 0, "", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_msgid_attr", DATA_T_STRING, 0, "", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("log_info_attr", DATA_T_STRING, 0, "", NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("ratelimit_time", DATA_T_INTEGER, 1, NULL, NULL));
	xaAddItem(&SMTP_INF.DefaultRootAttributes, smtp_internal_CreateAttribute("domlimit_time", DATA_T_INTEGER, 5, NULL, NULL));

    return 0;
    }

/*** smtp_internal_GetStructAttributes - Loads the attributes from the node into
 *** the SMTP object.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_GetStructAttributes(pStructInf structInf, pSmtpData inf)
    {
    pSmtpAttribute attr = NULL;
    pStructInf currentAttr = NULL;
    int i;

	for (i = 0; i < structInf->nSubInf; i++)
	    {
	    currentAttr = structInf->SubInf[i];

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

/*** smtp_internal_CreateRoot - Creates a root smtp node.
 *** Returns the newly created root node or NULL (if creation failed).
 ***/
pSnNode
smtp_internal_CreateRootNode(pObject obj, int mask)
    {
    pSnNode node = NULL;
    pSmtpAttribute currentAttr = NULL;
    pStructInf currentParam = NULL;
    int i;

	/** Create the node object **/
	node = snNewNode(obj, "system/smtp");
	if (!node)
	    {
	    mssError(0, "SMTP", "Could not create new node object");
	    return NULL;
	    }

	/** If globals are not yet initialized, initialize them. **/
	/** We don't always need globals, but when we do, they should be initialized. **/
	smtp_internal_InitGlobals();

	/** Iterate through all the default root attributes. **/
	for (i = 0; i < SMTP_INF.DefaultRootAttributes.nItems; i ++)
	    {
	    currentAttr = SMTP_ATTR(SMTP_INF.DefaultRootAttributes.Items[i]);

	    /** Add the attribute to the node. **/
	    currentParam = stAddAttr(node->Data, currentAttr->Name);
	    if (!currentParam)
		{
		mssError(0, "SMTP", "Could not add attribute value %s", currentAttr->Name);
		return NULL;
		}

	    /** Set the attribute to it's default value. **/
	    if (stSetAttrValue(currentParam, currentAttr->Type, &currentAttr->Value, 0))
		{
		mssError(0, "SMTP", "Could not set attribute value %s", currentAttr->Name);
		return NULL;
		}
	    }

	/** Write the root node structure file. **/
	snWriteNode(obj, node);

    return node;
    }

/*** smtp_internal_OpenGeneral - Loads attributes common to all SMTP objects.
 *** Returns 0 on success and -1 on failure.
 ***/
int
smtp_internal_OpenGeneral(pSmtpData inf, char* usrtype)
    {
    pSnNode node = NULL;

	
	/** Try to open the root node first. **/
	if (!node)
	    {
	    node = snReadNode(inf->Obj->Prev);
	    }

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((inf->Obj->Mode & O_CREAT) && (inf->Obj->Mode & O_EXCL) && (inf->Obj->SubPtr == inf->Obj->Pathname->nElements))
	    {
	    if (node)
		{
		mssError(0, "SMTP", "Node exists and CREAT and EXCL flags are set. Cannot create new node.");
		return -1;
		}

	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
	    if (!node)
		{
		mssError(0,"SMTP", "Could not create new node object");
		return -1;
		}
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (inf->Obj->Mode & O_CREAT) && (inf->Obj->SubPtr == inf->Obj->Pathname->nElements))
	    {
	    node = smtp_internal_CreateRootNode(inf->Obj, inf->Mask);
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

	if (smtp_internal_GetStructAttributes(inf->Node->Data, inf))
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
    pXString emailStructurePath = NULL;
    pFile emailStructureFile = NULL;
    pStructInf emailStructure = NULL;

	inf->Type = SMTP_T_EML;

	emailPath = (pXString)nmMalloc(sizeof(XString));
	if (!emailPath)
	    {
	    mssError(1, "SMTP", "Unable to allocate space for email path.");
	    goto error;
	    }
	memset(emailPath, 0, sizeof(XString));
	xsInit(emailPath);

	emailStructurePath = nmMalloc(sizeof(XString));
	if (!emailStructurePath)
	    {
	    mssError(1, "SMTP", "Unable to allocate space for email structure path.");
	    goto error;
	    }
	memset(emailStructurePath, 0, sizeof(XString));
	xsInit(emailStructurePath);

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

	/** Get the path of the email structure file. **/
	if (xsCopy(emailStructurePath, emailPath->String, emailPath->Length - 4) ||
	    xsConcatenate(emailStructurePath, ".struct", -1))
	    {
	    mssError(1, "SMTP", "Could not construct email structure file path.");
	    goto error;
	    }

	/** Open the email structure file. **/
	emailStructureFile = fdOpen(emailStructurePath->String, 
					inf->Obj->Mode, 
					inf->Mask);
	if (!emailStructureFile)
	    {
	    mssError(1, "SMTP", "Could not open email structure file (%s).", emailStructurePath->String);
	    goto error;
	    }
	
	/** Parse the structure file. **/
	emailStructure = stParseMsg(emailStructureFile, 0);
	if (!emailStructure)
	    {
	    mssError(0, "SMTP", "Could not parse the email structure file.");
	    goto error;
	    }

	/** Get the structure's attribues **/
	if (smtp_internal_GetStructAttributes(emailStructure, inf))
	    {
	    mssError(0, "SMTP", "Could not load email attributes.");
	    goto error;
	    }

    return 0;

    error:
	if (emailPath)
	    {
	    nmFree(emailPath, sizeof(XString));
	    }

	if (emailStructurePath)
	    {
	    nmFree(emailStructurePath, sizeof(XString));
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
	inf->Obj = obj;

	/** Determine the type of the object. **/
	if (inf->Obj->SubPtr == inf->Obj->Pathname->nElements)
	    {
	    inf->Obj->SubCnt = 1;

	    if (smtp_internal_OpenGeneral(inf, usrtype))
		{
		goto error;
		}
		
	    if (smtp_internal_OpenRoot(inf))
		{
		goto error;
		}
	    }
	else if (inf->Obj->SubPtr+1 == inf->Obj->Pathname->nElements &&
		!strcmp(inf->Obj->Pathname->Pathbuf + strlen(inf->Obj->Pathname->Pathbuf) - 4, ".eml"))
	    {
	    inf->Obj->SubCnt = 2;

	    if (smtp_internal_OpenGeneral(inf, usrtype))
		{
		goto error;
		}
		
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

	/** We're closing the object... let the world know. **/
	inf->Node->OpenCnt--;

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
    pSnNode node = NULL;

	/** Determine the type of the object. **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    node = snReadNode(obj);
	    if (node)
		{
		mssError(1, "SMTP", "Unable to create root node because it already exists.");
		return -1;
		}

	    node = smtp_internal_CreateRootNode(obj, mask);
	    if (!node)
		{
		mssError(1, "SMTP", "Unable to create root node.");
		return -1;
		}
	    }
	else if (obj->SubPtr+1 == obj->Pathname->nElements &&
		!strcmp(obj->Pathname->Pathbuf + strlen(obj->Pathname->Pathbuf) - 4, ".eml"))
	    {
	    mssError(1, "SMTP", "Not currently handling email creation.");
	    return -1;
	    }
	else
	    {
	    mssError(1,"SMTP","Could not create file");
	    return -1;
	    }

    return 0;
    }


/*** smtpDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
smtpDelete(pObject obj, pObjTrxTree* oxt)
    {
	/** Determine the type of the object. **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    objDeleteObj(obj->Prev);
	    return 0;
	    }
	else if (obj->SubPtr+1 == obj->Pathname->nElements &&
		!strcmp(obj->Pathname->Pathbuf + strlen(obj->Pathname->Pathbuf) - 4, ".eml"))
	    {
	    mssError(1, "SMTP", "Not currently handling email deletion");
	    }
	else
	    {
	    mssError(1,"SMTP","Could not delete indicated object");
	    }

    return -1;
    }


/*** smtpRead - Read from the SMTP object
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


/*** smtpWrite - Write to the SMTP object
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
	    inf->Obj = obj;

	    if (smtp_internal_OpenGeneral(inf, "system/smtp-message") || smtp_internal_OpenEml(inf))
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
    pSmtpData inf = NULL;
    pSmtpAttribute attr = NULL;
	
	/** If the attribute does not exist, return no type. **/
	if (!inf_v)
	    {
	    return -1;
	    }

	inf = SMTP(inf_v);

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
    pSmtpData inf = NULL;
    pSmtpAttribute attr = NULL;

	if (!inf_v)
	    {
	    mssError(1, "SMTP", "Attribute not found '%s'", attrname);
	    return -1;
	    }

	inf = SMTP(inf_v);

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
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;

    pSnNode rootNode = NULL;

    pSmtpAttribute spoolDir = NULL;
    pXString emlStructPath = NULL;
    pFile emlStructFile = NULL;
    pStructInf emlStruct = NULL;

	/** Get the requested attribute. **/
	attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	if (!attr)
	    {
	    /** Add the attribute if it is not found. **/
	    if (smtpAddAttr(inf, attrname, datatype, val, oxt))
		{
		mssError(0, "SMTP", "Unable to create the requested attribute object.");
		goto error;
		}

	    /** Get the newly created attribute. **/
	    attr = SMTP_ATTR(xhLookup(inf->Attributes, attrname));
	    if (!attr)
		{
		mssError(1, "SMTP", "Unable to open requested attribute object.");
		goto error;
		}
	    }
	
	/** Check the requested datatype. **/
	if (attr->Type != datatype)
	    {
	    if (datatype < OBJ_TYPE_NAMES_CNT && attr->Type < OBJ_TYPE_NAMES_CNT)
		{
		mssError(1 ,"SMTP", "Attempt to assign invalid data type to attribute. (Assigning %s to %s)", obj_type_names[datatype], obj_type_names[attr->Type]);
		}
	    else
		{
		mssError(1 ,"SMTP", "Attempt to assign invalid data type to attribute. (Assigning %d to %d)", datatype, attr->Type);
		}
	    goto error;
	    }

	/** Store the data according to its data type. **/
	if (datatype == DATA_T_STRING)
	    {
	    attr->Value.String = val->String;
	    }
	else if (datatype == DATA_T_INTEGER)
	    {
	    attr->Value.Integer = val->Integer;
	    }
	else
	    {
	    /** This might result in broken behaviour... FIXME or at least TESTME. **/
	    attr->Value = *val;
	    }

	/** Store the attribute into the correct file. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Read the root node into the node structure. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (!rootNode)
		{
		mssError(1, "SMTP", "Unable to open root node for writing");
		goto error;
		}

	    /** Set the attribute value in the root node. **/
	    if (stSetAttrValue(stLookup(rootNode->Data, attrname), datatype, val, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Mark root node DIRTY so that it will be written. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node back to the OS tree. **/
	    if (snWriteNode(inf->Obj->Prev, rootNode))
		{
		mssError(1, "SMTP", "Unable to write data to the root node");
		goto error;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Allocate the email structure path. **/
	    emlStructPath = nmMalloc(sizeof(XString));
	    if (!emlStructPath)
		{
		mssError(1, "SMTP", "Unable to allocate space for email structure path.");
		goto error;
		}
	    memset(emlStructPath, 0, sizeof(XString));
	    xsInit(emlStructPath);

	    /** Calculate the real path of the email struct file. **/
	    spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	    if (!spoolDir)
		{
		mssError(1, "SMTP", "Unable to get the spool directory path.");
		goto error;
		}

	    if (xsCopy(emlStructPath, spoolDir->Value.String, strlen(spoolDir->Value.String)))
		{
		mssError(1, "SMTP", "Unable to copy spool directory path into the email path.");
		goto error;
		}

	    if (xsConcatenate(emlStructPath, "/", 1) ||
		xsConcatenate(emlStructPath, inf->Name, strlen(inf->Name) - 4) ||
		xsConcatenate(emlStructPath, ".struct", -1))
		{
		mssError(1, "SMTP", "Unable to append email name to email path.");
		goto error;
		}

	    /** Open the email structure file. **/
	    emlStructFile = fdOpen(emlStructPath->String, 
					    inf->Obj->Mode, 
					    inf->Mask);
	    if (!emlStructFile)
		{
		mssError(1, "SMTP", "Could not open email structure file (%s).", emlStructPath->String);
		goto error;
		}
	    
	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFile, 0);
	    if (!emlStruct)
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
		goto error;
		}

	    /** Set the given attribute value. **/
	    if (stSetAttrValue(stLookup(emlStruct, attrname), datatype, val, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Write changes to the email struct file. **/
	    if (stGenerateMsg(emlStructFile, emlStruct, O_WRONLY | O_TRUNC | O_CREAT))
		{
		mssError(1, "SMTP", "Unable to write to the attribute to the email struct file.");
		goto error;
		}
	    }

	/** Free appropriate memory and close appropriate files. **/
	if (emlStructPath)
	    {
	    nmFree(emlStructPath, sizeof(XString));
	    }

	if (emlStructFile)
	    {
	    fdClose(emlStructFile, 0);
	    }

    return 0;

    error:
	if (emlStructPath)
	    {
	    nmFree(emlStructPath, sizeof(XString));
	    }

	if (emlStructFile)
	    {
	    fdClose(emlStructFile, 0);
	    }

	return -1;
    }


/*** smtpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
smtpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pSmtpData inf = SMTP(inf_v);
    pSmtpAttribute attr = NULL;
    pStructInf createdStruct = NULL;

    pSnNode rootNode = NULL;

    pSmtpAttribute spoolDir = NULL;
    pXString emlStructPath = NULL;
    pFile emlStructFile = NULL;
    pStructInf emlStruct = NULL;

	/** Initialize the new attribute. **/
	attr = nmMalloc(sizeof(SmtpAttribute));
	if (!attr)
	    {
	    mssError(1,"SMTP","Could not create new attribute object.");
	    return -1;
	    }

	/** Set the meta-data fields of the new attribute. **/
	attr->Name = attrname;
	attr->Type = type;

	/** Add the new attribute to the attribute name list. **/
	xaAddItem(inf->AttributeNames, attr->Name);

	/** Set the default value appropriately if it is a string. **/
	if (attr->Type == DATA_T_STRING)
	    {
	    attr->Value.String = "";
	    }

	/** Set the default value appropriately if it is a integer. **/
	if (attr->Type == DATA_T_INTEGER)
	    {
	    attr->Value.Integer = 0;
	    }

	/** Add the attribute to the attribute hash. **/
	xhAdd(inf->Attributes, attr->Name, (char*)attr);

	/** Create the attribute in the correct location according to object type. **/
	if (inf->Type == SMTP_T_ROOT)
	    {
	    /** Open the root node. **/
	    rootNode = snReadNode(inf->Obj->Prev);
	    if (!rootNode)
		{
		mssError(1, "SMTP", "Unable to open root node.");
		goto error;
		}

	    /** Add the attribute to the root node. **/
	    createdStruct = stAddAttr(rootNode->Data, attr->Name);
	    if (!createdStruct)
		{
		mssError(1, "SMTP", "Unable to add new attribute to the root node.");
		goto error;
		}

	    /** Set the default attribute value. **/
	    if (stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Set the root node to DIRTY so it will be written to the file. **/
	    rootNode->Status = SN_NS_DIRTY;

	    /** Write the changes to the root node. **/
	    if (snWriteNode(inf->Obj->Prev, rootNode))
		{
		mssError(1, "SMTP", "Unable to write root node.");
		goto error;
		}
	    }
	else if (inf->Type == SMTP_T_EML)
	    {
	    /** Allocate the email structure path. **/
	    emlStructPath = nmMalloc(sizeof(XString));
	    if (!emlStructPath)
		{
		mssError(1, "SMTP", "Unable to allocate space for email structure path.");
		goto error;
		}
	    memset(emlStructPath, 0, sizeof(XString));
	    xsInit(emlStructPath);

	    /** Calculate the real path of the email struct file. **/
	    spoolDir = SMTP_ATTR(xhLookup(inf->Attributes, "spool_dir"));
	    if (!spoolDir)
		{
		mssError(1, "SMTP", "Unable to get the spool directory path.");
		goto error;
		}

	    if (xsCopy(emlStructPath, spoolDir->Value.String, strlen(spoolDir->Value.String)))
		{
		mssError(1, "SMTP", "Unable to copy spool directory path into the email path.");
		goto error;
		}

	    if (xsConcatenate(emlStructPath, "/", 1) ||
		xsConcatenate(emlStructPath, inf->Name, strlen(inf->Name) - 4) ||
		xsConcatenate(emlStructPath, ".struct", -1))
		{
		mssError(1, "SMTP", "Unable to append email name to email path.");
		goto error;
		}

	    /** Open the email structure file. **/
	    emlStructFile = fdOpen(emlStructPath->String, 
					    inf->Obj->Mode, 
					    inf->Mask);
	    if (!emlStructFile)
		{
		mssError(1, "SMTP", "Could not open email structure file (%s).", emlStructPath->String);
		goto error;
		}
	    
	    /** Parse the structure file. **/
	    emlStruct = stParseMsg(emlStructFile, 0);
	    if (!emlStruct)
		{
		mssError(0, "SMTP", "Could not parse the email structure file.");
		goto error;
		}

	    /** Add the attribute to the email struct. **/
	    createdStruct = stAddAttr(emlStruct, attr->Name);
	    if (!createdStruct)
		{
		mssError(1, "SMTP", "Could not add attribute to the email struct.");
		goto error;
		}
	    
	    /** Set the default attribute value. **/
	    if (stSetAttrValue(createdStruct, attr->Type, &attr->Value, 0))
		{
		mssError(1, "SMTP", "Unable to write to the given attribute");
		goto error;
		}

	    /** Write changes to the email struct file. **/
	    if (stGenerateMsg(emlStructFile, emlStruct, O_WRONLY | O_TRUNC | O_CREAT))
		{
		mssError(1, "SMTP", "Unable to write to the attribute to the email struct file.");
		goto error;
		}
	    }

	/** Free appropriate memory and close appropriate files. **/
	if (emlStructPath)
	    {
	    nmFree(emlStructPath, sizeof(XString));
	    }

	if (emlStructFile)
	    {
	    fdClose(emlStructFile, 0);
	    }

    return 0;

    error:
	/** Free appropriate memory and close appropriate files. **/
	if (emlStructPath)
	    {
	    nmFree(emlStructPath, sizeof(XString));
	    }

	if (emlStructFile)
	    {
	    fdClose(emlStructFile, 0);
	    }

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
