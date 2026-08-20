#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

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
/* Copyright (C) 1999-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_window.c      					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 16, 1999					*/
/* Description:	HTML Widget driver for a window -- a DHTML layer that	*/
/*		can be dragged around the screen and appears to have	*/
/*		a 'titlebar' with a close (X) button on it.		*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTWIN;


/*** htwinRender - generate the HTML code for the page.
 ***/
int
htwinRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int visible = 1;
    char background_style[128] = "";
    char header_background_style[128] = "";
    char text_color[64] = "";
    int has_titlebar = 1;
    char title[128];
    int is_dialog_style = 0;
    int gshade = 0;
    int is_toplevel = 0;
    int is_modal = 0;
    char icon[128];
    int shadow_offset, shadow_radius, shadow_angle;
    char shadow_color[128];
    int border_radius;
    char border_color[128];
    char border_style[32];
    int border_width;

	/** Get an id for this. **/
	const int id = (HTWIN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTWIN", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	int x, y, w, h;
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x = 0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y = 0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'height' property");
	    return -1;
	    }

	/** Border radius, color, and style. **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;
	if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_color, "#ffffff");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree,"border_style",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));
	if (!strcmp(border_style, "none") || !strcmp(border_style, "hidden"))
	    border_width=0;
	else
	    border_width=1;

	/** Drop shadow **/
	shadow_offset=0;
	if (wgtrGetPropertyValue(tree, "shadow_offset", DATA_T_INTEGER, POD(&shadow_offset)) == 0 && shadow_offset > 0)
	    shadow_radius = shadow_offset+1;
	else
	    shadow_radius = 0;
	wgtrGetPropertyValue(tree, "shadow_radius", DATA_T_INTEGER, POD(&shadow_radius));
	if (shadow_radius > 0)
	    {
	    if (wgtrGetPropertyValue(tree, "shadow_color", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(shadow_color, ptr, sizeof(shadow_color));
	    else
		strcpy(shadow_color, "black");
	    }
	if (wgtrGetPropertyValue(tree, "shadow_angle", DATA_T_INTEGER, POD(&shadow_angle)) != 0)
	    shadow_angle = 135;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Is this a toplevel window? **/
	is_toplevel = htrGetBoolean(tree, "toplevel", 0);

	/** Is this a modal window? **/
	is_modal = htrGetBoolean(tree, "modal", 0);

	/** Check background color **/
	htrGetBackground(tree, NULL, 1, background_style, sizeof(background_style));

	/** Check header background color/image **/
	if (htrGetBackground(tree, "hdr", 1, header_background_style, sizeof(header_background_style)) != 0)
	    strcpy(header_background_style, background_style);

	/** Check title text color. **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(text_color, ptr, sizeof(text_color));
	else
	    strcpy(text_color, "black");

	/** Check window title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(title,ptr, sizeof(title));
	else
	    strcpy(title,name);

	/** Marked not visible? **/
	visible = htrGetBoolean(tree, "visible", 1);

	/** No titlebar? **/
	has_titlebar = htrGetBoolean(tree, "titlebar", 1);

	/** Dialog or window style? **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
	    is_dialog_style = 1;

	/** Graphical window shading? **/
	gshade = htrGetBoolean(tree, "gshade", 0);

	/** Window icon? **/
	if (wgtrGetPropertyValue(tree, "icon", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(icon,ptr,sizeof(icon));
	    }
	else
	    {
	    strcpy(icon, "/sys/images/centrallix_18x18.gif");
	    }

	/** Compute titlebar width & height - includes edge below titlebar. **/
	int title_bar_height = (has_titlebar) ? 24 : 0;

	/** Error if the specified size is too small to draw. **/
	int min_width = 2*border_width;
	int min_height = 2*border_width + title_bar_height;
	if (w < min_width || h < min_height)
	    {
	    mssError(1, "HTWIN",
		"Window \"%s\" is %dx%d, too small to draw: a window with %s "
		"and a %dpx border must be at least %dx%d.",
		tree->Name, w, h,
		(has_titlebar) ? "a titlebar" : "no titlebar",
		border_width, min_width, min_height
	    );
	    goto err;
	    }

	/** Draw the main window layer and outer edge. **/
	/*** We don't even bother making the position flex responsively because
	 *** the JS re-places the window on resize anyway.  The size is not
	 *** responsive: a window keeps the size it was given.
	 ***
	 *** border-box means width/height are the size the app asked for, with
	 *** the border drawn inside them, so no border arithmetic is needed here.
	 ***/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#wn%POSbase { "
		"position:absolute; "
		"visibility:%STR; "
		"overflow:hidden; "
		"box-sizing:border-box; "
		"left:%INTpx; "
		"top:%INTpx; "
		"width:%POSpx; "
		"height:%POSpx; "
		"z-index:%POS; "
		"border-style:%STR&CSSVAL; "
		"border-width:%INTpx; "
		"border-color:%STR&CSSVAL; "
		"border-radius:%INTpx; "
	    "}\n",
	    id,
	    (visible) ? "inherit" : "hidden",
	    x, y, w, h,
	    z + 100,
	    border_style,
	    border_width,
	    border_color,
	    border_radius
	) != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write CSS for window base.");
	    goto err;
	    }
	if (shadow_radius > 0)
	    {
	    double shadow_angle_radians = (double)shadow_angle * M_PI/180;
	    if (htrAddStylesheetItem_va(s,
		"\t\t#wn%POSbase { "
		    "box-shadow: "
			"%DBLpx "
			"%DBLpx "
			"%POSpx "
			"%STR&CSSVAL; "
		"}\n", id,
		sin(shadow_angle_radians) *   shadow_offset,
		cos(shadow_angle_radians) * (-shadow_offset),
		shadow_radius,
		shadow_color
	    ) != 0)
		{
		mssError(0, "HTWIN", "Failed to write CSS for window shadow.");
		goto err;
		}
	    }

	/*** The inner container fills what is left of the base below the titlebar.
	 *** The base is border-box, so its content box is the size it was given
	 *** less its border.
	 ***/
	int main_width = w - 2*border_width;
	int main_height = h - 2*border_width - title_bar_height;

	/** Only the edges around the inner container depend on dialog vs. window style **/
	int main_side_width, main_top_width;
	char* border_color_str;
	if (is_dialog_style)
	    {
	    main_side_width = 0;
	    main_top_width = (has_titlebar) ? 1 : 0;
	    border_color_str = "white";
	    }
	else
	    {
	    main_side_width = 1;
	    main_top_width = (has_titlebar) ? 0 : 1;
	    border_color_str = "gray white white gray";
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t#wn%POSmain { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"box-sizing:border-box; "
		"left:0px; "
		"top:%INTpx; "
		"width:%POSpx; "
		"height:%POSpx; "
		"clip:rect(0px, %INTpx, %INTpx, 0px); "
		"border-style:solid; "
		"border-color:%STR&CSSVAL; "
		"border-width:%POSpx %POSpx %POSpx %POSpx; "
		"z-index:%POS; "
		"%STR"
	    "}\n",
	    id,
	    title_bar_height,
	    main_width,
	    main_height,
	    main_width, main_height,
	    border_color_str,
	    main_top_width, main_side_width, main_side_width, main_side_width,
	    z + 1,
	    background_style
	) != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write CSS for window main container.");
	    goto err;
	    }

	/** Write JS globals and includes. **/
	if (htrAddScriptGlobal(s, "wn_clicked", "0",     0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_current", "null",  0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_list",    "[]",    0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_moved",   "0",     0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_msx",     "null",  0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_msy",     "null",  0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_new_x",   "null",  0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_new_y",   "null",  0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_top_z",   "10000", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "wn_topwin",  "null",  0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_window.js",    0) != 0) goto err;

	/** DOM Linkages **/
	if (htrAddWgtrObjLinkage_va(s, tree, "wn%POSbase", id) != 0) goto err;
	if (htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, 'wn%POSmain')", id) != 0) goto err;

	/** Event handler for mousedown/up/click/etc **/
	if (htrAddEventHandlerFunction(s, "document", "DBLCLICK",  "wn", "wn_dblclick")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "wn", "wn_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "wn", "wn_mouseup")   != 0) goto err;

	/** Mouse move event handler -- when user drags the window **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "wn", "wn_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "wn", "wn_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "wn", "wn_mouseout")  != 0) goto err;

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\twn_init({ "
		"mainlayer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"clayer:wgtrGetContainer(wgtrGetNodeRef(ns, '%STR&SYM')), "
		"titlebar:%[htr_subel(wgtrGetNodeRef(ns, '%STR&SYM'), 'wn%POStitlebar')%]%[null%], "
		"gshade:%INT, "
		"toplevel:%INT, "
		"modal:%INT, "
		"centeredx:%INT, "
		"centeredy:%INT, "
	    "});\n",
	    name,
	    name,
	    has_titlebar, name, id, !has_titlebar,
	    gshade,
	    is_toplevel,
	    is_modal,
	    !!(tree->Flags & WGTR_F_CENTEREDX),
	    !!(tree->Flags & WGTR_F_CENTEREDY)
	) != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write JS init call.");
	    goto err;
	    }

	/** Write HTML for the child window. **/
	if (htrAddBodyItem_va(s, "<div id='wn%POSbase' class='wnbase'>\n", id) != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write HTML for window container.");
	    goto err;
	    }
	if (has_titlebar)
	    {
	    /** Write CSS and HTML for the title bar. **/
	    if (htrAddStylesheetItem_va(s,
		"\t\t#wn%POStitlebar { "
		    "position:absolute; "
		    "visibility:inherit; "
		    "overflow:hidden; "
		    "box-sizing:border-box; "
		    "cursor:grab; "
		    "left:0px; "
		    "top:0px; "
		    "height:%POSpx; "
		    "width:100%%; "
		    "z-index:%POS; "
		    "color:%STR&CSSVAL; "
		    "border-style:solid; "
		    "border-width:0px 0px 1px 0px; "
		    "border-color:gray; "
		    "%STR"
		"}\n",
		id,
		title_bar_height,
		z + 1,
		text_color,
		header_background_style
	    ) != 0)
		{
		mssError(0, "HTWIN", "Failed to write styles for window title bar.");
		goto err;
		}
	    if (htrAddBodyItem_va(s,
		/*** Note: The user grabs the title bar to drag a window.
		 *** Thus, its child layers need pointer-events:none to avoid
		 *** stealing this event.  The close button is an exception.
		 ***/
		"<div id='wn%POStitlebar' class='wntitlebar'>"
		    "<img style='position:absolute; top:2px; left:4px; width:18px; height:18px; pointer-events:none;' name='icon' src='%STR&HTE'>"
		    "<div style='position:absolute; top:4px; left:30px; right:24px; overflow:hidden; white-space:nowrap; text-overflow:ellipsis; color:%STR&HTE; font-weight:bold; pointer-events:none;'>%STR&HTE</div>"
		    "<img style='position:relative; margin-top:3px; margin-right:3px; float:right; cursor:pointer;' name='close' src='/sys/images/01bigclose.gif'>"
		"</div>\n",
		id,
		icon,
		text_color, title
	    ) != 0)
		{
		mssError(0, "HTWIN", "Failed to write HTML for window title bar.");
		goto err;
		}
	    }

	/** Render child widgets inside window container. **/
	if (htrAddBodyItem_va(s,"<div class='wnborder'><div id='wn%POSmain'>\n",id) != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write HTML opening tag for window container.");
	    goto err;
	    }
	if (htrRenderSubwidgets(s, tree, z + 2) != 0) goto err;
	if (htrAddBodyItem(s,"</div></div></div>\n") != 0)
	    {
	    mssError(0, "HTWIN", "Failed to write HTML closing tag for window container.");
	    goto err;
	    }

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTWIN",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
    }


/*** htwinInitialize - register with the ht_render module.
 ***/
int
htwinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Child Window Widget Driver");
	strcpy(drv->WidgetName,"childwindow");
	drv->Render = htwinRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTWIN.idcnt = 0;

    return 0;
    }
