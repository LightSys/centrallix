#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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

/**CVSDATA***************************************************************

    $Id: htdrv_checkbox.c,v 1.1 2001/08/13 18:00:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_checkbox.c,v $

    $Log: htdrv_checkbox.c,v $
    Revision 1.1  2001/08/13 18:00:49  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:56  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTCB;


/* 
   htcbVerify - not written yet.
*/
int htcbVerify() {
   return 0;
}


/* 
   htcbRender - generate the HTML code for the page.
*/
int htcbRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char sbuf[160];
   int x=-1,y=-1;
   int id;

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;

   /** Ok, write the style header items. **/
   sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"\t#cb%dpane1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"\t#cb%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   /** Checkbox initializer **/
   htrAddScriptFunction(s, "checkbox_init", "\n"
      "function checkbox_init(pane1, pane2) {\n"
      "   pane1.kind = 'checkbox';\n"
      "   pane2.kind = 'checkbox';\n"
      "   pane1.document.images[0].kind = 'checkbox';\n"
      "   pane2.document.images[0].kind = 'checkbox';\n"
      "   pane1.checkedLayer = pane2;\n"
      "   pane1.uncheckedLayer = pane1;\n"
      "   pane2.checkedLayer = pane2;\n"
      "   pane2.uncheckedLayer = pane1;\n"
      "   pane1.document.images[0].checkedLayer = pane2;\n"
      "   pane1.document.images[0].uncheckedLayer = pane1;\n"
      "   pane2.document.images[0].checkedLayer = pane2;\n"
      "   pane2.document.images[0].uncheckedLayer = pane1;\n"
      "}\n", 0);

   /** Checkbox toggle mode function **/
   htrAddScriptFunction(s, "checkbox_toggleMode", "\n"
      "function checkbox_toggleMode(layer) {\n"
      "   if (layer.uncheckedLayer.visibility == 'hide') {\n"
      "      layer.uncheckedLayer.visibility = 'inherit';\n"
      "      layer.checkedLayer.visibility = 'hidden';\n"
      "   } else {\n"
      "      layer.checkedLayer.visibility = 'inherit';\n"
      "      layer.uncheckedLayer.visibility = 'hidden';\n"
      "   }\n"
      "}\n", 0);

   htrAddEventHandler(s, "document","MOUSEDOWN", "checkbox", 
      "\n"
      "   if (e.target != null && e.target.kind == 'checkbox') {\n"
      "      if (e.target.layer != null)\n"
      "         layer = e.target.layer;\n"
      "      else\n"
      "         layer = e.target;\n"
      "      checkbox_toggleMode(layer);\n"
      "   }\n"
      "\n");

   /** Script initialization call. **/
   sprintf(sbuf,"   checkbox_init(%s.layers.cb%dpane1, %s.layers.cb%dpane2);\n",
      parentname, id, parentname, id);
   htrAddScriptInit(s, sbuf);

   /** HTML body <DIV> element for the layers. **/
   sprintf(sbuf,"   <DIV ID=\"cb%dpane1\"><IMG SRC=/sys/images/checkbox_unchecked.gif></DIV>\n",id);
   htrAddBodyItem(s, sbuf);
   sprintf(sbuf,"   <DIV ID=\"cb%dpane2\"><IMG SRC=/sys/images/checkbox_checked.gif></DIV>\n",id);
   htrAddBodyItem(s, sbuf);

   return 0;
}


/* 
   htcbInitialize - register with the ht_render module.
*/
int htcbInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Checkbox Driver");
   strcpy(drv->WidgetName,"checkbox");
   drv->Render = htcbRender;
   drv->Verify = htcbVerify;
   xaInit(&(drv->PosParams),16);
   xaInit(&(drv->Properties),16);
   xaInit(&(drv->Events),16);
   xaInit(&(drv->Actions),16);

#if 00
   /** Add the 'load page' action **/
   action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
   strcpy(action->Name,"LoadPage");
   xaInit(&action->Parameters,16);
   param = (pHtParam)nmSysMalloc(sizeof(HtParam));
   strcpy(param->ParamName,"Source");
   param->DataType = DATA_T_STRING;
   xaAddItem(&action->Parameters,(void*)param);
   xaAddItem(&drv->Actions,(void*)action);
#endif

   /** Register. **/
   htrRegisterDriver(drv);

   HTCB.idcnt = 0;

   return 0;
}
