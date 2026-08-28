/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core							*/
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
/* Module: 	htdrv_button.c						*/
/* Author:	dkasper							*/
/* Creation:	June 21, 2007						*/
/* Description:	HTML Widget driver for a 'generic' button widget based	*/
/* 		off of the imagebutton and textbutton widgets button.	*/
/************************************************************************/

#include <stdbool.h>
#include <string.h>

#include "cxlib/datatypes.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "expression.h"
#include "ht_render.h"
#include "obj.h"
#include "wgtr.h"


/*** Where an image sits relative to the button's text.  'image_position' and
 *** the image-bearing 'type' values are two spellings of the same thing, so
 *** both are parsed into this and the table below maps between them.
 ***/
enum htbtn_image_positions { Top=0, Bottom=1, Left=2, Right=3 };

static const struct
    {
    char*	Position;	/* the deprecated 'image_position' spelling */
    char*	Type;		/* the 'type' spelling */
    }
    HTBTN_ImagePositions[] =
	{
	{ "top",    "topimage"    },
	{ "bottom", "bottomimage" },
	{ "left",   "leftimage"   },
	{ "right",  "rightimage"  },
	};
#define HTBTN_N_IMAGE_POSITIONS ((int)(sizeof(HTBTN_ImagePositions) / sizeof(HTBTN_ImagePositions[0])))


/** globals **/
static struct
    {
    int idcnt;
    }
    HTBTN;


/*** htbtnRender - generate the HTML code for the page.
 ***/
