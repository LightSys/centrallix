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
/* Module: 	htdrv_scrollpane.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 30, 1998					*/
/* Description:	HTML Widget driver for a scrollpane -- a css layer with	*/
/*		a scrollable layer and a scrollbar for scrolling the	*/
/*		layer.  Can contain most objects, except for framesets.	*/
/************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "ht_render.h"
#include "obj.h"


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTSPANE;


/*** htspaneRender - generate the HTML code for the page.
 ***/
int
htspaneRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* str;
	
	/** Get an id for this scrollpane. **/
	const int id = (HTSPANE.idcnt++);
	
	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTSPANE", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }
	
	/** Get name. **/
	char name[64];
	if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&str)) != 0)
	    {
	    mssError(1, "HTSPANE", "widget/scrollpane must have a 'name' property.");
	    goto err;
	    }
	strtcpy(name, str, sizeof(name));
	
	/** Get layout data (required). **/
	int x, y, w, h;
	if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&x)) != 0) 
	    {
	    mssError(1, "HTSPANE", "widget/scrollpane must have an 'x' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&y)) != 0)
	    {
	    mssError(1, "HTSPANE", "widget/scrollpane must have a 'y' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&w)) != 0)
	    {
	    mssError(1, "HTSPANE", "widget/scrollpane must have a 'width' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&h)) != 0)
	    {
	    mssError(1, "HTSPANE", "widget/scrollpane must have a 'height' property.");
	    goto err;
	    }
	
	/** Get the background color or image. **/
	char background_color[64] = "";
	if (wgtrGetPropertyValue(tree, "bgcolor", DATA_T_STRING, POD(&str)) == 0)
	    {
	    strtcpy(background_color, str, sizeof(background_color));
	    }
	char background_image[64] = "";
	if (wgtrGetPropertyValue(tree, "background", DATA_T_STRING, POD(&str)) == 0)
	    {
	    strtcpy(background_image, str, sizeof(background_image));
	    }
	
	/** Get visibility. **/
	const int visible = htrGetBoolean(tree, "visible", 1);
	if (visible == -1) goto err;
	
 	/** Link the widget and container to DOM nodes. **/
	if (htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, 'sp%POSarea')", id) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to add container linkage.");
	    goto err;
	    }
	if (htrAddWgtrObjLinkage_va(s, tree, "sp%POSpane", id) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to add object linkage.");
	    goto err;
	    }
	
	/** Include scrollpane script and its dependencies. **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js",  0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_scrollpane.js", 0) != 0) goto err;
	
	/** Add the event handling scripts **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "sp", "sp_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "sp", "sp_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "sp", "sp_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "sp", "sp_mouseup")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "WHEEL",     "sp", "sp_wheel")     != 0) goto err;
	
	/** Init the JS scripts. **/
	if (htrAddScriptInit_va(s,
	    "\tsp_init({"
		"layer: wgtrGetNodeRef(ns, '%STR&SYM'), "
		"area_name: 'sp%POSarea', "
		"thumb_name: 'sp%POSthumb', "
	    "});\n",
	    name, id, id
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write JS init call.");
	    goto err;
	    }
	
	/** Write the scrollpane. **/
	if (htrAddBodyItem_va(s,
	    "<div id='sp%POSpane' "
		"style='"
		    "position:absolute; "
		    "visibility:%STR; "
		    "overflow:clip; "
		    "left:"ht_flex_format"; "
		    "top:"ht_flex_format"; "
		    "width:"ht_flex_format"; "
		    "height:"ht_flex_format"; "
		    "z-index:%POS; "
		"'"
	    ">\n",
	    id,
	    (visible) ? "inherit" : "hidden",
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    ht_flex_h(h, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for main scrollpane div.");
	    goto err;
	    }
	
	/** Write shared CSS for the following UI elements. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t.sp%POS_scroll { "
		"position:absolute; "
		"left:"ht_flex_format"; "
	    "}\n",
	    id,
	    ht_flex(w - 18, tree->width, 1.0)
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write shared CSS ui elements.");
	    goto err;
	    }
	
	/** Write the up button. **/
	if (htrAddBodyItem_va(s,
	    "<img "
		"data-type='up' "
		"id='sp%POSup' "
		"class='sp%POS_scroll' "
		"src='/sys/images/ico13b.gif' "
		"alt='up_button' "
		"style='"
		    "top:0px; "
		    "cursor:pointer; "
		"'"
	    ">",
	    id,
	    id
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for the down button.");
	    goto err;
	    }
	
	/** Write the scroll bar. **/
	if (htrAddBodyItem_va(s,
	    "<img "
		"data-type='bar' "
		"id='sp%POSbar' "
		"class='sp%POS_scroll' "
		"src='/sys/images/trans_1.gif' "
		"alt='scroll_bar' "
		"style='"
		    "top:18px; "
		    "width:18px; "
		    "height:"ht_flex_format"; "
		"'"
	    ">",
	    id,
	    id,
	    ht_flex(h - 36, tree->height, 1.0)
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for the scroll bar.");
	    goto err;
	    }
	
	/** Write the down button. **/
	if (htrAddBodyItem_va(s,
	    "<img "
		"data-type='down' "
		"id='sp%POSdown' "
		"class='sp%POS_scroll' "
		"src='/sys/images/ico12b.gif' "
		"alt='down_button' "
		"style='"
		    "top:"ht_flex_format"; "
		    "cursor:pointer; "
		"'"
	    ">",
	    id,
	    id,
	    ht_flex(h - 18, tree->height, 1.0)
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for the down button.");
	    goto err;
	    }
	
	/** Write the scroll thumb. **/
	if (htrAddBodyItem_va(s,
	    "<div "
		"id='sp%POSthumb' "
		"class='sp%POS_scroll' "
		"style='"
		    "visibility:inherit; "
		    "top:18px; "
		    "width:18px; "
		    "z-index:%POS; "
		"'"
	    ">"
		"<img data-type='thumb' src='/sys/images/ico14b.gif' alt='scroll_thumb'>"
	    "</div>\n",
	    id,
	    id,
	    z + 1
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for the scroll thumb.");
	    goto err;
	    }
	
	/** Write the scroll area. **/
	if (htrAddBodyItem_va(s,
	    "<div "
		"id='sp%POSarea' "
		"style='"
		    "position:absolute; "
		    "visibility:inherit; "
		    "left:0px; "
		    "top:0px; "
		    "width:"ht_flex_format"; "
		    "height:"ht_flex_format"; "
		    "z-index:%POS; "
		"'"
	    ">",
	    id,
	    ht_flex(w - 18, ht_get_parent_w(tree), 1.0),
	    ht_flex(h,      ht_get_parent_h(tree), 1.0),
	    z + 1
	) != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write HTML for the scroll area.");
	    goto err;
	    }
	
	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 2) != 0) goto err;
	
	/** Close the final <div>s. **/
	if (htrAddBodyItem(s,"</div></div>\n") != 0)
	    {
	    mssError(0, "HTSPANE", "Failed to write closing HTML tags.");
	    goto err;
	    }
	
	return 0;
	
    err:
	mssError(0, "HTTAB",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htspaneInitialize - register with the ht_render module.
 ***/
int
htspaneInitialize()
    {
    pHtDriver drv;
    
	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;
	
	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Widget/scrollpane Driver");
	strcpy(drv->WidgetName,"scrollpane");
	drv->Render = htspaneRender;
	
	/** Events **/ 
	htrAddEvent(drv, "Scroll");
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "Wheel");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");
	
	/** Register. **/
	htrRegisterDriver(drv);
	
	htrAddSupport(drv, "dhtml");
	HTSPANE.idcnt = 0;
    
    return 0;
    }
