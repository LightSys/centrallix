
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
/* Module: 	Interface Module     					*/
/* Author:	Matt McGill (MJM)					*/
/* Creation:	July 26, 2004   					*/
/* Description:	Provies the functionality for handling interfaces in a  */
/*		general way, to be used by any Centrallix module that   */
/*		has need of support for interfaces.                    	*/
/************************************************************************/

/**CVSDATA***************************************************************

 **END-CVSDATA***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iface.h"
#include "iface_private.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "obj.h"
#include "centrallix.h"
#include "ht_render.h"


/*** ifc_internal_EncodeHtml - take an XString, and HTMLify it
 *** Same idea as nht_internal_EncodeHTML, but with an XString
 ***/
int
ifc_internal_EncodeHTML(pXString str)
    {
    XString tmp;
    int i;
    char ch;
    char buf[10];

	xsInit(&tmp);
	memset(buf, 0, 10);
	for (i=0;i<strlen(str->String);i++)
	    {
	    ch = str->String[i];
	    if (ch == '<') snprintf(buf, 10, "&lt;");
	    else if (ch == '>') snprintf(buf, 10, "&gt;");
	    else if (ch == '&') snprintf(buf, 10, "&amp;");
	    else snprintf(buf, 10, "%c", ch);
	    xsConcatenate(&tmp, buf, -1);
	    }
	xsCopy(str, tmp.String, -1);
	xsDeInit(&tmp);
    
	return 0;
    }


/*** ifc_internal_ObjToJS - generate a JS inline object instantiation from an OSML object, recursively
 ***/
int ifc_internal_ObjToJS(pXString str, pObject obj)
    {
    pObject subobj;
    pObjQuery qy;
    ObjData val;
    char* name;
    int type;
    XString expr;

	xsConcatenate(str, "{", 1);

	/** output the type **/
	objGetAttrValue(obj, "outer_type", DATA_T_STRING, &val);
	xsConcatPrintf(str, "type:\"%s\"", val.String);
	/** get the properties **/
	name = objGetFirstAttr(obj);
	while (name)
	    {
	    type = objGetAttrType(obj, name);
	    if (objGetAttrValue(obj, name, type, &val) < 0)
		{
		mssError(0, "IFC", "ifc_internal_ObjToJS - couldn't get value for property %s", name);
		name = objGetNextAttr(obj);
		continue;
		}
	    xsConcatenate(str, ",", 1);
	    switch (type)
		{
		case DATA_T_STRING:
		    xsConcatPrintf(str, "%s: \"%s\"", name, val.String);
		    break;
		case DATA_T_INTEGER:
		    xsConcatPrintf(str, "%s: %d", name, val.Integer);
		    val.Integer=0;
		    break;
		case DATA_T_DOUBLE:
		    xsConcatPrintf(str, "%s: %f", name, val.Double);
		    break;
		case DATA_T_CODE:
		    xsInit(&expr);
		    expGenerateText((pExpression)val.Generic, NULL, xsWrite, &expr, 0, "javascript", EXPR_F_RUNCLIENT);
		    xsConcatPrintf(str, "%s: \"%s\"", name, expr.String);
		    xsDeInit(&expr);
		    break;
		/** TODO: Support for other datatypes should probably be added **/
		default:
		    mssError(1, "IFC", "Can't convert properties of type %d in ObjToJS - FIXME!", type);
		    break;
		}
	    name = objGetNextAttr(obj);
	    }

	/** now get sub-objects **/
	if ( (qy = objOpenQuery(obj, NULL, NULL, NULL, NULL)) != NULL)
	    {
	    while ( (subobj = objQueryFetch(qy, O_RDONLY)) != NULL)
		{
		xsConcatenate(str, ",", 1);
		objGetAttrValue(subobj, "name", DATA_T_STRING, &val);
		xsConcatPrintf(str, "%s:", val.String);
		ifc_internal_ObjToJS(str, subobj);
		objClose(subobj);
		}
	    objQueryClose(qy);
	    }

	/** all done **/
	xsConcatenate(str, "}", 1);

	return 0;
    }


/*** ifcToHtml - writes the interface out into html code that would be
 *** easy to parse by a browser. Uses a bunch of nested tables to represent
 *** the info.
 ***
 ***/
