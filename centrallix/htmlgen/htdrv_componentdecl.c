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
/* Module: 	htdrv_componentdecl.c             			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 11, 2003					*/
/* Description:	HTML Widget driver for component widget definition,	*/
/*		which is used to declare a component, its structure,	*/
/*		and its interface/params.  Use the normal component	*/
/*		widget (htdrv_component.c) to insert a component into	*/
/*		an application.						*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cxlib/cxsec.h"
#include "cxlib/datatypes.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "cxlib/xarray.h"
#include "ht_render.h"
#include "wgtr.h"


/** globals **/
static struct 
    {
    unsigned int id_count;
    }
    HTCMPD = {0u};


/*** htcmpd_internal_GetJSFnName - get the name for the JS function to declare
 *** the given component-decl-TYPE sub-widget.  Returns NULL if the widget
 *** type isn't handled here.
 ***/
static char*
htcmpd_internal_GetJSFnName(pWgtrNode node)
    {
    char* ptr;

	wgtrGetPropertyValue(node, "outer_type", DATA_T_STRING, POD(&ptr));
	if (strncmp(ptr, "widget/component-decl-", 22) != 0) return NULL;
	char* decl_type = ptr + 22;

	if (strcmp(decl_type, "action") == 0) return "addAction";
	if (strcmp(decl_type, "event") == 0) return "addEvent";
	if (strcmp(decl_type, "cprop") == 0) return "addProp";

    return NULL;
    }


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
    pWgtrNode sub_tree = NULL;
    pWgtrNode conn_tree = NULL;
    int is_visual = 1;
    char gbuf[256] = "";
    char* gname;

	/** Get an id for this. **/
	const unsigned int id = (HTCMPD.id_count++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTCMPD", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Is this a visual component? **/
	is_visual = htrGetBoolean(tree, "visual", 1);
	if (is_visual < 0) goto err;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name, ptr, sizeof(name));
	if (cxsecVerifySymbol(name) < 0)
	    {
	    mssError(1,"HTCMPD","Invalid name '%s' for component", name);
	    goto err;
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

	/** Include the js module **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_componentdecl.js", 0) != 0) goto err;

	/** DOM Linkages **/
	if (s->GraftPoint)
	    {
	    bool graft_successful = false;
	    strtcpy(gbuf, s->GraftPoint, sizeof(gbuf));
	    gname = strchr(gbuf,':');
	    if (!gname)
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto graft_done;
		}
	    *(gname++) = '\0';
	    if (strpbrk(gname,"'\"\\<>\r\n\t ") || strspn(gbuf,"w0123456789abcdef_") != strlen(gbuf))
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto graft_done;
		}

	    /** Add linkage. **/
	    if (htrAddWgtrCtrLinkage_va(s, tree, 
		"wgtrGetContainer(wgtrGetNode('%STR&SYM', '%STR&SYM'))",
		gbuf, gname
	    ) != 0)
		{
		mssError(0, "HTCMPD", "Failed to add container linkage.");
		goto graft_done;
		}

	    /** Add HTML DOM node. **/
	    if (htrAddBodyItem_va(s,
		"<a id='dname' target='%STR&SYM' href='.'></a>",
		s->Namespace->DName
	    ) != 0)
		{
		mssError(0, "HTCMPD", "Failed to write HTML linkage node.");
		goto graft_done;
		}

	    /** Success. **/
	    graft_successful = true;
	
    graft_done:
	    if (!graft_successful)
		{
		mssError(0, "HTCMPD", "Failed to mount graft point: \"%s\"", s->GraftPoint);
		goto err;
		}
	    }
	else
	    {
	    strcpy(gbuf,"");
	    gname="";
	    if (htrAddWgtrCtrLinkage(s, tree, "_parentctr") != 0)
		{
		mssError(0, "HTCMPD", "Failed to add container linkage.");
		goto err;
		}
	    }

	/** Warning message. **/
	if (gname[0] == '\0')
	    {
	    fprintf(stderr,
		"Warning: No graft point, so gname is empty, but it is required "
		"to be a valid symbol. Expect this component to fail to render.\n"
	    );
	    }

	/** Write the declNAME variable. **/
	if (htrAddScriptInit_va(s, "\n\t// Start of %STR component.\n"
	    "\tconst decl%POS = wgtrGetNodeRef(ns, '%STR&SYM');\n",
	    name, id, name
	) != 0)
	    {
	    mssError(0, "HTCMPD", "Failed to write the start of the JS init call.");
	    goto err;
	    }

	/** Init component **/
	const bool has_gbuf = (gbuf[0] != '\0');
	if (htrAddScriptInit_va(s,
	    "\tcmpd_init(wgtrGetNodeRef(ns, '%STR&SYM'), { "
		"vis:%POS, "
		"gns:%['%STR&SYM'%]%[null%], "
		"gname:'%STR&SYM'"
		"%[, expe:'%STR&SYM'%]"
		"%[, expa:'%STR&SYM'%]"
		"%[, expp:'%STR&SYM'%]"
		"%[, applyhint:'%STR&SYM'%]"
	    "});\n", 
	    name,
	    is_visual,
	    (has_gbuf), gbuf, (!has_gbuf),
	    gname,
	    (expose_events_for[0]  != '\0'), expose_events_for,
	    (expose_actions_for[0] != '\0'), expose_actions_for,
	    (expose_props_for[0]   != '\0'), expose_props_for,
	    (apply_hints_to[0]     != '\0'), apply_hints_to
	) != 0)
	    {
	    mssError(0, "HTCMPD", "Failed to write JS init call.");
	    goto err;
	    }

	/** Declare actions, events, and cprops for use in connectors. **/
	const unsigned int n_sub_trees = xaCount(&(tree->Children));
	for (unsigned int i = 0u; i < n_sub_trees; i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);

	    /** Get component action/event/cprop name **/
	    wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
	    strtcpy(subobj_name, ptr, sizeof(subobj_name));
	    if (cxsecVerifySymbol(subobj_name) < 0)
		{
		mssError(1,"HTCMPD","Invalid name '%s' for action/event/cprop in component '%s'", subobj_name, name);
		goto err;
		}

	    /** Only component-decl-TYPE sub-widgets are declared here. **/
	    char* fn_name = htcmpd_internal_GetJSFnName(sub_tree);
	    if (fn_name == NULL) continue;

	    /** Write the requested declaration function call. **/
	    if (htrAddScriptInit_va(s, "\tdecl%POS.%STR('%STR&SYM');\n", id, fn_name, subobj_name) != 0)
		{
		mssError(0, "HTCMPD", "Failed to write JS function init call.");
		goto err;
		}
	    sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
	    }

	/** Render sub-widgets. **/
	for (unsigned int i = 0u; i < n_sub_trees; i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    char* fn_name = htcmpd_internal_GetJSFnName(sub_tree);

	    /** Widget isn't a component-decl type we handle, render normally. **/
	    if (fn_name == NULL)
		{
		if (htrRenderWidget(s, sub_tree, z + 2) != 0)
		    {
		    mssError(0, "HTCMPD", "Failed to render widget in declared component.");
		    goto err;
		    }
		continue;
		}

	    /** Search for connectors inside component-decl-action widgets. **/
	    if (strcmp(fn_name, "addAction") == 0)
		{
		const unsigned int n_con_trees = xaCount(&(sub_tree->Children));
		for (unsigned int j = 0u; j < n_con_trees; j++)
		    {
		    conn_tree = xaGetItem(&(sub_tree->Children), j);
		    wgtrGetPropertyValue(conn_tree, "outer_type", DATA_T_STRING, POD(&ptr));
		    if (strcmp(ptr,"widget/connector") == 0)
			{
			/** Render connector. **/
			if (htrRenderWidget(s, conn_tree, z + 2) != 0)
			    {
			    mssError(0, "HTCMPD", "Failed to render connector in component-decl-action widget.");
			    goto err;
			    }
			}
		    }
		}
	    }

	/** Write JS init call. **/
	if (htrAddScriptInit_va(s, "\tcmpd_endinit(decl%POS);\n", id) != 0)
	    {
	    mssError(0, "HTCMPD", "Failed to write the end of the JS init call.");
	    goto err;
	    }

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTCMPD",
	    "Failed to render \"%s\":\"%s\" (id: %u).",
	    tree->Name, tree->Type, id
	);
	return -1;
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

	/** Register. **/
	htrRegisterDriver(drv);

	/** Declare support for DHTML user interface class **/
	htrAddSupport(drv, "dhtml");

    return 0;
    }
