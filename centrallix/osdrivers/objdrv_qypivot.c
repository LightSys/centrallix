#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "expression.h"
#include "cxlib/mtsession.h"
#include "hints.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_qypivot.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 19, 2012 					*/
/* Description:	Pivot Query object driver.  This driver provides a     	*/
/*		"pivot" conversion of underlying EAV-structured data	*/
/*		into a conventional object.				*/
/************************************************************************/


#define QYP_MAX_KEYS		(16)
#define QYP_MAX_ATTRFIELDS	(16)


/*** Node data ***/
typedef struct
    {
    char	NodePath[OBJSYS_MAX_PATH + 1];
    char	SourcePath[OBJSYS_MAX_PATH + 1];
    char*	KeyNames[QYP_MAX_KEYS];
    int		KeyTypes[QYP_MAX_KEYS];
    pObjPresentationHints KeyHints[QYP_MAX_KEYS];
    int		nKeys;
    char*	AttrNameField;
    char*	ValueTypeField;
    char*	ValueNames[QYP_MAX_ATTRFIELDS];
    int		ValueTypes[QYP_MAX_ATTRFIELDS];
    pObjPresentationHints ValueHints[QYP_MAX_ATTRFIELDS];
    int		nValues;
    char*	CreateDateField;
    char*	CreateUserField;
    char*	ModifyDateField;
    char*	ModifyUserField;
    pSnNode	BaseNode;
    pStructInf	NodeData;
    }
    QypNode, *pQypNode;


/*** Info for a single datum ***/
typedef struct
    {
    char	Name[64];		/* attribute name for this datum */
    char	SourceObjName[64];	/* name of object under the SourcePath */
    int		SourceValueID;		/* index into ValueNames/ValueTypes in the node structure */
    TObjData	Data;
    DateTime	CreateDate;
    char	CreateBy[32];
    DateTime	ModifyDate;
    char	ModifyBy[32];
    int		Flags;			/* QYP_DATUM_F_xxx */
    }
    QypDatum, *pQypDatum;

#define QYP_DATUM_F_DIRTY	(1)
#define QYP_DATUM_F_NEW	    	(2)
#define QYP_DATUM_F_KEY	    	(4)


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	ObjName[64];
    int		Flags;		/* QYP_F_xxx */
    pObject	Obj;
    int		Mask;
    pQypNode	Node;
    XArray	PivotData;	/* array of pQypDatum */
    int		CurAttr;	/* for FirstAttr/NextAttr */
    }
    QypData, *pQypData;

#define QYP_F_ISROW		(1)
#define QYP_F_NEW		(2)


#define QYP(x) ((pQypData)(x))


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pQypData	ObjInf;
    pObjQuery	Query;
    char	NameBuf[OBJSYS_MAX_PATH + 1];
    char*	QyText;
    pObject	LLQueryObj;
    pObjQuery	LLQuery;
    pObject	NextSubobj;
    }
    QypQuery, *pQypQuery;


/*** Globals ***/
struct
    {
    XHashTable	NodeCache;
    }
    QYP_INF;


/***
 *** Pivot configuration:
 ***
 ***  - path to underlying data source
 ***  - entity key fields (list)
 ***  - attribute name field
 ***  - attribute type field (optional)
 ***  - attribute value field(s)
 ***  - metadata fields: create user/date, modify user/date
 ***/


/*** qyp_internal_ReadNode() - access the structure file data for the
 *** node to get our configuration, and store it in the QypNode structure
 *** for later use.
 ***/
