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
typedef enum { ResTypeCollection, ResTypeElement } nhtResType_t;
typedef enum { ResFormatAttrs, ResFormatAuto, ResFormatContent, ResFormatBoth } nhtResFormat_t;
typedef enum { ResAttrsBasic, ResAttrsFull, ResAttrsNone } nhtResAttrs_t;



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

	/** We begin our object with just a { **/
	fdPrintf(conn->ConnFD, "{\r\n");

	/** First thing we need to output is the URI of the element **/
	path = objGetPathname(obj);
	strtcpy(pathbuf, path, sizeof(pathbuf));
	fdQPrintf(conn->ConnFD, "\"cx__uri\":\"%STR&JSONSTR?cx__mode=rest&cx__res_format=%STR&JSONSTR%[&cx__res_attrs=%STR&JSONSTR%]\"",
		pathbuf,
		(res_format == ResFormatAttrs)?"attrs":"both",
		(res_attrs != ResAttrsBasic),
		(res_attrs == ResAttrsFull)?"full":"none"
		);

	/** Write any attrs? **/
	if (res_attrs != ResAttrsNone)
	    {
	    /** Next, we output some system (hidden) attributes **/
	    nht_internal_RestWriteAttr(conn, obj, "name", res_attrs, 1);
	    nht_internal_RestWriteAttr(conn, obj, "annotation", res_attrs, 1);
	    nht_internal_RestWriteAttr(conn, obj, "inner_type", res_attrs, 1);
	    nht_internal_RestWriteAttr(conn, obj, "outer_type", res_attrs, 1);

	    /** Now we loop through the main attributes that are iterable **/
	    for(attr=objGetFirstAttr(obj); attr; attr=objGetNextAttr(obj))
		{
		nht_internal_RestWriteAttr(conn, obj, attr, res_attrs, 1);
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
nht_internal_RestGetCollection(pNhtConn conn, pObject obj, nhtResFormat_t res_format, nhtResAttrs_t res_attrs)
    {
    pObjQuery query;
    pObject subobj;
    char* objname;
    char* objtype;
    int is_first = 1;

	/** We open our list with just a { **/
	fdPrintf(conn->ConnFD, "{\r\n");

	/** Open the query and loop through subobjects **/
	query = objOpenQuery(obj, "", NULL, NULL, NULL);
	if (query)
	    {
	    while((subobj = objQueryFetch(query, O_RDONLY)) != NULL)
		{
		if (objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&objname)) == 0)
		    {
		    /** Add a comma between elements in the list **/
		    if (!is_first)
			fdPrintf(conn->ConnFD, ",");
		    else
			is_first = 0;

		    /** Print the name of the object. **/
		    fdQPrintf(conn->ConnFD, "\"%STR&JSONSTR\":", objname);

		    /** Print it as an Element.  The res_format and res_attrs
		     ** settings are set to provide logical defaults for doing
		     ** this.
		     **/
		    if (objGetAttrValue(subobj, "inner_type", DATA_T_STRING, POD(&objtype)) != 0)
			objtype = "system/void";
		    nht_internal_RestGetElement(conn, subobj, res_format, res_attrs, objtype);
		    objClose(subobj);
		    }
		}
	    objQueryClose(query);
	    }

	/** And close it with } **/
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
	    }
	if (res_type == ResTypeCollection)
	    {
	    /** Default settings for collections **/
	    res_format = ResFormatAttrs;
	    res_attrs = ResAttrsNone;
	    }
	else
	    {
	    /** Default settings for Elements **/
	    res_format = ResFormatContent;
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

	/** Sanity check -- collection and content are incompatible **/
	if (res_type == ResTypeCollection && res_format == ResFormatContent)
	    res_format = ResFormatAttrs;

	/** If collection and 'auto' are both set, use 'attrs' instead **/
	if (res_type == ResTypeCollection && res_format == ResFormatAuto)
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
	if (res_type == ResTypeElement)
	    {
	    if (res_format == ResFormatContent)
		rval = nht_internal_RestGetElementContent(conn, obj, res_format, res_attrs, mime_type);
	    else
		rval = nht_internal_RestGetElement(conn, obj, res_format, res_attrs, mime_type);
	    }
	else
	    {
	    rval = nht_internal_RestGetCollection(conn, obj, res_format, res_attrs);
	    }

    return rval;
    }

