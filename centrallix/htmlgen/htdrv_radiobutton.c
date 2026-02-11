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
#include "cxlib/util.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_radiobutton.c                                     */
/* Author:      Nathan Ehresman (NRE)                                   */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for a radiobutton panel and          */
/*              radio button.                                           */
/************************************************************************/


/** globals **/
static struct
    {
    int idcnt;
    }
    HTRB;


/** htrbRender - generate the HTML code for the page.  **/
int htrbRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    
    /** Verify required capabilities. **/
    if (!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	{
	mssError(1, "HTRB", "Netscape 4.x or W3C DOM support required");
	return -1;
	}
    
    /** Get an id for this widget. **/
    const int id = (HTRB.idcnt++);
    
    /** Get x,y,w,h of this object. **/
    int x, y, w, h, spacing;
    if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&x)) != 0) x = 0;
    if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&y)) != 0) y = 0;
    if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&w)) != 0)
	{
	mssError(1,"HTRB","RadioButtonPanel widget must have a 'width' property");
	return -1;
	}
   if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&h)) != 0)
	{
	mssError(1,"HTRB","RadioButtonPanel widget must have a 'height' property");
	return -1;
	}
    if (wgtrGetPropertyValue(tree, "spacing", DATA_T_INTEGER, POD(&spacing)) != 0) spacing = 10;
    
    /** Get the name and title attributes. **/
    char name[64] = "", title[64] = "";
    if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0)
	{
	mssError(1, "HTRB", "RadioButtonPanel widget must have a 'name' property");
	return -1;
	}
    strtcpy(name, ptr, sizeof(name));
    if (wgtrGetPropertyValue(tree, "title", DATA_T_STRING, POD(&ptr)) != 0)
	{
	mssError(1, "HTRB", "RadioButtonPanel widget must have a 'title' property");
	return -1;
	}
    strtcpy(title, ptr, sizeof(title));
    
    /** Get text color attribute. **/
    char textcolor[32];
    if (wgtrGetPropertyValue(tree, "textcolor", DATA_T_STRING, POD(&ptr)) == 0)
	strtcpy(textcolor, ptr, sizeof(textcolor));
    else
	strcpy(textcolor, "black");
    
    /** Get background attributes. **/
    char main_background[128] = "";
    char outline_background[128] = "";
    htrGetBackground(tree, NULL, !s->Capabilities.Dom0NS, main_background, sizeof(main_background));
    htrGetBackground(tree, "outline", !s->Capabilities.Dom0NS, outline_background, sizeof(outline_background));
    
    /** User requesting expression for selected tab? **/
    htrCheckAddExpression(s, tree, name, "value");
    
    /** User requesting expression for selected tab using integer index value? **/
    htrCheckAddExpression(s, tree, name, "value_index");
    
    /** Get fieldname and form attributes. **/
    char fieldname[32] = "", form[64] = "";
    if (wgtrGetPropertyValue(tree, "fieldname", DATA_T_STRING, POD(&ptr)) == 0) 
	strtcpy(fieldname, ptr, sizeof(fieldname));
    if (wgtrGetPropertyValue(tree, "form", DATA_T_STRING, POD(&ptr)) == 0)
	strtcpy(form, ptr, sizeof(form));
    
    
    /** Include scripts. **/
    htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    
    /** Link DOM node to widget data. **/
    htrAddWgtrObjLinkage_va(s, tree, "rb%POSparent", id);
    htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(htr_subel(_obj, 'rb%POSborder'), 'rb%POScover')", id, id);
    
    /** Script initialization call. **/
    if (strlen(main_background) > 0)
	{
	htrAddScriptInit_va(s, "{ "
	    "const parentPane = wgtrGetNodeRef(ns, '%STR&SYM'); "
	    "const borderPane = htr_subel(parentPane, 'rb%POSborder'); "
	    "const coverPane = htr_subel(borderPane, 'rb%POScover'); "
	    "const titlePane = htr_subel(parentPane, 'rb%POStitle'); "
	    "const easterEgg2 = 'Easter Egg #2';"
	    "radiobuttonpanel_init({ "
		"parentPane, borderPane, coverPane, titlePane, "
		"fieldname:'%STR&JSSTR', "
		"mainBackground:'%STR&JSSTR', "
		"outlineBackground:'%STR&JSSTR', "
		"form:'%STR&JSSTR', "
	    "}); }\n",
	    name, id, id, id,
	    fieldname,
	    main_background,
	    outline_background,
	    form
	);
	}
    else
	{
	htrAddScriptInit_va(s,
	    "radiobuttonpanel_init({ "
		"parentPane:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"fieldname:'%STR&JSSTR', "
		"borderPane:0, "
		"coverPane:0, "
		"titlePane:0, "
		"mainBackground:0, "
		"outlineBackground:0, "
		"form:'%STR&JSSTR', "
	    "});\n",
	    name,
	    fieldname,
	    form
	);
	}
    
    /** Add event listenners. **/
    htrAddEventHandlerFunction(s, "document", "MOUSEUP", "radiobutton", "radiobutton_mouseup");
    htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "radiobutton", "radiobutton_mousedown");
    htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "radiobutton", "radiobutton_mouseover");
    htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "radiobutton", "radiobutton_mousemove");
    
    
    /** Write style headers for container DOM nodes. **/
    const int para_height = s->ClientInfo->ParagraphHeight;
    const int top_offset = (para_height * 3) / 4 + 1;
    htrAddStylesheetItem_va(s,
	"#rb%POSparent { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	    "left:"ht_flex_format"; "
	    "top:"ht_flex_format"; "
	    "width:"ht_flex_format"; "
	    "height:"ht_flex_format"; "
	    "z-index:%POS; "
	"}\n",
	id,
	ht_flex_x(x, tree),
	ht_flex_y(y, tree),
	ht_flex_w(w, tree),
	ht_flex_h(h, tree),
	z
    );
    htrAddStylesheetItem_va(s,
	"#rb%POSborder { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	    "left:3px; "
	    "top:%POSpx; "
	    "width:calc(100%% - 6px); "
	    "height:calc(100%% - %POSpx); "
	    "z-index:%POS; "
	"}\n",
	id,
	top_offset,
	top_offset + 3,
	z + 1
    );
    htrAddStylesheetItem_va(s,
	"#rb%POScover { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	    "left:1px; "
	    "top:1px; "
	    "width:calc(100%% - 2px); "
	    "height:calc(100%% - 2px); "
	    "z-index:%POS; "
	"}\n",
	id,
	z + 2
    );
    htrAddStylesheetItem_va(s,
	"#rb%POStitle { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	    "left:10px; "
	    "top:1px; "
	    "width:50%%; "
	    "height:%POSpx; "
	    "z-index:%POS; "
	"}\n",
	id,
	para_height,
	z + 3
    );
    
    
    /** Write HTML to contain the radio buttons. **/
    htrAddBodyItem_va(s, "<div id='rb%POSparent'>\n", id);
    htrAddBodyItem_va(s, "  <div id='rb%POSborder'>\n", id);
    htrAddBodyItem_va(s, "    <div id='rb%POScover'>\n", id);
    
    /** Search child array for radio buttons. **/
    XArray radio_buttons;
    xaInit(&radio_buttons, tree->Children.nItems);
    for (int i = 0; i < tree->Children.nItems; i++)
	{
	pWgtrNode child = tree->Children.Items[i];
	
	/** Mark child as no-object. **/
	child->RenderFlags |= HT_WGTF_NOOBJECT;
	
	/** Add radio buttons to the array, render other widgets immediately (so we can forget about them). **/
	wgtrGetPropertyValue(child, "outer_type", DATA_T_STRING, POD(&ptr));
	if (strcmp(ptr, "widget/radiobutton") == 0) xaAddItem(&radio_buttons, child);
	else htrRenderWidget(s, child, z + 1);
	}
    
    /** Write style for radio buttons. **/
    const int top_padding = 12;
    const int button_height = para_height + 2;
    const int content_height = top_padding + (button_height * radio_buttons.nItems) + 8;
    for (int i = 0; i < radio_buttons.nItems; i++)
	{
	pWgtrNode radio_button = radio_buttons.Items[i];
	
	/** Store data for the individual radio button. **/
	char value_buf[64] = "", label_buf[64] = "";
	if (wgtrGetPropertyValue(radio_button, "value", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(value_buf, ptr, sizeof(value_buf));
	if (wgtrGetPropertyValue(radio_button, "label", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(label_buf, ptr, sizeof(label_buf));
	const int is_selected = (htrGetBoolean(radio_button, "selected", 0) > 0);
	wgtrGetPropertyValue(radio_button, "name", DATA_T_STRING, POD(&ptr));
	
	/** Create pointers to data. **/
	char* name = ptr; /* Name temporarily stored in the wgtrGetPropertyValue() buffer. */
	char* value = value_buf;
	char* label = (label_buf[0] == '\0') ? value_buf : label_buf;
	
	/** Link the radio button DOM node to widget data. **/
	htrAddWgtrObjLinkage_va(s, radio_button, "rb%POSoption%POS", id, i);
	
	/** Write the initialization call. **/
	htrAddScriptInit_va(s, "{ "
	    "const rbitem = wgtrGetNodeRef('%STR&SYM', '%STR&SYM');"
	    "add_radiobutton(rbitem, { "
		"selected:%POS, "
		"buttonset:htr_subel(rbitem, 'rb%POSbuttonset%POS'), "
		"buttonunset:htr_subel(rbitem, 'rb%POSbuttonunset%POS'), "
		"value:htr_subel(rbitem, 'rb%POSvalue%POS'), "
		"label:htr_subel(rbitem, 'rb%POSlabel%POS'), "
		"valuestr:'%STR&JSSTR', "
		"labelstr:'%STR&JSSTR', "
	    "}); }\n", 
	    wgtrGetNamespace(radio_button), name,
	    is_selected,
	    id, i, id, i,
	    id, i, id, i,
	    value, label
	);
	
	/** Write CSS for the radio button container. **/
	const int base_top = top_padding + (button_height * i);
	const double percent_space_above = (100.0 / radio_buttons.nItems) * i;
	const double content_above = ((double)content_height / radio_buttons.nItems) * i;
	htrAddStylesheetItem_va(s,
	    "#rb%POSoption%POS { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"left:7px; "
		"top:calc(0px "
		    "+ %POSpx "
		    "+ min(%DBL%% - %DBLpx, %POSpx) "
		"); "
		"width:calc(100%% - 14px); "
		"height:%POSpx; "
		"z-index:%POS; "
	    "}\n",
	    id, i,
	    base_top,
	    percent_space_above, content_above, spacing * i,
	    button_height,
	    z + 2
	);
	
	/** Write CSS for the radio button elements. **/
	htrAddStylesheetItem_va(s,
	    "#rb%POSbuttonset%POS, "
	    "#rb%POSbuttonunset%POS { "
		"position:absolute; "
		"overflow:hidden; "
		"left:5px; "
		"top:%INTpx; "
		"width:12px; "
		"height:12px; "
		"z-index:%POS; "
		"cursor:pointer; "
	    "}\n",
	    id, i,
	    id, i,
	    (para_height / 2) - 3,
	    z + 2
	);
	htrAddStylesheetItem_va(s,
	    "#rb%POSvalue%POS { "
		"position:absolute; "
		"visibility:hidden; "
		"overflow:hidden; "
		"left:5px; "
		"top:6px; "
		"width:12px; "
		"height:12px; "
		"z-index:%POS; "
	    "}\n",
	    id, i,
	    z + 2
	);
	htrAddStylesheetItem_va(s,
	    "#rb%POSlabel%POS { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"left:27px; "
		"top:3px; "
		"width:calc(100%% - 27px); "
		"height:calc(100%% - 1px); "
		"z-index:%POS; "
		"cursor:pointer; "
	    "}\n",
	    id, i,
	    z + 2
	);
	
	/** Write radio button HTML. **/
	htrAddBodyItem_va(s, "      <div id='rb%POSoption%POS'>\n", id, i);
	htrAddBodyItem_va(s, "        <div id='rb%POSbuttonset%POS' style='visibility:hidden;'><img src='/sys/images/radiobutton_set.gif'></div>\n", id, i);
	htrAddBodyItem_va(s, "        <div id='rb%POSbuttonunset%POS' style='visibility:inherit;'><img src='/sys/images/radiobutton_unset.gif'></div>\n", id, i);
	htrAddBodyItem_va(s, "        <div id='rb%POSlabel%POS' style='color:\"%STR&HTE\";' nowrap>%STR&HTE</div>\n", id, i, textcolor, label);
	htrAddBodyItem_va(s, "        <div id='rb%POSvalue%POS' visibility='hidden'><a href='.'>%STR&HTE</a></div>\n", id, i, value);
	htrAddBodyItem   (s, "      </div>\n");
	}
    
    htrAddBodyItem_va(s,
	"    </div>\n"
	"  </div>\n"
	"  <div id=\"rb%POStitle\"><table><tr><td style='color:\"%STR&HTE\";' nowrap>%STR&HTE</td></tr></table></div>\n"
	"</div>\n",
	id, textcolor, title
    );
    
    return 0;
    }


/** htrbInitialize - register with the ht_render module.  **/
int htrbInitialize() {
   pHtDriver drv;
   
   /** Initialize globals. */
   HTRB.idcnt = 0;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML RadioButton Driver");
   strcpy(drv->WidgetName,"radiobuttonpanel");
   drv->Render = htrbRender;

   /** Events **/ 
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");

   /** Register with dhtml support. **/
   htrRegisterDriver(drv);
   htrAddSupport(drv, "dhtml");

   return 0;
}