pQypNode
qyp_internal_ReadNode(char* nodepath, pSnNode nodestruct)
    {
    pQypNode node;
    char* ptr;
    int i;
    pStructInf field_inf;
    char* datatype;
    char* usage;
    char* fieldname;
    int t;

	/** Cached? **/
	node = (pQypNode)xhLookup(&QYP_INF.NodeCache, nodepath);
	if (node) return node;

	/** Allocate and set up **/
	node = (pQypNode)nmMalloc(sizeof(QypNode));
	if (!node)
	    goto error;
	memset(node, 0, sizeof(QypNode));
	node->BaseNode = nodestruct;
	node->NodeData = nodestruct->Data;
	strtcpy(node->NodePath, nodepath, sizeof(node->NodePath));

	/** Lookup parameters **/
	if (stAttrValue(stLookup(node->NodeData, "source"), NULL, &ptr, 0) != 0)
	    {
	    mssError(1, "QYP", "'source' must be specified for a pivot object");
	    goto error;
	    }
	strtcpy(node->SourcePath, ptr, sizeof(node->SourcePath));

	/** Loop through field definitions **/
	for(i=0;i<node->NodeData->nSubInf;i++)
	    {
	    field_inf = node->NodeData->SubInf[i];
	    if (stStructType(field_inf) != ST_T_SUBGROUP) continue;

	    /** Get type, usage, and field name **/
	    if (stAttrValue(stLookup(field_inf, "usage"), NULL, &usage, 0) != 0)
		usage = "value";
	    if (stAttrValue(stLookup(field_inf, "type"), NULL, &datatype, 0) != 0)
		{
		if (!strcmp(usage,"createdate") || !strcmp(usage,"modifydate"))
		    datatype = "datetime";
		else
		    datatype = "string";
		}
	    if (stAttrValue(stLookup(field_inf, "field"), NULL, &fieldname, 0) != 0)
		fieldname = field_inf->Name;

	    /** Valid type? **/
	    if ((t = objTypeID(datatype)) < 0)
		{
		mssError(1, "QYP", "data type '%s' for field '%s' is not valid", datatype, fieldname);
		goto error;
		}

	    /** Based on field usage... **/
	    if (!strcmp(usage, "key"))
		{
		if (node->nKeys >= QYP_MAX_KEYS)
		    {
		    mssError(1, "QYP", "too many keys - maximum is %d", QYP_MAX_KEYS);
		    goto error;
		    }
		if (t != DATA_T_STRING && t != DATA_T_INTEGER)
		    {
		    mssError(1, "QYP", "key fields can only be string or integer");
		    goto error;
		    }
		node->KeyNames[node->nKeys] = nmSysStrdup(fieldname);
		node->KeyTypes[node->nKeys] = t;
		node->nKeys++;
		}
	    else if (!strcmp(usage, "name"))
		{
		if (node->AttrNameField)
		    {
		    mssError(1, "QYP", "exactly one attribute name field is allowed");
		    goto error;
		    }
		if (t != DATA_T_STRING)
		    {
		    mssError(1, "QYP", "attribute name field must be a string");
		    goto error;
		    }
		node->AttrNameField = nmSysStrdup(fieldname);
		}
	    else if (!strcmp(usage, "type"))
		{
		if (node->ValueTypeField)
		    {
		    mssError(1, "QYP", "exactly one attribute value type field is allowed");
		    goto error;
		    }
		if (t != DATA_T_STRING)
		    {
		    mssError(1, "QYP", "attribute value type field must be a string");
		    goto error;
		    }
		node->ValueTypeField = nmSysStrdup(fieldname);
		}
	    else if (!strcmp(usage, "value"))
		{
		if (node->nValues >= QYP_MAX_ATTRFIELDS)
		    {
		    mssError(1, "QYP", "too many value fields - maximum is %d", QYP_MAX_ATTRFIELDS);
		    goto error;
		    }
		node->ValueNames[node->nValues] = nmSysStrdup(fieldname);
		node->ValueTypes[node->nValues] = t;
		node->nValues++;
		}
	    else if (!strcmp(usage, "createdate"))
		{
		if (node->CreateDateField)
		    {
		    mssError(1, "QYP", "only one create date field is allowed");
		    goto error;
		    }
		if (t != DATA_T_DATETIME)
		    {
		    mssError(1, "QYP", "create date field must be a datetime type");
		    goto error;
		    }
		node->CreateDateField = nmSysStrdup(fieldname);
		}
	    else if (!strcmp(usage, "modifydate"))
		{
		if (node->ModifyDateField)
		    {
		    mssError(1, "QYP", "only one modify date field is allowed");
		    goto error;
		    }
		if (t != DATA_T_DATETIME)
		    {
		    mssError(1, "QYP", "modify date field must be a datetime type");
		    goto error;
		    }
		node->ModifyDateField = nmSysStrdup(fieldname);
		}
	    else if (!strcmp(usage, "createuser"))
		{
		if (node->CreateUserField)
		    {
		    mssError(1, "QYP", "only one create user field is allowed");
		    goto error;
		    }
		if (t != DATA_T_STRING)
		    {
		    mssError(1, "QYP", "create user field must be a string type");
		    goto error;
		    }
		node->CreateUserField = nmSysStrdup(fieldname);
		}
	    else if (!strcmp(usage, "modifyuser"))
		{
		if (node->ModifyUserField)
		    {
		    mssError(1, "QYP", "only one modify user field is allowed");
		    goto error;
		    }
		if (t != DATA_T_STRING)
		    {
		    mssError(1, "QYP", "modify user field must be a string type");
		    goto error;
		    }
		node->ModifyUserField = nmSysStrdup(fieldname);
		}
	    }

	/** Enough info supplied? **/
	if (!node->AttrNameField)
	    {
	    mssError(1, "QYP", "attribute name field must be specified");
	    goto error;
	    }
	if (node->nKeys == 0)
	    {
	    mssError(1, "QYP", "at least one entity key field must be specified");
	    goto error;
	    }
	if (node->nValues == 0)
	    {
	    mssError(1, "QYP", "at least one value field must be specified");
	    goto error;
	    }

	/** Cache it **/
	xhAdd(&QYP_INF.NodeCache, node->NodePath, (void*)node);

	return node;

    error:
	if (node)
	    {
	    if (node->AttrNameField) nmSysFree(node->AttrNameField);
	    if (node->ValueTypeField) nmSysFree(node->ValueTypeField);
	    if (node->CreateUserField) nmSysFree(node->CreateUserField);
	    if (node->CreateDateField) nmSysFree(node->CreateDateField);
	    if (node->ModifyUserField) nmSysFree(node->ModifyUserField);
	    if (node->ModifyDateField) nmSysFree(node->ModifyDateField);
	    for(i=0;i<node->nKeys;i++)
		nmSysFree(node->KeyNames[i]);
	    for(i=0;i<node->nValues;i++)
		nmSysFree(node->ValueNames[i]);
	    nmFree(node, sizeof(QypNode));
	    }
	return NULL;
    }


/*** qyp_internal_LoadDatum() - take one source object from the underlying
 *** data source, and build a single QytDatum structure from it.
 ***/