int 
ifcToHtml(pFile file, pObjSession s, char* def_str)
    {
    int i, j, k, l;
    pIfcDefinition def;
    pIfcMajorVersion maj_v;
    char path[512];
    pObject obj;
    XString js_obj;

	/** make sure we get an absolute path **/
	if (def_str[0] == '/') strncpy(path, def_str, 512);
	else snprintf(path, 512, "%s/%s", IFC.IfaceDir, def_str);
	    
	/** see if we can look up the definition **/
	if ( (def = (pIfcDefinition)xhLookup(&(IFC.Definitions), path)) == NULL)
	    {
	    /** hasn't been parsed yet - parse it **/
	    if ( (def = ifc_internal_NewIfcDef(s, path)) == NULL)
		{
		mssError(0, "IFC", "Couldn't parse interface definition '%s'", def_str);
		return -1;
		}
	    if (xhAdd(&(IFC.Definitions), def->Path, (void*)def) < 0) mssError(0, "IFC", "Couldn't cache");
	    }
	
	/** we've got an interface definition, one way or another. Time to translate**/
	xsInit(&js_obj);
	for (i=0;i<xaCount(&(def->MajorVersions));i++)
	    {
	    if ( (maj_v = xaGetItem(&(def->MajorVersions), i)) == NULL) continue;

	    fdPrintf(file, "<a target=\"start\" href=\"http://v%d\"></a>\n", i);
	    for (j=0;j<maj_v->NumMinorVersions;j++)
		{
		fdPrintf(file, "  <a target=\"start\" href=\"http://v%d\"></a>\n", j);
		for (k=0;k<IFC.NumCategories[def->Type];k++)
		    {
		    fdPrintf(file, "    <a target=\"start\" href=\"http://%s\"></a>\n",
			IFC.CategoryNames[def->Type][k]);
		    for (l=(intptr_t)xaGetItem(&(maj_v->Offsets[k]), j);l<xaCount(&(maj_v->Members[k]));l++)
			{
			fdPrintf(file, "      <a target=\"start\" href=\"http://%s\">", 
			    xaGetItem(&(maj_v->Members[k]), l));
			if ( (obj = objOpen(s, xaGetItem(&(maj_v->Properties[k]), l), O_RDONLY, 0400, NULL)) != NULL)
			    {
			    xsDeInit(&js_obj);
			    xsInit(&js_obj);
			    ifc_internal_ObjToJS(&js_obj, obj);
			    ifc_internal_EncodeHTML(&js_obj);
			    fdPrintf(file, "%s", js_obj.String);
			    objClose(obj);
			    }
			fdPrintf(file, "</a>\n");
//			fdPrintf(file, "      <a target=\"end\" href=\"\"></a>\n", 
//			    xaGetItem(&(maj_v->Members[k]), l));
			}
		    fdPrintf(file, "    <a target=\"end\" href=\"\"></a>\n");
		    }
		fdPrintf(file, "  <a target=\"end\" href=\"http://v%d\"></a>\n", j);
		}
	    fdPrintf(file, "<a target=\"end\" href=\"http://v%d\"></a>\n",	i);
	    }
	xsDeInit(&js_obj);

	return 0;
    }

/*** ifcHtmlInit_r - recursive function for adding as many interface definitions as
 *** we can to the client-side code, so they don't have to be loaded from the server
 *** dynamically
 ***/
