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

    $Id: htdrv_checkbox.c,v 1.2 2002/02/23 03:28:51 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_checkbox.c,v $

    $Log: htdrv_checkbox.c,v $
    Revision 1.2  2002/02/23 03:28:51  lkehresman
    Reworked the Checkbox widget to be form-aware.  In theory this works..

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

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
   sprintf(sbuf,"\t#cb%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   /** Get value function **/
   htrAddScriptFunction(s, "checkbox_getvalue", "\n"
      "function checkbox_getvalue()\n"
      "    {\n"
	  "    return this.is_checked;\n"
      "    }\n",0);

   /** Set value function **/
   htrAddScriptFunction(s, "checkbox_setvalue", "\n"
      "function checkbox_setvalue(v,f)\n"
      "    {\n"
      "    }\n",0);

   /** Checkbox initializer **/
   htrAddScriptFunction(s, "checkbox_init", "\n"
      "function checkbox_init(l) {\n"
	  "   l.kind = 'checkbox';\n"
	  "   l.is_checked = 0;\n"
	  "   l.document.images[0].kind = 'checkbox';\n"
	  "   l.document.images[0].is_checked = l.is_checked;\n"
	  "   l.document.images[0].uncheckedImage = new Image();\n"
	  "   l.document.images[0].uncheckedImage.kind = 'checkbox';\n"
	  "   l.document.images[0].uncheckedImage.src = \"/sys/images/checkbox_unchecked.gif\";\n"
	  "   l.document.images[0].uncheckedImage.is_checked = l.is_checked;\n"
	  "   l.document.images[0].checkedImage = new Image();\n"
	  "   l.document.images[0].checkedImage.kind = 'checkbox';\n"
	  "   l.document.images[0].checkedImage.src = \"/sys/images/checkbox_checked.gif\";\n"
	  "   l.document.images[0].checkedImage.is_checked = l.is_checked;\n"
	  "   if (fm_current) fm_current.Register(l);\n"
      "}\n", 0);

   /** Checkbox toggle mode function **/
   htrAddScriptFunction(s, "checkbox_toggleMode", "\n"
      "function checkbox_toggleMode(layer) {\n"
      "   if (layer.is_checked) {\n"
      "       layer.src = layer.uncheckedImage.src;\n"
      "       layer.is_checked = 0;\n"
      "   } else {\n"
      "       layer.src = layer.checkedImage.src;\n"
      "       layer.is_checked = 1;\n"
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
   sprintf(sbuf,"   checkbox_init(%s.layers.cb%dmain);\n", parentname, id);
   htrAddScriptInit(s, sbuf);

   /** HTML body <DIV> element for the layers. **/
   sprintf(sbuf,"   <DIV ID=\"cb%dmain\">\n",id);
   htrAddBodyItem(s, sbuf);
   sprintf(sbuf,"     <IMG SRC=/sys/images/checkbox_unchecked.gif>\n");
   htrAddBodyItem(s, sbuf);
   sprintf(sbuf,"   </DIV>\n");
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