pQypDatum
qyp_internal_LoadDatum(pQypData inf, pObject source_subobj)
    {
    pQypDatum one_datum = NULL;
    int i;
    pDateTime dt;
    pMoneyType money;
    char* ptr;
    int given_type;
    int actual_type;

	/** allocate the datum **/
	one_datum = (pQypDatum)nmMalloc(sizeof(QypDatum));
	if (!one_datum)
	    goto error;
	memset(one_datum, 0, sizeof(QypDatum));

	/** Get source object name **/
	objGetAttrValue(source_subobj, "name", DATA_T_STRING, POD(&ptr));
	strtcpy(one_datum->SourceObjName, ptr, sizeof(one_datum->SourceObjName));

	/** Get the attribute name **/
	if (objGetAttrValue(source_subobj, inf->Node->AttrNameField, DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(0, "QYP", "Attribute name field (%s) must be valid and non-null", inf->Node->AttrNameField);
	    goto error;
	    }
	strtcpy(one_datum->Name, ptr, sizeof(one_datum->Name));

	/** Data type provided? **/
	given_type = DATA_T_UNAVAILABLE;
	if (inf->Node->ValueTypeField)
	    {
	    if (objGetAttrValue(source_subobj, inf->Node->AttrNameField, DATA_T_STRING, POD(&ptr)) != 0)
		{
		mssError(0, "QYP", "Attribute value type field (%s) must be valid and non-null", inf->Node->ValueTypeField);
		goto error;
		}
	    given_type = objTypeID(ptr);
	    if (given_type < 0 || given_type == DATA_T_UNAVAILABLE)
		{
		mssError(0, "QYP", "Attribute value type field (%s) must be a valid data type", inf->Node->ValueTypeField);
		goto error;
		}
	    }

	/** Load in hints data for entity (key) fields if needed **/
	for(i=0;i<inf->Node->nKeys;i++)
	    {
	    if (!inf->Node->KeyHints[i])
		{
		inf->Node->KeyHints[i] = objPresentationHints(source_subobj, inf->Node->KeyNames[i]);
		inf->Node->KeyHints[i]->Style |= OBJ_PH_STYLE_KEY;
		inf->Node->KeyHints[i]->StyleMask |= OBJ_PH_STYLE_KEY;
		}
	    }

	/** Loop through the value fields, finding one with a valid value **/
	one_datum->Data.DataType = DATA_T_UNAVAILABLE;
	for(i=0;i<inf->Node->nValues;i++)
	    {
	    if (given_type == DATA_T_UNAVAILABLE || given_type == inf->Node->ValueTypes[i])
		{
		actual_type = objGetAttrType(source_subobj, inf->Node->ValueNames[i]);
		if (actual_type < 0 || actual_type == DATA_T_UNAVAILABLE)
		    continue;
		if (given_type != DATA_T_UNAVAILABLE && actual_type != given_type)
		    {
		    mssError(1, "QYP", "Type mismatch on attribute value field '%s'", inf->Node->ValueNames[i]);
		    goto error;
		    }
		if (objGetAttrValue(source_subobj, inf->Node->ValueNames[i], actual_type, &(one_datum->Data.Data)) != 0)
		    continue;
		one_datum->Data.DataType = actual_type;
		one_datum->Data.Flags = 0;
		one_datum->SourceValueID = i;

		/** Get hints information on this field, if we haven't already **/
		if (!inf->Node->ValueHints[i])
		    {
		    inf->Node->ValueHints[i] = objPresentationHints(source_subobj, inf->Node->ValueNames[i]);
		    inf->Node->ValueHints[i]->Style &= ~OBJ_PH_STYLE_KEY;
		    inf->Node->ValueHints[i]->StyleMask |= OBJ_PH_STYLE_KEY;
		    }

		/** Make our own copy of the referenced data **/
		if (actual_type == DATA_T_STRING)
		    one_datum->Data.Data.String = nmSysStrdup(one_datum->Data.Data.String);
		else if (actual_type == DATA_T_DATETIME)
		    {
		    dt = one_datum->Data.Data.DateTime;
		    one_datum->Data.Data.DateTime = nmMalloc(sizeof(DateTime));
		    memcpy(one_datum->Data.Data.DateTime, dt, sizeof(DateTime));
		    }
		else if (actual_type == DATA_T_MONEY)
		    {
		    money = one_datum->Data.Data.Money;
		    one_datum->Data.Data.Money = nmMalloc(sizeof(MoneyType));
		    memcpy(one_datum->Data.Data.Money, money, sizeof(MoneyType));
		    }
		break;
		}
	    }

	/** Retrieve the metadata? **/
	if (inf->Node->CreateDateField)
	    {
	    if (objGetAttrValue(source_subobj, inf->Node->CreateDateField, DATA_T_DATETIME, POD(&dt)) == 0)
		memcpy(&one_datum->CreateDate, dt, sizeof(DateTime));
	    }
	if (inf->Node->CreateUserField)
	    {
	    if (objGetAttrValue(source_subobj, inf->Node->CreateUserField, DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(one_datum->CreateBy, ptr, sizeof(one_datum->CreateBy));
	    }
	if (inf->Node->ModifyDateField)
	    {
	    if (objGetAttrValue(source_subobj, inf->Node->ModifyDateField, DATA_T_DATETIME, POD(&dt)) == 0)
		memcpy(&one_datum->ModifyDate, dt, sizeof(DateTime));
	    }
	if (inf->Node->ModifyUserField)
	    {
	    if (objGetAttrValue(source_subobj, inf->Node->ModifyUserField, DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(one_datum->ModifyBy, ptr, sizeof(one_datum->ModifyBy));
	    }

	/** Didn't find any valid values?  Treat as NULL if so. **/
	if (one_datum->Data.DataType == DATA_T_UNAVAILABLE)
	    {
	    one_datum->Data.DataType = given_type;
	    one_datum->Data.Flags |= DATA_TF_NULL;
	    }

	return one_datum;

    error:
	if (one_datum) nmFree(one_datum, sizeof(QypDatum));
	return NULL;
    }


/*** qyp_internal_KeyToDatum() - take a key value and convert it to
 *** a datum.
 ***/
pQypDatum
qyp_internal_KeyToDatum(pQypData inf, char* keyval, int keyindex)
    {
    pQypDatum one_datum = NULL;

	one_datum = (pQypDatum)nmMalloc(sizeof(QypDatum));
	if (!one_datum)
	    return NULL;
	memset(one_datum, 0, sizeof(QypDatum));
	strtcpy(one_datum->Name, inf->Node->KeyNames[keyindex], sizeof(one_datum->Name));
	one_datum->Data.DataType = inf->Node->KeyTypes[keyindex];
	one_datum->Data.Flags = 0;
	if (one_datum->Data.DataType == DATA_T_INTEGER)
	    one_datum->Data.Data.Integer = strtoi(keyval, NULL, 10);
	else if (one_datum->Data.DataType == DATA_T_STRING)
	    one_datum->Data.Data.String = nmSysStrdup(keyval);
	one_datum->Flags |= QYP_DATUM_F_KEY;

    return one_datum;
    }


/*** qyp_internal_NameToExpression() - given an object name, construct
 *** an expression tree with lookup criteria.
 ***/
pExpression
qyp_internal_NameToExpression(pQypNode node, char* keyptrs[])
    {
    pExpression criteria = NULL;
    pExpression item, operator;
    int i;

	/** Build the criteria expression tree **/
	criteria = NULL;
	for(i=0;i<node->nKeys;i++)
	    {
	    /** compare node **/
	    operator = expAllocExpression();
	    operator->NodeType = EXPR_N_COMPARE;
	    operator->CompareType = MLX_CMP_EQUALS;

	    /** property node **/
	    item = expAllocExpression();
	    item->NodeType = EXPR_N_PROPERTY;
	    item->Name = node->KeyNames[i];
	    item->ObjID = 0;
	    item->ObjCoverageMask = 1;
	    expAddNode(operator, item);

	    /** constant node **/
	    item = expAllocExpression();
	    item->DataType = node->KeyTypes[i];
	    item->NodeType = expDataTypeToNodeType(item->DataType);
	    if (item->DataType == DATA_T_INTEGER)
		item->Integer = strtoi(keyptrs[i], NULL, 10);
	    else
		{
		item->String = nmSysStrdup(keyptrs[i]);
		item->Alloc = 1;
		}
	    expAddNode(operator, item);

	    /** Link in with existing expression so far? **/
	    if (criteria)
		{
		item = operator;
		operator = expAllocExpression();
		operator->NodeType = EXPR_N_AND;
		expAddNode(operator, item);
		expAddNode(operator, criteria);
		}
	    criteria = operator;
	    }

    return criteria;
    }


/*** qyp_internal_ProcessOpen() - does the underlying work to take the
 *** given pivot object name and look up the underlying set of objects
 *** in the underlying data source to obtain the attributes for this
 *** pivot object name.
 ***/
int
qyp_internal_ProcessOpen(pQypData inf)
    {
    char keybuf[64];
    char* keyptrs[QYP_MAX_KEYS];
    int n_objname_keys;
    char *ptr;
    pExpression criteria;
    int i;
    pObject source_obj = NULL;
    pObjQuery source_qy = NULL;
    pObject source_subobj = NULL;
    pQypDatum one_datum = NULL;
    int rval = 0;

	/** Break up the object name into separate concat key values **/
	strtcpy(keybuf, inf->ObjName, sizeof(keybuf));
	n_objname_keys = 0;
	ptr = strtok(keybuf, "|");
	while (ptr && n_objname_keys < QYP_MAX_KEYS)
	    {
	    keyptrs[n_objname_keys] = ptr;
	    n_objname_keys++;
	    ptr = strtok(NULL,"|");
	    }

	/** Correct key count? **/
	if (n_objname_keys != inf->Node->nKeys)
	    {
	    mssError(1, "QYP", "Invalid concat key count in object name");
	    goto error;
	    }

	/** Get the selection criteria from the name **/
	criteria = qyp_internal_NameToExpression(inf->Node, keyptrs);

	/** Open the source object using the criteria **/
	source_obj = objOpen(inf->Obj->Session, inf->Node->SourcePath, O_RDONLY, 0600, "system/directory");
	if (!source_obj)
	    goto error;
	source_qy = objOpenQuery(source_obj, NULL, NULL, criteria, NULL);
	if (!source_qy)
	    goto error;

	/** Retrieve the resulting rows, one attribute/value per row... **/
	while((source_subobj = objQueryFetch(source_qy, O_RDONLY)) != NULL)
	    {
	    one_datum = qyp_internal_LoadDatum(inf, source_subobj);
	    if (!one_datum)
		goto error;

	    /** Add this datum to our object **/
	    xaAddItem(&inf->PivotData, (void*)one_datum);

	    objClose(source_subobj);
	    source_subobj = NULL;
	    }

	/** Clean up from the query **/
	objQueryClose(source_qy);
	source_qy = NULL;
	objClose(source_obj);
	source_obj = NULL;

	/** Row does not exist? **/
	if (inf->PivotData.nItems == 0)
	    {
	    /*mssError(1, "QYP", "Object '%s' does not exist", inf->ObjName);*/
	    rval = 1;
	    }

	/** Set up datums for the key values too **/
	for(i=0;i<inf->Node->nKeys;i++)
	    {
	    one_datum = qyp_internal_KeyToDatum(inf, keyptrs[i], i);
	    if (!one_datum)
		goto error;
	    xaInsertBefore(&inf->PivotData, i, (void*)one_datum);
	    }

	expFreeExpression(criteria);

	return rval;

    error:
	if (one_datum) nmFree(one_datum, sizeof(QypDatum));
	if (source_subobj) objClose(source_subobj);
	if (source_qy) objQueryClose(source_qy);
	if (source_obj) objClose(source_obj);
	if (criteria) expFreeExpression(criteria);
	return -1;
    }


/*** qyp_internal_ObjToObjname() - take an open subobject from the underlying
 *** data source, and create an object name divvied up by key elements.
 ***/
int
qyp_internal_ObjToObjname(pQypNode node, pObject obj, int maxlen, char* name, char* keyptrbuf, char* keyptrs[])
    {
    int i;
    int offset;
    char* strval;
    int intval;
    char intbuf[12];
    int len;

	/** Loop through the key values, copying them to the buffers **/
	offset = 0;
	for(i=0;i<node->nKeys;i++)
	    {
	    if (node->KeyTypes[i] == DATA_T_INTEGER)
		{
		if (objGetAttrValue(obj, node->KeyNames[i], DATA_T_INTEGER, POD(&intval)) != 0)
		    return -1;
		snprintf(intbuf, sizeof(intbuf), "%d", intval);
		strval = intbuf;
		}
	    else
		{
		if (objGetAttrValue(obj, node->KeyNames[i], DATA_T_STRING, POD(&strval)) != 0)
		    return -1;
		}
	    len = strlen(strval);
	    if (offset + len + 1 >= maxlen)
		return -1;

	    /** copy to the buffers **/
	    keyptrs[i] = keyptrbuf+offset;
	    strcpy(name+offset, strval);
	    name[offset+len] = '|';
	    strcpy(keyptrbuf+offset, strval);
	    offset += (len+1);
	    }
	if (offset) name[offset-1] = '\0';

    return 0;
    }


/*** qyp_internal_Update() - look through the row for any data that needs
 *** to be written back to the underlying source, and perform those updates.
 *** If a datum is marked DIRTY, then an update/SetAttrValue is done on the
 *** underlying source.  If marked both DIRTY and NEW, then a Create/insert
 *** is done on the underlying source.  Both flags are reset upon completion.
 *** Create and Modify user/date fields are also updated as needed.
 ***/
int
qyp_internal_Update(pQypData inf)
    {
    int i;
    pQypDatum datum, entity_datum;
    pObject source_obj = NULL;
    char source_path[OBJSYS_MAX_PATH + 1];
    int j;
    char* ptr;

	/** Loop through all data items **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];

	    /** We create/update source objects for all dirty datums that are
	     ** not key (entity) values.  Entity data is inherent to EVERY
	     ** source object.
	     **/
	    if ((datum->Flags & QYP_DATUM_F_DIRTY) && !(datum->Flags & QYP_DATUM_F_KEY))
		{
		objCurrentDate(&datum->ModifyDate);
		strtcpy(datum->ModifyBy, mssUserName(), sizeof(datum->ModifyBy));
		if (datum->Flags & QYP_DATUM_F_NEW)
		    {
		    /** new datum - do a Create **/
		    objCurrentDate(&datum->CreateDate);
		    strtcpy(datum->CreateBy, mssUserName(), sizeof(datum->CreateBy));
		    snprintf(source_path, sizeof(source_path), "%s/*", inf->Node->SourcePath);
		    source_obj = objOpen(inf->Obj->Session, source_path, O_RDWR | O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");

		    /** Set entity fields **/
		    for(j=0;j<inf->PivotData.nItems;j++)
			{
			entity_datum = (pQypDatum)inf->PivotData.Items[j];
			if (entity_datum->Flags & QYP_DATUM_F_KEY)
			    {
			    if (objSetAttrValue(source_obj, entity_datum->Name, entity_datum->Data.DataType, &entity_datum->Data.Data) < 0)
				goto error;
			    }
			}

		    /** Set attribute name field **/
		    ptr = datum->Name;
		    if (objSetAttrValue(source_obj, inf->Node->AttrNameField, DATA_T_STRING, POD(&ptr)) < 0)
			goto error;

		    /** Set Data type field, if applicable **/
		    if (inf->Node->ValueTypeField)
			{
			if (datum->Data.DataType > 0)
			    {
			    ptr = obj_type_names[datum->Data.DataType];
			    if (objSetAttrValue(source_obj, inf->Node->ValueTypeField, DATA_T_STRING, POD(&ptr)) < 0)
				goto error;
			    }
			}

		    /** Choose a usable value field, and set the value. **/
		    if (datum->Data.DataType > 0)
			{
			for(j=0;j<inf->Node->nValues;j++)
			    {
			    if (inf->Node->ValueTypes[j] == DATA_T_UNAVAILABLE || inf->Node->ValueTypes[j] == datum->Data.DataType)
				{
				if (objSetAttrValue(source_obj, inf->Node->ValueNames[j], datum->Data.DataType, &datum->Data.Data) == 0)
				    {
				    datum->SourceValueID = j;
				    break;
				    }
				}
			    }
			}

		    /** Update the create and modify user/date fields, if applicable **/
		    if (inf->Node->CreateUserField)
			{
			ptr = datum->CreateBy;
			objSetAttrValue(source_obj, inf->Node->CreateUserField, DATA_T_STRING, POD(&ptr));
			}
		    if (inf->Node->CreateDateField)
			objSetAttrValue(source_obj, inf->Node->CreateDateField, DATA_T_DATETIME, POD(&datum->CreateDate));
		    if (inf->Node->ModifyUserField)
			{
			ptr = datum->ModifyBy;
			objSetAttrValue(source_obj, inf->Node->ModifyUserField, DATA_T_STRING, POD(&ptr));
			}
		    if (inf->Node->ModifyDateField)
			objSetAttrValue(source_obj, inf->Node->ModifyDateField, DATA_T_DATETIME, POD(&datum->ModifyDate));

		    /** Commit the changes **/
		    if (objCommit(source_obj->Session) < 0)
			goto error;

		    /** Get the underlying object name **/
		    ptr = NULL;
		    if (objGetAttrValue(source_obj, "name", DATA_T_STRING, POD(&ptr)) != 0 || !ptr)
			goto error;
		    strtcpy(datum->SourceObjName, ptr, sizeof(datum->SourceObjName));
		    }
		else
		    {
		    /** existing but modified datum - do a SetAttrValue **/
		    if (datum->Flags & QYP_DATUM_F_KEY)
			{
			mssError(1,"QYP","Cannot update key field '%s'", datum->Name);
			goto error;
			}
		    snprintf(source_path, sizeof(source_path), "%s/%s", inf->Node->SourcePath, datum->SourceObjName);
		    source_obj = objOpen(inf->Obj->Session, source_path, O_RDWR, 0600, "system/object");
		    if (!source_obj)
			goto error;
		    if (datum->Data.Flags & DATA_TF_NULL)
			{
			if (objSetAttrValue(source_obj, inf->Node->ValueNames[datum->SourceValueID], datum->Data.DataType, NULL) < 0)
			    goto error;
			}
		    else
			{
			if (objSetAttrValue(source_obj, inf->Node->ValueNames[datum->SourceValueID], datum->Data.DataType, &datum->Data.Data) < 0)
			    goto error;
			}

		    /** Update the modify user/date fields, if applicable **/
		    if (inf->Node->ModifyUserField)
			{
			ptr = datum->ModifyBy;
			objSetAttrValue(source_obj, inf->Node->ModifyUserField, DATA_T_STRING, POD(&ptr));
			}
		    if (inf->Node->ModifyDateField)
			objSetAttrValue(source_obj, inf->Node->ModifyDateField, DATA_T_DATETIME, POD(&datum->ModifyDate));
		    }

		/** Close the object - we're done **/
		objClose(source_obj);
		source_obj = NULL;
		datum->Flags &= ~(QYP_DATUM_F_DIRTY | QYP_DATUM_F_NEW);
		}
	    }

	return 0;

    error:
	if (source_obj) objClose(source_obj);
	return -1;
    }


/*** qyp_internal_New() - create a new object.  This function is called when
 *** an objCommit or objClose is done after setattr's on a newly created
 *** object.
 ***/
int
qyp_internal_New(pQypData inf)
    {
    int i,j;
    pQypDatum datum;
    char* ptr;
    int offset;
    char intbuf[12];
    int len;

	/** Mark KEY datum items as NEW.  This is because they may have been
	 ** created from the provided object name, and not marked NEW then.
	 **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (datum->Flags & QYP_DATUM_F_KEY)
		datum->Flags |= QYP_DATUM_F_NEW;
	    }

	/** Send the updates to the underlying objects **/
	if (qyp_internal_Update(inf) < 0)
	    return -1;

	/** Create our object name **/
	if (!inf->ObjName[0])
	    {
	    offset = 0;
	    for(i=0;i<inf->Node->nKeys;i++)
		{
		for(j=0;j<inf->PivotData.nItems;j++)
		    {
		    datum = (pQypDatum)inf->PivotData.Items[j];
		    if (!strcmp(inf->Node->KeyNames[i], datum->Name))
			{
			if (datum->Data.DataType == DATA_T_INTEGER)
			    {
			    snprintf(intbuf,sizeof(intbuf),"%d",datum->Data.Data.Integer);
			    ptr = intbuf;
			    }
			else
			    {
			    ptr = datum->Data.Data.String;
			    }
			len = strlen(ptr);
			if (offset + len + 1 + 1 >= sizeof(inf->ObjName))
			    return -1;
			strcpy(inf->ObjName + offset, ptr);
			offset += len;
			strcpy(inf->ObjName + offset, "|");
			offset++;
			}
		    }
		}
	    if (offset) inf->ObjName[offset-1] = '\0';
	    }

	/** Object is no longer "new" **/
	inf->Flags &= ~QYP_F_NEW;

    return 0;
    }


/*** qypOpen - open a query pivot object.
 ***/
void*
qypOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pQypData inf;
    pSnNode node = NULL;
    char buf[1];
    pQypNode qyp_node;
    int rval;

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		mssError(0,"QYP","Could not create new querypivot node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    objRead(obj->Prev, buf, 0, 0, OBJ_U_SEEK);
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
	    mssError(0,"QYP","Could not open querypivot node object");
	    return NULL;
	    }

	/** Read in configuration for this node **/
	qyp_node = qyp_internal_ReadNode(obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr), node);
	if (!qyp_node)
	    {
	    return NULL;
	    }

	/** Allocate the structure **/
	inf = (pQypData)nmMalloc(sizeof(QypData));
	if (!inf) return NULL;
	memset(inf, 0, sizeof(QypData));
	xaInit(&inf->PivotData, 16);
	inf->Obj = obj;
	inf->Mask = mask;
	obj->SubCnt = obj->Pathname->nElements - obj->SubPtr + 1;
	inf->Node = qyp_node;
	inf->Node->BaseNode->OpenCnt++;

	/** Read in the row data? **/
	if (obj->SubPtr < obj->Pathname->nElements)
	    {
	    inf->Flags |= QYP_F_ISROW;
	    strtcpy(inf->ObjName, obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1), sizeof(inf->ObjName));

	    /** autoname requested? **/
	    if (!strcmp(inf->ObjName,"*") && (obj->Mode & OBJ_O_AUTONAME))
		inf->ObjName[0] = '\0';

	    /** only call processopen if we have something to go on... **/
	    if (inf->ObjName[0])
		rval = qyp_internal_ProcessOpen(inf);
	    else
		rval = 1;

	    /** Exit out on a fail condition **/
	    if (rval < 0 || (rval == 1 && !(obj->Mode & O_CREAT)) || (rval == 0 && (obj->Mode & O_EXCL)))
		{
		xaDeInit(&inf->PivotData);
		nmFree(inf, sizeof(QypData));
		return NULL;
		}

	    /** Creating a new object **/
	    if (rval == 1)
		inf->Flags |= QYP_F_NEW;
	    }
	obj_internal_PathPart(obj->Pathname,0,0);

    return (void*)inf;
    }


/*** qypClose - close an open file or directory.
 ***/
int
qypClose(void* inf_v, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum;

	/** Send any changes to the underlying data source **/
	if (inf->Flags & QYP_F_NEW)
	    qyp_internal_New(inf);

    	/** Write the node first **/
	snWriteNode(inf->Obj->Prev, inf->Node->BaseNode);

	/** Clear out the data attribute/values **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (datum->Data.DataType == DATA_T_STRING)
		nmSysFree(datum->Data.Data.String);
	    else if (datum->Data.DataType == DATA_T_DATETIME)
		nmFree(datum->Data.Data.DateTime, sizeof(DateTime));
	    else if (datum->Data.DataType == DATA_T_MONEY)
		nmFree(datum->Data.Data.Money, sizeof(MoneyType));
	    nmFree(datum, sizeof(QypDatum));
	    }

	/** Release the memory **/
	inf->Node->BaseNode->OpenCnt --;
	xaDeInit(&inf->PivotData);
	nmFree(inf,sizeof(QypData));

    return 0;
    }


/*** qypCreate - create a new file without actually opening that 
 *** file.
 ***/
int
qypCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = qypOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	qypClose(inf, oxt);

    return 0;
    }


/*** qypDeleteObj - delete an existing object that is already open.
 ***/
int
qypDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum;
    char delete_pathname[OBJSYS_MAX_PATH+1];

	/** Loop through the data items, deleting the source objects **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (!(datum->Flags & QYP_DATUM_F_KEY) && datum->SourceObjName[0])
		{
		/** Found an object to delete **/
		if (strlen(inf->Node->SourcePath) + strlen(datum->SourceObjName) + 1 + 1 >= sizeof(delete_pathname))
		    return -1;
		snprintf(delete_pathname,sizeof(delete_pathname),"%s/%s",inf->Node->SourcePath,datum->SourceObjName);
		if (objDelete(inf->Obj->Session, delete_pathname) < 0)
		    return -1;
		}
	    }

	qypClose(inf, oxt);

    return 0;
    }


