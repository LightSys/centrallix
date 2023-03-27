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

int nht_i_RestGetElementContent(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type);
int nht_i_RestGetElement(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type);
int nht_i_RestGetCollection(pNhtConn conn, pObject obj, nhtResType_t res_type, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int levels, pStruct url_inf);
int nht_i_RestGetBoth(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int res_levels, char* mime_type, pStruct url_inf);


/*** nht_i_EncodeCriteria() - look for criteria in the url_inf and encode
 *** them into a query criteria string.  Returns NULL if no criteria were
 *** found.
 ***/
 pXString
 nht_i_EncodeCriteria(pStruct url_inf)
    {
    pXString xs = NULL;
    int i, n_criteria = 0, n, j;
    pStruct criteria_inf;
    char* val;
    char* op;
    char* ops[] = { "=", "!=", "<", "<=", ">", ">=", "*", "_*", "*_", NULL };


	/** Loop through criteria looking for non-cx__xxyyzz params **/
	for(i=0; i<url_inf->nSubInf; i++)
	    {
	    criteria_inf = url_inf->SubInf[i];
	    if (criteria_inf->Type == ST_T_ATTRIB && strncmp(criteria_inf->Name, "cx__", 4) != 0)
		{
		if (stAttrValue_ne(criteria_inf, &val) == 0)
		    {
		    n_criteria++;
		    if (!xs)
			{
			xs = xsNew();
			if (!xs) return NULL;
			}

		    /** Comparison operator supplied? **/
		    op = NULL;
		    for(j=0; ops[j]; j++)
			{
			if (!strncmp(val, ops[j], strlen(ops[j])) && val[strlen(ops[j])] == ':')
			    {
			    op = ops[j];
			    val += (strlen(ops[j]) + 1);
			    break;
			    }
			}

		    /** Build the criteria **/
		    if (op && !strcmp(op, "*"))
			{
			xsConcatQPrintf(xs, "%STRcharindex(%STR&QUOT, :%STR&QUOT) > 0", (n_criteria > 1)?" and ":"", val, criteria_inf->Name);
			}
		    else if (op && !strcmp(op, "_*"))
			{
			xsConcatQPrintf(xs, "%STRcharindex(%STR&QUOT, :%STR&QUOT) == 1", (n_criteria > 1)?" and ":"", val, criteria_inf->Name);
			}
		    else if (op && !strcmp(op, "*_"))
			{
			xsConcatQPrintf(xs, "%STRcharindex(%STR&QUOT, :%STR&QUOT) == char_length(:%STR&QUOT) - char_length(%STR&QUOT) + 1", (n_criteria > 1)?" and ":"", val, criteria_inf->Name, criteria_inf->Name, val);
			}
		    else
			{
			xsConcatQPrintf(xs, "%STR:%STR&QUOT %STR ", (n_criteria > 1)?" and ":"", criteria_inf->Name, op?op:"=");

			/** What type are we dealing with?  This duck-typing isn't ideal,
			 ** but we don't currently have a better way of working this out.
			 **/
			if (strlen(val) == strspn(val, "-0123456789"))
			    {
			    /** integer **/
			    n = strtol(val, NULL, 10);
			    xsConcatQPrintf(xs, "%INT", n);
			    }
			else
			    {
			    /** string **/
			    xsConcatQPrintf(xs, "%STR&QUOT", val);
			    }
			}
		    }
		}
	    }

    return xs;
    }


/*** nht_i_RestGetElementContent() - get a REST element's content.
 ***/
int
nht_i_RestGetElementContent(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type)
    {
    char xfer_buf[256];
    int rcnt, wcnt;

	/** Read the object's content and send it to the network connection **/
	while ((rcnt = objRead(obj, xfer_buf, sizeof(xfer_buf), 0, 0)) > 0)
	    {
	    /** Try to write all of the data that was read to the network **/
	    wcnt = nht_i_WriteConn(conn, xfer_buf, rcnt, 0);

	    /** If we wrote less than we read, then something just burped on
	     ** the network connection.
	     **/
	    if (wcnt < rcnt)
		break;
	    }

    return 0;
    }



