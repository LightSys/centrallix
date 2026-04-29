#include <stdbool.h>
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
/* Copyright (C) 2000-2026 LightSys Technology Services, Inc.		*/
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
    int rval = -1;
    char* ptr;
    XArray radio_buttons = { nAlloc: 0};
    
    /** Get an id for this widget. **/
    const int id = (HTRB.idcnt++);
    
    /** Verify browser capabilities. **/
    if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	{
	mssError(1, "HTRB", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	goto end_free;
	}
    
    /** Get x,y,w,h of this object. **/
    int x, y, w, h, spacing;
    if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&x)) != 0) x = 0;
    if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&y)) != 0) y = 0;
    if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&w)) != 0)
	{
	mssError(1,"HTRB","RadioButtonPanel widget must have a 'width' property");
	goto end_free;
	}
   if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&h)) != 0)
	{
	mssError(1,"HTRB","RadioButtonPanel widget must have a 'height' property");
	goto end_free;
	}
    if (wgtrGetPropertyValue(tree, "spacing", DATA_T_INTEGER, POD(&spacing)) != 0) spacing = 10;
    
    /** Get the name and title attributes. **/
    char name[64] = "", title[64] = "";
    if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0)
	{
	mssError(1, "HTRB", "RadioButtonPanel widget must have a 'name' property");
	goto end_free;
	}
    strtcpy(name, ptr, sizeof(name));
    if (wgtrGetPropertyValue(tree, "title", DATA_T_STRING, POD(&ptr)) != 0)
	{
	mssError(1, "HTRB", "RadioButtonPanel widget must have a 'title' property");
	goto end_free;
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
    if (htrGetBackground(tree, NULL, true, main_background, sizeof(main_background)) != 0) goto end_free;
    if (htrGetBackground(tree, "outline", true, outline_background, sizeof(outline_background)) != 0) goto end_free;
    
    /** User requesting expression for selected tab? **/
    if (htrCheckAddExpression(s, tree, name, "value") < 0) goto end_free;
    
    /** User requesting expression for selected tab using integer index value? **/
    if (htrCheckAddExpression(s, tree, name, "value_index") < 0) goto end_free;
    
    /** Get fieldname and form attributes. **/
    char fieldname[32] = "", form[64] = "";
    if (wgtrGetPropertyValue(tree, "fieldname", DATA_T_STRING, POD(&ptr)) == 0) 
	strtcpy(fieldname, ptr, sizeof(fieldname));
    if (wgtrGetPropertyValue(tree, "form", DATA_T_STRING, POD(&ptr)) == 0)
	strtcpy(form, ptr, sizeof(form));
    
    
    /** Include scripts. **/
    if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto end_free;
    if (htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0) != 0) goto end_free;
    
    /** Link DOM node to widget data. **/
    if (htrAddWgtrObjLinkage_va(s, tree, "rb%POSparent", id) != 0) goto end_free;
    if (htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(htr_subel(_obj, 'rb%POSborder'), 'rb%POScover')", id, id) != 0) goto end_free;
    
    /** Script initialization call. **/
    if (strlen(main_background) > 0)
	{
	if (htrAddScriptInit_va(s, "\t{ "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write JS init code.");
	    goto end_free;
	    }
	}
    else
	{
	if (htrAddScriptInit_va(s,
	    "\tradiobuttonpanel_init({ "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write JS init call.");
	    goto end_free;
	    }
	}
    
    /** Add event listenners. **/
    if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "radiobutton", "radiobutton_mousedown") != 0) goto end_free;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "radiobutton", "radiobutton_mousemove") != 0) goto end_free;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "radiobutton", "radiobutton_mouseover") != 0) goto end_free;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "radiobutton", "radiobutton_mouseup")   != 0) goto end_free;
    
    
    /** Write style headers for container DOM nodes. **/
    const int para_height = s->ClientInfo->ParagraphHeight;
    const int top_offset = (para_height * 3) / 4 + 1;
    if (htrAddStylesheetItem_va(s,
	"\t\t.rb%POSall { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	"}\n",
	id, id, id, id
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write shared CSS.");
	goto end_free;
	}
    if (htrAddStylesheetItem_va(s,
	"\t\t#rb%POSparent { "
	    "cursor:default; "
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
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write parent (container) CSS.");
	goto end_free;
	}
    if (htrAddStylesheetItem_va(s,
	"\t\t#rb%POSborder { "
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
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write border CSS.");
	goto end_free;
	}
    if (htrAddStylesheetItem_va(s,
	"\t\t#rb%POScover { "
	    "left:1px; "
	    "top:1px; "
	    "width:calc(100%% - 2px); "
	    "height:calc(100%% - 2px); "
	    "z-index:%POS; "
	"}\n",
	id,
	z + 2
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write cover CSS.");
	goto end_free;
	}
    if (htrAddStylesheetItem_va(s,
	"\t\t#rb%POStitle { "
	    "left:10px; "
	    "top:1px; "
	    "width:50%%; "
	    "height:%POSpx; "
	    "z-index:%POS; "
	"}\n",
	id,
	para_height,
	z + 3
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write title CSS.");
	goto end_free;
	}
    
    
    /** Write HTML to contain the radio buttons. **/
    if (htrAddBodyItem_va(s,
	"<div id='rb%POSparent' class='rb%POSall'>"
	"<div id='rb%POSborder' class='rb%POSall'>"
	"<div id='rb%POScover' class='rb%POSall'>\n",
	id, id, id, id, id, id
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write HTML for containers.");
	goto end_free;
	}
    
    /** Search child array for radio buttons. **/
    if (check(xaInit(&radio_buttons, tree->Children.nItems)) != 0) goto end_free;
    for (int i = 0; i < tree->Children.nItems; i++)
	{
	pWgtrNode child = check_ptr(tree->Children.Items[i]);
	if (child == NULL)
	    {
	    mssError(1, "HTRB", "Child widget #%d/%d is NULL.", i + 1, tree->Children.nItems);
	    goto end_free;
	    }
	
	/** Mark child as no-object. **/
	child->RenderFlags |= HT_WGTF_NOOBJECT;
	
	/** Add radio buttons to the array, render other widgets immediately (so we can forget about them). **/
	wgtrGetPropertyValue(child, "outer_type", DATA_T_STRING, POD(&ptr));
	if (strcmp(ptr, "widget/radiobutton") == 0)
	    {
	    if (check_neg(xaAddItem(&radio_buttons, child)) < 0) goto end_free;
	    }
	else if (htrRenderWidget(s, child, z + 1) != 0) goto end_free;
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
	if (htrAddWgtrObjLinkage_va(s, radio_button, "rb%POSoption%POS", id, i) != 0) goto err_option;
	
	/** Write the initialization call. **/
	if (htrAddScriptInit_va(s, "\t{ "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write JS add_radiobutton() call.");
	    goto err_option;
	    }
	
	/** Write CSS for the radio button container. **/
	const int base_top = top_padding + (button_height * i);
	const double percent_space_above = (100.0 / radio_buttons.nItems) * i;
	const double content_above = ((double)content_height / radio_buttons.nItems) * i;
	if (htrAddStylesheetItem_va(s,
	    "\t\t#rb%POSoption%POS { "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write option CSS.");
	    goto err_option;
	    }
	
	/** Write CSS for the radio button elements. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#rb%POSbuttonset%POS, "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write option set CSS.");
	    goto err_option;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t#rb%POSvalue%POS { "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write option value CSS.");
	    goto err_option;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t#rb%POSlabel%POS { "
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
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write option label CSS.");
	    goto err_option;
	    }
	
	/** Write radio button HTML. **/
	if (htrAddBodyItem_va(s,
	    "  <div id='rb%POSoption%POS' class='rb%POSall'>\n"
	    "    <div id='rb%POSbuttonset%POS' style='visibility:hidden;'><img src='/sys/images/radiobutton_set.gif'></div>\n"
	    "    <div id='rb%POSbuttonunset%POS' style='visibility:inherit;'><img src='/sys/images/radiobutton_unset.gif'></div>\n"
	    "    <div id='rb%POSlabel%POS' style='color:\"%STR&HTE\";' nowrap>%STR&HTE</div>\n"
	    "    <div id='rb%POSvalue%POS' style='visibility:hidden;'><a href='.'>%STR&HTE</a></div>\n"
	    "  </div>\n",
	    id, i, id,
	    id, i,
	    id, i,
	    id, i, textcolor, label,
	    id, i, value
	) != 0)
	    {
	    mssError(0, "HTRB", "Failed to write option HTML.");
	    goto err_option;
	    }
	
	/** Success. **/
	continue;
	
    err_option:
	mssError(0, "HTRB", "Failed to write option #%d/%d.", i + 1, radio_buttons.nItems);
	goto end_free;
	}
    
    /** Close divs and write title. **/
    if (htrAddBodyItem_va(s,
	" </div></div>\n"
	" <div id='rb%POStitle'><table><tr><td style='color:\"%STR&HTE\";' nowrap>%STR&HTE</td></tr></table></div>\n"
	"</div>\n",
	id, textcolor, title
    ) != 0)
	{
	mssError(0, "HTRB", "Failed to write title and closing HTML tags.");
	goto end_free;
	}
    
    /** Success. **/
    rval = 0;

    end_free:
    if (rval != 0)
	{
	mssError(0, "HTRB",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
	}
    
    /** Clean up. **/
    if (radio_buttons.nAlloc != 0) xaDeInit(&radio_buttons);
    
    return rval;
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
