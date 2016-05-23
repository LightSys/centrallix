#include "net_http.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2014 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_http_rest.c              				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 5, 2014	      					*/
/* Description:	REST implementation for HTTP interface.			*/
/************************************************************************/


/*** Some enums to control how we're responding to the request ***/
typedef enum { ResTypeCollection, ResTypeElement, ResTypeBoth } nhtResType_t;
typedef enum { ResFormatAttrs, ResFormatAuto, ResFormatContent, ResFormatBoth } nhtResFormat_t;
typedef enum { ResAttrsBasic, ResAttrsFull, ResAttrsNone } nhtResAttrs_t;

int nht_internal_RestGetElementContent(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type);
int nht_internal_RestGetElement(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type);
int nht_internal_RestGetCollection(pNhtConn conn, pObject obj, nhtResType_t res_type, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int levels);
int nht_internal_RestGetBoth(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int res_levels, char* mime_type);


/*** nht_internal_RestGetElementContent() - get a REST element's content.
 ***/
int
nht_internal_RestGetElementContent(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type)
    {
    char xfer_buf[256];
    int rcnt, wcnt;

	/** Read the object's content and send it to the network connection **/
	while ((rcnt = objRead(obj, xfer_buf, sizeof(xfer_buf), 0, 0)) > 0)
	    {
	    /** Try to write all of the data that was read to the network **/
	    wcnt = fdWrite(conn->ConnFD, xfer_buf, rcnt, 0, FD_U_PACKET);

	    /** If we wrote less than we read, then something just burped on
	     ** the network connection.
	     **/
	    if (wcnt < rcnt)
		break;
	    }

    return 0;
    }



/*** nht_internal_RestWriteAttrValue() - write one attribute value in JSON
 *** format.
 ***/
int
nht_internal_RestWriteAttrValue(pNhtConn conn, pObject obj, char* attrname, int data_type)
    {
    ObjData od;
    int rval;

	/** Get the data value **/
	rval = objGetAttrValue(obj, attrname, data_type, &od);
	if (rval != 0)
	    {
	    /** NULL or an error condition -- just send a null value **/
	    fdPrintf(conn->ConnFD, "null");
	    return rval;
	    }

	/** Format it based on the data type **/
	switch(data_type)
	    {
	    case DATA_T_INTEGER:
		fdPrintf(conn->ConnFD, "%d", od.Integer);
		break;

	    case DATA_T_STRING:
		fdQPrintf(conn->ConnFD, "\"%STR&JSONSTR\"", od.String);
		break;

	    case DATA_T_DOUBLE:
		fdQPrintf(conn->ConnFD, "%DBL", od.Double);
		break;

	    case DATA_T_DATETIME:
		fdQPrintf(conn->ConnFD, "{ \"year\":%INT, \"month\":%INT, \"day\":%INT%[, \"hour\":%INT, \"minute\":%INT, \"second\":%INT%] }",
			od.DateTime->Part.Year + 1900,
			od.DateTime->Part.Month+1,
			od.DateTime->Part.Day+1,
			(od.DateTime->Part.Hour || od.DateTime->Part.Minute || od.DateTime->Part.Second),
			od.DateTime->Part.Hour,
			od.DateTime->Part.Minute,
			od.DateTime->Part.Second
			);
		break;

	    case DATA_T_MONEY:
		fdQPrintf(conn->ConnFD, "{ \"wholepart\":%INT, \"fractionpart\":%INT }",
			od.Money->WholePart,
			od.Money->FractionPart
			);
		break;

	    default:
		/** Unknown or unimplemented data type **/
		fdPrintf(conn->ConnFD, "null");
		return -1;
	    }
	
    return 0;
    }



/*** nht_internal_RestWriteAttr() - write one attribute into JSON format.  On
 *** entry here, res_attrs will be either ResAttrsBasic or ResAttrsFull.
 ***/
