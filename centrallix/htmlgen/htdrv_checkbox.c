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
/* Module:      htdrv_checkbox.c                                        */
/* Author:      Nathan Ehresman (NRE)                                   */
/* Creation:    Feb. 21, 2000                                           */
/* Description: HTML Widget driver for a checkbox                       */
/************************************************************************/


/** globals **/
static struct {
   int     idcnt;
} HTCB;


int htcbRender(pHtSession s, pWgtrNode tree, int z) {
   char fieldname[HT_FIELDNAME_SIZE];
   int x=-1,y=-1,checked=0;
   int id, i;
   char *ptr;
   char name[64];
   char form[64];
   int enabled = 0;

   if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
      {
      mssError(1,"HTCB","Netscape DOM support or W3C DOM Level 1 HTML required");
      return -1;
      }

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

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

   /** Write named global **/
   htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"cb%INTmain\")", id);
   htrAddWgtrCtrLinkage(s, tree, "_obj");

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#cb%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; HEIGHT:13px; WIDTH:13px; Z-INDEX:%POS; }\n",id,x,y,z);
   htrAddScriptInclude(s,"/sys/js/htdrv_checkbox.js",0);
   htrAddScriptInclude(s,"/sys/js/ht_utils_hints.js",0);

   htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "checkbox", "checkbox_mousedown");
   htrAddEventHandlerFunction(s, "document","MOUSEUP", "checkbox", "checkbox_mouseup");
   htrAddEventHandlerFunction(s, "document","MOUSEOVER", "checkbox", "checkbox_mouseover");
   htrAddEventHandlerFunction(s, "document","MOUSEOUT", "checkbox", "checkbox_mouseout");
   htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "checkbox", "checkbox_mousemove");
   
   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    checkbox_init({layer:nodes[\"%STR&SYM\"], fieldname:\"%STR&JSSTR\", checked:%INT, enabled:%INT, form:\"%STR&JSSTR\"});\n", name, fieldname,checked,enabled,form);

   /** HTML body <DIV> element for the layers. **/
   htrAddBodyItemLayerStart(s, 0, "cb%POSmain", id);
   switch(checked)
	{
	case 1:
	    htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_checked%[_dis%].gif\">\n",!enabled);
	    break;
	case 0:
	    htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_unchecked%[_dis%].gif\">\n",!enabled);
	    break;
	case -1: /* null */
	    htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_null%[_dis%].gif\">\n",!enabled);
	    break;
	}

   htrAddBodyItemLayerEnd(s, 0);

   /** Check for more sub-widgets **/
    for (i=0;i<xaCount(&(tree->Children));i++)
	 htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

   return 0;
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