/*** qypDelete - delete an existing file or directory.
 ***/
int
qypDelete(pObject obj, pObjTrxTree* oxt)
    {
    pQypData inf;

	inf = qypOpen(obj, 0600, NULL, "system/object", oxt);
	if (!inf) return -1;

    return qypDeleteObj(inf, oxt);
    }


/*** qypRead - Attempt to read from the underlying object.
 ***/
int
qypRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pQypData inf = QYP(inf_v);*/
    return -1;
    }


/*** qypWrite - As above, attempt to read if we found an actual object.
 ***/
int
qypWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pQypData inf = QYP(inf_v);*/
    return -1;
    }


/*** qypOpenQuery - open a new query.
 ***/
void*
qypOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    pQypQuery qy;
    XString orderby;
    int i;

	/** Queries not supported on row objects **/
	if (inf->Flags & QYP_F_ISROW)
	    return NULL;

	/** Allocate the query structure **/
	qy = (pQypQuery)nmMalloc(sizeof(QypQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(QypQuery));
	qy->ObjInf = inf;
	qy->Query = query;

	/** Build the order by list, so we get objects grouped by entitiy **/
	xsInit(&orderby);
	for(i=0;i<inf->Node->nKeys;i++)
	    {
	    if (i) xsConcatenate(&orderby, ", ", 2);
	    xsConcatenate(&orderby, ":'", 2);
	    if (strchr(inf->Node->KeyNames[i], '\''))
		return NULL;
	    xsConcatenate(&orderby, inf->Node->KeyNames[i], -1);
	    xsConcatenate(&orderby, "'", 1);
	    }

	/** Open the query **/
	qy->LLQueryObj = objOpen(inf->Obj->Session, inf->Node->SourcePath, O_RDONLY, 0600, "system/directory");
	if (!qy->LLQueryObj)
	    {
	    nmFree(qy, sizeof(QypQuery));
	    xsDeInit(&orderby);
	    return NULL;
	    }
	objUnmanageObject(inf->Obj->Session, qy->LLQueryObj);
	qy->LLQuery = objOpenQuery(qy->LLQueryObj, NULL, orderby.String, NULL, NULL);
	if (!qy->LLQuery)
	    {
	    objClose(qy->LLQueryObj);
	    nmFree(qy, sizeof(QypQuery));
	    xsDeInit(&orderby);
	    return NULL;
	    }
	objUnmanageQuery(inf->Obj->Session, qy->LLQuery);
	qy->NextSubobj = objQueryFetch(qy->LLQuery, O_RDONLY);
	if (qy->NextSubobj)
	    objUnmanageObject(inf->Obj->Session, qy->NextSubobj);

	xsDeInit(&orderby);

    return (void*)qy;
    }


