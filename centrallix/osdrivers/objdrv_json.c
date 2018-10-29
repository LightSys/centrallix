#include <stdio.h>
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
/** module definintions **/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#include <sys/types.h>
#include "json/json.h"
#include "json/json_util.h"

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
/* Module: 	JSON Objectsystem driver				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 15, 2013				    	*/
/* Description:	Provides access to JSON encoded data through the	*/
/*		Centrallix ObjectSystem.				*/
/*									*/
/*		Current Shortcomings:					*/
/*		  - JSON strings can contain NULs.  CX ones cannot.	*/
/*		  - JSON data cannot yet be modified.			*/
/*									*/
/************************************************************************/


/** the element used in the document cache **/
typedef struct
    {
    struct json_object*	JObj;
    DateTime		LastModified;
    int			LinkCnt;
    char		Pathname[OBJSYS_MAX_PATH];
    }
    JsonCacheObj, *pJsonCacheObj;


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char		Pathname[OBJSYS_MAX_PATH];
    int			Flags;
    pObject		Obj;
    int			Mask;
    struct json_object*	CurNode;
    char		CurNodeName[OBJSYS_MAX_PATH];
    int			Offset;
    int			CurAttr;
    pJsonCacheObj	CacheObj;
    char*		ContentType;
    DateTime		Date;
    MoneyType		Money;
    }
    JsonData, *pJsonData;


#define JSON(x) ((pJsonData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pJsonData	Data;
    int		ItemCnt;
    int		IterCnt;
    XArray	ItemList;
    XArray	NameList;
    }
    JsonQuery, *pJsonQuery;


/*** GLOBALS ***/
struct
    {
    XHashTable	Cache;
    }
    JSON_INF;


struct json_object*
json_internal_Get(struct json_object* jobj)
    {
    struct json_object* rval;
    rval = json_object_get(jobj);
    //fprintf(stderr, "GET JOBJ %8.8llx ref %d\n", jobj, ((int*)(jobj))[6]);
    return rval;
    }

void
json_internal_Put(struct json_object* jobj)
    {
    int cnt = ((int*)(jobj))[6];
    json_object_put(jobj);
    //fprintf(stderr, "PUT JOBJ %8.8llx ref %d\n", jobj, cnt);
    return;
    }


int
json_internal_CacheUnlink(pJsonCacheObj cache_obj)
    {

	cache_obj->LinkCnt--;
	json_internal_Put(cache_obj->JObj);

	/** Release if the link cnt went to zero **/
	if (cache_obj->LinkCnt <= 0)
	    {
	    nmFree(cache_obj, sizeof(JsonCacheObj));
	    }

    return 0;
    }


int
json_internal_CacheLink(pJsonCacheObj cache_obj)
    {

	cache_obj->LinkCnt++;
	json_internal_Get(cache_obj->JObj);

    return 0;
    }


/*** json_internal_ReadDoc - read a new json document or else look up an
 *** existing parsed one in the document cache
 ***/
