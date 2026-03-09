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
    
	if (!(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS) && !s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE)
	    {
	    mssError(1, "HTSPANE", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS or IE DOM or Netscape DOM required.");
	    goto err;
	    }
	
	/** Get an id for this scrollpane. **/
	const int id = (HTSPANE.idcnt++);
	
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
	
	/** DOM Linkages. **/
	htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, \"sp%POSarea\")",id);
	htrAddWgtrObjLinkage_va(s, tree, "sp%POSpane",id);
	
	/** Include scrollpane script and its dependencies. **/
	htrAddScriptInclude(s, "/sys/js/htdrv_scrollpane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	
	/** Init the JS scripts. **/
	htrAddScriptInit_va(s,
	    "\tsp_init({"
		"layer: wgtrGetNodeRef(ns, '%STR&SYM'), "
		"area_name: 'sp%POSarea', "
		"thumb_name: 'sp%POSthumb', "
	    "});\n",
	    name, id, id
	);
	
	/** Write html and styles. **/
	if (s->Capabilities.Dom1HTML)
	    {
	    /** Write the scrollpane. **/
	    htrAddBodyItem_va(s,
		"<div id='sp%POSpane' "
		    "style='"
			"position:absolute; "
			"visibility:%STR; "
			"left:"ht_flex_format"; "
			"top:"ht_flex_format"; "
			"width:"ht_flex_format"; "
			"height:"ht_flex_format"; "
			"overflow:clip; "
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
	    );
	    
	    /** Write shared CSS for the following UI elements. **/
	    htrAddStylesheetItem_va(s,
		"\t\t.sp%POS_scroll { "
		    "position:absolute; "
		    "left:"ht_flex_format"; "
		"}\n",
		id,
		ht_flex(w - 18, tree->width, 1.0)
	    );
	    
	    /** Write the up button. **/
	    htrAddBodyItem_va(s,
		"<img "
		    "name='u' "
		    "id='sp%POSup' "
		    "class='sp%POS_scroll' "
		    "src='/sys/images/ico13b.gif' "
		    "style='"
			"top:0px; "
			"cursor:pointer; "
		    "'"
		"/>",
		id,
		id
	    );
	    
	    /** Write the scroll bar. **/
	    htrAddBodyItem_va(s,
		"<img "
		    "name='b' "
		    "id='sp%POSbar' "
		    "class='sp%POS_scroll' "
		    "src='/sys/images/trans_1.gif' "
		    "style='"
			"top:18px; "
			"width:18px; "
			"height:"ht_flex_format"; "
		    "'"
		"/>",
		id,
		id,
		ht_flex(h - 36, tree->height, 1.0)
	    );
	    
	    /** Write the down button. **/
	    htrAddBodyItem_va(s,
		"<img "
		    "name='d' "
		    "id='sp%POSdown' "
		    "class='sp%POS_scroll' "
		    "src='/sys/images/ico12b.gif' "
		    "style='"
			"top:"ht_flex_format"; "
			"cursor:pointer; "
		    "'"
		"/>",
		id,
		id,
		ht_flex(h - 18, tree->height, 1.0)
	    );
	    
	    /** Write the scroll thumb. **/
	    htrAddBodyItem_va(s,
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
		    "<img name='t' src='/sys/images/ico14b.gif'>"
		"</div>\n",
		id,
		id,
		z + 1
	    );
	    
	    /** Write the scroll area. **/
	    htrAddBodyItem_va(s,
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
	    );
	    }
	else if (s->Capabilities.Dom0NS)
	    {
	    /** Write CSS for everything. **/
	    htrAddStylesheetItem_va(s,
		"\t\t#sp%POSpane { "
		    "position:absolute; "
		    "visibility:%STR; "
		    "overflow:hidden; "
		    "left:"ht_flex_format"; "
		    "top:"ht_flex_format"; "
		    "width:"ht_flex_format"; "
		    "height:"ht_flex_format"; "
		    "z-index:%POS; "
		"}\n",
		id,
		(visible) ? "inherit" : "hidden",
		ht_flex_x(x, tree),
		ht_flex_y(y, tree),
		ht_flex_w(w, tree),
		ht_flex_h(h, tree),
		z
	    );
	    htrAddStylesheetItem_va(s,
		"\t\t#sp%POSarea { "
		    "position:absolute; "
		    "visibility:inherit; "
		    "left:0px; "
		    "top:0px; "
		    "width:"ht_flex_format"; "
		    "z-index:%POS; "
		"}\n",
		id,
		ht_flex(w - 18, ht_get_parent_w(tree), 1.0),
		z + 1
	    );
	    htrAddStylesheetItem_va(s,
		"\t\t#sp%POSthumb { "
		    "position:absolute; "
		    "visibility:inherit; "
		    "left:"ht_flex_format"; "
		    "top:18px; "
		    "width:18px; "
		    "z-index:%POS; "
		"}\n",
		id,
		ht_flex(w - 18, tree->width, 1.0),
		z + 1
	    );
	    
	    /** Write the scrollpane. **/
	    htrAddBodyItem_va(s,
		"<div id=\"sp%POSpane\">"
		    "<table "
			"%[bgcolor=\"%STR&HTE\"%] "
			"%[background=\"%STR&HTE\"%] "
			"border='0' "
			"cellspacing='0' "
			"cellpadding='0' "
			"width='%POS'"
		    ">",
		id,
		*background_color, background_color,
		*background_image, background_image,
		w
	    );
	    
	    htrAddBodyItem_va(s,
		/** Write the up button. **/
		"<tr><td align=right>"
		    "<img name='u' src='/sys/images/ico13b.gif'>"
		"</td></tr>"
		
		/** Write the scroll bar. **/
		"<tr><td align=right>"
		    "<img name='b' src='/sys/images/trans_1.gif' height='%POSpx' width='18px'>"
		"</td></tr>"
		
		/** Write the down button. **/
		"<tr><td align=right>"
		    "<img name='d'src='/sys/images/ico12b.gif'>"
		"</td></tr>"
		
		/** Close the scrollpane table (see above). **/
		"</table> <!-- Easter Egg #3 -->\n",
		h - 36
	    );
	    
	    /** Write the scroll thumb. **/
	    htrAddBodyItem_va(s,
		"<div id=\"sp%POSthumb\">"
		    "<img name='t' src='/sys/images/ico14b.gif'>"
		"</div>\n",
		id
	    );
	    
	    /** Write the scroll area. **/
	    htrAddBodyItem_va(s,
		"<div ID=\"sp%POSarea\">"
		    "<table "
			"border='0' "
			"cellpadding='0' "
			"cellspacing='0' "
			"width='%POSpx' "
			"height='%POSpx' "
		    "><tr><td>",
		id,
		w - 2,
		h - 2
	    );
	    }
	
	/** Render children/subwidgets. **/
	for (int i = 0; i < xaCount(&(tree->Children)); i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z + 2);
	
	/** Close the final <div>s. **/
	if (s->Capabilities.Dom1HTML)
	    {
	    htrAddBodyItem(s,"</div></div>\n");
	    }
	else if (s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem(s,"</td></tr></table></div></div>\n");
	    }
	
	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "sp", "sp_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "sp", "sp_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "sp", "sp_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "sp", "sp_mouseover");
	htrAddEventHandlerFunction(s, "document", "WHEEL",     "sp", "sp_wheel");
	
	return 0;
	
    err:
	mssError(0, "HTSPANE", "Failed to render widget/scrollpane.");
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
