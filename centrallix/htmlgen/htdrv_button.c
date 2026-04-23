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


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTBTN;


/*** htbtnRender - generate the HTML code for the page.
 ***/
int
htbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char type[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    char bgstyle[128];
    char disable_color[64];
    char n_img[128];
    char p_img[128];
    char c_img[128];
    char d_img[128];
    int x,y,w,h,spacing;
    int is_ts = 1;
    char* dptr;
    int is_enabled = 1;
    pExpression code;

	/** Get an id for this. **/
	const int id = (HTBTN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTBTN", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTBTN","Button widget must have an 'x' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTBTN","Button widget must have a 'y' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTBTN","Button widget must have a 'width' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));
	
	if (wgtrGetPropertyValue(tree,"type",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(type,ptr,sizeof(type));

	/* if not a text only button */
	if(strcmp(type,"text"))
	    {
	    if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	    strtcpy(n_img,ptr,sizeof(n_img));
	    }

        if (wgtrGetPropertyValue(tree,"pointimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(p_img,ptr,sizeof(p_img));
        else
            strcpy(p_img, n_img);

        if (wgtrGetPropertyValue(tree,"clickimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(c_img,ptr,sizeof(c_img));
        else
            strcpy(c_img, n_img);

        if (wgtrGetPropertyValue(tree,"disabledimage",DATA_T_STRING,POD(&ptr)) == 0)
            strtcpy(d_img,ptr,sizeof(d_img));
        else
            strcpy(d_img, n_img);
	
		/** Threestate button or twostate? **/
		if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

		if(strcmp(type,"image"))
		    if (wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
			{
			mssError(1,"HTBTN","Button widget must have a 'text' property");
			goto err;
			}
		strtcpy(text,ptr,sizeof(text));

		/** Get fgnd colors 1,2, and background color **/
		htrGetBackground(tree, NULL, 0, bgcolor, sizeof(bgcolor));
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

	/* spacing between image and text if both are supplied */
	if (wgtrGetPropertyValue(tree,"spacing",DATA_T_INTEGER,POD(&spacing)) != 0)  spacing=5;
	
	/** User requesting expression for enabled? **/
		if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
		    {
		    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
		    is_enabled = 0;
		    htrAddExpression(s, name, "enabled", code);
		    }
	
	/** Include the JS for the button. **/
	if (htrAddScriptGlobal(s, "gb_cur_img", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "gb_current", "null", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_button.js", 0) != 0) goto err;
	if (htrAddWgtrObjLinkage_va(s, tree, "gb%POSpane", id) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to add object linkage.");
	    goto err;
	    }
	dptr = wgtrGetDName(tree);
	if (htrAddScriptInit_va(s, "\t%STR&SYM = wgtrGetNodeRef(ns, '%STR&SYM');\n", dptr, name) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write JS.");
	    goto err;
	    }
	
	/** Write the button container. **/
	if (htrAddBodyItem_va(s, "<div id='gb%POSpane'>\n", id) != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write HTML to open button pane.");
	    goto err;
	    }
	
	/* image only button - based on imagebutton */
	if (strcmp(type, "image") == 0 || strcmp(type, "textoverimage") == 0)
	    {
	    /** Button styles. **/
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane { "
		    "position:absolute; "
		    "visibility:inherit; "
		    "left:"ht_flex_format"; "
		    "top:"ht_flex_format"; "
		    "width:"ht_flex_format"; "
		    "z-index:%POS; "
		"}\n",
		id,
		ht_flex_x(x, tree),
		ht_flex_y(y, tree),
		ht_flex_w(w, tree),
		z
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    
	    /** Button click animation. **/
	    if (is_enabled)
		{
		if (htrAddStylesheetItem_va(s,
		    "\t\t#tb%POSpane:active { "
			"transform:translate(1px, 1px); "
		    "}\n",
		    id
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write CSS click animation.");
		    goto err;
		    }
		}
	    
	    /** HTML body <div> elements for the layers. **/
	    if (htrAddBodyItem_va(s,
		"<img src='%STR&HTE' border='0' %[width='%POS' height='%POS'%]>\n",
		(is_enabled) ? n_img : d_img, (h >= 0), w, h
	    ) != 0)
		{
		mssError(0, "HTBTN",
		    "Failed to write HTML for %sabled image button.",
		    (is_enabled) ? "en" : "dis"
		);
		goto err;
		}
	    
	    /* text over image */
	    if(!strcmp(type,"textoverimage"))
		{
		if (htrAddStylesheetItem_va(s,
		    "\t\t#gb%POSpane2 { "
			"position:absolute; "
			"visibility:inherit; "
			"left:0px; "
			"top:0px; "
			"width:"ht_flex_format"; "
			"z-index:%POS; "
		    "}\n",
		    id,
		    ht_flex_w(w, tree),
		    z + 1
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write CSS for text over image button.");
		    goto err;
		    }
		
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane2' style='color:%STR&HTE; text-align:center; display:flex; justify-content:center; font-weight:700;align-items:flex-start; padding-top:1px;'>%STR&HTE</div>\n",
		    id, fgcolor1, text
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
		
	    /** Write the init script. **/
	    /** Note: We only need to specify layer2 for text-over-image. **/
	    if (htrAddScriptInit_va(s,
		"\tgb_init({ "
		    "layer:%STR&SYM, "
		    "%[layer2:htr_subel(%STR&SYM, 'gb%POSpane2'), %]"
		    "n:'%STR&JSSTR', "
		    "p:'%STR&JSSTR', "
		    "c:'%STR&JSSTR', "
		    "d:'%STR&JSSTR', "
		    "width:%INT, "
		    "height:%INT, "
		    "name:'%STR&SYM', "
		    "enable:%INT, "
		    "type:'%STR&JSSTR', "
		    "text:'%STR&JSSTR', "
		"});\n",
		dptr,
		(strcmp(type, "image") != 0), dptr, id,
		n_img, p_img, c_img, d_img,
		w, h, name, is_enabled, type, text
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write JS init call.");
		goto err;
		}
	    }
	else /* other types of buttons - based on textbutton */
	    {
	    /** Write common CSS for all three panes. **/
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane, #gb%POSpane1, #gb%POSpane2, #gb%POSpane3 { "
		    "position:absolute; "
		    "%[cursor:pointer; %]"
		    "text-align:center; "
		    "display:flex; "
		    "justify-content:center; "
		    "font-weight:700; "
		"}\n",
		id, id, id, id,
		is_enabled
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    
	    /** Write unique CSS each pane individually. **/
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane { "
		    "visibility:inherit; "
		    "overflow:hidden; "
		    "left:"ht_flex_format"; "
		    "top:"ht_flex_format"; "
		    "width:"ht_flex_format"; "
		    "z-index:%POS; "
		    "border-width:1px; "
		    "border-style:solid; "
		    "border-color:white gray gray white; "
		    "%STR "
		"}\n",
		id,
		ht_flex_x(x, tree),
		ht_flex_y(y, tree),
		ht_flex_w(w - 3, tree),
		z,
		bgstyle
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane1 { "
		    "color:%STR&HTE; "
		"}\n",
		id,
		fgcolor2
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane2 { "
		    "visibility:%STR; "
		    "left:-1px; "
		    "top:-1px; "
		    "width:"ht_flex_format"; "
		    "color:%STR&HTE; "
		    "z-index:%INT; "
		"}\n",
		id,
		(is_enabled) ? "inherit" : "hidden",
		ht_flex_w(w - 3, tree),
		fgcolor1,
		z + 1
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    if (htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane3 { "
		    "visibility:%STR; "
		    "left:0px; "
		    "top:0px; "
		    "width:"ht_flex_format"; "
		    "color:%STR&HTE; "
		    "z-index:%INT; "
		"}\n",
		id,
		(is_enabled) ? "hidden" : "inherit",
		ht_flex_w(w - 3, tree),
		disable_color,
		z + 1
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    
	    /** Write CSS heights, if specified. **/
	    if (h >= 0 && htrAddStylesheetItem_va(s,
		"\t\t#gb%POSpane #gb%POSpane1, #gb%POSpane2, #gb%POSpane3 { height: "ht_flex_format"; }\n",
		id, id, id, id,
		ht_flex_h(h - 3, tree)
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write CSS.");
		goto err;
		}
	    
	    if (strcmp(type, "text") == 0)
		{
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane1' style='align-items:flex-start; padding-top:1px;'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane2' style='align-items:flex-start; padding-top:1px;'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane3' style='align-items:flex-start; padding-top:1px;'>%STR&HTE</div>\n",
		    id, text,
		    id, text,
		    id, text
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
	    else if (strcmp(type, "topimage") == 0)
		{
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane1' style='align-items:center; flex-direction:column; gap:0.25rem;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane2' style='align-items:center; flex-direction:column; gap:0.25rem;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane3' style='align-items:center; flex-direction:column; gap:0.25rem;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n",
		    id, n_img, text,
		    id, n_img, text,
		    id, n_img, text
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
	    else if (strcmp(type, "bottomimage") == 0)
		{
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane1' style='align-items:center; flex-direction:column; gap:0.25rem;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n"
		    "<div id='gb%POSpane2' style='align-items:center; flex-direction:column; gap:0.25rem;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n"
		    "<div id='gb%POSpane3' style='align-items:center; flex-direction:column; gap:0.25rem;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n",
		    id, text, n_img,
		    id, text, n_img,
		    id, text, n_img
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
	    else if (strcmp(type, "leftimage") == 0)
		{
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane1' style='align-items:center; flex-direction:row;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane2' style='align-items:center; flex-direction:row;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n"
		    "<div id='gb%POSpane3' style='align-items:center; flex-direction:row;'><img src='%STR&HTE' alt='top img'>%STR&HTE</div>\n",
		    id, n_img, text,
		    id, n_img, text,
		    id, n_img, text
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
	    else if (strcmp(type, "rightimage") == 0)
		{
		if (htrAddBodyItem_va(s,
		    "<div id='gb%POSpane1' style='align-items:center; flex-direction:row;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n"
		    "<div id='gb%POSpane2' style='align-items:center; flex-direction:row;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n"
		    "<div id='gb%POSpane3' style='align-items:center; flex-direction:row;'>%STR&HTE<img src='%STR&HTE' alt='bottom img'></div>\n",
		    id, text, n_img,
		    id, text, n_img,
		    id, text, n_img
		) != 0)
		    {
		    mssError(0, "HTBTN", "Failed to write pane HTML.");
		    goto err;
		    }
		}
	    else
		{
		mssError(1, "HTBTN", "Unknown button type \"%s\".", type);
		goto err;
		}
	    
	    /** Script initialization call. **/
	    if (htrAddScriptInit_va(s,
		"\tgb_init({ "
		    "layer:%STR&SYM, "
		    "layer2:htr_subel(%STR&SYM, 'gb%POSpane2'), "
		    "layer3:htr_subel(%STR&SYM, 'gb%POSpane3'), "
		    "top:null, "
		    "bottom:null, "
		    "right:null, "
		    "left:null, "
		    "width:%INT, "
		    "height:%INT, "
		    "tristate:%INT, "
		    "name:'%STR&SYM', "
		    "text:'%STR&JSSTR', "
		    "n:'%STR&JSSTR', "
		    "p:'%STR&JSSTR', "
		    "c:'%STR&JSSTR', "
		    "d:'%STR&JSSTR', "
		    "type:'%STR&JSSTR', "
		"});\n",
		dptr, dptr, id, dptr, id,
		w, h, is_ts, name, text,
		n_img, p_img, c_img, d_img, type
	    ) != 0)
		{
		mssError(0, "HTBTN", "Failed to write JS init call.");
		goto err;
		}
	    }
	
	/** Close the button container. **/
	if (htrAddBodyItem(s, "</div>") != 0)
	    {
	    mssError(0, "HTBTN", "Failed to write HTML closing tag.");
	    goto err;
	    }

	/** Add the event handling scripts **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "gb", "gb_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "gb", "gb_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "gb", "gb_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "gb", "gb_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "gb", "gb_mouseup")   != 0) goto err;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 3) != 0) goto err;

	return 0;

    err:
	mssError(0, "HTBTN",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htbtnInitialize - register with the ht_render module.
 ***/
int
htbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Button Widget Driver");
	strcpy(drv->WidgetName,"button");
	drv->Render = htbtnRender;

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

	HTBTN.idcnt = 0;

    return 0;
    }