pJsonCacheObj
json_internal_ReadDoc(pObject obj)
    {
    char* path;
    pJsonCacheObj cache_obj = NULL;
    pDateTime dt = NULL;
    struct json_tokener* jtok = NULL;
    enum json_tokener_error jerr;
    char rbuf[256];
    int rcnt;
    int first_read;

	/** Already cached? **/
	path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);
	if ((cache_obj = (pJsonCacheObj)xhLookup(&JSON_INF.Cache, path)))
	    {
	    /** Found match in cache -- check modification time **/
	    if (objGetAttrValue(obj->Prev, "last_modification", DATA_T_DATETIME, POD(&dt)) == 0)
		{
		if (!cache_obj->JObj || (dt && dt->Value != cache_obj->LastModified.Value))
		    {
		    /** modification time changed -- discard cached copy **/
		    //fprintf(stderr, "DONE JOBJ %8.8llx ref %d\n", cache_obj->JObj, ((int*)(cache_obj->JObj))[6]);
		    xhRemove(&JSON_INF.Cache, cache_obj->Pathname);
		    json_internal_CacheUnlink(cache_obj);
		    cache_obj = NULL;
		    }
		}
	    }

	/** Newly parsed document? **/
	if (!cache_obj)
	    {
	    cache_obj = (pJsonCacheObj)nmMalloc(sizeof(JsonCacheObj));
	    if (!cache_obj)
		goto error;
	    memset(cache_obj, 0, sizeof(JsonCacheObj));
	    cache_obj->LinkCnt = 1;
	    strtcpy(cache_obj->Pathname, path, sizeof(cache_obj->Pathname));
	    xhAdd(&JSON_INF.Cache, cache_obj->Pathname, (void*)cache_obj);
	    if (objGetAttrValue(obj->Prev, "last_modification", DATA_T_DATETIME, POD(&dt)) != 0)
		dt = NULL;
	    }

	/** Set modification date **/
	if (dt)
	    memcpy(&cache_obj->LastModified, dt, sizeof(DateTime));

	/** Parse document **/
	if (!cache_obj->JObj)
	    {
	    jtok = json_tokener_new();
	    if (!jtok)
		{
		mssError(1,"JSON","Could not initialize JSON parser");
		goto error;
		}

	    /** Parse it one chunk at a time **/
	    first_read = 1;
	    do  {
		rcnt = objRead(obj->Prev, rbuf, sizeof(rbuf), 0, first_read?OBJ_U_SEEK:0);
		if (rcnt < 0 || (rcnt == 0 && first_read))
		    {
		    mssError(0,"JSON","Could not read JSON document");
		    goto error;
		    }
		if (rcnt == 0)
		    break;
		cache_obj->JObj = json_tokener_parse_ex(jtok, rbuf, rcnt);
		first_read = 0;
		} while((jerr = json_tokener_get_error(jtok)) == json_tokener_continue);
	    //if (cache_obj->JObj)
		//fprintf(stderr, "NEW JOBJ %8.8llx ref %d\n", cache_obj->JObj, ((int*)(cache_obj->JObj))[6]);

	    /** Got it? **/
	    if (jerr == json_tokener_continue)
		{
		mssError(1,"JSON","Incomplete JSON document read");
		goto error;
		}
	    if (jerr != json_tokener_success)
		{
		mssError(1,"JSON","Error processing JSON document: %s", json_tokener_error_desc(jerr));
		goto error;
		}
	    json_tokener_free(jtok);
	    jtok = NULL;
	    }

	/** Return the parsed JSON document **/
	json_internal_CacheLink(cache_obj);
	return cache_obj;

    error:
	if (jtok)
	    json_tokener_free(jtok);
	return NULL;
    }


/*** json_internal_IsSubobjectNode() - returns 1 if the json node should be
 *** handled as a Centrallix sub-object; 0 otherwise.
 ***/
int
json_internal_IsSubobjectNode(struct json_object* jobj)
    {
    return json_object_is_type(jobj, json_type_array) || (json_object_is_type(jobj, json_type_object) && !jutilIsDateTimeObject(jobj) && !jutilIsMoneyObject(jobj));
    }


/*** jsonOpen - open an object.
 ***/