int
nht_internal_RestWriteAttr(pNhtConn conn, pObject obj, char* attrname, nhtResAttrs_t res_attrs, int do_comma)
    {
    int data_type;
    int err;
    pObjPresentationHints hints;
    XString hints_str;

	/** Get data type of the attribute **/
	data_type = objGetAttrType(obj, attrname);
	if (data_type < 0 || data_type == DATA_T_UNAVAILABLE)
	    return -1;

	/** Comma separated from previous attr **/
	if (do_comma)
	    fdPrintf(conn->ConnFD, ",\r\n");

	/** Name of attribute **/
	fdQPrintf(conn->ConnFD, "\"%STR&JSONSTR\":", attrname);

	/** Attribute format **/
	if (res_attrs == ResAttrsBasic)
	    {
	    /** Basic form **/
	    err = nht_internal_RestWriteAttrValue(conn, obj, attrname, data_type);
	    }
	else
	    {
	    /** Full form **/
	    fdPrintf(conn->ConnFD, " { \"v\":");
	    err = nht_internal_RestWriteAttrValue(conn, obj, attrname, data_type);
	    fdQPrintf(conn->ConnFD, ", \"t\":\"%STR&JSONSTR\"", obj_type_names[data_type]);
	    if (err < 0)
		fdPrintf(conn->ConnFD, ", \"e\":\"error\"");
	    hints = objPresentationHints(obj, attrname);
	    if (hints)
		{
		xsInit(&hints_str);
		if (hntEncodeHints(hints, &hints_str) > 0)
		    {
		    fdQPrintf(conn->ConnFD, ", \"h\":\"%STR&JSONSTR\"", hints_str.String);
		    }
		xsDeInit(&hints_str);
		objFreeHints(hints);
		}
	    fdPrintf(conn->ConnFD, " }");
	    }

    return 0;
    }



/*** nht_internal_RestGetElement() - get a REST element, which will be
 *** the list of its attributes, possibly including an objcontent attribute.
 ***
 *** On entry here, res_format will be ResFormatAttrs or ResFormatBoth.
 *** res_attrs could be ResAttrsBasic, ResAttrsFull, or ResAttrsNone.
 ***/
int
nht_internal_RestGetElement(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type)
    {
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];
    char* attr;
    char xfer_buf[256];
    int rcnt;
    char* sys_attrs[] = { "name", "annotation", "inner_type", "outer_type" };
    char sent_sys_attrs[sizeof(sys_attrs)/sizeof(char*)];
    int i;

	/** We begin our object with just a { **/
	fdPrintf(conn->ConnFD, "{\r\n");

	/** First thing we need to output is the URI of the element **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	fdQPrintf(conn->ConnFD, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=full%]\"",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs == ResAttrsFull)
		);

	/** Write any attrs? **/
	if (res_attrs != ResAttrsNone)
	    {
	    /** Keep track of any sys attrs that were sent with the main ones **/
	    memset(sent_sys_attrs, 0, sizeof(sent_sys_attrs));

	    /** We loop through the main attributes that are iterable **/
	    for(attr=objGetFirstAttr(obj); attr; attr=objGetNextAttr(obj))
		{
		for(i=0; i<sizeof(sent_sys_attrs); i++)
		    {
		    if (!strcmp(attr, sys_attrs[i]))
			{
			sent_sys_attrs[i] = 1;
			break;
			}
		    }
		nht_internal_RestWriteAttr(conn, obj, attr, res_attrs, 1);
		}

	    /** Next, we output some system (hidden) attributes **/
	    for(i=0; i<sizeof(sent_sys_attrs); i++)
		{
		if (!sent_sys_attrs[i])
		    nht_internal_RestWriteAttr(conn, obj, sys_attrs[i], res_attrs, 1);
		}

	    /** Content? **/
	    if (res_format == ResFormatBoth)
		{
		fdPrintf(conn->ConnFD, ",\r\n\"cx__objcontent\":\"");
		while((rcnt = objRead(obj, xfer_buf, sizeof(xfer_buf), 0, 0)) > 0)
		    {
		    fdQPrintf(conn->ConnFD, "%*STR&JSONSTR", rcnt, xfer_buf);
		    }
		fdPrintf(conn->ConnFD, "\"");
		}
	    }

	/** And end it with } **/
	fdPrintf(conn->ConnFD, "\r\n}\r\n");

    return 0;
    }



