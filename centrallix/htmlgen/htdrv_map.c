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
/* Copyright (C) 2012-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_map.c		       				*/
/* Author:	Andrew Dillon / Eliezer Rodriguez			*/
/* Creation:	March 20, 2012						*/
/* Description:	HTML Widget driver for maps using OpenLayers		*/
/************************************************************************/

/** globals **/
static struct
{
	int idcnt;
} HTMAP;

/*** htmapRender - generate the HTML code for the page.
 ***/
int htmapRender(pHtSession s, pWgtrNode map_node, int z)
{
	char *ptr;
	char name[64];
	char main_bg[128];
	char osrc[64];
	int x = -1, y = -1, w, h;
	int allow_select, show_select;

	/** Get an id for this. **/
	const int id = (HTMAP.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTMAP", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(map_node, "x", DATA_T_INTEGER, POD(&x)) != 0)
		x = 0;
	if (wgtrGetPropertyValue(map_node, "y", DATA_T_INTEGER, POD(&y)) != 0)
		y = 0;
	if (wgtrGetPropertyValue(map_node, "width", DATA_T_INTEGER, POD(&w)) != 0)
	{
		mssError(1, "HTMAP", "Pane widget must have a 'width' property");
		return -1;
	}
	if (wgtrGetPropertyValue(map_node, "height", DATA_T_INTEGER, POD(&h)) != 0)
	{
		mssError(1, "HTMAP", "Pane widget must have a 'height' property");
		return -1;
	}

	/** Background color/image? **/
	htrGetBackground(map_node, NULL, s->Capabilities.CSS2, main_bg, sizeof(main_bg));

	/** objectsource specified? **/
	if (wgtrGetPropertyValue(map_node, "source", DATA_T_STRING, POD(&ptr)) != 0)
		strcpy(osrc, "");
	else
		strtcpy(osrc, ptr, sizeof(osrc));

	/** allow selection of objects? **/
	allow_select = htrGetBoolean(map_node, "allow_selection", 0);

	/** show current selection? **/
	show_select = htrGetBoolean(map_node, "show_selection", 0);

	/** Get name **/
	if (wgtrGetPropertyValue(map_node, "name", DATA_T_STRING, POD(&ptr)) != 0)
		return -1;
	strtcpy(name, ptr, sizeof(name));

	/** Add css item for the layer **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#map%POSbase { "
	        "position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"left:%INTpx; "
		"top:%INTpx; "
		"width:%POSpx; "
		"height:%POSpx; "
		"z-index:%POS; "
		"%STR "
	    "}\n",
	    id, x, y, w, h, z, main_bg
	) != 0) 
	    {
	    mssError(0, "HTMAP", "Failed to write base CSS.");
	    goto err;
	    }

 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, map_node, "map%POSbase", id) != 0) goto err;

	/** Include supporting JS files, **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_map.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/openlayers/build/ol.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/openlayers/build/ol.js.map", 0) != 0) goto err;
	//if (htrAddScriptInclude(s, "https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js", 0) != 0) goto err;
	//if (htrAddScriptInclude(s, "https://stackpath.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js", 0) != 0) goto err;

	/** Add event handlers. **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP", "map", "map_mouseup") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "map", "map_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "map", "map_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "map", "map_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "map", "map_mouseout") != 0) goto err;

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\tmap_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"osrc:%[wgtrGetNodeRef(ns, '%STR&SYM')%]%[null%], "
		"name:'%STR&SYM', "
		"allow_select:%INT, "
		"show_select:%INT, "
	    "});\n",
	    name, (osrc[0] != '\0'), osrc, (osrc[0] == '\0'), name,
	    allow_select, show_select
	) != 0)
	    {
	    mssError(0, "HTMAP", "Failed to write JS init call.");
	    goto err;
	    }

	/** HTML body <DIV> element to be used by the OpenLayers map. **/
	if (htrAddBodyItem_va(s, "<div id='map%POSbase'>\n", id) != 0)
	    {
	    mssError(0, "HTMAP", "Failed to write HTML for map open tag.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, map_node, z + 2) != 0) goto err;

	/** End the containing div. **/
	if (htrAddBodyItem(s, "</div>\n") != 0)
	    {
	    mssError(0, "HTMAP", "Failed to write HTML for map closing tag.");
	    goto err;
	    }

	/** Success. **/
	return 0;
    
	err:
	mssError(0, "HTMAP",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    map_node->Name, map_node->Type, id
	);
	return -1;
}

/*** htmapInitialize - register with the ht_render module.
 ***/
int htmapInitialize()
{
	pHtDriver drv;

	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv)
		return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name, "DHTML Map Driver");
	strcpy(drv->WidgetName, "map");
	drv->Render = htmapRender;

	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");
	htrAddEvent(drv, "Click");
	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTMAP.idcnt = 0;

	return 0;
}