/*** qypQueryFetch - get the next entry as an open object.
 ***/
void*
qypQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pQypQuery qy = ((pQypQuery)(qy_v));
    pQypData inf;
    char cur_objname[64];
    char cur_objnamebuf[64];
    char* cur_keyptrs[QYP_MAX_KEYS];
    char next_objname[64];
    char next_objnamebuf[64];
    char* next_keyptrs[QYP_MAX_KEYS];
    pQypDatum datum;
    int i;

	/** End of query already? **/
	if (!qy->NextSubobj)
	    return NULL;

	/** Note the object name **/
	if (qyp_internal_ObjToObjname(qy->ObjInf->Node, qy->NextSubobj, sizeof(cur_objname), cur_objname, cur_objnamebuf, cur_keyptrs) < 0)
	    return NULL;
	if (obj_internal_AddToPath(obj->Pathname, cur_objname) < 0)
	    return NULL;

	/** Allocate... **/
	inf = (pQypData)nmMalloc(sizeof(QypData));
	if (!inf)
	    return NULL;
	memset(inf, 0, sizeof(QypData));
	xaInit(&inf->PivotData, 16);
	strtcpy(inf->ObjName, cur_objname, sizeof(inf->ObjName));
	inf->Node = qy->ObjInf->Node;
	inf->Node->BaseNode->OpenCnt++;
	inf->Obj = obj;
	inf->Flags = QYP_F_ISROW;

	/** Load the attributes into the PivotData **/
	while(1)
	    {
	    datum = qyp_internal_LoadDatum(inf, qy->NextSubobj);
	    if (!datum)
		{
		qypClose(inf, NULL);
		return NULL;
		}
	    xaAddItem(&inf->PivotData, (void*)datum);
	    objClose(qy->NextSubobj);
	    qy->NextSubobj = objQueryFetch(qy->LLQuery, O_RDONLY);
	    if (!qy->NextSubobj)
		break;
	    objUnmanageObject(inf->Obj->Session, qy->NextSubobj);

	    /** Compare the object entity names **/
	    if (qyp_internal_ObjToObjname(qy->ObjInf->Node, qy->NextSubobj, sizeof(next_objname), next_objname, next_objnamebuf, next_keyptrs) < 0)
		{
		qypClose(inf, NULL);
		return NULL;
		}
	    if (strcmp(cur_objname, next_objname) != 0)
		break;
	    }

	/** Set up datums for the key values too **/
	for(i=0;i<inf->Node->nKeys;i++)
	    {
	    datum = qyp_internal_KeyToDatum(inf, cur_keyptrs[i], i);
	    if (!datum)
		{
		qypClose(inf, NULL);
		return NULL;
		}
	    xaInsertBefore(&inf->PivotData, i, (void*)datum);
	    }

    return (void*)inf;
    }