/*** nht_i_RestWriteAttrValue() - write one attribute value in JSON
 *** format.
 ***/
int
nht_i_RestWriteAttrValue(pNhtConn conn, pObject obj, char* attrname, int data_type)
    {
    ObjData od;
    int rval;

	/** Get the data value **/
	rval = objGetAttrValue(obj, attrname, data_type, &od);
	if (rval != 0)
	    {
	    /** NULL or an error condition -- just send a null value **/
	    nht_i_WriteConn(conn, "null", 4, 0);
	    return rval;
	    }

	/** Format it based on the data type **/
	switch(data_type)
	    {
	    case DATA_T_INTEGER:
		nht_i_QPrintfConn(conn, 0, "%INT", od.Integer);
		break;

	    case DATA_T_STRING:
		nht_i_QPrintfConn(conn, 0, "\"%STR&JSONSTR\"", od.String);
		break;

	    case DATA_T_DOUBLE:
		nht_i_QPrintfConn(conn, 0, "%DBL", od.Double);
		break;

	    case DATA_T_DATETIME:
		nht_i_QPrintfConn(conn, 0, "{ \"year\":%INT, \"month\":%INT, \"day\":%INT%[, \"hour\":%INT, \"minute\":%INT, \"second\":%INT%] }",
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
		nht_i_QPrintfConn(conn, 0, "{ \"wholepart\":%INT, \"fractionpart\":%INT }",
			od.Money->WholePart,
			od.Money->FractionPart
			);
		break;

	    default:
		/** Unknown or unimplemented data type **/
		nht_i_WriteConn(conn, "null", 4, 0);
		return -1;
	    }
	
    return 0;
    }



/*** nht_i_RestWriteAttr() - write one attribute into JSON format.  On
 *** entry here, res_attrs will be either ResAttrsBasic or ResAttrsFull.
 ***/
int
nht_i_RestWriteAttr(pNhtConn conn, pObject obj, char* attrname, nhtResAttrs_t res_attrs, int do_comma)
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
	    nht_i_WriteConn(conn, ",\r\n", -1, 0);

	/** Name of attribute **/
	nht_i_QPrintfConn(conn, 0, "\"%STR&JSONSTR\":", attrname);

	/** Attribute format **/
	if (res_attrs == ResAttrsBasic)
	    {
	    /** Basic form **/
	    err = nht_i_RestWriteAttrValue(conn, obj, attrname, data_type);
	    }
	else
	    {
	    /** Full form **/
	    nht_i_WriteConn(conn, " { \"v\":", -1, 0);
	    err = nht_i_RestWriteAttrValue(conn, obj, attrname, data_type);
	    nht_i_QPrintfConn(conn, 0, ", \"t\":\"%STR&JSONSTR\"", obj_type_names[data_type]);
	    if (err < 0)
		nht_i_WriteConn(conn, ", \"e\":\"error\"", -1, 0);
	    hints = objPresentationHints(obj, attrname);
	    if (hints)
		{
		xsInit(&hints_str);
		if (hntEncodeHints(hints, &hints_str) > 0)
		    {
		    nht_i_QPrintfConn(conn, 0, ", \"h\":\"%STR&JSONSTR\"", hints_str.String);
		    }
		xsDeInit(&hints_str);
		objFreeHints(hints);
		}
	    nht_i_WriteConn(conn, " }", 2, 0);
	    }

    return 0;
    }



/*** nht_i_RestGetElement() - get a REST element, which will be
 *** the list of its attributes, possibly including an objcontent attribute.
 ***
 *** On entry here, res_format will be ResFormatAttrs or ResFormatBoth.
 *** res_attrs could be ResAttrsBasic, ResAttrsFull, or ResAttrsNone.
 ***/
