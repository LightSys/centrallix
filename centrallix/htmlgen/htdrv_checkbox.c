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

    $Id: htdrv_checkbox.c,v 1.26 2003/06/21 23:07:26 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_checkbox.c,v $

    $Log: htdrv_checkbox.c,v $
    Revision 1.26  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.25  2003/06/03 19:27:08  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.24  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.23  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.22  2002/07/26 04:13:28  pfinley
    oops, that shouldn't have been there.

    Revision 1.21  2002/07/25 20:33:29  pfinley
    changed 'Change' event to be the standard 'DataChange'.

    Revision 1.20  2002/07/24 20:51:24  pfinley
    updated to incorporate the change to cn_activate (reduce redundant code)

    Revision 1.19  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.18  2002/07/19 21:17:48  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.17  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.16  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.15  2002/07/07 00:18:17  jorupp
     * initial support for checbox in Mozilla -- NOT COMPLETE -- barely works, only tested on 1.1a

    Revision 1.14  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.13  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.12  2002/05/03 01:40:55  jheth
    Defined fieldname size to be 60 (from 30) in ht_render.h - HT_FIELDNAME_SIZE

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

int htcbRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char fieldname[HT_FIELDNAME_SIZE];
   int x=-1,y=-1,checked=0;
   int id;
   char *ptr;
   pObject sub_w_obj;
   pObjQuery qy;
   char name[64];
   char* nptr;

   if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
      {
      mssError(1,"HTCB","Netscape DOM support or W3C DOM Level 1 HTML required");
      return -1;
      }

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get name **/
   if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,63);
   name[63] = 0;

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
   else 
      fieldname[0]='\0';

   if (objGetAttrValue(w_obj,"checked",DATA_T_STRING,POD(&ptr)) == 0 && ptr && (!strcasecmp(ptr,"yes") || !strcasecmp(ptr,"no")))
      checked = 1;
   else
      checked = 0;

   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);
   htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#cb%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; HEIGHT:13px; WIDTH:13px; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddScriptInclude(s,"/sys/js/htdrv_checkbox.js",0);

   htrAddEventHandler(s, "document","MOUSEDOWN", "checkbox", 
      "\n"
      "    if (ly.kind == 'checkbox' && ly.enabled)\n"
      "       {\n"
      "       checkbox_toggleMode(ly);\n"
      "       cn_activate(ly, 'MouseDown');\n"
      "       }\n"
      "\n");
   
   htrAddEventHandler(s, "document","MOUSEUP", "checkbox", 
      "\n"
      "    if (ly.kind == 'checkbox' && ly.enabled) cn_activate(ly, 'MouseUp');\n"
      "\n");

   htrAddEventHandler(s, "document","MOUSEOVER", "checkbox", 
      "\n"
      "    if (ly.kind == 'checkbox' && ly.enabled) cn_activate(ly, 'MouseOver');\n"
      "\n");
   
   htrAddEventHandler(s, "document","MOUSEOUT", "checkbox", 
      "\n"
      "    if (ly.kind == 'checkbox' && ly.enabled) cn_activate(ly, 'MouseOut');\n"
      "\n");
   
   htrAddEventHandler(s, "document","MOUSEMOVE", "checkbox", 
      "\n"
      "    if (ly.kind == 'checkbox' && ly.enabled) cn_activate(ly, 'MouseMove');\n"
      "\n");
   
   /** Script initialization call. **/
   if(s->Capabilities.Dom1HTML)
       {
       htrAddScriptInit_va(s,"    %s=checkbox_init(%s.getElementById('cb%dmain'),\"%s\",%d);\n", nptr, parentname, id,fieldname,checked);
       }
   else if(s->Capabilities.Dom0NS)
       {
       htrAddScriptInit_va(s,"    %s=checkbox_init(%s.layers.cb%dmain,\"%s\",%d);\n", nptr, parentname, id,fieldname,checked);
       }

   /** HTML body <DIV> element for the layers. **/
   htrAddBodyItemLayerStart(s, 0, "cb%dmain", id);
   if (checked)
      htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_checked.gif\">\n");
   else
      htrAddBodyItem_va(s,"     <IMG SRC=\"/sys/images/checkbox_unchecked.gif\">\n");
   htrAddBodyItemLayerEnd(s, 0);

   /** Check for more sub-widgets **/
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy)
	{
	while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	    {
	    htrRenderWidget(s, sub_w_obj, z+1, parentname, nptr);
	    objClose(sub_w_obj);
	    }
	objQueryClose(qy);
	}

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
   drv->Verify = htcbVerify;

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