/*** qypQueryClose - close the query.
 ***/
int
qypQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pQypQuery qy = ((pQypQuery)(qy_v));

	/** Close our underlying data source **/
	if (qy->NextSubobj)
	    objClose(qy->NextSubobj);
	if (qy->LLQuery)
	    objQueryClose(qy->LLQuery);
	if (qy->LLQueryObj)
	    objClose(qy->LLQueryObj);

	/** Free the structure **/
	nmFree(qy,sizeof(QypQuery));

    return 0;
    }


/*** qypGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
qypGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	if (!strcmp(attrname,"last_modification")) return DATA_T_DATETIME;

	/** Look inside the list of attributes **/
	for(i=0; i<inf->PivotData.nItems; i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (!strcmp(attrname, datum->Name))
		{
		return datum->Data.DataType;
		}
	    }

	/*mssError(1,"QYP","Invalid attribute '%s' for querypivot object", attrname);*/

    return DATA_T_UNAVAILABLE;
    }


/*** qypGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
qypGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYP","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (!inf->ObjName[0])
		return 1; /* null while autoname still pending */
	    val->String = inf->ObjName;
	    return 0;
	    }

	/** annotation? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYP","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYP","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "system/row";
	    return 0;
	    }
	else if ((!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type")))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"QYP","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "system/void";
	    return 0;
	    }

	/** Look in the list of attributes **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (!strcmp(attrname, datum->Name))
		{
		if (datum->Data.Flags & DATA_TF_NULL)
		    return 1;
		if (datum->Data.DataType != datatype)
		    {
		    mssError(1,"QYP","Type mismatch accessing attribute '%s'", attrname);
		    return -1;
		    }
		switch(datum->Data.DataType)
		    {
		    case DATA_T_INTEGER:
			val->Integer = datum->Data.Data.Integer;
			break;
		    case DATA_T_DOUBLE:
			val->Double = datum->Data.Data.Double;
			break;
		    default: /* covers strings, date, money, etc... */
			val->Generic = datum->Data.Data.Generic;
			break;
		    }
		return 0;
		}
	    }

	if (!strcmp(attrname,"last_modification")) return 1; /* null */
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	mssError(1,"QYP","getattr: invalid attribute '%s' for querypivot object", attrname);

    return -1;
    }