int
nht_i_RestGetElement(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, char* mime_type)
    {
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];
    char* attr;
    char xfer_buf[255];
    int rcnt, rcnt2, padcnt;
    char* sys_attrs[] = { "name", "annotation", "inner_type", "outer_type" };
    char sent_sys_attrs[sizeof(sys_attrs)/sizeof(char*)];
    int i;

	/** We begin our object with just a { **/
	nht_i_WriteConn(conn, "{\r\n", -1, 0);

	/** First thing we need to output is the URI of the element **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	nht_i_QPrintfConn(conn, 0, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=full%]\"",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs == ResAttrsFull)
		);

	/** Write any attrs? **/
	if (res_attrs != ResAttrsNone)
	    {
	    /** Content?  If so, we do content first in order to ensure metadata
	     ** values are set; with some content the metadata values are generated
	     ** during content generation.
	     **/
	    if (res_format == ResFormatBoth)
		{
		nht_i_WriteConn(conn, ",\r\n\"cx__objcontent\":\"", -1, 0);
		while((rcnt = objRead(obj, xfer_buf, sizeof(xfer_buf), 0, 0)) > 0)
		    {
		    /** Base64 source data needs to be multiples of 3 bytes except
		     ** at the tail end.
		     **/
		    padcnt = (3 - (rcnt%3))%3;
		    while(padcnt > 0)
			{
			rcnt2 = objRead(obj, xfer_buf+rcnt, padcnt, 0, 0);
			if (rcnt2 <= 0)
			    break;
			rcnt += rcnt2;
			padcnt = (3 - (rcnt%3))%3;
			}
		    
		    /** Encode it **/
		    nht_i_QPrintfConn(conn, 0, "%*STR&B64", rcnt, xfer_buf);
		    }
		nht_i_WriteConn(conn, "\"", -1, 0);

		/** Filename? **/
		nht_i_RestWriteAttr(conn, obj, "cx__download_as", res_attrs, 1);
		}

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
		nht_i_RestWriteAttr(conn, obj, attr, res_attrs, 1);
		}

	    /** Next, we output some system (hidden) attributes **/
	    for(i=0; i<sizeof(sent_sys_attrs); i++)
		{
		if (!sent_sys_attrs[i])
		    nht_i_RestWriteAttr(conn, obj, sys_attrs[i], res_attrs, 1);
		}
	    }

	/** And end it with } **/
	nht_i_WriteConn(conn, "\r\n}\r\n", -1, 0);

    return 0;
    }



/*** nht_i_RestGetCollection() - get a REST collection, which will be
 *** a list of subobjects for the given object, with or without details on
 *** the attributes for each one.
 ***/
int
nht_i_RestGetCollection(pNhtConn conn, pObject obj, nhtResType_t res_type, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int levels, pStruct url_inf)
    {
    pObjQuery query;
    pObject subobj;
    char* objname;
    char* objtype;
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];
    pXString criteria = NULL;

	/** We open our list with just a { **/
	nht_i_WriteConn(conn, "{\r\n", -1, 0);

	/** First thing we need to output is the URI of the collection **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	nht_i_QPrintfConn(conn, 0, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_type=collection&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=%STR&JSONSTR%]\"\r\n",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs != ResAttrsNone),
		(res_attrs == ResAttrsFull)?"full":"basic"
		);

	/** Check for query criteria **/
	criteria = nht_i_EncodeCriteria(url_inf);

	/** Open the query and loop through subobjects **/
	query = objOpenQuery(obj, criteria?criteria->String:"", NULL, NULL, NULL, 0);
	if (query)
	    {
	    while((subobj = objQueryFetch(query, O_RDONLY)) != NULL)
		{
		if (objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&objname)) == 0)
		    {
		    /** Print the name of the object. **/
		    nht_i_QPrintfConn(conn, 0, ",\"%STR&JSONSTR\":", objname);

		    /** Print it as an Element.  The res_format and res_attrs
		     ** settings are set to provide logical defaults for doing
		     ** this.
		     **/
		    if (objGetAttrValue(subobj, "inner_type", DATA_T_STRING, POD(&objtype)) != 0)
			objtype = "system/void";
		    if (levels <= 1)
			{
			nht_i_RestGetElement(conn, subobj, res_format, res_attrs, objtype);
			}
		    else
			{
			if (res_type == ResTypeBoth)
			    nht_i_RestGetBoth(conn, subobj, res_format, res_attrs, levels-1, objtype, url_inf);
			else
			    nht_i_RestGetCollection(conn, subobj, ResTypeCollection, res_format, res_attrs, levels-1, url_inf);
			}
		    objClose(subobj);
		    }
		}
	    objQueryClose(query);
	    }

	/** And close it with } **/
	nht_i_WriteConn(conn, "}\r\n", -1, 0);

	if (criteria) xsFree(criteria);

    return 0;
    }