void*
jsonOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pJsonData inf = NULL;
    char* pathelement;
    struct json_object* starting_obj;
    int idx;
    char* endptr;

	/** Allocate the structure **/
	inf = (pJsonData)nmMalloc(sizeof(JsonData));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(JsonData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Set object params. **/
	strtcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0), sizeof(inf->Pathname));

	/** Obtain the parsed document **/
	if (!(inf->CacheObj=json_internal_ReadDoc(obj)))
	    {
	    nmFree(inf,sizeof(JsonData));
	    return NULL;
	    }

	/** Walk the JSON tree to find the obj or array being requested.  It is not an
	 ** error if we do not find the entire path - we can leave that potentially for
	 ** another driver.  If the OSML doesn't find another driver to finish things
	 ** off, then it will generate an error then.
	 **/
	obj->SubCnt = 1;
	inf->CurNode = inf->CacheObj->JObj;
	while(1)
	    {
	    /** Find the next element of the path **/
	    pathelement = obj_internal_PathPart(obj->Pathname, obj->SubPtr + obj->SubCnt - 1, 1);
	    if (!pathelement)
		{
		/** No more path elements - we're done. **/
		break;
		}
	    strtcpy(inf->CurNodeName, pathelement, sizeof(inf->CurNodeName));

	    /** Find the path element in the JSON tree **/
	    starting_obj = inf->CurNode;
	    if (json_object_is_type(starting_obj, json_type_object))
		{
		if (json_object_object_get_ex(starting_obj, pathelement, &inf->CurNode) == (json_bool)0)
		    {
		    /** Not found **/
		    inf->CurNode = starting_obj;
		    break;
		    }
		}
	    else if (json_object_is_type(starting_obj, json_type_array))
		{
		endptr = pathelement;
		idx = strtol(pathelement, &endptr, 10);
		if (*pathelement == '\0' || *endptr != '\0')
		    {
		    /** Not found - invalid form **/
		    break;
		    }
		inf->CurNode = json_object_array_get_idx(starting_obj, idx);
		if (!inf->CurNode)
		    {
		    /** Not found **/
		    inf->CurNode = starting_obj;
		    break;
		    }
		}
	    else
		{
		/** Can't handle this type **/
		break;
		}
	    obj->SubCnt++;
	    }

	/** Set content type based on what we found **/
	if (json_object_is_type(inf->CurNode, json_type_string))
	    inf->ContentType = "application/octet-stream";
	else if (json_object_is_type(inf->CurNode, json_type_array) || json_object_is_type(inf->CurNode, json_type_object))
	    inf->ContentType = "system/void";
	else
	    {
	    mssError(1,"JSON","Invalid direct reference to non-string/array/object JSON tree element '%s'", inf->CurNodeName);
	    goto error;
	    }

	return (void*)inf;

    error:
	if (inf)
	    {
	    if (inf->CacheObj)
		{
		json_internal_CacheUnlink(inf->CacheObj);
		inf->CacheObj = NULL;
		}
	    nmFree(inf, sizeof(JsonData));
	    }
	return NULL;
    }


/*** jsonClose - close an open object.
 ***/
int
jsonClose(void* inf_v, pObjTrxTree* oxt)
    {
    pJsonData inf = JSON(inf_v);

	/** Release the memory **/
	if (inf->CacheObj)
	    {
	    json_internal_CacheUnlink(inf->CacheObj);
	    inf->CacheObj = NULL;
	    }
	nmFree(inf,sizeof(JsonData));

    return 0;
    }


/*** jsonCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
jsonCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = jsonOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	jsonClose(inf, oxt);

    return 0;
    }


/*** jsonDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
jsonDelete(pObject obj, pObjTrxTree* oxt)
    {
    /** Unimplemented **/
    return -1;
    }


/*** jsonRead - Read from the JSON element
 ***/
int
jsonRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pJsonData inf = JSON(inf_v);
    int len;
    char* str;
    int rcnt;

	/** Cannot read if this is not a string **/
	if (!json_object_is_type(inf->CurNode, json_type_string))
	    {
	    mssError(1,"JSON","Cannot read content from non-string JSON tree element");
	    goto error;
	    }

	/** Set seek position? **/
	len = json_object_get_string_len(inf->CurNode);
	if (flags & OBJ_U_SEEK)
	    {
	    if (offset < 0 || offset > len)
		{
		mssError(1,"JSON","Invalid seek offset");
		goto error;
		}
	    inf->Offset = offset;
	    }

	/** Copy the string data **/
	str = (char*)json_object_get_string(inf->CurNode);
	rcnt = maxcnt;
	if (rcnt > len - inf->Offset)
	    rcnt = len - inf->Offset;
	if (rcnt)
	    {
	    memcpy(buffer, str + inf->Offset, rcnt);
	    inf->Offset += rcnt;
	    }

	return rcnt;

    error:
	return -1;
    }


