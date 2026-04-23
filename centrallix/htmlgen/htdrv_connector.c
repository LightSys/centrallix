#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/util.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "wgtr.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_connector.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 24, 1998 					*/
/* Description:	HTML Widget driver for a 'connector', which joins an 	*/
/*		event on the parent widget with an action on another	*/
/*		specified widget.					*/
/************************************************************************/


/*** htconnRender - generate the HTML code for the page.
 ***/
int
htconnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char event[32] = "";
    char target[128];
    char source[128];
    char action[32] = "";
    XString xs = { AllocLen: 0 };
    int rval = -1;

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTCONN", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto end;
	    }

	/** Inside a component-decl-action? **/
	bool inside_action = false;
	if (tree->Parent && wgtrGetPropertyValue(tree->Parent, "outer_type", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "widget/component-decl-action"))
	    {
	    inside_action = true;
	    wgtrGetPropertyValue(tree->Parent, "name", DATA_T_STRING, POD(&ptr));
	    strtcpy(event, ptr, sizeof(event));
	    }

	/** Get the event and action linkage information **/
	if (wgtrGetPropertyValue(tree,"event",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    if (inside_action)
		{
		mssError(1,"HTCONN","Connector inside componentdecl-action cannot have an 'event' property");
		goto end;
		}
	    strtcpy(event,ptr,sizeof(event));
	    }
	if (!*event)
	    {
	    mssError(1,"HTCONN","Connector must have an 'event' property");
	    goto end;
	    }

	if (wgtrGetPropertyValue(tree,"action",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(action,ptr,sizeof(action));
	    }
	if (!*action)
	    {
	    mssError(1,"HTCONN","Connector must have an 'action' property");
	    goto end;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto end;
	strtcpy(name,ptr,sizeof(name));

	/** Source/target **/
	if (wgtrGetPropertyValue(tree,"target",DATA_T_STRING,POD(&ptr)) != 0)
	    target[0] = '\0';
	else
	    strtcpy(target,ptr,sizeof(target));

	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    source[0] = '\0';
	else
	    {
	    if (inside_action)
		{
		mssError(1,"HTCONN","Connector inside componentdecl-action cannot have a 'source' property");
		goto end;
		}
	    strtcpy(source, ptr, sizeof(source));
	    }

	/** Build the param list **/
	if (check(xsInit(&xs)) != 0) goto end;
	bool first = true;
	for(ptr = wgtrFirstPropertyName(tree); ptr; ptr = wgtrNextPropertyName(tree))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action") || !strcmp(ptr, "source") || !strcmp(ptr, "condition")) continue;
	    const int property_type = wgtrGetPropertyType(tree, ptr);
	    if (!first && check(xsConcatenate(&xs, ",", 1)) != 0) goto err_param;
	    first = false;
	    switch (property_type)
		{
		case DATA_T_CODE:
		    {
		    pExpression code;
		    wgtrGetPropertyValue(tree, ptr, DATA_T_CODE, POD(&code));
		    xsConcatQPrintf(&xs,"%STR&SYM:{type:'exp', value:function(_context,_this,ep) { with(ep) { return ", ptr);
		    expGenerateText(code, NULL, xsWrite, &xs, '\0', "javascript", EXPR_F_RUNCLIENT);
		    xsConcatenate(&xs,"; } } }",7);
		    break;
		    }

		case DATA_T_INTEGER:
		    {
		    int vint;
		    wgtrGetPropertyValue(tree, ptr, DATA_T_INTEGER, POD(&vint));
		    if (xsConcatQPrintf(&xs, "%STR&SYM:{type:'int', value:%INT}", ptr, vint) < 0)
			{
			mssError(1, "HTCONN", "Failed to write int JSON.");
			goto err_param;
			}
		    break;
		    }

		case DATA_T_DOUBLE:
		    {
		    double vdbl;
		    wgtrGetPropertyValue(tree, ptr, DATA_T_DOUBLE,POD(&vdbl));
		    if (xsConcatQPrintf(&xs, "%STR&SYM:{type:'dbl', value:%DBL}", ptr, vdbl) < 0)
			{
			mssError(1, "HTCONN", "Failed to write int JSON.");
			goto err_param;
			}
		    break;
		    }

		case DATA_T_STRING:
		    {
		    char* vstr;
		    wgtrGetPropertyValue(tree, ptr, DATA_T_STRING, POD(&vstr));
		    if (strpbrk(vstr, " !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|") == NULL)
			{
			if (xsConcatQPrintf(&xs, "%STR&SYM:{type:'sym', value:'%STR&SYM', namespace:ns}", ptr, vstr) < 0)
			    {
			    mssError(1, "HTCONN", "Failed to write string symbol parameter JSON.");
			    goto err_param;
			    }
			}
		    else
			{
			if (xsConcatQPrintf(&xs, "%STR&SYM:{type:'str', value:'%STR&JSSTR'}", ptr, vstr) < 0)
			    {
			    mssError(1, "HTCONN", "Failed to write string parameter JSON.");
			    goto err_param;
			    }
			}
		    break;
		    }

		default:
		    mssError(1, "HTCONN", "Unsupported type %d.", property_type);
		    goto err_param;
		}

	    /** Success. **/
	    continue;
	    
    err_param:
	    mssError(0, "HTCONN",
		"Failed to write JSON for connector parameter %s of type %d.",
		ptr, property_type
	    );
	    goto end;
	    }

	/** Determine the target in a new scope. **/
	const int no_source = (source == NULL || source[0] == '\0');
	if (htrAddScriptInit_va(s, "\t{ "
	    "let target = wgtrGetNodeRef(ns, '%STR&SYM'); "
	    "%[target = wgtrGetParent(target); %]"
	    "%[target = wgtrGetParent(target); %]",
	    (no_source) ? name : source,
	    (no_source),
	    (inside_action) // only true when no_source is true.
	) != 0)
	    {
	    mssError(0, "HTCONN", "Failed to write JS target.");
	    goto end;
	    }
	
	/** Call the functions on the target, and close the scope. **/
	const int no_target = (target == NULL || target[0] == '\0');
	if (htrAddScriptInit_va(s, "target"
	    ".ifcProbe(ifEvent)"
	    ".Connect('%STR&SYM', '%STR&SYM', '%STR&SYM', {%STR}, ns); "
	    "}\n",
	    event, (no_target) ? tree->Parent->Name : target, action, xs.String
	) != 0)
	    {
	    mssError(0, "HTCONN", "Failed to write JS event connect.");
	    goto end;
	    }
	xsDeInit(&xs);
	xs.AllocLen = 0;

	/** Include the connector library. **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0) != 0) goto end;

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto end;

	/** Success. **/
	rval = 0;

    end:
	if (rval != 0)
	    {
	    mssError(0, "HTCONN",
		"Failed to render \"%s\":\"%s\".",
		tree->Name, tree->Type
	    );
	    }
	
	/** Clean up. **/
	if (xs.AllocLen != 0) check(xsDeInit(&xs)); /** Failure ignored. **/
	
	return rval;
    }


/*** htconnInitialize - register with the ht_render module.
 ***/
int
htconnInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Event-Action Connector Driver");
	strcpy(drv->WidgetName,"connector");
	drv->Render = htconnRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    return 0;
    }
