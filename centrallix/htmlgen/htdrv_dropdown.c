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
/* Module:      htdrv_dropdown.c                                        */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a drop down list                 */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_dropdown.c,v 1.21 2002/07/16 17:52:00 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
    Revision 1.21  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.20  2002/07/09 14:09:04  lkehresman
    Added first revision of the datetime widget.  No form interatction, and no
    time setting functionality, only date.  This has been on my laptop for a
    while and I wanted to get it into CVS for backup purposes.  More functionality
    to come soon.

    Revision 1.19  2002/06/26 00:46:17  lkehresman
    * Added keyhandler to dropdown (you can type the first letter of options
      to jump to that option and bounce around)
    * Improved scrolling by adding all items to a layer and created universal
      scrolling methods
    * Fixed some GUI bugs

    Revision 1.18  2002/06/24 15:33:09  lkehresman
    Improvements and bugfixes to the dropdown widget:
     * Uses pg_addarea() function so mouseovers work
     * Handles mouse clicks better for showing/hiding the dropdown

    Revision 1.17  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.16  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.15  2002/06/06 17:12:20  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.14  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.13  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.12  2002/05/03 03:42:16  gbeeley
    Added objClose to close objects returned from fetches in the dropdown
    list SQL query.

    Revision 1.11  2002/04/29 19:23:13  lkehresman
    Fixed scrolling on dropdowns.  It now works properly

    Revision 1.9  2002/03/16 04:30:45  lkehresman
    * Added scrollbar to dropdown list (only arrows work currently, not drag box)
    * Added fieldname property

    Revision 1.8  2002/03/14 22:02:58  jorupp
     * bugfixes, dropdown doesn't throw errors when being cleared/reset

    Revision 1.7  2002/03/14 15:48:43  lkehresman
    * Added enable, disable, readonly functions
    * Improved GUI quite a bit.. looks purdy
    * Added/improved forms interaction functions (setvalue, getvalue...)

    Revision 1.6  2002/03/13 19:05:44  lkehresman
    Beautified the dropdown widget
    added basic form interaction

    Revision 1.5  2002/03/13 02:50:38  lkehresman
    Dropdown now works!  Everything except the form functions.. that is to come
    shortly.

    Revision 1.4  2002/03/13 00:38:23  lkehresman
    Layer improvements for dropdown widget.  You can now click anywhere on the
    layer and get the dropdown to appear.

    Revision 1.3  2002/03/11 14:10:16  lkehresman
    Added basic functionality for the dropdown widget.

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.1  2002/03/07 00:20:00  lkehresman
    Added shell for the drop down list form widget.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTDD;


/* 
   htddVerify - not written yet.
*/
int htddVerify() {
   return 0;
}