/*** nht_internal_RestGetCollection() - get a REST collection, which will be
 *** a list of subobjects for the given object, with or without details on
 *** the attributes for each one.
 ***/
int
nht_internal_RestGetCollection(pNhtConn conn, pObject obj, nhtResType_t res_type, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int levels)
    {
    pObjQuery query;
    pObject subobj;
    char* objname;
    char* objtype;
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];

	/** We open our list with just a { **/
	fdPrintf(conn->ConnFD, "{\r\n");

	/** First thing we need to output is the URI of the collection **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	fdQPrintf(conn->ConnFD, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_type=collection&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=%STR&JSONSTR%]\"\r\n",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs != ResAttrsNone),
		(res_attrs == ResAttrsFull)?"full":"basic"
		);

	/** Open the query and loop through subobjects **/
	query = objOpenQuery(obj, "", NULL, NULL, NULL);
	if (query)
	    {
	    while((subobj = objQueryFetch(query, O_RDONLY)) != NULL)
		{
		if (objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&objname)) == 0)
		    {
		    /** Print the name of the object. **/
		    fdQPrintf(conn->ConnFD, ",\"%STR&JSONSTR\":", objname);

		    /** Print it as an Element.  The res_format and res_attrs
		     ** settings are set to provide logical defaults for doing
		     ** this.
		     **/
		    if (objGetAttrValue(subobj, "inner_type", DATA_T_STRING, POD(&objtype)) != 0)
			objtype = "system/void";
		    if (levels <= 1)
			{
			nht_internal_RestGetElement(conn, subobj, res_format, res_attrs, objtype);
			}
		    else
			{
			if (res_type == ResTypeBoth)
			    nht_internal_RestGetBoth(conn, subobj, res_format, res_attrs, levels-1, objtype);
			else
			    nht_internal_RestGetCollection(conn, subobj, ResTypeCollection, res_format, res_attrs, levels-1);
			}
		    objClose(subobj);
		    }
		}
	    objQueryClose(query);
	    }

	/** And close it with } **/
	fdPrintf(conn->ConnFD, "}\r\n");

    return 0;
    }



/*** nht_internal_RestGetBoth() - get both an element and a collection, combined
 *** into a two-part JSON document.
 ***/
int
nht_internal_RestGetBoth(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int res_levels, char* mime_type)
    {
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];

	/** We open our list with just a { **/
	fdPrintf(conn->ConnFD, "{\r\n");

	/** First thing we need to output is the URI of the collection/element **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	fdQPrintf(conn->ConnFD, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_type=both&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=%STR&JSONSTR%]\"\r\n",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs != ResAttrsNone),
		(res_attrs == ResAttrsFull)?"full":"basic"
		);

	/** Output the element **/
	fdPrintf(conn->ConnFD, ",\"cx__element\":");
	nht_internal_RestGetElement(conn, obj, res_format, res_attrs, mime_type);

	/** Output the collection **/
	fdPrintf(conn->ConnFD, ",\"cx__collection\":");
	nht_internal_RestGetCollection(conn, obj, ResTypeBoth, res_format, res_attrs, res_levels);

	/** Close the JSON container **/
	fdPrintf(conn->ConnFD, "}\r\n");

    return 0;
    }



/*** nht_internal_RestGet() - perform a RESTful GET operation, returning
 *** JSON data or just a normal document.
 ***/
