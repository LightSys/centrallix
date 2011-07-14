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

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2010 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_uawindow.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 22, 2010					*/
/* Description:	HTML Widget driver for a user-agent window        	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTUAWIN;


/*** htuawinRender - generate the HTML code for the page.
 ***/
int
htuawinRender(pHtSession s, pWgtrNode tree, int z)
    {
    int id;
    char name[64];
    int is_shared = 0;
    int is_multi = 0;
    int action_routing = 0; /* 0=most recently opened; 1=topmost; 2=all */
    char path[OBJSYS_MAX_PATH+1];
    int width = 640;
    int height = 480;
    char* ptr;

    	/** Get an id for this. **/
	id = (HTUAWIN.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Path for the window .app file **/
	if (wgtrGetPropertyValue(tree,"path",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(path,ptr,sizeof(path));

	/** width/height **/
	wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&width));
	wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&height));

	/** Share this window with other references in the same session context? (no) **/
	is_shared = htrGetBoolean(tree, "shared", 0);

	/** Allow multiple instances of the window? (no) **/
	is_multi = htrGetBoolean(tree, "multiple_instantiation", 0);

	/** Routing of actions to windows when there are multiple of them **/
	if (wgtrGetPropertyValue(tree, "action_routing", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr, "mostrecent")) action_routing = 0;
	    else if (!strcmp(ptr, "topmost")) action_routing = 1;
	    else if (!strcmp(ptr, "all")) action_routing = 2;
	    else
		{
		mssError(1, "HTUAWIN", "Invalid action_routing '%s' for widget '%s'", ptr, name);
		return -1;
		}
	    }

	/** widget init **/
	htrAddScriptInit_va(s, "    uw_init(nodes[\"%STR&SYM\"], {shared:%INT, multi:%INT, routing:%INT, path:\"%STR&JSSTR\", w:%INT, h:%INT} );\n",
		name, is_shared, is_multi, action_routing, path, width, height
		);

	/** JavaScript include file **/
	htrAddScriptInclude(s, "/sys/js/htdrv_uawindow.js", 0);

	/** object linkages **/
	htrAddWgtrCtrLinkage(s, tree, "_parentctr");

	/** Check for more sub-widgets within the vbl entity. **/
	htrRenderSubwidgets(s, tree, z+2);

    return 0;
    }


/*** htuawinInitialize - register with the ht_render module.
 ***/
int
htuawinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"User-Agent Window Object Driver");
	strcpy(drv->WidgetName,"window");
	drv->Render = htuawinRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTUAWIN.idcnt = 0;

    return 0;
    }
