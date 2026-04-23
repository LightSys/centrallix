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
#include "stparse_ne.h"

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
/* Module: 	htdrv_parameter.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 15, 2007    					*/
/* Description:	HTML Widget driver for a 'parameter', which is used to	*/
/*		pass information to/from an application or component.	*/
/************************************************************************/


/*** htparamRender - generate the HTML code for the page.
 ***/
int
htparamRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char paramname[64];
    char type[32];
    char findcontainer[64];
    pStruct find_inf;
    int deploy_to_client;

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTPARAM", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Get name and type **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1, "HTPARAM", "Failed to get name.");
	    goto err;
	    }
	strtcpy(name,ptr,sizeof(name));
	if (wgtrGetPropertyValue(tree,"type",DATA_T_STRING,POD(&ptr)) != 0)
	    ptr = "string";
	strtcpy(type,ptr,sizeof(type));
	const int datatype = objTypeID(type);
	if (datatype < 0 && strcmp(type, "object") != 0)
	    {
	    mssError(1, "HTPARAM", "Invalid datatype %d.", datatype);
	    goto err;
	    }

	/** Specify name of param (in case param widget name isn't the desired param name) **/
	if (wgtrGetPropertyValue(tree,"param_name",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(paramname, ptr, sizeof(paramname));
	else
	    strtcpy(paramname, name, sizeof(paramname));

	/** Make param available for client expressions? **/
	deploy_to_client = htrGetBoolean(tree, "deploy_to_client", s->IsDynamic || !strcmp(type, "object") || (tree->Parent && strcmp(tree->Parent->Type, "widget/component-decl") != 0));

	if (!strcmp(type, "object") && deploy_to_client && 
		wgtrGetPropertyValue(tree,"find_container",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(findcontainer, ptr, sizeof(findcontainer));
	else
	    findcontainer[0] = '\0';

	/** JavaScript include file **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_parameter.js", 0) != 0) goto err;

	/** Value supplied? **/
	if (deploy_to_client)
	    find_inf = stLookup_ne(s->Params, name);
	else
	    find_inf = NULL;

	/** Parameter has presentation-hints components to it.  Set those. **/
	const int has_find_inf = (find_inf != NULL && find_inf->StrVal != NULL && find_inf->StrVal[0] != '\0');
	if (deploy_to_client)
	    {
	    bool successful = false;
	    XString xs = { AllocLen: 0 };
	    pObjPresentationHints hints = NULL;

	    /** Script init **/
	    if (htrAddScriptInit_va(s,
		"\tpa_init(wgtrGetNodeRef(ns, '%STR&SYM'), { "
		    "name:'%STR&JSSTR', "
		    "type:'%STR&JSSTR', "
		    "findc:'%STR&JSSTR', "
		    "val:%['%STR&HEX'%]%[null%], "
		"});\n", 
		name, paramname, type, findcontainer,
		(has_find_inf), (has_find_inf) ? find_inf->StrVal : "null", (!has_find_inf)
	    ) != 0)
		{
		mssError(0, "HTPARAM", "Failed to write JS init call.");
		goto err_deploy;
		}

	    /** Get presentation hints. **/
	    hints = wgtrWgtToHints(tree);
	    if (hints == NULL)
		{
		mssError(0, "HTPARAM", "Failed to get presentation hints.");
		goto err_deploy;
		}

	    /** Write presentation hints. **/
	    if (check(xsInit(&xs)) != 0) goto err_deploy;
	    if (hntEncodeHints(hints, &xs) < 0)
		{
		mssError(0, "HTPARAM", "Failed to encode presentation hints.");
		goto err_deploy;
		}
	    if (htrAddScriptInit_va(s,
		"\tcx_set_hints(wgtrGetNodeRef(ns, '%STR&SYM'), '%STR&JSSTR', 'app');\n",
		name, xs.String
	    ) != 0)
		{
		mssError(0, "HTPARAM", "Failed to write JS for presentation hints.");
		goto err_deploy;
		}

	    if (htrAddScriptInit_va(s, "\twgtrGetNodeRef(ns, '%STR&SYM').Verify();\n", name) != 0)
		{
		mssError(0, "HTPARAM", "Failed to write JS to verify presentation hints.");
		goto err_deploy;
		}

	    /** Success. **/
	    successful = true;

    err_deploy:
	    /** Clean up. **/
	    if (check(xsDeInit(&xs)) != 0) goto err_deploy;
	    if (objFreeHints(hints) != 0)
		{
		mssError(0, "HTPARAM", "Failed to free presentation hints.");
		goto err_deploy;
		}

	    /** Handle errors. **/
	    if (!successful) goto err;
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NORENDER;
	    }

	/** Nonvisual widget. **/
	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;
	
	/** Success. **/
	return 0;

    err:
	mssError(0, "HTPARAM",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
    }


/*** htparamInitialize - register with the ht_render module.
 ***/
int
htparamInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Parameter Driver");
	strcpy(drv->WidgetName,"parameter");
	drv->Render = htparamRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    return 0;
    }