/*** qypGetNextAttr - get the next attribute name for this object.
 ***/
char*
qypGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pQypData inf = QYP(inf_v);
    pQypDatum datum;

	if (inf->CurAttr >= inf->PivotData.nItems)
	    return NULL;

	datum = (pQypDatum)inf->PivotData.Items[inf->CurAttr++];

    return datum->Name;
    }


/*** qypGetFirstAttr - get the first attribute name for this object.
 ***/
char*
qypGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pQypData inf = QYP(inf_v);
    inf->CurAttr = 0;
    return qypGetNextAttr(inf, oxt);
    }


/*** qypSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
qypSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum = NULL;
    pQypDatum new_datum = NULL;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    /** unsupported **/
	    return -1;
	    }

	/** Try to find the attribute being set **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (!strcmp(datum->Name, attrname))
		{
		break;
		}
	    datum = NULL;
	    }

	/** No attribute found? Add a new one **/
	if (!datum)
	    {
	    if (datatype <= 0)
		return -1;
	    new_datum = (pQypDatum)nmMalloc(sizeof(QypDatum));
	    if (!new_datum)
		return -1;
	    memset(new_datum, 0, sizeof(QypDatum));
	    strtcpy(new_datum->Name, attrname, sizeof(new_datum->Name));
	    new_datum->Data.DataType = datatype;
	    new_datum->Flags = QYP_DATUM_F_NEW;
	    datum = new_datum;

	    /** Is this a key? **/
	    for(i=0;i<inf->Node->nKeys;i++)
		{
		if (!strcmp(attrname, inf->Node->KeyNames[i]))
		    {
		    if (datatype != inf->Node->KeyTypes[i])
			{
			mssError(1,"QYP","Type mismatch setting key attribute value for '%s'", attrname);
			goto error;
			}
		    new_datum->Flags |= QYP_DATUM_F_KEY;
		    break;
		    }
		}
	    }

	/** Modify the datum **/
	if ((datum->Flags & QYP_DATUM_F_KEY) && !(inf->Flags & QYP_F_NEW))
	    {
	    mssError(1,"QYP","Setting entity/key value '%s' not supported on existing objects",attrname);
	    goto error;
	    }
	if (datum->Data.DataType != datatype)
	    {
	    mssError(1,"QYP","Type mismatch setting value '%s'", attrname);
	    goto error;
	    }
	datum->Flags |= QYP_DATUM_F_DIRTY;
	if (!val)
	    {
	    if (datum->Flags & QYP_DATUM_F_KEY)
		{
		mssError(1,"QYP","Key value '%s' cannot be null",attrname);
		goto error;
		}
	    datum->Data.Flags |= DATA_TF_NULL;
	    }
	else
	    {
	    datum->Data.Flags &= ~DATA_TF_NULL;
	    switch(datatype)
		{
		case DATA_T_INTEGER:
		    datum->Data.Data.Integer = val->Integer;
		    break;
		case DATA_T_DOUBLE:
		    datum->Data.Data.Double = val->Double;
		    break;
		case DATA_T_STRING:
		    if (datum->Data.Data.String) nmSysFree(datum->Data.Data.String);
		    datum->Data.Data.String = nmSysStrdup(val->String);
		    break;
		case DATA_T_DATETIME:
		    if (!datum->Data.Data.DateTime) datum->Data.Data.DateTime = nmMalloc(sizeof(DateTime));
		    memcpy(datum->Data.Data.DateTime, val->DateTime, sizeof(DateTime));
		    break;
		case DATA_T_MONEY:
		    if (!datum->Data.Data.Money) datum->Data.Data.Money = nmMalloc(sizeof(MoneyType));
		    memcpy(datum->Data.Data.Money, val->Money, sizeof(MoneyType));
		    break;
		default:
		    goto error;
		}
	    }

	/** If this is a new one, add it to the list **/
	if (new_datum)
	    xaAddItem(&inf->PivotData, (void*)new_datum);
	new_datum = NULL;

	/** Send the changes to the underlying object, unless entire object is
	 ** new, in which case we wait until objCommit() or objClose().
	 **/
	if (!(inf->Flags & QYP_F_NEW))
	    {
	    if (qyp_internal_Update(inf) < 0)
		goto error;
	    }

	return 0;

    error:
	if (new_datum) nmFree(new_datum,sizeof(QypDatum));
	return -1;
    }