/*** jsonWrite - Write to the JSON element
 ***/
int
jsonWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pJsonData inf = JSON(inf_v);*/
    /** Unimplemented **/
    return -1;
    }


/*** jsonOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
jsonOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pJsonData inf = JSON(inf_v);
    pJsonQuery qy;
    struct json_object_iter iter;
    int i,len;
    char nbuf[16];
    struct json_object* jobj;

	/** Check type **/
	if (!(json_object_is_type(inf->CurNode, json_type_array) || json_object_is_type(inf->CurNode, json_type_object)))
	    {
	    mssError(1,"JSON","Object '%s' does not support queries", inf->CurNodeName);
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pJsonQuery)nmMalloc(sizeof(JsonQuery));
	if (!qy)
	    return NULL;
	memset(qy, 0, sizeof(JsonQuery));
	qy->Data = inf;
	qy->ItemCnt = qy->IterCnt = 0;
	xaInit(&qy->ItemList, 16);
	xaInit(&qy->NameList, 16);

	/** Grab the list of items from the json tree. **/
	if (json_object_is_type(inf->CurNode, json_type_object))
	    {
	    /** Iterate over an object **/
	    json_object_object_foreachC(inf->CurNode, iter)
		{
		if (json_internal_IsSubobjectNode(iter.val))
		    {
		    json_internal_Get(iter.val);
		    xaAddItem(&qy->ItemList, (void*)iter.val);
		    xaAddItem(&qy->NameList, nmSysStrdup(iter.key));
		    }
		}
	    }
	else if (json_object_is_type(inf->CurNode, json_type_array))
	    {
	    /** Iterate over an array **/
	    len = json_object_array_length(inf->CurNode);
	    for(i=0;i<len;i++)
		{
		snprintf(nbuf, sizeof(nbuf), "%d", i);
		jobj = json_object_array_get_idx(inf->CurNode, i);
		if (!jobj)
		    return NULL; /* should never happen */
		if (json_internal_IsSubobjectNode(jobj))
		    {
		    json_internal_Get(jobj);
		    xaAddItem(&qy->ItemList, (void*)jobj);
		    xaAddItem(&qy->NameList, nmSysStrdup(nbuf));
		    }
		}
	    }
	qy->ItemCnt = qy->ItemList.nItems;

    return (void*)qy;
    }


/*** jsonQueryFetch - get the next directory entry as an open object.
 ***/
void*
jsonQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pJsonQuery qy = ((pJsonQuery)(qy_v));
    pJsonData inf;

	/** Past end of results? **/
	if (qy->IterCnt >= qy->ItemCnt)
	    return NULL;

	/** Alloc the structure **/
	inf = (pJsonData)nmMalloc(sizeof(JsonData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(JsonData));
	inf->CacheObj = qy->Data->CacheObj;
	inf->CurNode = qy->ItemList.Items[qy->IterCnt];
	strtcpy(inf->CurNodeName, qy->NameList.Items[qy->IterCnt], sizeof(inf->CurNodeName));
	qy->IterCnt++;
	inf->Obj = obj;
	inf->Mask = qy->Data->Mask;

	/** Build path name **/
	if (obj_internal_AddToPath(obj->Pathname, inf->CurNodeName) < 0)
	    return NULL;

	/** Link **/
	json_internal_CacheLink(inf->CacheObj);

    return (void*)inf;
    }


/*** jsonQueryClose - close the query.
 ***/
int
jsonQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pJsonQuery qy = ((pJsonQuery)(qy_v));
    int i;

	/** Release the names and objects **/
	for(i=0;i<qy->ItemCnt;i++)
	    {
	    nmSysFree((char*)qy->NameList.Items[i]);
	    json_internal_Put((struct json_object*)qy->ItemList.Items[i]);
	    }

	/** Free the structure **/
	xaDeInit(&qy->ItemList);
	xaDeInit(&qy->NameList);
	nmFree(qy,sizeof(JsonQuery));

    return 0;
    }


