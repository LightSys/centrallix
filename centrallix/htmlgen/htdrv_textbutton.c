#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
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
/* Module: 	htdrv_textbutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 5, 1998  					*/
/* Description:	HTML Widget driver for a 'text button', which frames a	*/
/*		text string in a 3d-like box that simulates the 3d	*/
/*		clicking action when the user points and clicks.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBTN;


/*** httbtnRender - generate the HTML code for the page.
 ***/
int
httbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgstyle[128];
    char disable_color[64];
    int x,y,w,h;
    int is_ts = 1;
    int is_enabled = 1;
    pExpression code;
    int box_offset;
    int border_radius;
    char border_style[32];
    char border_color[64];
    char image_position[16]; /* top, left, right, bottom */
    char image[OBJSYS_MAX_PATH];
    bool has_image;
    char h_align[16];
    int image_width=0, image_height=0, image_margin=0;

	/** Get an id for this. **/
	const int id = (HTTBTN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTTERM", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Border radius, color, and style.  For style, we only support outset,
	 ** solid, and none here.
	 **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;
	if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_color, "#c0c0c0");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree,"border_style",DATA_T_STRING,POD(&ptr)) != 0 || (strcmp(ptr,"outset") && strcmp(ptr,"solid") && strcmp(ptr,"none")))
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));

	/** Alignment **/
	if (wgtrGetPropertyValue(tree,"align",DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"left") || !strcmp(ptr,"right") || !strcmp(ptr,"center")))
	    strtcpy(h_align, ptr, sizeof(h_align));
	else
	    strcpy(h_align, "center");

	/** Image location **/
	if (wgtrGetPropertyValue(tree,"image_position",DATA_T_STRING,POD(&ptr)) != 0 || (strcmp(ptr,"top") && strcmp(ptr,"right") && strcmp(ptr,"bottom") && strcmp(ptr, "left")))
	    strcpy(image_position, "top");
	else
	    strtcpy(image_position, ptr, sizeof(image_position));

	/** Image source **/
	if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    strcpy(image, "");
	    has_image = false;
	    }
	else
	    {
	    strtcpy(image, ptr, sizeof(image));
	    has_image = true;
	    }

	/** Image sizing **/
	if (wgtrGetPropertyValue(tree,"image_width",DATA_T_INTEGER, POD(&image_width)) != 0)
	    image_width = 0;
	if (wgtrGetPropertyValue(tree,"image_height",DATA_T_INTEGER, POD(&image_height)) != 0)
	    image_height = 0;
	if (wgtrGetPropertyValue(tree,"image_margin",DATA_T_INTEGER, POD(&image_margin)) != 0)
	    image_margin = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** box adjustment... arrgh **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;
	//clip_offset = s->Capabilities.CSSClip?1:0;

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Threestate button or twostate? **/
	if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	ptr = "-";
	if (!htrCheckAddExpression(s, tree, name, "text") && wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	strtcpy(text,ptr,sizeof(text));

	/** Get fgnd colors 1,2, and background color **/
	htrGetBackground(tree, NULL, 1, bgstyle, sizeof(bgstyle));

	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
	else
	    strcpy(fgcolor1,"white");
	if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
	else
	    strcpy(fgcolor2,"black");
	if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(disable_color,ptr,sizeof(disable_color));
	else
	    strcpy(disable_color,"#808080");


	/** DOM Linkages **/
	if (htrAddWgtrObjLinkage_va(s, tree, "tb%POSpane",id) != 0) goto err;

	/** Write JS globals and includes for the textbutton. **/
	if (htrAddScriptGlobal(s, "tb_current", "null", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;

	/** Calculate size adjustment. **/
	const int adj = (2 * box_offset) + 1;
	
	/** Write CSS for the container that will hold the button. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tb%POSpane { "
		"position:absolute; "
		"visibility:inherit; "
		"cursor:pointer; "
		"display:table; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"%[height:"ht_flex_format"; %]"
		"z-index:%POS; "
	    "}\n",
	    id,
	              ht_flex_x(x,       tree),
	              ht_flex_y(y,       tree),
	              ht_flex_w(w - adj, tree),
	    (h >= 0), ht_flex_h(h - adj, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTTBTN", "Failed to write CSS for main button pane.");
	    goto err;
	    }
	
	/** Button click animation. **/
	if (is_enabled) {
	    if (htrAddStylesheetItem_va(s,
		"\t\t#tb%POSpane:active { transform: translate(1px, 1px); }\n",
		id
	    ) != 0)
		{
		mssError(0, "HTTBTN", "Failed to write CSS for button click animation.");
		goto err;
		}
	}
	
	/** Write CSS for the button content, inside the border. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tb%POSpane .cell { "
		"height:100%%; "
		"width:100%%; "
		"vertical-align:middle; "
		"display:table-cell; "
		"padding:1px; "
		"font-weight:bold; "
		"text-align:%STR; "
		"border-width:1px; "
		"border-style:%STR&CSSVAL; "
		"border-color:%STR&CSSVAL; "
		"border-radius:%INTpx; "
		"color:%STR&CSSVAL; "
		"%[text-shadow:1px 1px %STR&CSSVAL; %]"
		"%STR "
	    "}\n",
	    id,
	    h_align,
	    border_style,
	    border_color,
	    border_radius,
	    (is_enabled) ? fgcolor1 : disable_color,
	    (is_enabled), fgcolor2,
	    bgstyle
	) != 0)
	    {
	    mssError(0, "HTTBTN", "Failed to write CSS for button content.");
	    goto err;
	    }

	/** Write CSS for image on the button. **/
	if (has_image && (image_width != 0 || image_height != 0 || image_margin != 0))
	    {
	    if (htrAddStylesheetItem_va(s,
		"\t\t#tb%POSpane img { "
		    "%[height:%POSpx; %]"
		    "%[width:%POSpx; %]"
		    "%[margin:%POSpx; %]"
		"}\n",
		id,
		(image_height != 0), image_height,
		(image_width  != 0), image_width,
		(image_margin != 0), image_margin
	    ) != 0)
		{
		mssError(0, "HTTBTN", "Failed to write CSS for button image.");
		goto err;
		}
	    }

	/** We need two DIVs here because of a long-outstanding Firefox bug :( **/
	if (htrAddBodyItem_va(s,
	    "<div id='tb%POSpane'>"
		"<div class='cell'>"
		    "%[<img border='0' src='%STR&HTE'/><br>%]"
		    "%[<img border='0' src='%STR&HTE' style='vertical-align:middle;'/>%]"
		    "<span>%STR&HTE</span>"
		    "%[<img border='0' src='%STR&HTE' style='vertical-align:middle;'/>%]"
		    "%[<br><img border='0' src='%STR&HTE'/>%]"
		"</div>"
	    "</div>",
	    id,
	    (has_image && strcmp(image_position, "top") == 0), image,
	    (has_image && strcmp(image_position, "left") == 0), image,
	    text,
	    (has_image && strcmp(image_position, "right") == 0), image,
	    (has_image && strcmp(image_position, "bottom") == 0), image
	) != 0)
	    {
	    mssError(0, "HTTBTN", "Failed to write HTML for text button.");
	    goto err;
	    }

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\ttb_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"name:'%STR&SYM', "
		"text:'%STR&JSSTR', "
		"ena:%INT, "
		"tristate:%INT, "
		"c1:'%STR&JSSTR', "
		"c2:'%STR&JSSTR', "
		"dc1:'%STR&JSSTR', "
		"top:null, "
		"bottom:null, "
		"right:null, "
		"left:null, "
		"width:%INT, "
		"height:%INT, "
	    "});\n",
	    name, name, text,
	    is_enabled, is_ts,
	    fgcolor1, fgcolor2,
	    disable_color,
	    w, h
	) != 0)
	    {
	    mssError(0, "HTTBTN", "Failed to write JS init call.");
	    goto err;
	    }

	/** Add event handlers. **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN",  "tb", "tb_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE",  "tb", "tb_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",   "tb", "tb_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER",  "tb", "tb_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",    "tb", "tb_mouseup")   != 0) goto err;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTTBTN",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** httbtnInitialize. - register with the ht_render module.
 ***/
int
httbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Text Button Widget Driver");
	strcpy(drv->WidgetName,"textbutton");
	drv->Render = httbtnRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBTN.idcnt = 0;

    return 0;
    }
