#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
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
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTPARAM;


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
    int id;
    int i;
    XString xs;
    pObjPresentationHints hints;
    pStruct find_inf;
    int deploy_to_client;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML )
	    {
	    mssError(1,"HTPARAM","Netscape DOM or W3C DOM1HTML support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTPARAM.idcnt++);

	/** Get name and type **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0)
	    return -1;
	strtcpy(name,ptr,sizeof(name));
	if (wgtrGetPropertyValue(tree,"type",DATA_T_STRING,POD(&ptr)) != 0)
	    ptr = "string";
	strtcpy(type,ptr,sizeof(type));
	if (objTypeID(type) < 0 && strcmp(type,"object"))
	    {
	    mssError(1,"HTPARAM","Invalid parameter data type for '%s'", name);
	    return -1;
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
	htrAddScriptInclude(s, "/sys/js/htdrv_parameter.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	/** Value supplied? **/
	if (deploy_to_client)
	    find_inf = stLookup_ne(s->Params, name);
	else
	    find_inf = NULL;

	/** Parameter has presentation-hints components to it.  Set those. **/
	if (deploy_to_client)
	    {
	    /** Script init **/
	    htrAddScriptInit_va(s, "    pa_init(wgtrGetNodeRef(ns,\"%STR&SYM\"), {name:'%STR&JSSTR', type:'%STR&JSSTR', findc:'%STR&JSSTR', val:%[null%]%[\"%STR&HEX\"%]});\n", 
		    name, paramname, type, findcontainer, !find_inf || !find_inf->StrVal, find_inf && find_inf->StrVal, find_inf?find_inf->StrVal:"");

	    hints = wgtrWgtToHints(tree);
	    if (!hints)
		{
		mssError(0, "HTPARAM", "Error in parameter data");
		return -1;
		}
	    xsInit(&xs);
	    hntEncodeHints(hints, &xs);
	    htrAddScriptInit_va(s, "    cx_set_hints(wgtrGetNodeRef(ns,\"%STR&SYM\"), '%STR&JSSTR', 'app');\n",
		    name, xs.String);
	    xsDeInit(&xs);
	    objFreeHints(hints);

	    htrAddScriptInit_va(s, "    wgtrGetNodeRef(ns,\"%STR&SYM\").Verify();\n", name);
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NORENDER;
	    }

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Check for more sub-widgets within the param **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
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

	HTPARAM.idcnt = 0;

    return 0;
    }