/*** nht_i_RestGetBoth() - get both an element and a collection, combined
 *** into a two-part JSON document.
 ***/
int
nht_i_RestGetBoth(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs, int res_levels, char* mime_type, pStruct url_inf)
    {
    char* path;
    char pathbuf[OBJSYS_MAX_PATH];

	/** We open our list with just a { **/
	nht_i_WriteConn(conn, "{\r\n", -1, 0);

	/** First thing we need to output is the URI of the collection/element **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	nht_i_QPrintfConn(conn, 0, "\"@id\":\"%STR&JSONSTR?cx__mode=rest&cx__res_type=both&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=%STR&JSONSTR%]\"\r\n",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs != ResAttrsNone),
		(res_attrs == ResAttrsFull)?"full":"basic"
		);

	/** Output the element **/
	nht_i_WriteConn(conn, ",\"cx__element\":", -1, 0);
	nht_i_RestGetElement(conn, obj, res_format, res_attrs, mime_type);

	/** Output the collection **/
	nht_i_WriteConn(conn, ",\"cx__collection\":", -1, 0);
	nht_i_RestGetCollection(conn, obj, ResTypeBoth, res_format, res_attrs, res_levels, url_inf);

	/** Close the JSON container **/
	nht_i_WriteConn(conn, "}\r\n", -1, 0);

    return 0;
    }



/*** nht_i_RestGet() - perform a RESTful GET operation, returning
 *** JSON data or just a normal document.
 ***/
int
nht_i_RestGet(pNhtConn conn, pStruct url_inf, pObject obj)
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
	conn->NoCache = 1;

	/** MIME type of response **/
	strtcpy(conn->ResponseContentType,
		(res_type == ResTypeElement && res_format == ResFormatContent)?mime_type:"application/json",
		sizeof(conn->ResponseContentType));

	/** Remove content disposition header for a JSON response. **/
	if (res_type != ResTypeElement || res_format != ResFormatContent)
	    {
	    nht_i_AddResponseHeader(conn, "Content-Disposition", NULL, 0);
	    }

	/** End of headers - we don't have anything else to add at this point. Send the HTTP response. **/
	nht_i_WriteResponse(conn, 200, "OK", NULL);

	/** Ok, generate the document **/
	if (res_type == ResTypeBoth)
	    {
	    rval = nht_i_RestGetBoth(conn, obj, res_format, res_attrs, res_levels, mime_type, url_inf);
	    }
	else if (res_type == ResTypeElement)
	    {
	    if (res_format == ResFormatContent)
		rval = nht_i_RestGetElementContent(conn, obj, res_format, res_attrs, mime_type);
	    else
		rval = nht_i_RestGetElement(conn, obj, res_format, res_attrs, mime_type);
	    }
	else
	    {
	    rval = nht_i_RestGetCollection(conn, obj, ResTypeCollection, res_format, res_attrs, res_levels, url_inf);
	    }

    return rval;
    }