int
htbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    /*** Scratch for every wgtrGetPropertyValue() string fetch below.  The value
     *** it points at is only valid until the next fetch, so each one is copied.
     ***/
    char* ptr;

	/** Get an id for this. **/
	const int id = (HTBTN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTBTN", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Get x, y, w, h of this object **/
	int x, y, w, h;
	if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&x)) != 0)
	    {
	    mssError(1, "HTBTN", "Button widget must have an 'x' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&y)) != 0)
	    {
	    mssError(1, "HTBTN", "Button widget must have a 'y' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&w)) != 0)
	    {
	    mssError(1, "HTBTN", "Button widget must have a 'width' property.");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&h)) != 0) h = -1;
	if (tree->Flags & WGTR_F_AUTOHEIGHT) h = -1; /* let htdrv_button.js fit the text */

	/** Get name. **/
	char name[64];
	if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0) goto err;
	strtcpy(name, ptr, sizeof(name));

	/** Images.  Point, click and disabled fall back to the normal image. **/
	char image[OBJSYS_MAX_PATH];
	char point_image[OBJSYS_MAX_PATH];
	char click_image[OBJSYS_MAX_PATH];
	char disabled_image[OBJSYS_MAX_PATH];
	bool has_image;
	if (wgtrGetPropertyValue(tree, "image", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(image, ptr, sizeof(image));
	    has_image = true;
	    }
	else
	    {
	    image[0] = '\0';
	    has_image = false;
	    }
	if (wgtrGetPropertyValue(tree, "pointimage", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(point_image, ptr, sizeof(point_image));
	else
	    strtcpy(point_image, image, sizeof(point_image));
	if (wgtrGetPropertyValue(tree, "clickimage", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(click_image, ptr, sizeof(click_image));
	else
	    strtcpy(click_image, point_image, sizeof(click_image));
	if (wgtrGetPropertyValue(tree, "disabledimage", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(disabled_image, ptr, sizeof(disabled_image));
	else
	    strtcpy(disabled_image, image, sizeof(disabled_image));

	/** Where the image sits, from the deprecated 'image_position' spelling. **/
	enum htbtn_image_positions position = Top;
	if (wgtrGetPropertyValue(tree, "image_position", DATA_T_STRING, POD(&ptr)) == 0)
	    for (int i = 0; i < HTBTN_N_IMAGE_POSITIONS; i++)
		if (strcmp(ptr, HTBTN_ImagePositions[i].Position) == 0) position = i;

	/*** Button type.  Defaults based on the widget name and whether an image
	 *** was given.  "widget/imagebutton" is a deprecated path to a borderless,
	 *** text-free button.
	 ***/
	char type[64];
	if (wgtrGetPropertyValue(tree, "type", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(type, ptr, sizeof(type));
	else if (strcmp(tree->Type, "widget/imagebutton") == 0)
	    strcpy(type, "image");
	else if (has_image)
	    strtcpy(type, HTBTN_ImagePositions[position].Type, sizeof(type));
	else
	    strcpy(type, "text");

	/** An explicit 'type' wins over 'image_position'.  Fail if unknown. **/
	bool is_known_type = (
	       strcmp(type, "text") == 0
	    || strcmp(type, "image") == 0
	    || strcmp(type, "textoverimage") == 0
	);
	for (int i = 0; i < HTBTN_N_IMAGE_POSITIONS; i++)
	    {
	    if (strcmp(type, HTBTN_ImagePositions[i].Type) == 0)
		{
		position = i;
		is_known_type = true;
		}
	    }
	if (!is_known_type)
	    {
	    mssError(1, "HTBTN", "Unknown button type \"%s\".", type);
	    goto err;
	    }
	const bool is_text_over_image = (strcmp(type, "textoverimage") == 0);
	const bool has_text = (strcmp(type, "image") != 0);
	/** With no text there is nothing to position the image against. **/
	if (!has_text) position = Top;
	has_image = has_image && (strcmp(type, "text") != 0);

	/*** textoverimage paints its image as the cell's background, so it is the
	 *** only image-bearing type that emits no <img> element.
	 ***/
	const bool has_img_element = (has_image && !is_text_over_image);

	/** Text.  Can be a client-side expression. **/
	char text[64];
	text[0] = '\0';
	if (has_text)
	    {
	    ptr = "-";
	    if (!htrCheckAddExpression(s, tree, name, "text") &&
		    wgtrGetPropertyValue(tree, "text", DATA_T_STRING, POD(&ptr)) != 0)
		{
		mssError(1, "HTBTN", "Button widget must have a 'text' property.");
		goto err;
		}
	    strtcpy(text, ptr, sizeof(text));
	    }

	/** Enabled.  Can be a client-side expression. **/
	bool is_enabled;
	if (wgtrGetPropertyType(tree, "enabled") == DATA_T_CODE)
	    {
	    pExpression enabled_code;
	    wgtrGetPropertyValue(tree, "enabled", DATA_T_CODE, POD(&enabled_code));
	    htrAddExpression(s, name, "enabled", enabled_code);
	    is_enabled = false; /* Default to disabled while loading. */
	    }
	else
	    {
	    is_enabled = (htrGetBoolean(tree, "enabled", true));
	    }

	/** Threestate button or twostate? **/
	const bool is_tristate = (htrGetBoolean(tree, "tristate", true));

	/** Auto-repeat the Click event while the button is held down. **/
	const bool do_repeat = (htrGetBoolean(tree, "repeat", false));

	/*** Border radius, color, and style.  For style, we only support outset,
	 *** solid, and none.  Image-only buttons have never drawn a border.
	 ***/
	int border_radius;
	char border_color[64];
	char border_style[32];
	if (wgtrGetPropertyValue(tree, "border_radius", DATA_T_INTEGER, POD(&border_radius)) != 0)
	    border_radius = 0;
	if (wgtrGetPropertyValue(tree, "border_color", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(border_color, ptr, sizeof(border_color));
	else
	    strcpy(border_color, "#c0c0c0");
	if (wgtrGetPropertyValue(tree, "border_style", DATA_T_STRING, POD(&ptr)) == 0
	    && (strcmp(ptr, "outset") == 0 || strcmp(ptr, "solid") == 0 || strcmp(ptr, "none") == 0))
	    strtcpy(border_style, ptr, sizeof(border_style));
	else if (!has_text)
	    strcpy(border_style, "none");
	else
	    strcpy(border_style, "outset");

	/** Text alignment. **/
	char text_align[16];
	if (wgtrGetPropertyValue(tree, "align", DATA_T_STRING, POD(&ptr)) == 0
	    && (strcmp(ptr, "left") == 0 || strcmp(ptr, "right") == 0 || strcmp(ptr, "center") == 0))
	    strtcpy(text_align, ptr, sizeof(text_align));
	else
	    strcpy(text_align, "center");

	/** Background color/image.  Returns 1 when none is set, which is fine. **/
	char background_style[128];
	if (htrGetBackground(tree, NULL, 1, background_style, sizeof(background_style)) < 0) goto err;

	/*** 'fgcolor1' is the text color and 'fgcolor2' its drop shadow, which is
	 *** only drawn while the button is enabled.
	 ***/
	char text_color[64];
	char shadow_color[64];
	char disabled_text_color[64];
	if (wgtrGetPropertyValue(tree, "fgcolor1", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(text_color, ptr, sizeof(text_color));
	else
	    strcpy(text_color, "white");
	if (wgtrGetPropertyValue(tree, "fgcolor2", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(shadow_color, ptr, sizeof(shadow_color));
	else
	    strcpy(shadow_color, "black");
	if (wgtrGetPropertyValue(tree, "disable_color", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(disabled_text_color, ptr, sizeof(disabled_text_color));
	else
	    strcpy(disabled_text_color, "#808080");

	/** Tooltip **/
	char tooltip[256];
	if (wgtrGetPropertyValue(tree, "tooltip", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(tooltip, ptr, sizeof(tooltip));
	else
	    tooltip[0] = '\0';

	/** Image sizing.  'spacing' carries the deprecated 'image_margin'. **/
	int image_width, image_height, spacing;
	if (wgtrGetPropertyValue(tree, "image_width", DATA_T_INTEGER, POD(&image_width)) != 0)
	    image_width = 0;
	if (wgtrGetPropertyValue(tree, "image_height", DATA_T_INTEGER, POD(&image_height)) != 0)
	    image_height = 0;
	if (wgtrGetPropertyValue(tree, "spacing", DATA_T_INTEGER, POD(&spacing)) != 0)
	    spacing = 0;

	/*** A button carrying text is drawn as a framed cell, and reserves room for
	 *** that frame whether or not the border is currently visible.  An image-only
	 *** button has no frame unless it asked for a border, so by default it hugs
	 *** its image the way imagebutton always has.
	 ***/
	const bool has_frame = (has_text || strcmp(border_style, "none") != 0);
	const int cell_padding = (has_frame) ? 1 : 0;
	const int frame_adjust = (has_frame) ? 3 : 0;	/* the cell's 1px border and 1px padding, less 1px of legacy slop */

	/*** An image-only button *is* its image, so the image defaults to filling the
	 *** cell.  imagebutton only ever sized its image when a height was given, so
	 *** neither do we.  Alongside text the image keeps its natural size.
	 ***/
	if (!has_text && h >= 0)
	    {
	    if (image_width == 0) image_width = w - frame_adjust;
	    if (image_height == 0) image_height = h - frame_adjust;
	    }

	/** Supress invalid positive values. **/
	if (image_width < 0) image_width = 0;
	if (image_height < 0) image_height = 0;
	if (spacing < 0) spacing = 0;

	/** DOM Linkages **/
	if (htrAddWgtrObjLinkage_va(s, tree, "gb%POSpane", id) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to add object linkage.");
	    goto err;
	    }

	/*** Write the JS includes for the button.  Its globals are declared in
	 *** htdrv_button.js rather than emitted here.
	 ***/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_button.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;

	/** Write CSS for the container that will hold the button. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#gb%POSpane { "
		"position:absolute; "
		"visibility:inherit; "
		"display:table; "
		"%[overflow:hidden; %]"
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"%[height:"ht_flex_format"; %]"
		"z-index:%POS; "
	    "}\n",
	    id,
	    (!has_text),
		      ht_flex_x(x, tree),
		      ht_flex_y(y, tree),
		      ht_flex_w(w - frame_adjust, tree),
	    (h >= 0), ht_flex_h(h - frame_adjust, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write CSS for main button pane.");
	    goto err;
	    }

	/*** Cursor and click animation.  Both key off a class the JS maintains,
	 *** so they follow the button when it is enabled or disabled at runtime.
	 ***/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#gb%POSpane:not(.gb_disabled) { cursor:pointer; }\n"
	    "\t\t#gb%POSpane:not(.gb_disabled):active { transform: translate(1px, 1px); }\n",
	    id,
	    id
	) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write CSS for cursor and click animation.");
	    goto err;
	    }

	/** Write CSS for the button content, inside the border. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#gb%POSpane .cell { "
		"height:100%%; "
		"width:100%%; "
		"vertical-align:middle; "
		"display:table-cell; "
		"padding:%POSpx; "
		"font-weight:bold; "
		"text-align:%STR; "
		"border-width:1px; "
		"border-style:%STR&CSSVAL; "
		"border-color:%STR&CSSVAL; "
		"border-radius:%INTpx; "
		"color:%STR&CSSVAL; "
		"%[text-shadow:1px 1px %STR&CSSVAL; %]"
		"%[background-image:URL(%STR&CSSURL); background-size:100%% 100%%; %]"
		"%STR "
	    "}\n",
	    id,
	    cell_padding,
	    text_align,
	    border_style,
	    border_color,
	    border_radius,
	    (is_enabled) ? text_color : disabled_text_color,
	    (is_enabled), shadow_color,
	    (is_text_over_image && has_image), (is_enabled) ? image : disabled_image,
	    background_style
	) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write CSS for button content.");
	    goto err;
	    }

	/** Write CSS for image on the button. **/
	if (has_img_element && (image_width != 0 || image_height != 0 || spacing != 0))
	    {
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane img { "
		    "%[height:%POSpx; %]"
		    "%[width:%POSpx; %]"
		    "%[margin:%POSpx; %]"
		"}\n",
		id,
		(image_height != 0), image_height,
		(image_width  != 0), image_width,
		(spacing      != 0), spacing
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS for button image.");
		goto err;
		}
	    }

	/** We need two DIVs here because of a long-outstanding Firefox bug :( **/
	if (htrAddBodyItem_va(s,
	    "<div id='gb%POSpane'%[ class='gb_disabled'%]>"
		"<div class='cell'>"
		    "%[<img border='0' alt='%STR&HTE' src='%STR&HTE'/>%]"
		    "%[<br>%]"
		    "%[<img border='0' alt='%STR&HTE' src='%STR&HTE' style='vertical-align:middle;'/>%]"
		    "%[<span>%STR&HTE</span>%]"
		    "%[<img border='0' alt='%STR&HTE' src='%STR&HTE' style='vertical-align:middle;'/>%]"
		    "%[<br>%]"
		    "%[<img border='0' alt='%STR&HTE' src='%STR&HTE'/>%]"
		"</div>"
	    "</div>",
	    id,
	    (!is_enabled),
	    (has_img_element && position == Top), (has_text) ? "" : tooltip, (is_enabled) ? image : disabled_image,
	    (has_img_element && position == Top && has_text),
	    (has_img_element && position == Left), (has_text) ? "" : tooltip, (is_enabled) ? image : disabled_image,
	    has_text, text,
	    (has_img_element && position == Right), (has_text) ? "" : tooltip, (is_enabled) ? image : disabled_image,
	    (has_img_element && position == Bottom && has_text),
	    (has_img_element && position == Bottom), (has_text) ? "" : tooltip, (is_enabled) ? image : disabled_image
	) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write HTML for button.");
	    goto err;
	    }

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\tgb_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"name:'%STR&SYM', "
		"text:'%STR&JSSTR', "
		"enabled:%[true%]%[false%], "
		"tristate:%[true%]%[false%], "
		"repeat:%[true%]%[false%], "
		"type:'%STR&JSSTR', "
		"border_style:'%STR&JSSTR', "
		"border_color:'%STR&JSSTR', "
		"tooltip:'%STR&JSSTR', "
		"text_color:'%STR&JSSTR', "
		"shadow_color:'%STR&JSSTR', "
		"disabled_text_color:'%STR&JSSTR', "
		"image:'%STR&JSSTR', "
		"point_image:'%STR&JSSTR', "
		"click_image:'%STR&JSSTR', "
		"disabled_image:'%STR&JSSTR', "
		"width:%INT, "
		"height:%INT, "
	    "});\n",
	    name, name, text,
	    is_enabled, !is_enabled,
	    is_tristate, !is_tristate,
	    do_repeat, !do_repeat,
	    type,
	    border_style, border_color, tooltip,
	    text_color, shadow_color, disabled_text_color,
	    image, point_image, click_image, disabled_image,
	    w, h
	) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write JS init call.");
	    goto err;
	    }

	/** Add event handlers. **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "gb", "gb_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "gb", "gb_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "gb", "gb_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "gb", "gb_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "gb", "gb_mouseup")   != 0) goto err;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTBTN",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htbtnRegisterName - register the button renderer under one widget name.
 *** The deprecated textbutton and imagebutton names share this renderer and
 *** differ only in the defaults htbtnRender() picks for them.
 ***/
int
htbtnRegisterName(char* drv_name, char* widget_name)
    {
    pHtDriver drv;

	drv = htrAllocDriver();
	if (!drv) return -1;

	strtcpy(drv->Name, drv_name, sizeof(drv->Name));
	strtcpy(drv->WidgetName, widget_name, sizeof(drv->WidgetName));
	drv->Render = htbtnRender;

	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

    return 0;
    }


/*** htbtnInitialize - register with the ht_render module.
 ***/
int
htbtnInitialize()
    {
	/*** The deprecated textbutton and imagebutton names remain part of the
	 *** language and render through here.  Their own drivers are gone.
	 ***/
	if (htbtnRegisterName("HTML Button Widget Driver", "button") != 0) return -1;
	if (htbtnRegisterName("HTML Text Button Widget Driver", "textbutton") != 0) return -1;
	if (htbtnRegisterName("HTML Image Button Widget Driver", "imagebutton") != 0) return -1;

	HTBTN.idcnt = 0;

    return 0;
    }
