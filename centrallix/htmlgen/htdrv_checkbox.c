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

    $Id: htdrv_checkbox.c,v 1.11 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_checkbox.c,v $

    $Log: htdrv_checkbox.c,v $
    Revision 1.11  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.10  2002/03/08 23:11:11  lkehresman
    Added all of the specified functions to the checkbox:
      * resetvalue()
      * enable()
      * disable()
      * readonly()

    Revision 1.9  2002/03/05 01:59:32  lkehresman
    fixed my error

    Revision 1.8  2002/03/05 01:55:09  lkehresman
    Added "clearvalue" method to form widgets

    Revision 1.7  2002/03/05 00:31:40  lkehresman
    Implemented DataNotify form method in the radiobutton and checkbox widgets

    Revision 1.6  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.5  2002/02/23 19:35:28  lkehresman
    * Radio button widget is now forms aware.
    * Fixes a couple of oddities in the checkbox.
    * Fixed some formatting issues in the form.

    Revision 1.4  2002/02/23 03:50:41  lkehresman
    Implemented the setvalue() function (previously a stub).  If anything that
    would be evaluated as TRUE get sent, the checkbox gets checked.  Otherwise
    it will be unchecked.

    Revision 1.3  2002/02/23 03:32:24  lkehresman
    ...all theories are made to be broken.  Just like this checkbox was before
    I remapped a couple functions to work properly.

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
   char sbuf[HT_SBUF_SIZE];
   char fieldname[30];
   int x=-1,y=-1,checked=0;
   int id;
   char *ptr;

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
      {
      strncpy(fieldname,ptr,30);
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   if (objGetAttrValue(w_obj,"checked",POD(&ptr)) != 0)
      { 
      checked = 0;
      } 
   else
      { 
      checked = 1;
      } 

   /** Ok, write the style header items. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"\t#cb%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   /** Get value function **/
   htrAddScriptFunction(s, "checkbox_getvalue", "\n"
      "function checkbox_getvalue()\n"
      "    {\n"
      "    return this.document.images[0].is_checked;\n" /* not sure why, but it works - JDR */
      "    }\n",0);

   /** Set value function **/
   htrAddScriptFunction(s, "checkbox_setvalue", "\n"
      "function checkbox_setvalue(v)\n"
      "    {\n"
      "    if (v)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        this.document.images[0].is_checked = 1;\n"
      "        this.is_checked = 1;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        this.document.images[0].is_checked = 0;\n"
      "        this.is_checked = 0;\n"
      "        }\n"
      "    }\n",0);

   /** Clear function **/
   htrAddScriptFunction(s, "checkbox_clearvalue", "\n"
      "function checkbox_clearvalue()\n"
      "    {\n"
      "    this.setvalue(false);\n"
      "    }\n", 0);

   /** reset value function **/
   htrAddScriptFunction(s, "checkbox_resetvalue", "\n"
      "function checkbox_resetvalue()\n"
      "    {\n"
      "    this.setvalue(false);\n"
      "    }\n",0);

   /** enable function **/
   htrAddScriptFunction(s, "checkbox_enable", "\n"
      "function checkbox_enable()\n"
      "    {\n"
      "    this.enabled = true;\n"
      "    this.document.images[0].uncheckedImage.enabled = this.enabled;\n"
      "    this.document.images[0].checkedImage.enabled = this.enabled;\n"
      "    this.document.images[0].enabled = this.enabled;\n"
      "    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked.gif';\n"
      "    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked.gif';\n"
      "    if (this.is_checked)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        }\n"
      "    }\n",0);

   /** read-only function - marks the widget as "readonly" **/
   htrAddScriptFunction(s, "checkbox_readonly", "\n"
      "function checkbox_readonly()\n"
      "    {\n"
      "    this.enabled = false;\n"
      "    this.document.images[0].uncheckedImage.enabled = this.enabled;\n"
      "    this.document.images[0].checkedImage.enabled = this.enabled;\n"
      "    this.document.images[0].enabled = this.enabled;\n"
      "    }\n",0);

   /** disable function - disables the widget completely (visually too) **/
   htrAddScriptFunction(s, "checkbox_disable", "\n"
      "function checkbox_disable()\n"
      "    {\n"
      "    this.readonly();\n"
      "    this.document.images[0].uncheckedImage.src = '/sys/images/checkbox_unchecked_dis.gif';\n"
      "    this.document.images[0].checkedImage.src = '/sys/images/checkbox_checked_dis.gif';\n"
      "    if (this.is_checked)\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].checkedImage.src;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.document.images[0].src = this.document.images[0].uncheckedImage.src;\n"
      "        }\n"
      "    }\n",0);

   /** Checkbox initializer **/
   htrAddScriptFunction(s, "checkbox_init", "\n"
      "function checkbox_init(l,fieldname,checked) {\n"
      "   l.kind = 'checkbox';\n"
      "   l.fieldname = fieldname;\n"
      "   l.is_checked = checked;\n"
      "   l.enabled = true;\n"
      "   l.form = fm_current;\n"
      "   l.document.images[0].kind = 'checkbox';\n"
      "   l.document.images[0].form = l.form;\n"
      "   l.document.images[0].parentLayer = l;\n"
      "   l.document.images[0].is_checked = l.is_checked;\n"
      "   l.document.images[0].enabled = l.enabled;\n"
      "   l.document.images[0].uncheckedImage = new Image();\n"
      "   l.document.images[0].uncheckedImage.kind = 'checkbox';\n"
      "   l.document.images[0].uncheckedImage.src = \"/sys/images/checkbox_unchecked.gif\";\n"
      "   l.document.images[0].uncheckedImage.is_checked = l.is_checked;\n"
      "   l.document.images[0].uncheckedImage.enabled = l.enabled;\n"
      "   l.document.images[0].checkedImage = new Image();\n"
      "   l.document.images[0].checkedImage.kind = 'checkbox';\n"
      "   l.document.images[0].checkedImage.src = \"/sys/images/checkbox_checked.gif\";\n"
      "   l.document.images[0].checkedImage.is_checked = l.is_checked;\n"
      "   l.document.images[0].checkedImage.enabled = l.enabled;\n"
      "   l.setvalue   = checkbox_setvalue;\n"
      "   l.getvalue   = checkbox_getvalue;\n"
      "   l.clearvalue = checkbox_clearvalue;\n"
      "   l.resetvalue = checkbox_resetvalue;\n"
      "   l.enable     = checkbox_enable;\n"
      "   l.readonly   = checkbox_readonly;\n"
      "   l.disable    = checkbox_disable;\n"
      "   if (fm_current) fm_current.Register(l);\n"
      "}\n", 0);

   /** Checkbox toggle mode function **/
   htrAddScriptFunction(s, "checkbox_toggleMode", "\n"
      "function checkbox_toggleMode(layer) {\n"
      "   if (layer.form) {\n"
      "       if (layer.parentLayer) {\n"
      "           layer.form.DataNotify(layer.parentLayer);\n"
      "       } else {\n"
      "           layer.form.DataNotify(layer);\n"
      "       }\n"
      "   }\n"
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
      "      if (layer.enabled)\n"
      "         checkbox_toggleMode(layer);\n"
      "   }\n"
      "\n");

   /** Script initialization call. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    checkbox_init(%s.layers.cb%dmain,\"%s\",%d);\n", parentname, id,fieldname,checked);
   htrAddScriptInit(s, sbuf);

   /** HTML body <DIV> element for the layers. **/
   snprintf(sbuf,HT_SBUF_SIZE,"   <DIV ID=\"cb%dmain\">\n",id);
   htrAddBodyItem(s, sbuf);
   if (checked)
      {
      snprintf(sbuf,HT_SBUF_SIZE,"     <IMG SRC=/sys/images/checkbox_checked.gif>\n");
      }
   else
      {
      snprintf(sbuf,HT_SBUF_SIZE,"     <IMG SRC=/sys/images/checkbox_unchecked.gif>\n");
      }
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"   </DIV>\n");
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