/*** jsonGetAttrType - get the type (DATA_T_json) of an attribute by name.
 ***/
int
jsonGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pJsonData inf = JSON(inf_v);
    struct json_object* jval;

    	/** Some basic attribute types **/
	if (!strcmp(attrname,"last_modification")) return DATA_T_DATETIME;

	/** literal escape? **/
	if (!strncmp(attrname, "__cx_literal_", 13))
	    attrname += 13;

	/** Search for the key **/
	if (json_object_object_get_ex(inf->CurNode, attrname, &jval) == (json_bool)0)
	    return DATA_T_UNAVAILABLE;

	/** Return type based on jval **/
	if (json_object_is_type(jval, json_type_int))
	    return DATA_T_INTEGER;
	else if (json_object_is_type(jval, json_type_boolean))
	    return DATA_T_INTEGER;
	else if (json_object_is_type(jval, json_type_string))
	    return DATA_T_STRING;
	else if (json_object_is_type(jval, json_type_double))
	    return DATA_T_DOUBLE;
	else if (jutilIsDateTimeObject(jval))
	    return DATA_T_DATETIME;
	else if (jutilIsMoneyObject(jval))
	    return DATA_T_MONEY;
	else if (json_object_is_type(jval, json_type_null))
	    return DATA_T_STRING; /* or, should this be DATA_T_UNAVAILABLE? */

    return -1;
    }


/*** jsonGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
jsonGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pJsonData inf = JSON(inf_v);
    struct json_object* jval;

	/** inner_type is an alias for content_type **/
	if (!strcmp(attrname,"inner_type") || !strcmp(attrname, "content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->ContentType;
	    return 0;
	    }
    
	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->CurNodeName;
	    return 0;
	    }

	/** If outer type, and it wasn't specified in the JSON **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "application/x-json-node";
	    return 0;
	    }

	/** take last_modification from underlying object if it has one **/
	if (!strcmp(attrname,"last_modification"))
	    {
	    if (datatype != DATA_T_DATETIME)
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s' (should be datetime)", attrname);
		return -1;
		}
	    val->DateTime = &inf->CacheObj->LastModified;
	    return 0;
	    }

	/** literal escape? **/
	if (!strncmp(attrname, "__cx_literal_", 13))
	    attrname += 13;

	/** Try to find the json attribute object **/
	if (json_object_object_get_ex(inf->CurNode, attrname, &jval) == (json_bool)1)
	    {
	    /** Null? **/
	    if (json_object_is_type(jval, json_type_null))
		return 1;
	    else if (json_object_is_type(jval, json_type_int) && datatype == DATA_T_INTEGER)
		val->Integer = json_object_get_int(jval);
	    else if (json_object_is_type(jval, json_type_boolean) && datatype == DATA_T_INTEGER)
		val->Integer = (int)json_object_get_boolean(jval);
	    else if (json_object_is_type(jval, json_type_string) && datatype == DATA_T_STRING)
		val->String = (char*)json_object_get_string(jval);
	    else if (json_object_is_type(jval, json_type_double) && datatype == DATA_T_DOUBLE)
		val->Double = json_object_get_double(jval);
	    else if (jutilIsDateTimeObject(jval) && datatype == DATA_T_DATETIME)
		{
		val->DateTime = &inf->Date;
		if (jutilGetDateTimeObject(jval, &inf->Date) < 0)
		    return -1;
		}
	    else if (jutilIsMoneyObject(jval) && datatype == DATA_T_MONEY)
		{
		val->Money = &inf->Money;
		if (jutilGetMoneyObject(jval, &inf->Money) < 0)
		    return -1;
		}
	    else
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s'", attrname);
		return -1;
		}
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"JSON","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

    return 1; /* null if not there presently */
    }


