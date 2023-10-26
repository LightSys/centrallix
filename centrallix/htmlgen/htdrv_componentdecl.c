#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "hints.h"
#include "cxlib/cxsec.h"
#include "cxlib/util.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_componentdecl.c             			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 11, 2003					*/
/* Description:	HTML Widget driver for component widget definition,	*/
/*		which is used to declare a component, its structure,	*/
/*		and its interface/params.  Use the normal component	*/
/*		widget (htdrv_component.c) to insert a component into	*/
/*		an application.						*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCMPD;


/** structure defining a param to the component **/
typedef struct
    {
    pObjPresentationHints	Hints;
    TObjData			TypedObjData;
    char*			StrVal;
    char*			Name;
    }
    HTCmpdParam, *pHTCmpdParam;


/*** htcmpdRender - generate the HTML code for the component.
 ***/
int
htcmpdRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char subobj_name[64];
    char expose_events_for[64] = "";
    char expose_actions_for[64] = "";
    char expose_props_for[64] = "";
    char apply_hints_to[64] = "";
    int id;
    /*char* nptr;*/
//    pObject subobj = NULL;
    pWgtrNode sub_tree = NULL;
    pWgtrNode conn_tree = NULL;
    XArray attrs;
    pHTCmpdParam param;
    int i, j;
    int rval = 0;
    int is_visual = 1;
    char gbuf[256];
    char* gname;
    char* xptr;
    char* yptr;
    int xoffs, yoffs;

	/** Verify capabilities **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCMPD","Either Netscape DOM or W3C DOM1 HTML and W3C CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCMPD.idcnt++);

	/** Is this a visual component? **/
	if ((is_visual = htrGetBoolean(tree, "visual", 1)) < 0)
	    {
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name, ptr, sizeof(name));
	if (cxsecVerifySymbol(name) < 0)
	    {
	    mssError(1,"HTCMPD","Invalid name '%s' for component", name);
	    return -1;
	    }

	/** Need to relocate the widgets via xoffset/yoffset? **/
	xptr = htrParamValue(s, "cx__xoffset");
	yptr = htrParamValue(s, "cx__yoffset");
	if (xptr || yptr)
	    {
	    xoffs = yoffs = 0;
	    if (xptr) xoffs = strtoi(xptr, NULL, 10);
	    if (yptr) yoffs = strtoi(yptr, NULL, 10);
	    wgtrMoveChildren(tree, xoffs, yoffs);
	    }

	/** Expose all events on a subwidget? **/
	if (wgtrGetPropertyValue(tree, "expose_events_for", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(expose_events_for, ptr, sizeof expose_events_for);

	/** Expose all actions on a subwidget? **/
	if (wgtrGetPropertyValue(tree, "expose_actions_for", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(expose_actions_for, ptr, sizeof expose_actions_for);

	/** Expose all values on a subwidget? **/
	if (wgtrGetPropertyValue(tree, "expose_properties_for", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(expose_props_for, ptr, sizeof expose_props_for);

	/** Apply presentation hints to a subwidget? **/
	if (wgtrGetPropertyValue(tree, "apply_hints_to", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(apply_hints_to, ptr, sizeof apply_hints_to);

	/** Write named global **/
	/*nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);*/
	/*htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);*/

	/** Include the js module **/
	htrAddScriptInclude(s, "/sys/js/htdrv_componentdecl.js", 0);

	/** parameters for this component **/
	xaInit(&attrs, 16);

	/** DOM Linkages **/
	if (s->GraftPoint)
	    {
	    strtcpy(gbuf, s->GraftPoint, sizeof(gbuf));
	    gname = strchr(gbuf,':');
	    if (!gname)
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto htcmpd_cleanup;
		}
	    *(gname++) = '\0';
	    if (strpbrk(gname,"'\"\\<>\r\n\t ") || strspn(gbuf,"w0123456789abcdef_") != strlen(gbuf))
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto htcmpd_cleanup;
		}
	    htrAddWgtrCtrLinkage_va(s, tree, 
		    "wgtrGetContainer(wgtrGetNode(\"%STR&SYM\",\"%STR&SYM\"))", gbuf, gname);
	    htrAddBodyItem_va(s, "<a id=\"dname\" target=\"%STR&SYM\" href=\".\"></a>", s->Namespace->DName);
	    }
	else
	    {
	    strcpy(gbuf,"");
	    gname="";
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    }

	/** Init component **/
	htrAddScriptInit_va(s, "    cmpd_init(wgtrGetNodeRef(ns,\"%STR&SYM\"), {vis:%POS, gns:%[\"%STR&SYM\"%]%[null%], gname:'%STR&SYM'%[, expe:'%STR&SYM'%]%[, expa:'%STR&SYM'%]%[, expp:'%STR&SYM'%]%[, applyhint:'%STR&SYM'%]});\n", 
		name, is_visual, *gbuf, gbuf, !*gbuf, gname, *expose_events_for, expose_events_for, *expose_actions_for, expose_actions_for,
		*expose_props_for, expose_props_for, *apply_hints_to, apply_hints_to);
#if 0
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    /** Loop through each param we get **/
	    sub_tree = xaGetItem(&(tree->Children), i);
	    if (!strcmp(sub_tree->Type, "system/parameter"))
		{
		param = (pHTCmpdParam)nmMalloc(sizeof(HTCmpdParam));
		if (!param) break;
		xaAddItem(&attrs, param);

		/** Get component parameter name **/
		wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
		param->Name = nmSysStrdup(ptr);
		if (cxsecVerifySymbol(param->Name) < 0)
		    {
		    mssError(1,"HTCMPD","Invalid name '%s' for parameter in component '%s'", param->Name, name);
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Does the param have a value? **/
		param->StrVal = htrParamValue(s, param->Name);

		/** Get component type **/
		if (wgtrGetPropertyValue(sub_tree, "type", DATA_T_STRING, POD(&ptr)) == 0)
		    {
		    t = objTypeID(ptr);
		    if (t < 0)
			{
			mssError(1,"HTCMPD","Component '%s' parameter '%s' has unknown/invalid data type '%s'", name, param->Name, ptr);
			rval = -1;
			goto htcmpd_cleanup;
			}
		    param->TypedObjData.DataType = t;
		    }
		else
		    {
		    /** default type is string **/
		    param->TypedObjData.DataType = DATA_T_STRING;
		    }

		/** Get hints **/
		param->Hints = wgtrWgtToHints(sub_tree);
		if (!param->Hints)
		    {
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Close the object **/
		sub_tree = NULL;
		}
	    subobj_qy = NULL;
	    }
#endif

	/** Build the typed pod values for each data value **/
	for(i=0;i<attrs.nItems;i++)
	    {
	    param = (pHTCmpdParam)(attrs.Items[i]);
	    if (!param->StrVal)
		{
		param->TypedObjData.Flags = DATA_TF_NULL;
		}
	    else
		{
		param->TypedObjData.Flags = 0;
		switch(param->TypedObjData.DataType)
		    {
		    case DATA_T_STRING:
			param->TypedObjData.Data.String = param->StrVal;
			break;
		    case DATA_T_INTEGER:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to integer", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Integer = strtoi(param->StrVal,&ptr,10);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to integer", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    case DATA_T_DOUBLE:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to double", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Double = strtod(param->StrVal,&ptr);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to double", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    default:
			mssError(1,"HTCMPD","Unsupported type for param '%s'", param->Name);
			rval= -1;
			goto htcmpd_cleanup;
		    }
		}

	    /** Verify the thing **/
	    if (hntVerifyHints(param->Hints, &(param->TypedObjData), &ptr, NULL, s->ObjSession) < 0)
		{
		mssError(1,"HTCMPD","Invalid value '%s' for component '%s' param '%s': %s", param->StrVal, name, param->Name, ptr);
		rval = -1;
		goto htcmpd_cleanup;
		}
	    }

	/** Add actions, events, and client properties **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);

	    /** Get component action/event/cprop name **/
	    wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
	    strtcpy(subobj_name, ptr, sizeof(subobj_name));
	    if (cxsecVerifySymbol(subobj_name) < 0)
		{
		mssError(1,"HTCMPD","Invalid name '%s' for action/event/cprop in component '%s'", subobj_name, name);
		rval = -1;
		goto htcmpd_cleanup;
		}

	    /** Get type **/
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING, POD(&ptr));
	    if (!strcmp(ptr,"widget/component-decl-action"))
		{
		htrAddScriptInit_va(s, "    wgtrGetNodeRef(ns,\"%STR&SYM\").addAction('%STR&SYM');\n", name, subobj_name);
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		}
	    else if (!strcmp(ptr,"widget/component-decl-event"))
		{
		htrAddScriptInit_va(s, "    wgtrGetNodeRef(ns,\"%STR&SYM\").addEvent('%STR&SYM');\n", name, subobj_name);
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		}
	    else if (!strcmp(ptr,"widget/component-decl-cprop"))
		{
		htrAddScriptInit_va(s, "    wgtrGetNodeRef(ns,\"%STR&SYM\").addProp('%STR&SYM');\n", name, subobj_name);
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		}

	    sub_tree = NULL;
	    }

	/** Do subwidgets **/
	/*htrRenderSubwidgets(s, tree, z+2);*/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING, POD(&ptr));
	    if (strcmp(ptr,"widget/component-decl-action") && 
		    strcmp(ptr,"widget/component-decl-event") &&
		    strcmp(ptr,"widget/component-decl-cprop"))
		{
		htrRenderWidget(s, sub_tree, z+2);
		}
	    else if (strcmp(ptr,"widget/component-decl-action") == 0)
		{
		/** allow connectors inside component-decl-action **/
		for (j=0;j<xaCount(&(sub_tree->Children));j++)
		    {
		    conn_tree = xaGetItem(&(sub_tree->Children), j);
		    wgtrGetPropertyValue(conn_tree, "outer_type", DATA_T_STRING, POD(&ptr));
		    if (strcmp(ptr,"widget/connector") == 0)
			{
			htrRenderWidget(s, conn_tree, z+2);
			}
		    }
		}
	    sub_tree = NULL;
	    }

	/** End init for component **/
	htrAddScriptInit_va(s, "    cmpd_endinit(wgtrGetNodeRef(ns,\"%STR&SYM\"));\n", name);

    htcmpd_cleanup:
//	if (subobj) objClose(subobj);
//	if (subobj_qy) objQueryClose(subobj_qy);
	for(i=0;i<attrs.nItems;i++)
	    {
	    if (attrs.Items[i])
		{
		param = (pHTCmpdParam)(attrs.Items[i]);
		if (param->Hints) objFreeHints(param->Hints);
		if (param->Name) nmSysFree(param->Name);
		nmFree(param, sizeof(HTCmpdParam));
		}
	    }
	xaDeInit(&attrs);

    return rval;
    }


/*** htcmpdInitialize - register with the ht_render module.
 ***/
int
htcmpdInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Component Declaration Driver");
	strcpy(drv->WidgetName,"component-decl");
	drv->Render = htcmpdRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	/** Declare support for DHTML user interface class **/
	htrAddSupport(drv, "dhtml");

	HTCMPD.idcnt = 0;

    return 0;
    }

