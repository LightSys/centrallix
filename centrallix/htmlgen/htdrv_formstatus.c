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
   int id;
   char name[64];
   char form[64];
   char* ptr;
   char* style;
   int w;

   if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
       {
       mssError(1,"HTFS","Netscape DOM or W3C DOM1 HTML and CSS1 support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTFS.idcnt++);

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
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(name,ptr,sizeof(name));
   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) != 0)
       form[0] = '\0';
   else
       strtcpy(form,ptr,sizeof(form));
   htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"fs%POSmain\")", id);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#fs%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; HEIGHT:13px; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);

   htrAddScriptInclude(s, "/sys/js/htdrv_formstatus.js", 0);

   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    fs_init({layer:nodes[\"%STR&SYM\"],form:\"%STR&JSSTR\",style:\"%STR&JSSTR\"});\n",
	    name, form, style);

   /** HTML body <DIV> element for the layers. **/
   if (!strcmp(style,"large"))
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%POSmain\"><IMG SRC=/sys/images/formstatL05.png></DIV>\n", id);
   else if (!strcmp(style,"largeflat"))
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%POSmain\"><IMG SRC=/sys/images/formstatLF05.png></DIV>\n", id);
   else
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%POSmain\"><IMG SRC=/sys/images/formstat05.gif></DIV>\n", id);

   htrAddEventHandlerFunction(s,"document","MOUSEDOWN","fs","fs_mousedown");
   htrAddEventHandlerFunction(s,"document","MOUSEUP",  "fs","fs_mouseup");
   htrAddEventHandlerFunction(s,"document","MOUSEOVER","fs","fs_mouseover");
   htrAddEventHandlerFunction(s,"document","MOUSEOUT", "fs","fs_mouseout");
   htrAddEventHandlerFunction(s,"document","MOUSEMOVE","fs","fs_mousemove");

   htrRenderSubwidgets(s, tree, z+2);

   return 0;
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