/*** nht_i_RestSetOneAttr() - set one attribute given its JSON
 *** new value representation
 ***/
int
nht_i_RestSetOneAttr(pObject obj, char* attrname, struct json_object* j_attr_obj)
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



/*** nht_i_RestPatch() - perform a RESTful PATCH operation, then
 *** return the modified document.  We send the response.
 ***/
int
nht_i_RestPatch(pNhtConn conn, pStruct url_inf, pObject obj, struct json_object* j_obj)
    {
    char* ptr;
    char* attrname;
    nhtResType_t res_type = ResTypeElement;
    struct json_object_iter iter;
    struct json_object* j_attr_obj;

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
	    if (nht_i_RestSetOneAttr(obj, attrname, j_attr_obj) < 0)
		return -1;
	    }

	/** Ok, issue the HTTP header for this one. **/
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	/*nht_i_WriteResponse(conn, 200, "OK", NULL);  -- RestGet sends response header **/

	/** Call out to the GET functionality to return the modified document **/
	nht_i_RestGet(conn, url_inf, obj);

    return 0;
    }



/*** nht_i_RestPost() - perform a RESTful POST operation, then
 *** return the newly created document.  A POST will perform an autoname
 *** create operation, and the new name of the created object will be a part
 *** of the @id property of the returned document.
 ***/
