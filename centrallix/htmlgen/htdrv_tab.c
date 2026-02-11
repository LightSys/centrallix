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
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_tab.c             				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 28, 1998 					*/
/* Description:	HTML Widget driver for a tab control.			*/
/************************************************************************/


/** globals **/
static struct
    {
    int		idcnt;
    }
    HTTAB;


enum httab_locations { Top=0, Bottom=1, Left=2, Right=3, None=4 };


/*** httabRender - generate the HTML code for the page.
 ***/
int
httabRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr; int tmp = 0;
    char* type = NULL, *field;
    char name[64];
    char text_color[128];
    char main_bg[128];
    char inactive_bg[128];
    char page_type[32];
    char fieldname[128];
    int sel_idx = -1;
    int x = -1, y = -1;
    int w, h; /* width & height of the tab control. */
    int id, tab_count, i;
    enum httab_locations tloc;
    int tab_w = 0, tab_h = 0;
    int is_auto_tab_w = 0; /* 1 if tab_w should be computed client-side. */
    int xoffset, yoffset, xtoffset, ytoffset;
    char* tabname;
    pWgtrNode children[32];
    int border_radius;
    char border_style[32];
    char border_color[64];
    int border_width;
    int shadow_offset, shadow_radius, shadow_angle;
    char shadow_color[128];
	
	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE &&(!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTAB","NS4 or W3C DOM Support required");
	    return -1;
	    }
	
	/** Reserve the next tab widget ID. **/
	id = (HTTAB.idcnt++);
	
	/** Get the tab widget name. **/
	if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0) return -1;
	strtcpy(name, ptr, sizeof(name));
	
	/** Get x, y, w, & h of this object. **/
	if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&x)) != 0) x = 0;
	if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&y)) != 0) y = 0;
	if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&w)) != 0)
	    {
	    mssError(0, "HTTAB", "Tab widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&h)) != 0)
	    {
	    mssError(0, "HTTAB", "Tab widget must have a 'height' property");
	    return -1;
	    }
	
	/** Get drop shadow data. **/
	shadow_offset = 0;
	if (wgtrGetPropertyValue(tree, "shadow_offset", DATA_T_INTEGER, POD(&shadow_offset)) == 0 && shadow_offset > 0)
	    shadow_radius = shadow_offset+1;
	else
	    shadow_radius = 0;
	wgtrGetPropertyValue(tree, "shadow_radius", DATA_T_INTEGER, POD(&shadow_radius));
	strcpy(shadow_color, "black");
	if (shadow_radius > 0)
	    {
	    if (wgtrGetPropertyValue(tree, "shadow_color", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(shadow_color, ptr, sizeof(shadow_color));
	    }
	if (wgtrGetPropertyValue(tree, "shadow_angle", DATA_T_INTEGER, POD(&shadow_angle)) != 0)
	    shadow_angle = 135;
	
	/** Get border info (radius, color, and style). **/
	if (wgtrGetPropertyValue(tree, "border_radius", DATA_T_INTEGER, POD(&border_radius)) != 0)
	    border_radius = 0;
	if (wgtrGetPropertyValue(tree, "border_color", DATA_T_STRING, POD(&ptr)) != 0)
	    strcpy(border_color, "#ffffff");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree, "border_style", DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));
	if (!strcmp(border_style, "none") || !strcmp(border_style, "hidden"))
	    border_width = 0;
	else
	    border_width = 1;
	
	/** Get tab_location. **/
	if (wgtrGetPropertyValue(tree, "tab_location", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"top")) tloc = Top;
	    else if (!strcasecmp(ptr,"bottom")) tloc = Bottom;
	    else if (!strcasecmp(ptr,"left")) tloc = Left;
	    else if (!strcasecmp(ptr,"right")) tloc = Right;
	    else if (!strcasecmp(ptr,"none")) tloc = None;
	    else
		{
		mssError(1,"HTTAB","%s: '%s' is not a valid tab_location",name,ptr);
		return -1;
		}
	    }
	else
	    {
	    tloc = Top;
	    }
	
	/** Count the number of tabs. **/
	tab_count = wgtrGetMatchingChildList(tree, "widget/tabpage", children, sizeof(children) / sizeof(pWgtrNode));
	
	/** Get the selected tab. **/
	if (wgtrGetPropertyType(tree, "selected") == DATA_T_STRING &&
	    wgtrGetPropertyType(tree, "selected_index") == DATA_T_INTEGER)
	    {
	    mssError(1,"HTTAB","%s: cannot specify both 'selected' and 'selected_index'", name);
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree, "selected", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    /** Search for the tab with the indicated name. **/
	    for (i = 0; i < tab_count; i++)
		{
		char* tab_name;
		wgtrGetPropertyValue(children[i], "name", DATA_T_STRING, POD(&tab_name));
		if (strcmp(ptr, tab_name) == 0)
		    {
		    /** sel_idx is 1 based, but i is 0 based. **/
		    sel_idx = i + 1;
		    break;
		    }
		}
	    if (i >= tab_count) {
	        mssError(1, "HTTAB", "%s: cannot find tab with name '%s'", name, ptr);
		
		/** Attempt to give hint. **/
		if (tab_count <= 0) return -1;
		char* example_tab_name;
		wgtrGetPropertyValue(children[0], "name", DATA_T_STRING, POD(&example_tab_name));
		mssError(0, "HTTAB", "Hint: 'selected' should be a tab name, such as \"%s\".", example_tab_name);
		
		/** Fail. **/
	        return -1;
		}
	    }
	else if (wgtrGetPropertyValue(tree, "selected_index", DATA_T_INTEGER, POD(&sel_idx)) == 0)
	    {
	    if (sel_idx <= 0)
		{
		mssError(1, "HTTAB", "Invalid value for 'selected_index': %d.", sel_idx);
		if (sel_idx == 0) mssError(0, "HTTAB", "Hint: 'selected_index' is 1-based.");
		return -1;
		}
	    if (sel_idx > tab_count)
		{
		mssError(1, "HTTAB",
		    "Invalid value for 'selected_index': %d. Tab control only has %d tab%s.",
		    sel_idx, tab_count, (tab_count == 1) ? "" : "s"
		);
		return -1;
		}
	    }
	else
	    {
	    /** No specified selected tab, default to the first one. **/
	    sel_idx = 1;
	    }
	
	/** Handle user expressions for the selected tab. **/
	htrCheckAddExpression(s, tree, name, "selected");
	htrCheckAddExpression(s, tree, name, "selected_index");
	
	/** Get the background color/image. **/
	htrGetBackground(tree, NULL, s->Capabilities.Dom2CSS, main_bg, sizeof(main_bg));
	
	/** Get the inactive tab color/image. **/
	if (htrGetBackground(tree, "inactive", s->Capabilities.Dom2CSS, inactive_bg, sizeof(inactive_bg)) != 0)
	    strcpy(inactive_bg, main_bg);
	
	/** Get the text color. **/
	if (wgtrGetPropertyValue(tree, "textcolor", DATA_T_STRING, POD(&ptr)) == 0
	    && !strpbrk(ptr, "{};&<>\"\'"))
	    strtcpy(text_color, ptr, sizeof(text_color));
	else
	    strcpy(text_color,"black");
	
	/** Get the tab spacing and tab height. **/
	/** tab_w and tab_h are left as 0 if unset to tell the front end to calculate them dynamically. **/
	int tab_spacing = 2; /* Default to a 2px gap between tabs. */
	if (wgtrGetPropertyValue(tree, "tab_spacing", DATA_T_INTEGER, POD(&tmp)) == 0) tab_spacing = tmp;
	if (wgtrGetPropertyValue(tree, "tab_width", DATA_T_INTEGER, POD(&tmp)) == 0)
	    {
	    if (tmp <= 0)
		{
		mssError(1, "HTTAB", "%s: 'tab_width' expected positive nonzero int, got %d.", name, tmp);
		return -1;
		}
	    tab_w = tmp;
	    }
	else if (tloc == Right || tloc == Left)
	    {
	    mssError(1, "HTTAB", "%s: 'tab_width' must be specified for 'tab_location' of left or right", name);
	    return -1;
	    }
	else
	    {
	    /** Use a default value, updated client side. */
	    tab_w = 80;
	    is_auto_tab_w = 1;
	    }
	if (wgtrGetPropertyValue(tree, "tab_height", DATA_T_INTEGER, POD(&tmp)) == 0)
	    {
	    if (tmp <= 0)
		{
		mssError(1, "HTTAB", "%s: 'tab_height' expected positive nonzero int, got %d.", name, tmp);
		return -1;
		}
	    tab_h = tmp;
	    }
	else
	    {
	    /** Use default value, no client side calculation available. **/
	    tab_h = 24;
	    }
	
	/** Get macro selection translation values. **/
	/** CHANGE: Code previously used 1, but I think 0 is a better looking default. **/
	int along, out;
	if (wgtrGetPropertyValue(tree, "select_translate_along", DATA_T_INTEGER, POD(&along)) != 0) along = 0;
	if (wgtrGetPropertyValue(tree, "select_translate_out",   DATA_T_INTEGER, POD(&out))   != 0) out = 2;
	
	/** Determine offset to actual tab pages and offsets for selected tabs. **/
	int select_x_offset = 0, select_y_offset = 0;
	switch (tloc)
	    {
	    /*** Shift to cover boarder line:
	     *** Top:    ytoffset +1
	     *** Bottom: ytoffset -2
	     *** Left:   xtoffset +1
	     *** Right:  xtoffset -2
	     ***/
	    case Top:    xoffset = 0;     yoffset = tab_h; xtoffset = 0;   ytoffset = 0;   select_x_offset = -along; select_y_offset = -out;   break;
	    case Bottom: xoffset = 0;     yoffset = 0;     xtoffset = 0;   ytoffset = h-1; select_x_offset = -along; select_y_offset = +out;   break;
	    case Left:   xoffset = tab_w; yoffset = 0;     xtoffset = 0;   ytoffset = 0;   select_x_offset = -out;   select_y_offset = -along; break;
	    case Right:  xoffset = 0;     yoffset = 0;     xtoffset = w-1; ytoffset = 0;   select_x_offset = +out;   select_y_offset = -along; break;
	    case None:   xoffset = 0;     yoffset = 0;     xtoffset = 0;   ytoffset = 0;   select_x_offset =  0;     select_y_offset =  0;     break;
	    }
	
	/** Get coordinate-based selection translation values. **/
	if (wgtrGetPropertyValue(tree, "select_translate_x", DATA_T_INTEGER, POD(&tmp)) == 0) select_x_offset = tmp;
	if (wgtrGetPropertyValue(tree, "select_translate_y", DATA_T_INTEGER, POD(&tmp)) == 0) select_y_offset = tmp;
	
	/*** Apply the opposite of the selection offset to all tabs. This
	 *** prevents the offset from causing the selected tab to appear
	 *** detached from the tab control.
	 ***/
	xtoffset -= select_x_offset;
	ytoffset -= select_y_offset;
	
	/** Get the rendering type. **/
	/** Allows the developer to turn off JS client-side widget rendering for testing. **/
	int do_client_rendering = 1;
	if (wgtrGetPropertyValue(tree, "rendering", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (strcmp(ptr, "server-side") == 0) do_client_rendering = 0;
	    else if (strcmp(ptr, "client-side") == 0) do_client_rendering = 1;
	    else
		{
		mssError(1, "HTTAB", "%s: Unknown value for 'rendering': %s", name, ptr);
		mssError(0, "HTTAB", "HINT: Should be either 'server-side' or 'client-size'.");
		return -1;
		}
	    }
	if (!do_client_rendering && tab_w == 0 && (tloc == Top || tloc == Bottom))
	    {
	    /*** The dev has specified server-side rendering for Top/Bottom tabs
	     *** with dynamic width. This will probably look broken.
	     ***/
	    mssError(1, "HTTAB",
		"%s: 'rendering' value of \"server-side\" will break on tabs "
		"with dynamic widths because they cannot be calculated server-side!",
		name
	    );
	    mssError(0, "HTTAB", "HINT: Specify 'tab_width' when using \"server-side\" rendering.");
	    }
	
	/** Handle DOM linkages. **/
	htrAddWgtrObjLinkage_va(s, tree, "tc%POSctrl", id);
	
	/** Include the htdrv_tab.js script. **/
	htrAddScriptInclude(s, "/sys/js/htdrv_tab.js", 0);
	
	/** Send globals variables to the client to avoid needing to hard code them. **/
	const int bufsiz = 96;
	char* config_buf = nmSysMalloc(bufsiz);
	if (config_buf == NULL)
	    {
	    mssError(1, "HTTAB", "%s: nmSysMalloc(%d) failed.", name, bufsiz);
	    return -1;
	    }
	snprintf(
	    memset(config_buf, 0, bufsiz), bufsiz,
	    "{ tlocs: { Top:%d, Bottom:%d, Left:%d, Right:%d, None:%d } }",
	    Top, Bottom, Left, Right, None
	);
	htrAddScriptGlobal(s, "tc_config", config_buf, HTR_F_VALUEALLOC);
	/*** TODO: Greg - config_buf is definitely leaked because I can't
	 *** figure out how long it needs to remain in scope.
	 ***/
	
	/** Add globals for the master tabs listing. **/
	htrAddScriptGlobal(s, "tc_tabs", "null", 0);
	htrAddScriptGlobal(s, "tc_cur_mainlayer", "null", 0);
	
	/** Add mouse event handlers. **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tc", "tc_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "tc", "tc_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tc", "tc_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tc", "tc_mouseover");
	
	/** Script initialization call. **/
	char tloc_name[8];
	switch (tloc)
	    {
	    case Top:    strcpy(tloc_name, "Top");    break;
	    case Bottom: strcpy(tloc_name, "Bottom"); break;
	    case Left:   strcpy(tloc_name, "Left");   break;
	    case Right:  strcpy(tloc_name, "Right");  break;
	    case None:   strcpy(tloc_name, "None");   break;
	    }
	htrAddScriptInit_va(s,
	    "\ttc_init({"
		"layer:wgtrGetNodeRef(ns,'%STR&SYM'), "
		"tloc:'%STR', "
		"mainBackground:'%STR&JSSTR', "
		"inactiveBackground:'%STR&JSSTR', "
		"select_x_offset:%INT, "
		"select_y_offset:%INT, "
		"xtoffset:%INT, "
		"ytoffset:%INT, "
		"tab_spacing:%INT, "
		"tab_w:%INT, "
		"tab_h:%INT, "
		"do_client_rendering:%STR, "
	    "});\n",
	    name,
	    tloc_name,
	    main_bg,
	    inactive_bg,
	    select_x_offset,
	    select_y_offset,
	    xtoffset,
	    ytoffset,
	    tab_spacing,
	    (is_auto_tab_w) ? 0 : tab_w, /* 0 tells the front end that it should recalculate tab_w. */
	    tab_h,
	    (do_client_rendering) ? "true" : "false"
	);
	
	/** Check for tabpages within the tab control, to do the tabs at the top. **/
	if (tloc != None)
	    {
	    /*** Calculate offsets for spacing out tabs. This is overwritten
	     *** by the JS, but content with these values is visible for a
	     *** brief period while the page loads, so we try to make a good
	     *** guesses for it.
	     ***/
	    int i_offset_x = 0, i_offset_y = 0;
	    const int full_tab_spacing = tab_spacing + border_width * 2;
	    switch (tloc)
		{
		case Top:   case Bottom: i_offset_x = full_tab_spacing + tab_w; break;
		case Right: case Left:   i_offset_y = full_tab_spacing + tab_h; break;
		case None:; /* Unreachable, but the compiler doesn't believe me. */
		}
		
	    /** Calculate tab flex information. **/
	    /*** fl_x/y is enough flex to line up with the left/top of the tab
	     *** control. However, if the tab box changes size, tabs on the
	     *** right/bottom need to flex enough to handle that, too.
	     ***/
	    double tab_fl_x = ht_get_fl_x(tree), tab_fl_y = ht_get_fl_y(tree);
	    if      (tloc == Right)  tab_fl_x += ht_get_fl_w(tree);
	    else if (tloc == Bottom) tab_fl_y += ht_get_fl_h(tree);
	    
	    /** Inject tab_fl values for client-side rendering. **/
	    htrAddScriptInit_va(s,
		"{"
		    "const node = wgtrGetNodeRef(ns,'%STR&SYM'); "
		    "node.tab_fl_x = %DBL; "
		    "node.tab_fl_y = %DBL; "
		"}\n",
		name,
		tab_fl_x,
		tab_fl_y
	    );
	    
	    /** Loop over each tab. **/
	    for (i = 0; i < tab_count; i++)
		{
		pWgtrNode tab = children[i];
		
		/** Check if the tab is selected. **/
		int is_selected = (i == sel_idx - 1);
		
		/** Get type. **/
		wgtrGetPropertyValue(tab, "type", DATA_T_STRING, POD(&page_type));
		if (type == NULL || strcmp(type, "dynamic") != 0) strcpy(page_type, "static");
		
		/** Use the tab title, defaulting to the tab name if it isn't specified. **/
		if (wgtrGetPropertyValue(tab, "title", DATA_T_STRING, POD(&tabname)) != 0)
		    wgtrGetPropertyValue(tab, "name", DATA_T_STRING, POD(&tabname));
		
		/** Write tab CSS styles. **/
		int tab_x = (x + xtoffset) + (i_offset_x * i);
		int tab_y = (y + ytoffset) + (i_offset_y * i);
		htrAddStylesheetItem_va(s,
		    "\t#tc%POStab%POS { "
			"position:absolute; "
			"visibility:inherit; "
			"left:"ht_flex_format"; "
			"top:"ht_flex_format"; "
			"%[width:%POSpx; %]"  /* Tab width has 0 flexibility. */
			"%[height:%POSpx; %]" /* Tab height has 0 flexibility. */
			"overflow:hidden; "
			"z-index:%POS; "
			"cursor:default; "
			"border-radius:"
			    "%POSpx "
			    "%POSpx "
			    "%POSpx "
			    "%POSpx; "
			"border-style:%STR&CSSVAL; "
			"border-width:%POSpx %POSpx %POSpx %POSpx; "
			"border-color:%STR&CSSVAL; "
			"box-shadow:%DBLpx %DBLpx %POSpx %STR&CSSVAL; "
			"text-align:%STR&CSSVAL; "
			"color:%STR&CSSVAL; "
			"font-weight:bold; /*"
			"easter-egg-6:value;*/ "
			"background-position: %INTpx %INTpx; "
			"%STR "
		    "}\n",
		    id, i + 1,
		    ht_flex(tab_x, ht_get_parent_w(tree), tab_fl_x), // left
		    ht_flex(tab_y, ht_get_parent_h(tree), tab_fl_y), // top
		    (!is_auto_tab_w), tab_w, /* Tab width has 0 flexibility. */
		    (tab_h > 0), tab_h, /* Tab height has 0 flexibility. */
		    (is_selected) ? (z + 2) : z,
		    (tloc == Bottom || tloc == Right) ? 0 : border_radius,
		    (tloc == Bottom || tloc == Left) ? 0 : border_radius,
		    (tloc == Top || tloc == Left) ? 0 : border_radius,
		    (tloc == Top || tloc == Right) ? 0 : border_radius,
		    border_style,
		    (tloc != Bottom) ? 1 : 0, (tloc != Left) ? 1 : 0, (tloc != Top) ? 1 : 0, (tloc != Right) ? 1 : 0,
		    border_color,
		    sin(shadow_angle * M_PI/180) * shadow_offset, cos(shadow_angle * M_PI/180) * (-shadow_offset), shadow_radius, shadow_color,
		    (tloc != Right) ? "left" : "right",
		    text_color,
		    tab_x + 1, tab_y,
		    (is_selected) ? main_bg : inactive_bg
		);
		
		htrAddStylesheetItem_va(s,
		    "\t#tc%POStab%POS.tab_selected { transform: translate(%INTpx, %INTpx); }\n",
		    id, i + 1, select_x_offset, select_y_offset
		);
		
		/** Write tab HTML content. **/
		htrAddBodyItem_va(s,
		    "<div id='tc%POStab%POS' %[class='tab_selected'%]>"
		        "<p style='"
			    "white-space:nowrap; "
			    "margin:0px; "
			    "padding:0px; "
			"'>"
			    "%[<span>&nbsp;%STR&HTE&nbsp;</span>%]"
			    "<img src='/sys/images/tab_lft%POS.gif' style='width:5px; height:%POSpx; vertical-align:middle;'>"
			    "%[<span>&nbsp;%STR&HTE&nbsp;</span>%]"
			"</p>"
		    "</div>\n",
		    id, i + 1, (is_selected),
		    (tloc == Right), tabname,
		    (is_selected) ? 2 : 3, tab_h,
		    (tloc != Right), tabname
		);
		}
	    }
	
	/** Write tab control CSS and HTML. **/
	htrAddStylesheetItem_va(s,
	    "#tc%POSctrl {"
		"position:absolute; "
		"overflow:hidden; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
		"border-width:1px; "
		"border-style:%STR&CSSVAL; "
		"border-color:%STR&CSSVAL; "
		"border-radius:"
		    "%POSpx "
		    "%POSpx "
		    "%POSpx "
		    "%POSpx; "
		"box-shadow:%DBLpx %DBLpx %POSpx %STR&CSSVAL; "
		"background-position:%INTpx %INTpx; "
		"%STR "
	    "}\n",
	    id,
	    ht_flex_x(x + xoffset, tree),
	    ht_flex_y(y + yoffset, tree),
	    ht_flex_w(w - border_width * 2, tree),
	    ht_flex_h(h - border_width * 2, tree),
	    z + 1,
	    border_style,
	    border_color,
	    (tloc==Top || tloc==Left) ? 0 : border_radius,
	    (tloc==Right) ? 0 : border_radius,
	    border_radius,
	    (tloc==Bottom) ? 0 : border_radius,
	    sin(shadow_angle * M_PI/180) * shadow_offset, cos(shadow_angle * M_PI/180) * (-shadow_offset), shadow_radius, shadow_color,
	    x + xoffset, y + yoffset,
	    main_bg
	);
	htrAddBodyItem_va(s, "<div id='tc%POSctrl'>\n", id);
	
	/** Check for tab pages within the tab control entity, this time to do the pages themselves. **/
	for (i = 0; i < tab_count; i++)
	    {
	    pWgtrNode tab_page_tree = children[i];
	    
	    /** Handle namespace transition. **/
	    htrCheckNSTransition(s, tree, tab_page_tree);
	    
	    /** First, render the tabpage and add stuff for it **/
	    wgtrGetPropertyValue(tab_page_tree,"name",DATA_T_STRING,POD(&ptr));
	    
	    /** Check if the tab is selected. **/
	    int is_selected = (i == sel_idx - 1);
	    
	    /** Set type. **/
	    wgtrGetPropertyValue(tab_page_tree, "type", DATA_T_STRING, POD(&page_type));
	    if (type == NULL || strcmp(type, "dynamic") != 0) strcpy(page_type, "static");
	    
	    /** Set feildname. **/
	    if (strcmp(page_type, "dynamic") == 0 &&
	        wgtrGetPropertyValue(tab_page_tree, "fieldname", DATA_T_STRING, POD(&field)) == 0)
		strtcpy(fieldname, field, sizeof(fieldname));
	    else strcpy(fieldname, "");
	    
	    /** Add script initialization to add a new tabpage **/
	    if (tloc == None)
		{
		htrAddScriptInit_va(s,
		    "\twgtrGetNodeRef('%STR&SYM', '%STR&SYM')"
			".addTab(null, "
			    "wgtrGetContainer(wgtrGetNodeRef('%STR&SYM', '%STR&SYM')), "
			    "wgtrGetNodeRef('%STR&SYM', '%STR&SYM'), "
			    "'%STR&JSSTR', '%STR&JSSTR', '%STR&JSSTR'"
			");\n",
		    wgtrGetNamespace(tree), name,
		    wgtrGetNamespace(tab_page_tree), ptr,
		    wgtrGetNamespace(tree), name,
		    ptr, page_type, fieldname
		);
		}
	    else
		{
		htrAddScriptInit_va(s,
		    "\twgtrGetNodeRef('%STR&SYM','%STR&SYM')"
			".addTab("
			    "htr_subel("
				"wgtrGetParentContainer(wgtrGetNodeRef('%STR&SYM', '%STR&SYM')), "
				"'tc%POStab%POS'"
			    "), "
			    "wgtrGetContainer(wgtrGetNodeRef('%STR&SYM', '%STR&SYM')), "
			    "wgtrGetNodeRef('%STR&SYM', '%STR&SYM'), "
			    "'%STR&JSSTR', '%STR&JSSTR', '%STR&JSSTR'"
			");\n",
		    wgtrGetNamespace(tree), name,
		    wgtrGetNamespace(tree), name,
		    id, i + 1,
		    wgtrGetNamespace(tab_page_tree), ptr,
		    wgtrGetNamespace(tree), name,
		    ptr, page_type, fieldname
		);
		}
	    
	    /** Add named global for the tabpage. **/
	    htrAddWgtrObjLinkage_va(s, tab_page_tree, "tc%POSpane%POS", id, i+1);
	    htrAddWgtrCtrLinkage_va(s, tab_page_tree, "htr_subel(_parentobj, \"tc%POSpane%POS\")", id, i + 1);
	    
	    /** Add DIV section to contane the tabpage. **/
	    htrAddBodyItem_va(s,
		"<div "
		    "id=\"tc%POSpane%POS\" "
		    "style=\""
			"position:absolute; "
			"visibility:%STR&CSSVAL; "
			"left:0px; "
			"top:0px; "
			"width:100%%; "
			"height:100%%; "
			"z-index:%POS; "
		    "\""
		">\n",
		id, i + 1,
		(is_selected) ? "inherit" : "hidden",
		z + 2
	    );
	    
	    /** Handle sub-items within the tabpage. **/
	    for (int j = 0; j < xaCount(&(tab_page_tree->Children)); j++)
		htrRenderWidget(s, xaGetItem(&(tab_page_tree->Children), j), z+3);
	    
	    /** Close the tab page container. */
	    htrAddBodyItem(s, "</div>\n");
	    
	    /** Add the visible property. **/
	    htrCheckAddExpression(s, tab_page_tree, ptr, "visible");
	    
	    /** Handle namespace transition. **/
	    htrCheckNSTransitionReturn(s, tree, tab_page_tree);
	    }
	
	/** Handle other subwidgets (connectors, etc.). **/
	htrRenderSubwidgets(s, tree, z + 1);
	
	/** End the containing layer. **/
	htrAddBodyItem(s, "</div>\n");
    
    return 0;
    }

int 
httabRender_page(pHtSession s, pWgtrNode tabpage, int z) 
    {
    /** we already rendered subwidgets of the tabpage **/
    /*htrRenderSubwidgets(s, tabpage, z);*/
    return 0;
    }


/*** httabInitialize - register with the ht_render module.
 ***/
int
httabInitialize()
    {
    pHtDriver drv;

	/** Tab Control Driver. **/
	drv = htrAllocDriver();
	if (drv == NULL) return -1;
	strcpy(drv->Name, "DHTML Tab Control Driver");
	strcpy(drv->WidgetName, "tab");
	drv->Render = httabRender;
	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

	/** Tab Page Driver. **/
	drv = htrAllocDriver();
	if (drv == NULL) return -1;
	strcpy(drv->Name, "DHTML Tab Page Driver");
	strcpy(drv->WidgetName, "tabpage");
	drv->Render = httabRender_page;
	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

	HTTAB.idcnt = 0;

    return 0;
    }
