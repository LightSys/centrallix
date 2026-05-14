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
    HTPN;


/*** htpnRender - generate the HTML code for the page.
 ***/
int
htpnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char bdr[64];
    int x=-1,y=-1,w,h;
    int style = 1; /* 0 = lowered, 1 = raised, 2 = none, 3 = bordered */
    char* c1;
    char* c2;
    int box_offset;
    int border_radius;
    int shadow_offset, shadow_radius;
    char shadow_color[128];

	/** Get an id for this. **/
	const int id = (HTPN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTPN", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTPN","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTPN","Pane widget must have a 'height' property");
	    return -1;
	    }

	/** Border radius, for raised/lowered/bordered panes **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;

	/** Background color/image? **/
	htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_bg,sizeof(main_bg));

	/** figure out box offset fudge factor... stupid box model... **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

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

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	if (htrCheckAddExpression(s, tree, name, "enabled") < 0) goto err;
	if (htrCheckAddExpression(s, tree, name, "background") < 0) goto err;
	if (htrCheckAddExpression(s, tree, name, "bgcolor") < 0) goto err;

	/** Style of pane - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"lowered")) style = 0;
	    if (!strcmp(ptr,"raised")) style = 1;
	    if (!strcmp(ptr,"flat")) style = 2;
	    if (!strcmp(ptr,"bordered")) style = 3;
	    }
	    
	/** Computes styling colors. **/
	if (style == 1) /* raised */
	    {
	    c1 = "white";
	    c2 = "gray";
	    }
	else if (style == 0) /* lowered */
	    {
	    c1 = "gray";
	    c2 = "white";
	    }
	else if (style == 3) /* bordered */
	    {
	    if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
		strcpy(bdr, "black");
	    else
		strtcpy(bdr,ptr,sizeof(bdr));
	    }

	/** Write the CSS for borders on the pane dom node. **/
	int offset = 0;
	if (style == 2) { /* flat, the default style, nothing to do */ }
	else if (style == 0 || style == 1) /* lowered or raised */
	    {
	    offset = -2 * box_offset;
	    if (htrAddStylesheetItem_va(s,
		"\t\t#pn%POSmain {"
		    "border-style: solid; "
		    "border-width: 1px; "
		    "border-color: %STR %STR %STR %STR; "
		"}\n",
		id,
		c1, c2, c2, c1
	    ) != 0)
		{
		mssError(0, "HTPN",
		    "Failed to write %s pane CSS.",
		    (style == 0) ? "lowered" : "raised"
		);
		goto err;
		}
	    }
	else if (style == 3) /* bordered */
	    {
	    offset = -2 * box_offset;
	    if (htrAddStylesheetItem_va(s,
		"\t\t#pn%POSmain {"
		    "border-style: solid;"
		    "border-width: 1px; "
		    "border-color:%STR&CSSVAL; "
		"}\n",
		id,
		bdr
	    ) != 0)
		{
		mssError(0, "HTPN", "Failed to write bordered pane CSS.");
		goto err;
		}
	    }
	
	/** Apply the offset to the width and height. **/
	w += offset;
	h += offset;
	
	/** Write the main CSS for the pane DOM node. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#pn%POSmain {"
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
		"border-radius:%INTpx; "
		"%STR "
	    "}\n",
	    id,
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    ht_flex_h(h, tree),
	    z,
	    border_radius,
	    main_bg
	) != 0)
	    {
	    mssError(0, "HTPN", "Failed to write main CSS.");
	    goto err;
	    }

	if (shadow_radius > 0)
	    {
	    if (htrAddStylesheetItem_va(s,
		"\t\t#pn%POSmain { "
		    "box-shadow: %POSpx %POSpx %POSpx %STR&CSSVAL; "
		"}\n",
		id,
		shadow_offset, shadow_offset, shadow_radius, shadow_color
	    ) != 0)
		{
		mssError(0, "HTPN", "Failed to write shadow CSS.");
		goto err;
		}
	    }

	/** DOM linkages **/
	if (htrAddWgtrObjLinkage_va(s, tree, "pn%POSmain", id) != 0) goto err;

	/** Script include call **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_pane.js", 0) != 0) goto err;

	/** Event Handlers **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "pn", "pn_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "pn", "pn_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "pn", "pn_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "pn", "pn_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "pn", "pn_mouseup")   != 0) goto err;

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\tpn_init({"
		"mainlayer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
	    "});\n",
	    name, name
	) != 0)
	    {
	    mssError(0, "HTPN", "Failed to write JS init call.");
	    goto err;
	    }

	/** HTML body <DIV> element for the base layer. **/
	if (htrAddBodyItem_va(s,"<div id='pn%POSmain'>\n", id) != 0)
	    {
	    mssError(0, "HTPN", "Failed to write HTML opening tag.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Write the end of the container. **/
	if (htrAddBodyItem(s, "</div>\n") != 0)
	    {
	    mssError(0, "HTPN", "Failed to write HTML closing tag.");
	    goto err;
	    }

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTPN",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htpnInitialize - register with the ht_render module.
 ***/
int
htpnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"pane");
	drv->Render = htpnRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTPN.idcnt = 0;

    return 0;
    }