/* 
   htddRender - generate the HTML code for the page.
*/
int htddRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char bgstr[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char fieldname[30];
   char *ptr;
   char *sql;
   char *str;
   char *attr;
   int type, rval;
   int x,y,w;
   int id;
   int num_disp;
   ObjData od;
   pObject qy_obj;
   pObjQuery qy;

   /** Get an id for this. **/
   id = (HTDD.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	return -1;
   }

   if (objGetAttrValue(w_obj,"numdisplay",POD(&num_disp)) != 0) num_disp=3;

   if (objGetAttrValue(w_obj,"hilight",POD(&ptr)) == 0) {
	snprintf(hilight,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'hilight' property");
	return -1;
   }

   if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0) {
	snprintf(bgstr,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'bgcolor' property");
	return -1;
   }

   if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) {
	strncpy(fieldname,ptr,30);
   } else {
	fieldname[0]='\0';
   }

   /** Ok, write the style header items. **/
   htrAddHeaderItem_va(s,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem_va(s,"\t#dd%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:18; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
   htrAddHeaderItem_va(s,"    </STYLE>\n");

   htrAddScriptGlobal(s, "dd_current", "null", 0);
   htrAddScriptGlobal(s, "dd_lastkey", "null", 0);

   htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js", 0);

   htrAddEventHandler(s, "document","MOUSEOVER", "dropdown", 
	"\n"
	"   var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && dd_current == targetLayer.topLayer && targetLayer.subkind == 'dropdown_item' && dd_current.enabled == 'full') {\n"
	"      dd_hilight_item(targetLayer);\n"
	"   }\n"
	"\n");

   htrAddEventHandler(s, "document","MOUSEUP", "dropdown", 
	"\n"
	"   var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && targetLayer.subkind == 'dropdown_scroll' && dd_current.enabled == 'full') {\n" 
	"      if (targetLayer.name == 'up') {\n"
	"         targetLayer.src = '/sys/images/ico13b.gif';\n"
	"      } else if (targetLayer.name == 'down') {\n"
	"         targetLayer.src = '/sys/images/ico12b.gif';\n"
	"      }\n"
	"   }\n"
	"\n");

   htrAddEventHandler(s, "document","MOUSEDOWN", "dropdown", 
	"\n"
	"   var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && (targetLayer != dd_current || targetLayer.kind != 'dropdown')) {\n"
	"      if (targetLayer.subkind == 'dropdown_scroll' || targetLayer.subkind == 'dropdown_thumb') {\n"
	"          if (targetLayer.name == 'up' && dd_current.topLayer.scrollPanelLayer.viewRangeTop > 0) {\n"
	"              targetLayer.src = '/sys/images/ico13c.gif';\n"
	"              dd_current.topLayer.scrollPanelLayer.y += 8;\n"
	"              dd_current.topLayer.scrollPanelLayer.viewRangeTop -= 8;\n"
	"              dd_current.topLayer.scrollPanelLayer.viewRangeBottom -= 8;\n"
	"          } else if (targetLayer.name == 'down' && dd_current.topLayer.scrollPanelLayer.viewRangeBottom < dd_current.topLayer.scrollPanelLayer.clip.height) {\n"
	"              targetLayer.src = '/sys/images/ico12c.gif';\n"
	"              dd_current.topLayer.scrollPanelLayer.y -= 8;\n"
	"              dd_current.topLayer.scrollPanelLayer.viewRangeTop += 8;\n"
	"              dd_current.topLayer.scrollPanelLayer.viewRangeBottom += 8;\n"
	"          }\n"
	"      } else {\n"
	"          dd_select_item(targetLayer);\n"
	"      }\n"
	"   } else if (targetLayer != null && targetLayer.kind == 'dropdown') {\n"
	"      targetLayer.ddLayer.zIndex = 1000000;\n"
	"      targetLayer.ddLayer.pageX = targetLayer.pageX;\n"
	"      targetLayer.ddLayer.pageY = targetLayer.pageY + 18;\n"
	"      targetLayer.document.images[8].src = '/sys/images/ico15c.gif';\n"
	"      dd_current = targetLayer.topLayer;\n"
	"      dd_lastkey = null;\n"
	"      if(dd_current.form)\n"
	"          dd_current.form.FocusNotify(dd_current);\n"
	"      dd_current.ddLayer.visibility = 'inherit';\n"
	"   }\n"
	"\n");

   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    dd_init(%s.layers.dd%dmain, '%s', '%s', '%s', %d);\n", parentname, id, bgstr, hilight, fieldname, num_disp);

   /* Read and initialize the dropdown items */
   if (objGetAttrValue(w_obj,"sql",POD(&sql)) == 0) {
       if ((qy = objMultiQuery(w_obj->Session, sql))) {
	  while ((qy_obj = objQueryFetch(qy, O_RDONLY))) {
	     // Label
	     attr = objGetFirstAttr(qy_obj);
	     if (!attr) {
	        mssError(1, "HTDD", "SQL query must have two attributes: label and value.");
	        return -1;
	     }
	     type = objGetAttrType(qy_obj, attr);
	     rval = objGetAttrValue(qy_obj, attr, &od);
	     if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
	        str = objDataToStringTmp(type, (void*)(&od), 0);
	     } else {
	        str = objDataToStringTmp(type, (void*)(od.String), 0);
	     }
	     htrAddScriptInit_va(s,"    dd_additem(%s.layers.dd%dmain, '%s',", parentname, id, str);
	     // Value
	     attr = objGetNextAttr(qy_obj);
	     if (!attr) {
	        mssError(1, "HTDD", "SQL query must have two attributes: label and value.");
	        return -1;
	     }

	     type = objGetAttrType(qy_obj, attr);
	     rval = objGetAttrValue(qy_obj, attr, &od);
	     if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
	        str = objDataToStringTmp(type, (void*)(&od), 0);
	     } else {
	        str = objDataToStringTmp(type, (void*)(od.String), 0);
	     }
	     htrAddScriptInit_va(s," '%s');\n", str);
	     objClose(qy_obj);
	  }
	  objQueryClose(qy);
       }
   } else {
       if ((qy = objOpenQuery(w_obj,"",NULL,NULL,NULL))) {
	  while((w_obj = objQueryFetch(qy, O_RDONLY))) {
	     objGetAttrValue(w_obj,"outer_type",POD(&ptr));
	     if (!strcmp(ptr,"widget/dropdownitem")) {
	        if (objGetAttrValue(w_obj,"label",POD(&ptr)) != 0) {
	           mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	           return -1;
	           }
	        memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	        htrAddScriptInit_va(s,"    dd_additem(%s.layers.dd%dmain, '%s',", parentname, id, string);
    
	        if (objGetAttrValue(w_obj,"value",POD(&ptr)) != 0) {
	           mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	           return -1;
	           }
	        memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	        htrAddScriptInit_va(s,"'%s');\n", string);
	     }
	     objClose(w_obj);
	  }
	  objQueryClose(qy);
       }
   }

   /** HTML body <DIV> element for the layers. **/
   htrAddBodyItem_va(s,"<DIV ID=\"dd%dmain\">\n", id);
   htrAddBodyItem_va(s,"  <TABLE width=%d cellspacing=0 cellpadding=0 border=0 bgcolor=\"%s\"><TR><TD>\n",w,bgstr);
   htrAddBodyItem_va(s,"  <TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w-19);
   htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>\n");
   htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-20);
   htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>\n");
   htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=16 width=1></TD>\n");
   htrAddBodyItem_va(s,"       <TD ALIGN=right></TD>\n");
   htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=16 width=1></TD></TR>\n");
   htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>\n");
   htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-20);
   htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>\n");
   htrAddBodyItem_va(s,"  </TABLE></TD><TD width=18><IMG SRC=/sys/images/ico15b.gif></TD></TR></TABLE>\n");
   htrAddBodyItem_va(s,"</DIV>\n");

   return 0;
}


/* 
   htddInitialize - register with the ht_render module.
*/
int htddInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Drop Down Widget Driver");
   strcpy(drv->WidgetName,"dropdown");
   drv->Render = htddRender;
   drv->Verify = htddVerify;
   strcpy(drv->Target, "Netscape47x:default");
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

   HTDD.idcnt = 0;

   return 0;
}
