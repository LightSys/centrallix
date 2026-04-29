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
/* Module: 	htdrv_autolayout.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 21, 2007   					*/
/* Description:	HTML Widget driver for autolayout of its subwidgets.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTAL;


int
htalRenderSpacer(pHtSession s, pWgtrNode tree, int z)
    {

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

    return 0;
    }

/*** htalRender - generate the HTML code for the page.
 ***/
int
htalRender(pHtSession s, pWgtrNode tree, int z)
    {
    char name[64];
    int x=-1,y=-1,w,h;

	/** Get an id for this. **/
	const int id = (HTAL.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTAL", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object.  X and Y can be assumed to be zero if unset,
	 ** but the wgtr Verify routine should have taken care of width/height for us
	 ** if those were unspecified.  Bark!
	 **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTAL","Bark! Autolayout widget must have a 'width' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTAL","Bark! Autolayout widget must have a 'height' property");
	    goto err;
	    }

	/** Get name **/
	strtcpy(name,wgtrGetName(tree),sizeof(name));

	/** Add the stylesheet for the layer **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#al%POSbase { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:visible; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
	    "}\n ",
	    id,
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    ht_flex_h(h, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTAL", "Failed to write CSS.");
	    goto err;
	    }

 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "al%POSbase", id) != 0)
	    {
	    mssError(0, "HTAL", "Failed to add object linkage.");
	    goto err;
	    }

	/** Script include call **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_autolayout.js", 0) != 0) goto err;

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s, "\tal_init(wgtrGetNodeRef(ns, '%STR&SYM'), {});\n", name) != 0)
	    {
	    mssError(0, "HTAL", "Failed to write JS init call.");
	    goto err;
	    }

	/** Start of container **/
	if (htrAddBodyItemLayerStart(s, 0, "al%POSbase", id, NULL) != 0)
	    {
	    mssError(0, "HTAL", "Failed to start body container.");
	    goto err;
	    }

	/** Check for objects within this autolayout widget. **/
	const int n_children = xaCount(&(tree->Children));
	for (unsigned int i = 0u; i < n_children; i++)
	    {
	    pWgtrNode subtree = xaGetItem(&(tree->Children), i);
	    if (!strcmp(subtree->Type, "widget/autolayoutspacer")) 
		subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	    else if (htrRenderWidget(s, subtree, z + 1) != 0) goto err;
	    }
	
	/** End of container **/
	if (htrAddBodyItemLayerEnd(s, 0) != 0)
	    {
	    mssError(0, "HTAL", "Failed to end body container.");
	    goto err;
	    }

	return 0;
	
    err:
	mssError(0, "HTAL",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htalInitialize - register with the ht_render module.
 ***/
int
htalInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver");
	strcpy(drv->WidgetName,"autolayout");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - VBox");
	strcpy(drv->WidgetName,"vbox");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - HBox");
	strcpy(drv->WidgetName,"hbox");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - Spacer");
	strcpy(drv->WidgetName,"autolayoutspacer");
	drv->Render = htalRenderSpacer;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTAL.idcnt = 0;

    return 0;
    }
