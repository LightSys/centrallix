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
/* Module:      htdrv_checkbox.c                                        */
/* Author:      Nathan Ehresman (NRE)                                   */
/* Creation:    Feb. 21, 2000                                           */
/* Description: HTML Widget driver for a checkbox                       */
/************************************************************************/


/** globals **/
static struct {
   int     idcnt;
} HTCB;


int htcbRender(pHtSession s, pWgtrNode tree, int z)
    {
   char fieldname[HT_FIELDNAME_SIZE];
   int x=-1,y=-1,checked=0;
   char *ptr;
   char name[64];
   char form[64];
   int enabled = 0;

   /** Get an id for this. **/
   const int id = (HTCB.idcnt++);

    /** Verify browser capabilities. **/
    if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	{
	mssError(1, "HTCB", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	goto err;
	}

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(name, ptr, sizeof(name));

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      strtcpy(fieldname,ptr,HT_FIELDNAME_SIZE);
   else 
      fieldname[0]='\0';

   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
       strtcpy(form,ptr,sizeof(form));
   else
       form[0]='\0';

   /** Is it checked? **/
   checked = htrGetBoolean(tree, "checked", -1);

   /** Is it enabled? **/
   enabled = htrGetBoolean(tree, "enabled", 1);

    /** Link the widget to the DOM node. **/
    if (htrAddWgtrObjLinkage_va(s, tree, "cb%INTmain", id) != 0)
	{
	mssError(0, "HTCB", "Failed to add object linkage.");
	goto err;
	}
   
    /** Write style header. **/
    if (htrAddStylesheetItem_va(s,
	"\t\t#cb%POSmain { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "cursor:pointer; "
	    "left:"ht_flex_format"; "
	    "top:"ht_flex_format"; "
	    "height:13px; "
	    "width:13px; "
	    "z-index:%POS; "
	"}\n",
	id,
	ht_flex_x(x, tree),
	ht_flex_y(y, tree),
	z
     ) != 0)
	{
	mssError(0, "HTCB", "Failed to write CSS.");
	goto err;
	}
   
    /** Include scripts. **/
    if (htrAddScriptInclude(s,"/sys/js/ht_utils_hints.js", 0) != 0) goto err;
    if (htrAddScriptInclude(s,"/sys/js/htdrv_checkbox.js", 0) != 0) goto err;

    /** Register event handlers. **/
    if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "checkbox", "checkbox_mousedown") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "checkbox", "checkbox_mousemove") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "checkbox", "checkbox_mouseout")  != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "checkbox", "checkbox_mouseover") != 0) goto err;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "checkbox", "checkbox_mouseup")   != 0) goto err;

    /** Script initialization call. **/
    if (htrAddScriptInit_va(s,
	"\tcheckbox_init({ "
	    "layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
	    "fieldname:'%STR&JSSTR', "
	    "checked:%INT, "
	    "enabled:%INT, "
	    "form:'%STR&JSSTR', "
	"});\n",
	name, fieldname, checked, enabled, form
    ) != 0)
	{
	mssError(0, "HTCB", "Failed to write JS init call.");
	goto err;
	}

    /** Write HTML. **/
    if (htrAddBodyItemLayerStart(s, 0, "cb%POSmain", id, NULL) != 0)
	{
	mssError(0, "HTCB", "Failed to write HTML layer start.");
	goto err;
	}
    char* state_name;
    switch (checked)
	{
	case  1: state_name = "checked"; break;
	case  0: state_name = "unchecked"; break;
	case -1: state_name = "null"; break;
	default:
	    mssError(0, "HTCB", "Unexpected value %d for 'checked'.", checked);
	    goto err;
	}
    char src_path[48];
    snprintf(src_path, sizeof(src_path), "/sys/images/checkbox_%s%s.gif", state_name, (enabled) ? "" : "_dis");
    if (htrAddBodyItem_va(s, "     <img src='%STR'>\n", src_path) != 0)
	{
	mssError(0, "HTCB", "Failed to write HTML <img> tag.");
	goto err;
	}
    if (htrAddBodyItemLayerEnd(s, 0) != 0)
	{
	mssError(0, "HTCB", "Failed to write HTML layer start.");
	goto err;
	}

    /** Render children. **/
    if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

    /** Success. **/
    return 0;

    err:
    mssError(0, "HTCB",
	"Failed to render \"%s\":\"%s\" (id: %d).",
	tree->Name, tree->Type, id
    );
    return -1;
    }


/* 
   htcbInitialize - register with the ht_render module.
*/
int htcbInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Checkbox Driver");
   strcpy(drv->WidgetName,"checkbox");
   drv->Render = htcbRender;

   /** Events **/
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");
   
   HTCB.idcnt = 0;

   return 0;
}