/*** jsonGetNextAttr - get the next attribute name for this object.
 ***/
char*
jsonGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pJsonData inf = JSON(inf_v);
    int i;
    struct json_object_iter iter;

	/** Only allowed on objects **/
	if (!json_object_is_type(inf->CurNode, json_type_object))
	    return NULL;

	inf->CurAttr++;

	/** Search for the attribute **/
	i = 1;
	json_object_object_foreachC(inf->CurNode, iter)
	    {
	    if (!json_internal_IsSubobjectNode(iter.val))
		{
		if (inf->CurAttr == i)
		    return iter.key;
		i++;
		}
	    }

    return NULL;
    }


/*** jsonGetFirstAttr - get the first attribute name for this object.
 ***/
char*
jsonGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pJsonData inf = JSON(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = jsonGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** jsonSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
jsonSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    /*pJsonData inf = JSON(inf_v);*/
    return -1;
    }


/*** jsonAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
jsonAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    /*pJsonData inf = JSON(inf_v);*/
    return -1;
    }


/*** jsonOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
jsonOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** jsonGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
jsonGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** jsonGetNextMethod -- same as above.  Always fails. 
 ***/
char*
jsonGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** jsonExecuteMethod - No methods to execute, so this fails.
 ***/
int
jsonExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** jsonInfo - Return the capabilities of the object
 ***/
int
jsonInfo(void* inf_v, pObjectInfo info)
    {
    pJsonData inf = JSON(inf_v);

	if (json_internal_IsSubobjectNode(inf->CurNode))
	    {
	    info->Flags = ( OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
		OBJ_INFO_F_CANT_HAVE_CONTENT | OBJ_INFO_F_NO_CONTENT );
	    }
	else if (json_object_is_type(inf->CurNode, json_type_string))
	    {
	    info->Flags = ( OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
		OBJ_INFO_F_CAN_HAVE_CONTENT | OBJ_INFO_F_HAS_CONTENT | OBJ_INFO_F_CAN_SEEK_FULL |
		OBJ_INFO_F_CAN_SEEK_REWIND );
	    }
	else
	    {
	    info->Flags = ( OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
		OBJ_INFO_F_CANT_HAVE_CONTENT | OBJ_INFO_F_NO_CONTENT );
	    }

    return 0;
    }


/*** jsonInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
jsonInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&JSON_INF,0,sizeof(JSON_INF));
	xhInit(&JSON_INF.Cache,17,0);

	/** Setup the structure **/
	strcpy(drv->Name,"JSON - Javascript Object Notation OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	xaAddItem(&(drv->RootContentTypes),"text/json");
	xaAddItem(&(drv->RootContentTypes),"text/x-json");
	xaAddItem(&(drv->RootContentTypes),"application/json");

	/** Setup the function references. **/
	drv->Open = jsonOpen;
	drv->Close = jsonClose;
	drv->Create = jsonCreate;
	drv->Delete = jsonDelete;
	drv->OpenQuery = jsonOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = jsonQueryFetch;
	drv->QueryClose = jsonQueryClose;
	drv->Read = jsonRead;
	drv->Write = jsonWrite;
	drv->GetAttrType = jsonGetAttrType;
	drv->GetAttrValue = jsonGetAttrValue;
	drv->GetFirstAttr = jsonGetFirstAttr;
	drv->GetNextAttr = jsonGetNextAttr;
	drv->SetAttrValue = jsonSetAttrValue;
	drv->AddAttr = jsonAddAttr;
	drv->OpenAttr = jsonOpenAttr;
	drv->GetFirstMethod = jsonGetFirstMethod;
	drv->GetNextMethod = jsonGetNextMethod;
	drv->ExecuteMethod = jsonExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = jsonInfo;

	nmRegister(sizeof(JsonData),"JsonData");
	nmRegister(sizeof(JsonQuery),"JsonQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;


    return 0;
    }