int
nht_i_RestPost(pNhtConn conn, pStruct url_inf, int size, char* content)
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
    char* msg;
    int code;
    nhtResType_t res_type = ResTypeElement;
    nhtResFormat_t res_format;
    nhtResAttrs_t res_attrs;
    struct json_object_iter iter;
    struct json_object* j_attr_obj;
    char* attrname;
    char vbuf[3]; /* verify buf: stores broken utf-8 for json parser until can check full */
    int vlen;   /* length of vbuf */
    int vind;   /* index of errors for verify */

	/** Open the target object **/
	if (strlen(url_inf->StrVal) + 2 + 1 >= OBJSYS_MAX_PATH)
	    {
	    mssError(0,"NHT","REST POST pathname too long");
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }
	snprintf(new_obj_path, sizeof(new_obj_path), "%s/*", url_inf->StrVal);
	target_obj = objOpen(conn->NhtSession->ObjSess, new_obj_path, OBJ_O_RDWR | OBJ_O_CREAT | OBJ_O_AUTONAME | OBJ_O_EXCL | OBJ_O_TRUNC, 0600, "application/octet-stream");
	if (!target_obj)
	    {
	    mssError(0,"NHT","Could not open requested object for POST request");
	    msg = "Not Found";
	    code = 404;
	    goto error;
	    }

	/** Initialize the JSON tokenizer **/
	jtok = json_tokener_new();
	if (!jtok)
	    {
	    mssError(1,"NHT","Could not initialize JSON parser");
	    msg = "Internal Server Error";
	    code = 500;
	    goto error;
	    }

	/** Supplied as a URL parameter? **/
	if (content)
	    {
	    /** content is verified when the headers are parsed via mtlexer, so no need to verify **/
	    j_obj = json_tokener_parse_ex(jtok, content, strlen(content));
	    jerr = json_tokener_get_error(jtok);
	    }
	else
	    {
	    /** Read the document from the connection **/
	    total_rcnt = 0;
	    vlen = 0;
	    do  {
		/** if vlen > 0, sneak into buffer first */
		if(vlen > 0) memcpy(rbuf, vbuf, vlen);
		rcnt = fdRead(conn->ConnFD, rbuf + vlen, sizeof(rbuf)-vlen, 0, 0);
		if (rcnt <= 0)
		    {
		    mssError(1,"NHT","Could not read JSON object from HTTP connection");
		    msg = "Bad Request";
		    code = 400;
		    goto error;
		    }
		rcnt += vlen; /** correct length **/
		vlen = 0;
		total_rcnt += rcnt;
		if (total_rcnt > NHT_PAYLOAD_MAX)
		    {
		    mssError(1,"NHT","JSON object too large");
		    msg = "Bad Request";
		    code = 400;
		    goto error;
		    }
		/** verify results **/
		if((vind = nVerifyUTF8(rbuf, rcnt)) != UTIL_VALID_CHAR)
		    {
		    if(rcnt - vind <= 3) /* if at end, store for next loop */
			{
			vlen = rcnt - vind;
			memcpy(vbuf, rbuf+vind, vlen);
			rcnt -= vlen;
			total_rcnt -= vlen;
			}
		    else /* is an error */
			{
			mssError(1,"NHT","JSON object contained an invalid character");
			msg = "Bad Request";
			code = 400;
			goto error;
			}
		    }
		j_obj = json_tokener_parse_ex(jtok, rbuf, rcnt);
		} while((jerr = json_tokener_get_error(jtok)) == json_tokener_continue);
		
		if(vlen > 0) /* catch errors at the end of strings */
		    {
		    mssError(1,"NHT","JSON object contained an invalid character");
		    msg = "Bad Request";
		    code = 400;
		    goto error;
		    }
	    }

	/** Success? **/
	if (!j_obj || jerr != json_tokener_success)
	    {
	    mssError(1,"NHT","Invalid JSON object in POST request");
	    msg = "Bad Request";
	    code = 400;
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
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }

	/** Must be an object containing attributes to be set **/
	if (!json_object_is_type(j_obj, json_type_object))
	    {
	    mssError(1,"NHT","JSON data for POST must be a JSON object");
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }

	/** Loop through attributes to be set **/
	json_object_object_foreachC(j_obj, iter)
	    {
	    j_attr_obj = iter.val;
	    attrname = iter.key;
	    if (nht_i_RestSetOneAttr(target_obj, attrname, j_attr_obj) < 0)
		return -1;
	    }
	//objCommit(conn->NhtSession->ObjSess);
	objCommitObject(target_obj);

	/** Get the new name **/
	if (objGetAttrValue(target_obj, "name", DATA_T_STRING, POD(&ptr)) != 0 || strchr(ptr, '?') || !strcmp(ptr,"*"))
	    {
	    mssError(0,"NHT","Could not autoname new object for POST request");
	    msg = "Internal Server Error";
	    code = 500;
	    goto error;
	    }
	if (strlen(new_obj_path) + strlen(ptr) + 2 >= sizeof(new_obj_name))
	    {
	    mssError(1,"NHT","Path too long for new object for POST request");
	    msg = "Internal Server Error";
	    code = 500;
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
	    msg = "Internal Server Error";
	    code = 500;
	    goto error;
	    }

	/** Ok, issue the HTTP header for this one. **/
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	strtcpy(conn->ResponseContentType, "application/json", sizeof(conn->ResponseContentType));
	conn->NoCache = 1;
	nht_i_AddResponseHeaderQPrintf(conn, "Location", "%STR&PATH?cx__mode=rest&cx__res_type=element&cx__res_format=attrs&cx__res_attrs=basic", new_obj_name);
	nht_i_WriteResponse(conn, 201, "Created", NULL);

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
	nht_i_RestGetElement(conn, target_obj, res_format, res_attrs, "system/void");

	/** Cleanup and return **/
	json_object_put(j_obj);
	objClose(target_obj);
	return 0;

    error:
	nht_i_WriteErrResponse(conn, code, msg, "");

	if (jtok)
	    json_tokener_free(jtok);
	if (j_obj)
	    json_object_put(j_obj);
	if (target_obj)
	    objClose(target_obj);

	return -1;
    }



/*** nht_i_RestDelete() - perform a RESTful DELETE operation.  Caller sends
 *** HTTP response.
 ***/
int
nht_i_RestDelete(pNhtConn conn, pStruct url_inf, pObject obj)
    {

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

