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
/* Module:      htdrv_formstatus.c                                      */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a form status                    */
/************************************************************************/


/** globals **/
static struct {
   int     idcnt;
} HTFS;


/* 
   htfsRender - generate the HTML code for the page.
*/
int htfsRender(pHtSession s, pWgtrNode tree, int z) {
   int x=-1,y=-1;
   char name[64];
   char form[64];
   char* ptr;
   char* style;
   int w;

   /** Get an id for this. **/
   const int id = (HTFS.idcnt++);

    /** Verify browser capabilities. **/
    if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	{
	mssError(1, "HTFS", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	goto err;
	}

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;

   /** Get optional style **/
   if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&style)) != 0) style = "";
   if (!strcmp(style,"large") || !strcmp(style,"largeflat"))
       w = 90;
   else
       w = 13;

   /** Write named global **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
   strtcpy(name,ptr,sizeof(name));
   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) != 0)
       form[0] = '\0';
   else
       strtcpy(form,ptr,sizeof(form));

    /** Link the widget to the DOM node. **/
    if (htrAddWgtrObjLinkage_va(s, tree, "fs%POSmain", id) != 0)
	{
	mssError(0, "HTFS", "Failed to render object linkage.");
	goto err;
	}

   /** Ok, write the style header items. **/
    if (htrAddStylesheetItem_va(s,
	"\t\t#fs%POSmain { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "left:"ht_flex_format"; "
	    "top:"ht_flex_format"; "
	    "width:"ht_flex_format"; "
	    "height:13px; "
	    "z-index:%POS; "
	"}\n",
	id,
	ht_flex_x(x, tree),
	ht_flex_y(y, tree),
	ht_flex_w(w, tree),
	z
    ) != 0)
	{
	mssError(0, "HTFS", "Failed to write main CSS.");
	goto err;
	}

    if (htrAddScriptInclude(s, "/sys/js/htdrv_formstatus.js", 0) != 0) goto err;

   /** Script initialization call. **/
   if (htrAddScriptInit_va(s,
	"\tfs_init({ "
	    "layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
	    "form:'%STR&JSSTR', "
	    "style:'%STR&JSSTR', "
	"});\n",
	name, form, style
    ) != 0)
	{
	mssError(0, "HTFS", "Failed to write JS init call.");
	goto err;
	}

   /** HTML body <DIV> element for the layers. **/
    const char* type = "";
    if (strcmp(style, "large") == 0) type = "L";
    else if (strcmp(style, "largeflat") == 0)  type = "LF";
    if (htrAddBodyItem_va(s,
	"   <div id=\"fs%POSmain\"><img src=/sys/images/formstat%STR05.png></div>\n",
	id, type
    ) != 0)
	{
	mssError(0, "HTFS", "Failed to render child widgets.");
	goto err;
	}

    /** Register JS event handler functions. **/
    if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN","fs", "fs_mousedown") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE","fs", "fs_mousemove") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "fs", "fs_mouseout")  != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER","fs", "fs_mouseover") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",  "fs", "fs_mouseup")   != 0) goto err;

    /** Render children. **/
    if (htrRenderSubwidgets(s, tree, z + 2) != 0) goto err;

    /** Success. **/
    return 0;

    err:
    mssError(0, "HTFS",
	"Failed to render \"%s\":\"%s\" (id: %d).",
	tree->Name, tree->Type, id
    );
    return -1;
}


/* 
   htfsInitialize - register with the ht_render module.
*/
int htfsInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Form Status Driver");
   strcpy(drv->WidgetName,"formstatus");
   drv->Render = htfsRender;

   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTFS.idcnt = 0;

   return 0;
}