int
ifc_internal_HtmlInit_r(pHtSession s, pXString func, pWgtrNode tree, pXArray AlreadyProcessed)
    {
    int i, j, maj_v_num, min_v_num, cat, member, first_cat, first_member, first_majv, first_minv, len;
    IfcHandle h;
    char* ptr;
    pIfcDefinition def;
    pIfcMajorVersion maj_v;
    XString js_obj;
    pObject obj;

	xsInit(&js_obj);
	/** for each interface **/
	for (i=0;i<xaCount(&(tree->Interfaces));i++)
	    {
	    h = xaGetItem(&(tree->Interfaces), i);
	    len = xaCount(AlreadyProcessed);
	    /** try to find it in the AlreadyProcessed list **/
	    for (j=0;j<len;j++)
		{
		ptr = xaGetItem(AlreadyProcessed, j);
		if (!strcmp(ptr, h->DefPath)) break;
		}
	    /** did we find it? **/
	    if (j != len && len != 0) continue;

	    /** add this interface def to the list of defs we already got **/
	    xaAddItem(AlreadyProcessed, h->DefPath);

	    /** look up the definition this handle was generated from **/
	    if ( (def = (pIfcDefinition)xhLookup(&(IFC.Definitions), h->DefPath)) == NULL)
		{
		xsDeInit(&js_obj);
		mssError(1, "IFC", "Couldn't lookup definition '%s' that handle was made from", h->DefPath);
		return -1;
		}

	    /** add an inline declaration of this guy **/
	    xsConcatPrintf(func, "    IFC.Definitions['%s'] = new Object();\n", h->DefPath);
	    xsConcatPrintf(func, "    IFC.Definitions['%s'].Path='%s';\n", h->DefPath, h->DefPath);
	    xsConcatPrintf(func, "    IFC.Definitions['%s'].Type='%s';\n", h->DefPath, IFC.TypeNames[h->Type]);
	    xsConcatPrintf(func, "    IFC.Definitions['%s'].DEFINITION=true;\n", h->DefPath);
	    xsConcatPrintf(func, "    IFC.Definitions['%s'].MajorVersions=[", h->DefPath);
	    first_majv=1;
	    /** for each major version **/
	    for (maj_v_num=0;maj_v_num<xaCount(&(def->MajorVersions));maj_v_num++)
		{
		if (!first_majv) xsConcatenate(func, ",", 1);
		else first_majv=0;
		if ((maj_v = xaGetItem(&(def->MajorVersions), maj_v_num)) != NULL)
		    {
		    xsConcatPrintf(func, "[");
		    /** for each minor version **/
		    first_minv=1;
		    for (min_v_num=0;min_v_num<maj_v->NumMinorVersions;min_v_num++)
			{
			if (!first_minv) xsConcatenate(func, ",",1);
			else first_minv=0;
			xsConcatPrintf(func, "{");
			first_cat=1;
			/** for each category **/
			for (cat=0;cat<IFC.NumCategories[h->Type];cat++)
			    {
			    if (!first_cat) xsConcatPrintf(func, ",");
			    else first_cat = 0;
			    xsConcatPrintf(func, "%s:{", IFC.CategoryNames[h->Type][cat]);
			    first_member=1;
			    /** for each member **/
			    for (member=(intptr_t)xaGetItem(&(maj_v->Offsets[cat]), min_v_num);
				 member<xaCount(&(maj_v->Members[cat]));member++)
				{
				if (!first_member) xsConcatPrintf(func, ",");
				else first_member=0;
				xsDeInit(&js_obj);
				xsInit(&js_obj);
				if ( (obj=objOpen(s->ObjSession, xaGetItem(&(maj_v->Properties[cat]), member),
						    O_RDONLY, 0400, NULL)) != NULL)
				    {
				    ifc_internal_ObjToJS(&js_obj, obj);
				    }
				xsConcatPrintf(func, "%s:%s", xaGetItem(&(maj_v->Members[cat]), member), js_obj.String);
				} /** end for each member **/
			    xsConcatPrintf(func, "}");
			    } /** end for each category **/
			xsConcatPrintf(func, "}");
			} /** end for each minor version **/
		    xsConcatPrintf(func, "]");
		    } /** end if **/
		} /** end for each major version **/
	    xsConcatPrintf(func, "];"); 
	    } /** end for each interface **/

	xsDeInit(&js_obj);
	/** call ourselves recursively on each node **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    if (ifc_internal_HtmlInit_r(s, func, xaGetItem(&(tree->Children), i), AlreadyProcessed) < 0) return -1;
	return 0;
    }

/*** ifcHtmlInit - write the necessary DHTML that the client-side interface module will need
 ***/
int
ifcHtmlInit(pHtSession s, pWgtrNode tree)
    {
    XArray AlreadyProcessed;
    XString func;
    char* fnbuf;

	/** first add the necessary DHTML, and call to init **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_iface.js", 0);
	htrAddStylesheetItem(s, "        #ifc_layer {position: absolute; visibility: hidden;}\n");
	htrAddBodyItem(s, "<div id=\"ifc_layer\"></div>\n");
	htrAddScriptInit_va(s, "    ifcInitialize(\"%STR&JSSTR\");\n", IFC.IfaceDir);
	htrAddScriptInit(s, "    init_inline_interfaces();\n");

	/** now create all the interface info we know of in-line **/
	xaInit(&AlreadyProcessed, 16);
	xsInit(&func);
	xsConcatenate(&func, "\nfunction init_inline_interfaces()\n    {\n", -1);
	ifc_internal_HtmlInit_r(s, &func, tree, &AlreadyProcessed);
	xsConcatenate(&func, "\n    }\n", -1);

	/** print the generated function **/
	fnbuf = nmMalloc(strlen(func.String)+1);
	if (!fnbuf) return -1;
	strcpy(fnbuf, func.String);
	htrAddScriptFunction(s, "init_inline_interfaces", fnbuf, HTR_F_VALUEALLOC);
	xsDeInit(&func);
	xaDeInit(&AlreadyProcessed);
	return 0;
    }
   