/*** qypAddAttr - add an attribute to an object.  Passthrough to lowlevel.
 ***/
int
qypAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    /*pQypData inf = QYP(inf_v);*/

	/** Not yet supported **/

    return -1;
    }


/*** qypOpenAttr - open an attribute as an object.  Passthrough.
 ***/
void*
qypOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    /*pQypData inf = QYP(inf_v);*/

	/** Not supported **/

    return NULL;
    }


/*** qypGetFirstMethod -- passthrough.
 ***/
char*
qypGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    /*pQypData inf = QYP(inf_v);*/

	/** Not supported **/

    return NULL;
    }


/*** qypGetNextMethod -- passthrough.
 ***/
char*
qypGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    /*pQypData inf = QYP(inf_v);*/

	/** Not supported **/

    return NULL;
    }


/*** qypExecuteMethod - passthrough.
 ***/
int
qypExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    /*pQypData inf = QYP(inf_v);*/

	/** Not supported **/

    return -1;
    }


/*** qypCommit - send any unsaved changes or batched changes (such as multiple
 *** setattr's on an insert) to the underlying objects.
 ***/
int
qypCommit(void* inf_v, pObjTrxTree *oxt)
    {
    pQypData inf = QYP(inf_v);

	if (inf->Flags & QYP_F_NEW)
	    return qyp_internal_New(inf);

    return 0;
    }


/*** qypInfo - Return the capabilities of the object.
 ***/
int
qypInfo(void* inf_v, pObjectInfo info)
    {
    pQypData inf = QYP(inf_v);

	if (inf->Flags & QYP_F_ISROW)
	    {
	    info->Flags = OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ;
	    info->nSubobjects = 0;
	    }
	else
	    {
	    info->Flags = OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	    info->nSubobjects = 0;
	    }

    return 0;
    }


/*** qypPresentationHints - return the hints associated with the
 *** underlying object.
 ***/
pObjPresentationHints
qypPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQypData inf = QYP(inf_v);
    int i;
    pQypDatum datum = NULL;

	/** Find the datum **/
	for(i=0;i<inf->PivotData.nItems;i++)
	    {
	    datum = (pQypDatum)inf->PivotData.Items[i];
	    if (!strcmp(datum->Name, attrname))
		break;
	    datum = NULL;
	    }
	if (!datum) return NULL;

	/** Key? **/
	if (datum->Flags & QYP_DATUM_F_KEY)
	    {
	    for(i=0;i<inf->Node->nKeys;i++)
		{
		if (!strcmp(inf->Node->KeyNames[i], attrname) && inf->Node->KeyHints[i])
		    return objDuplicateHints(inf->Node->KeyHints[i]);
		}
	    return NULL;
	    }

	/** Regular data **/
	if (inf->Node->ValueHints[datum->SourceValueID])
	    return objDuplicateHints(inf->Node->ValueHints[datum->SourceValueID]);

    return NULL;
    }


/*** qypInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
qypInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&QYP_INF,0,sizeof(QYP_INF));
	xhInit(&QYP_INF.NodeCache, 255, 0);

	/** Setup the structure **/
	strcpy(drv->Name,"QYP - QueryPivot Translation Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/querypivot");

	/** Setup the function references. **/
	drv->Open = qypOpen;
	drv->Close = qypClose;
	drv->Create = qypCreate;
	drv->Delete = qypDelete;
	drv->DeleteObj = qypDeleteObj;
	drv->OpenQuery = qypOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = qypQueryFetch;
	drv->QueryClose = qypQueryClose;
	drv->Read = qypRead;
	drv->Write = qypWrite;
	drv->GetAttrType = qypGetAttrType;
	drv->GetAttrValue = qypGetAttrValue;
	drv->GetFirstAttr = qypGetFirstAttr;
	drv->GetNextAttr = qypGetNextAttr;
	drv->SetAttrValue = qypSetAttrValue;
	drv->AddAttr = qypAddAttr;
	drv->OpenAttr = qypOpenAttr;
	drv->GetFirstMethod = qypGetFirstMethod;
	drv->GetNextMethod = qypGetNextMethod;
	drv->ExecuteMethod = qypExecuteMethod;
	drv->Commit = qypCommit;
	drv->Info = qypInfo;
	drv->PresentationHints = qypPresentationHints;

	/** Register some structures **/
	nmRegister(sizeof(QypData),"QypData");
	nmRegister(sizeof(QypQuery),"QypQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

