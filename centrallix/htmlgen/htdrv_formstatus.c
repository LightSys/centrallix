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
/* Module:      htdrv_formstatus.c                                      */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a form status                    */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_formstatus.c,v 1.13 2003/05/30 17:39:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_formstatus.c,v $

    $Log: htdrv_formstatus.c,v $
    Revision 1.13  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.12  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.11  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.10  2002/07/29 19:13:38  lkehresman
    * Added standard events to formstatus widget (why would you ever want them?!)
    * Made formstatus a named widget

    Revision 1.9  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.8  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.7  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.4  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2002/03/07 00:12:58  lkehresman
    Reworked form status widget to take advantage of the new icons

    Revision 1.1  2002/03/06 23:31:07  lkehresman
    Added form status widget.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTFS;


/* 
   htfsVerify - not written yet.
*/
int htfsVerify() {
   return 0;
}


/* 
   htfsRender - generate the HTML code for the page.
*/
int htfsRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   int x=-1,y=-1;
   int id;
   char sbuf[160];
   char sbuf2[160];
   char name[64];
   char* nptr;
   char* ptr;
   char* style;
   int w;

   /** Get an id for this. **/
   id = (HTFS.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;

   /** Get optional style **/
   if (objGetAttrValue(w_obj,"style",DATA_T_STRING,POD(&style)) != 0) style = "";
   if (!strcmp(style,"large") || !strcmp(style,"largeflat"))
       w = 90;
   else
       w = 13;

   /** Write named global **/
   if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,63);
   name[63] = 0;
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);
   htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#fs%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

   htrAddScriptInclude(s, "/sys/js/htdrv_formstatus.js", 0);

   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    %s = fs_init(%s.layers.fs%dmain,\"%s\");\n", nptr, parentname, id, style);
   /** HTML body <DIV> element for the layers. **/
   if (!strcmp(style,"large"))
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%dmain\"><IMG SRC=/sys/images/formstatL01.png></DIV>\n", id);
   else if (!strcmp(style,"largeflat"))
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%dmain\"><IMG SRC=/sys/images/formstatLF01.png></DIV>\n", id);
   else
       htrAddBodyItem_va(s,"   <DIV ID=\"fs%dmain\"><IMG SRC=/sys/images/formstat01.gif></DIV>\n", id);

   htrAddEventHandler(s,"document","MOUSEDOWN","fs","\n    if (ly.kind == 'formstatus') cn_activate(ly, 'MouseDown');\n\n"); 
   htrAddEventHandler(s,"document","MOUSEUP",  "fs","\n    if (ly.kind == 'formstatus') cn_activate(ly, 'MouseUp');\n\n"); 
   htrAddEventHandler(s,"document","MOUSEOVER","fs","\n    if (ly.kind == 'formstatus') cn_activate(ly, 'MouseOver');\n\n"); 
   htrAddEventHandler(s,"document","MOUSEOUT", "fs","\n    if (ly.kind == 'formstatus') cn_activate(ly, 'MouseOut');\n\n"); 
   htrAddEventHandler(s,"document","MOUSEMOVE","fs","\n    if (ly.kind == 'formstatus') cn_activate(ly, 'MouseMove');\n\n"); 
   
   snprintf(sbuf,160,"%s.document",nptr);
   snprintf(sbuf2,160,"%s",nptr);
   htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);
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
   drv->Verify = htfsVerify;
   htrAddSupport(drv, HTR_UA_NETSCAPE_47);

   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");

   /** Register. **/
   htrRegisterDriver(drv);

   HTFS.idcnt = 0;

   return 0;
}