int
nht_internal_RestGet(pNhtConn conn, pStruct url_inf, pObject obj)
    {
    int rval = 0;
    char mime_type[64];
    char* ptr;
    nhtResType_t res_type = ResTypeElement;
    nhtResFormat_t res_format;
    nhtResAttrs_t res_attrs;
    pObjectInfo info;
    int can_have_content = 1;
    int res_levels = 1;

	/** Get the MIME type of the object **/
	if (objGetAttrValue(obj, "inner_type", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(mime_type, ptr, sizeof(mime_type));
	else
	    strcpy(mime_type, "system/void");

	/** Can the object have content? **/
	if ((info = objInfo(obj)) != NULL)
	    {
	    if (info->Flags & OBJ_INFO_F_CAN_HAVE_CONTENT)
		can_have_content = 1;
	    if (info->Flags & OBJ_INFO_F_CANT_HAVE_CONTENT)
		can_have_content = 0;
	    }

	/** Resource Type -- element or collection? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_type"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "collection"))
		res_type = ResTypeCollection;
	    else if (!strcmp(ptr, "element"))
		res_type = ResTypeElement;
	    else if (!strcmp(ptr, "both"))
		res_type = ResTypeBoth;
	    }
	if (res_type == ResTypeCollection)
	    {
	    /** Default settings for collections **/
	    res_format = ResFormatAttrs;
	    res_attrs = ResAttrsNone;
	    }
	else if (res_type == ResTypeElement)
	    {
	    /** Default settings for Elements **/
	    res_format = ResFormatContent;
	    res_attrs = ResAttrsBasic;
	    }
	else
	    {
	    /** Default for "both" resource type **/
	    res_format = ResFormatAttrs;
	    res_attrs = ResAttrsBasic;
	    }

	/** Resource Format -- attributes or content? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_format"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "attrs"))
		res_format = ResFormatAttrs;
	    else if (!strcmp(ptr, "auto"))
		res_format = ResFormatAuto;
	    else if (!strcmp(ptr, "content"))
		res_format = ResFormatContent;
	    else if (!strcmp(ptr, "both"))
		res_format = ResFormatBoth;
	    }

	/** Attribute format -- basic or full? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_attrs"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "basic"))
		res_attrs = ResAttrsBasic;
	    else if (!strcmp(ptr, "full"))
		res_attrs = ResAttrsFull;
	    else if (!strcmp(ptr, "none"))
		res_attrs = ResAttrsNone;
	    }

	/** Number of levels for collection|both resources **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_levels"), &ptr) == 0)
	    {
	    res_levels = strtol(ptr, NULL, 10);
	    if (res_levels <= 0) res_levels = 1;
	    }

	/** Sanity check -- collection and content are incompatible **/
	if ((res_type == ResTypeCollection || res_type == ResTypeBoth) && res_format == ResFormatContent)
	    res_format = ResFormatAttrs;

	/** If collection and 'auto' are both set, use 'attrs' instead **/
	if ((res_type == ResTypeCollection || res_type == ResTypeBoth) && res_format == ResFormatAuto)
	    res_format = ResFormatAttrs;

	/** If we're in 'auto' mode, choose the right option based on the
	 ** ability of the object to have content.
	 **/
	if (res_format == ResFormatAuto)
	    {
	    if (can_have_content)
		res_format = ResFormatContent;
	    else
		res_format = ResFormatAttrs;
	    }

	/** Don't cache response.  Yes, I know this is REST and it is supposed
	 ** to be nicely cacheable, but this data is known to change frequently.
	 **/
	fdPrintf(conn->ConnFD, "Pragma: no-cache\r\n");

	/** MIME type of response **/
	if (res_type == ResTypeElement && res_format == ResFormatContent)
	    fdPrintf(conn->ConnFD, "Content-Type: %s\r\n", mime_type);
	else
	    fdPrintf(conn->ConnFD, "Content-Type: application/json\r\n");

	/** End of headers - we don't have anything else to add at this point. **/
	fdPrintf(conn->ConnFD, "\r\n");

	/** Ok, generate the document **/
	if (res_type == ResTypeBoth)
	    {
	    rval = nht_internal_RestGetBoth(conn, obj, res_format, res_attrs, res_levels, mime_type);
	    }
	else if (res_type == ResTypeElement)
	    {
	    if (res_format == ResFormatContent)
		rval = nht_internal_RestGetElementContent(conn, obj, res_format, res_attrs, mime_type);
	    else
		rval = nht_internal_RestGetElement(conn, obj, res_format, res_attrs, mime_type);
	    }
	else
	    {
	    rval = nht_internal_RestGetCollection(conn, obj, ResTypeCollection, res_format, res_attrs, res_levels);
	    }

    return rval;
    }



/*** nht_internal_RestSetOneAttr() - set one attribute given its JSON
 *** new value representation
 ***/
int
nht_internal_RestSetOneAttr(pObject obj, char* attrname, struct json_object* j_attr_obj)
    {
    int n;
    char* str;
    double d;
    int rval = -1;
    int t;
    struct json_object *j_part;
    DateTime dt;
    MoneyType m;
    ObjData od;

	/** Type of json element? **/
	if (json_object_is_type(j_attr_obj, json_type_int))
	    {
	    n = json_object_get_int(j_attr_obj);
	    rval = objSetAttrValue(obj, attrname, DATA_T_INTEGER, POD(&n));
	    }
	else if (json_object_is_type(j_attr_obj, json_type_string))
	    {
	    str = (char*)json_object_get_string(j_attr_obj);
	    rval = objSetAttrValue(obj, attrname, DATA_T_STRING, POD(&str));
	    }
	else if (json_object_is_type(j_attr_obj, json_type_double))
	    {
	    d = json_object_get_double(j_attr_obj);
	    rval = objSetAttrValue(obj, attrname, DATA_T_DOUBLE, POD(&d));
	    }
	else if (json_object_is_type(j_attr_obj, json_type_null))
	    {
	    t = objGetAttrType(obj, attrname);
	    if (t <= 0)
		return 0;
	    rval = objSetAttrValue(obj, attrname, t, NULL);
	    }
	else if (json_object_is_type(j_attr_obj, json_type_object))
	    {
	    /** Ok, we have an object.  Determine whether it is money or datetime **/
	    if (json_object_object_get_ex(j_attr_obj, "wholepart", &j_part))
		{
		/** Money type **/
		if (jutilGetMoneyObject(j_attr_obj, &m) < 0)
		    return -1;
		od.Money = &m;
		rval = objSetAttrValue(obj, attrname, DATA_T_MONEY, &od);
		}
	    else
		{
		/** Date/Time Type **/
		if (jutilGetDateTimeObject(j_attr_obj, &dt) < 0)
		    return -1;
		od.DateTime = &dt;
		rval = objSetAttrValue(obj, attrname, DATA_T_DATETIME, &od);
		}
	    }

    return rval;
    }



/*** nht_internal_RestPatch() - perform a RESTful PATCH operation, then
 *** return the modified document.
 ***/
int
nht_internal_RestPatch(pNhtConn conn, pStruct url_inf, pObject obj, struct json_object* j_obj)
    {
    char* ptr;
    char* attrname;
    nhtResType_t res_type = ResTypeElement;
    struct json_object_iter iter;
    struct json_object* j_attr_obj;
    struct tm* thetime;
    time_t tval;
    char tbuf[32];

	/** Resource Type -- element or collection? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_type"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "collection"))
		res_type = ResTypeCollection;
	    else if (!strcmp(ptr, "element"))
		res_type = ResTypeElement;
	    }

	/** Only elements are allowed **/
	if (res_type != ResTypeElement)
	    {
	    mssError(1,"NHT","Cannot PATCH a REST collection");
	    return -1;
	    }

	/** Must be an object containing attributes to be set **/
	if (!json_object_is_type(j_obj, json_type_object))
	    {
	    mssError(1,"NHT","JSON data for PATCH must be a JSON object");
	    return -1;
	    }

	/** Loop through attributes to be set **/
	json_object_object_foreachC(j_obj, iter)
	    {
	    j_attr_obj = iter.val;
	    attrname = iter.key;
	    if (nht_internal_RestSetOneAttr(obj, attrname, j_attr_obj) < 0)
		return -1;
	    }

	/** Ok, issue the HTTP header for this one. **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	fdQPrintf(conn->ConnFD,
		"HTTP/1.0 200 OK\r\n"
		"Date: %STR GMT\r\n"
		"Server: %STR\r\n"
		"%[Set-Cookie: %STR; path=/\r\n%]", 
		 tbuf, NHT.ServerString, conn->NhtSession->IsNewCookie, conn->NhtSession->Cookie);
	conn->NhtSession->IsNewCookie = 0;

	/** Call out to the GET functionality to return the modified document **/
	nht_internal_RestGet(conn, url_inf, obj);

    return 0;
    }



/*** nht_internal_RestPost() - perform a RESTful POST operation, then
 *** return the newly created document.  A POST will perform an autoname
 *** create operation, and the new name of the created object will be a part
 *** of the @id property of the returned document.
 ***/
int
nht_internal_RestPost(pNhtConn conn, pStruct url_inf, int size, char* content)
    {
    char* ptr;
    pObject target_obj;
    struct json_tokener* jtok = NULL;
    enum json_tokener_error jerr;
    char rbuf[256];
    int rcnt, total_rcnt;
    struct json_object*	j_obj = NULL;
    char new_obj_path[OBJSYS_MAX_PATH+1];
    char new_obj_name[OBJSYS_MAX_PATH+1];
    struct tm* thetime;
    time_t tval;
    char tbuf[32];
    char* msg;
    nhtResType_t res_type = ResTypeElement;
    nhtResFormat_t res_format;
    nhtResAttrs_t res_attrs;
    struct json_object_iter iter;
    struct json_object* j_attr_obj;
    char* attrname;

	/** Open the target object **/
	if (strlen(url_inf->StrVal) + 2 + 1 >= OBJSYS_MAX_PATH)
	    {
	    mssError(0,"NHT","REST POST pathname too long");
	    msg = "400 Bad Request";
	    goto error;
	    }
	snprintf(new_obj_path, sizeof(new_obj_path), "%s/*", url_inf->StrVal);
	target_obj = objOpen(conn->NhtSession->ObjSess, new_obj_path, OBJ_O_RDWR | OBJ_O_CREAT | OBJ_O_AUTONAME | OBJ_O_EXCL | OBJ_O_TRUNC, 0600, "application/octet-stream");
	if (!target_obj)
	    {
	    mssError(0,"NHT","Could not open requested object for POST request");
	    msg = "404 Not Found";
	    goto error;
	    }

	/** Initialize the JSON tokenizer **/
	jtok = json_tokener_new();
	if (!jtok)
	    {
	    mssError(1,"NHT","Could not initialize JSON parser");
	    msg = "500 Internal Server Error";
	    goto error;
	    }

	/** Supplied as a URL parameter? **/
	if (content)
	    {
	    j_obj = json_tokener_parse_ex(jtok, content, strlen(content));
	    jerr = json_tokener_get_error(jtok);
	    }
	else
	    {
	    /** Read the document from the connection **/
	    total_rcnt = 0;
	    do  {
		rcnt = fdRead(conn->ConnFD, rbuf, sizeof(rbuf), 0, 0);
		if (rcnt <= 0)
		    {
		    mssError(1,"NHT","Could not read JSON object from HTTP connection");
		    msg = "400 Bad Request";
		    goto error;
		    }
		total_rcnt += rcnt;
		if (total_rcnt > NHT_PAYLOAD_MAX)
		    {
		    mssError(1,"NHT","JSON object too large");
		    msg = "400 Bad Request";
		    goto error;
		    }
		j_obj = json_tokener_parse_ex(jtok, rbuf, rcnt);
		} while((jerr = json_tokener_get_error(jtok)) == json_tokener_continue);
	    }

	/** Success? **/
	if (!j_obj || jerr != json_tokener_success)
	    {
	    mssError(1,"NHT","Invalid JSON object in POST request");
	    msg = "400 Bad Request";
	    goto error;
	    }
	json_tokener_free(jtok);
	jtok = NULL;

	/** Resource Type -- element or collection? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_type"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "collection"))
		res_type = ResTypeCollection;
	    else if (!strcmp(ptr, "element"))
		res_type = ResTypeElement;
	    }

	/** Only elements are allowed **/
	if (res_type != ResTypeCollection)
	    {
	    mssError(1,"NHT","REST POST must be to a collection");
	    msg = "400 Bad Request";
	    goto error;
	    }

	/** Must be an object containing attributes to be set **/
	if (!json_object_is_type(j_obj, json_type_object))
	    {
	    mssError(1,"NHT","JSON data for POST must be a JSON object");
	    msg = "400 Bad Request";
	    goto error;
	    }

	/** Loop through attributes to be set **/
	json_object_object_foreachC(j_obj, iter)
	    {
	    j_attr_obj = iter.val;
	    attrname = iter.key;
	    if (nht_internal_RestSetOneAttr(target_obj, attrname, j_attr_obj) < 0)
		return -1;
	    }
	//objCommit(conn->NhtSession->ObjSess);
	objCommitObject(target_obj);

	/** Get the new name **/
	if (objGetAttrValue(target_obj, "name", DATA_T_STRING, POD(&ptr)) != 0 || strchr(ptr, '?') || !strcmp(ptr,"*"))
	    {
	    mssError(0,"NHT","Could not autoname new object for POST request");
	    msg = "500 Internal Server Error";
	    goto error;
	    }
	if (strlen(new_obj_path) + strlen(ptr) + 2 >= sizeof(new_obj_name))
	    {
	    mssError(1,"NHT","Path too long for new object for POST request");
	    msg = "500 Internal Server Error";
	    goto error;
	    }
	if (strlen(new_obj_path) >= 2)
	    {
	    /** trim the trailing slash and star **/
	    new_obj_path[strlen(new_obj_path)-2] = '\0';
	    }
	snprintf(new_obj_name, sizeof(new_obj_name), "%s/%s", new_obj_path, ptr);

	/** Do the reopen **/
	objClose(target_obj);
	target_obj = objOpen(conn->NhtSession->ObjSess, new_obj_name, OBJ_O_RDONLY, 0600, "application/octet-stream");
	if (!target_obj)
	    {
	    mssError(0,"NHT","Could not confirm newly created object");
	    msg = "500 Internal Server Error";
	    goto error;
	    }

	/** Ok, issue the HTTP header for this one. **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	fdQPrintf(conn->ConnFD,
		"HTTP/1.0 201 Created\r\n"
		"Date: %STR GMT\r\n"
		"Server: %STR\r\n"
		"Location: %STR&PATH?cx__mode=rest&cx__res_type=element&cx__res_format=attrs&cx__res_attrs=basic\r\n"
		"%[Set-Cookie: %STR; path=/\r\n%]"
		"Pragma: no-cache\r\n"
		"Content-Type: application/json\r\n\r\n",
		 tbuf, 
		 NHT.ServerString,
		 /*url_inf->StrVal,*/ new_obj_name,
		 conn->NhtSession->IsNewCookie, conn->NhtSession->Cookie);
	conn->NhtSession->IsNewCookie = 0;

	/** Resource Format -- attributes or content? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_format"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "attrs"))
		res_format = ResFormatAttrs;
	    else if (!strcmp(ptr, "auto"))
		res_format = ResFormatAuto;
	    else if (!strcmp(ptr, "both"))
		res_format = ResFormatBoth;
	    else /* "content" */
		res_format = ResFormatContent;
	    }

	/** Attribute format -- basic or full? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx__res_attrs"), &ptr) == 0)
	    {
	    if (!strcmp(ptr, "full"))
		res_attrs = ResAttrsFull;
	    else if (!strcmp(ptr, "none"))
		res_attrs = ResAttrsNone;
	    else /* "basic" */
		res_attrs = ResAttrsBasic;
	    }

	/** Call out to the GET functionality to return the created document **/
	nht_internal_RestGetElement(conn, target_obj, res_format, res_attrs, "system/void");

	/** Cleanup and return **/
	json_object_put(j_obj);
	objClose(target_obj);
	return 0;

    error:
	fdPrintf(conn->ConnFD,
		"HTTP/1.0 %s\r\n"
		"Server: %s\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<H1>%s</H1>\r\n",
		msg, NHT.ServerString, msg);

	if (jtok)
	    json_tokener_free(jtok);
	if (j_obj)
	    json_object_put(j_obj);
	if (target_obj)
	    objClose(target_obj);

	return -1;
    }



/*** nht_internal_RestDelete() - perform a RESTful DELETE operation.
 ***/
int
nht_internal_RestDelete(pNhtConn conn, pStruct url_inf, pObject obj)
    {
    char* msg;

	/** Attempt to delete the object **/
	if (objDeleteObj(obj) < 0)
	    {
	    mssError(0,"NHT","REST DELETE operation failed");
	    goto error;
	    }

	return 0;

    error:
	if (obj)
	    objClose(obj);

	return -1;
    }

