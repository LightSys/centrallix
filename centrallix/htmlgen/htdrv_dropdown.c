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

    $Id: htdrv_dropdown.c,v 1.6 2002/03/13 19:05:44 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
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
   char sbuf[HT_SBUF_SIZE];
   char bgstr[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char *ptr;
   int x,y,w;
   int id;
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

   if (objGetAttrValue(w_obj,"hilight",POD(&ptr)) == 0) {
	snprintf(hilight,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
   	strcpy(bgstr, "");
   }

   if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0) {
	snprintf(bgstr,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
   	strcpy(bgstr, "");
   }

   /** Ok, write the style header items. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"\t#dd%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:18; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   htrAddScriptGlobal(s, "dd_current", "null", 0);

   /** Get Value function **/
   htrAddScriptFunction(s, "dd_getvalue", "\n"
	"function dd_getvalue() {\n"
	"   return this.document.iLayer.value;"
	"}\n", 0);
   
   /** Set Value function **/
   htrAddScriptFunction(s, "dd_setvalue", "\n"
	"function dd_setvalue(v) {\n"
	"   for (i=0; i < this.values.length; i++) {\n"
	"      if (this.values[i] == v) {\n"
	"         dd_write_item(this.document.iLayer, this.labels[i], this.values[i], this);\n"
	"      }\n"
	"   }\n"
	"}\n", 0);
   
   /** Clear Value function **/
   htrAddScriptFunction(s, "dd_clearvalue", "\n"
	"function dd_clearvalue() {\n"
	"   dd_setvalue('');\n"
	"}\n", 0);
   
   /** Reset Value function **/
   htrAddScriptFunction(s, "dd_resetvalue", "\n"
	"function dd_resetvalue() {\n"
	"   dd_clearvalue();\n"
	"}\n", 0);
   
   /** Set Options function 
   *** The elements are passed in in the form of a hash where the
   *** key to the hash is the data value, and the value of the
   *** hash is the displayed label.
   **/
   htrAddScriptFunction(s, "dd_setoptions", "\n"
	"function dd_setoptions(ary) {\n"
	"}\n", 0);
   
   /** Enable function **/
   htrAddScriptFunction(s, "dd_enable", "\n"
	"function dd_enable() {\n"
	"}\n", 0);
   
   /** Read-Only function **/
   htrAddScriptFunction(s, "dd_readonly", "\n"
	"function dd_readonly() {\n"
	"}\n", 0);
   
   /** Disable function **/
   htrAddScriptFunction(s, "dd_disable", "\n"
	"function dd_disable() {\n"
	"}\n", 0);
   
   /** Disable function **/
   htrAddScriptFunction(s, "dd_write_item", "\n"
	"function dd_write_item(itemLayer, label, value, l) {\n"
	"   itemLayer.document.write('<table cellpadding=2 cellspacing=0 height=16 border=0><tr><td valign=middle>'+label+'</td></tr></table>');\n"
	"   itemLayer.label = label;\n"
	"   itemLayer.value = value;\n"
	"   itemLayer.document.close();\n"
	"}\n", 0);
   
   /** Adds an item to the dropdown layer l **/
   htrAddScriptFunction(s, "dd_additem", "\n"
	"function dd_additem(l, label, value) {\n"
	"   l.labels.push(label);\n"
	"   l.values.push(value);\n"
	"   tmpLayer = new Layer(1024, l.document.fullLayer);"
	"   tmpLayer.label = label;\n"
	"   tmpLayer.value = value;\n"
	"   tmpLayer.kind = 'dropdown';"
	"   tmpLayer.subkind = 'dropdownitem';"
	"   dd_write_item(tmpLayer, label, value, l);\n"
	"   tmpLayer.document.layer = tmpLayer;\n"
	"   tmpLayer.document.parentLayer = l;\n"
	"   tmpLayer.document.close();\n"
	"   tmpLayer.clip.width = l.defaultWidth-20;\n"
	"   tmpLayer.clip.top = 1;\n"
	"   tmpLayer.pageX = l.pageX+1;\n"
	"   tmpLayer.top = ((l.labels.length-1)*16) + 1;\n"
	"   tmpLayer.bgColor = l.bgColor;\n"
	"   tmpLayer.visibility = 'inherit';\n"
	"   l.itemLayers.push(tmpLayer);\n"
	"   l.document.fullLayer.clip.height += 16;\n"
	"}\n", 0);
   

   /** Form Status initializer **/
   htrAddScriptFunction(s, "dd_init", "\n"
	"function dd_init(l,w,color,hilight) {\n"
	"   l.document.layer = l;\n"
	"   l.width = w;\n"
	"   l.bgColor = color;\n"
	"   l.hilight = hilight;\n"
	"   l.kind = 'dropdown';\n"
	"   l.enabled = true;\n"
	"   l.labels = new Array();\n"
	"   l.values = new Array();\n"
	"   l.itemLayers = new Array();\n"
	"   l.defaultWidth = w;\n"
	"   l.document.iLayer = new Layer(1024);\n"
	"   l.document.iLayer.value = '';\n"
	"   l.document.iLayer.label = '';\n"
	"   l.document.iLayer.visibility = 'inherit';\n"
	"   l.document.iLayer.pageX = l.pageX+1;\n"
	"   l.document.iLayer.pageY = l.pageY;\n"
	"   l.document.iLayer.kind = 'dropdown';\n"
	"   l.document.iLayer.subkind = 'dropdownitem';\n"
	"   l.document.iLayer.bgColor = l.bgColor;\n"
	"   l.document.iLayer.clip.width = l.defaultWidth-20;\n"
	"   l.document.iLayer.clip.height = 16;\n"
	"   l.document.iLayer.clip.top = 1;\n"
	"   l.document.iLayer.document.layer = l;\n"
	"   l.document.fullLayer = new Layer(1024);\n"
	"   l.document.fullLayer.bgColor = l.bgColor;\n"
	"   l.document.fullLayer.clip.width = w-18;\n"
	"   l.document.fullLayer.clip.height = 4;\n"
	"   l.document.fullLayer.visibility = 'hidden';\n"
	"   l.document.fullLayer.pageX = l.pageX;\n"
	"   l.document.fullLayer.pageY = l.pageY + 18;\n"
	"   l.document.fullLayer.kind = 'dropdownItemlist'\n"
	"   l.document.fullLayer.document.layer = l.document.fullLayer;\n"
	"   for (i=0; i < l.document.images.length; i++) {\n"
	"      l.document.images[i].kind = 'dropdown';\n"
	"      l.document.images[i].enabled = true;\n"
	"      l.document.images[i].layer = l;\n"
	"   }\n"
	"   l.setvalue = dd_setvalue;\n"
	"   l.getvalue = dd_getvalue;\n"
	"   if (fm_current) fm_current.Register(l);\n"
	"}\n", 0);

   htrAddEventHandler(s, "document","MOUSEOVER", "dropdown", 
	"\n"
	"   targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && dd_current == targetLayer.document.parentLayer && targetLayer.subkind == 'dropdownitem') {\n"
	"      targetLayer.bgColor = dd_current.hilight;\n"
	"   }\n"
	"\n");

   htrAddEventHandler(s, "document","MOUSEOUT", "dropdown", 
	"\n"
	"   targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && targetLayer.subkind == 'dropdownitem') {\n"
	"      targetLayer.bgColor = dd_current.bgColor;\n"
	"   }\n"
	"\n");

   htrAddEventHandler(s, "document","MOUSEDOWN", "dropdown", 
	"\n"
	"   targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
	"   if (dd_current != null && targetLayer != dd_current) {\n"
	"      dd_current.document.fullLayer.visibility = 'hide';\n"
	"      if (targetLayer.subkind == 'dropdownitem') {\n"
	"         targetLayer.bgColor = dd_current.bgColor;\n"
	"         dd_write_item(dd_current.document.iLayer, targetLayer.label, targetLayer.value, dd_current);\n"
	"      }\n"
	"      dd_current = null;\n"
	"   } else if (targetLayer != null && targetLayer.kind == 'dropdown') {\n"
	"      if (targetLayer.enabled) {\n"
	"         if (targetLayer.document.fullLayer.visibility != 'hide') {\n"
	"            targetLayer.document.fullLayer.visibility = 'hide';\n"
	"         } else {\n"
	"            targetLayer.document.fullLayer.visibility = 'inherit';\n"
	"         }\n"
	"         dd_current = targetLayer;\n"
	"      }\n"
	"   }\n"
	"\n");

   /** Script initialization call. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    dd_init(%s.layers.dd%dmain, %d, '%s', '%s');\n", parentname, id, w, bgstr, hilight);
   htrAddScriptInit(s, sbuf);

   /* Read and initialize the dropdown items */
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy) {
      while((w_obj = objQueryFetch(qy, O_RDONLY))) {
         objGetAttrValue(w_obj,"outer_type",POD(&ptr));
         if (!strcmp(ptr,"widget/dropdownitem")) {
	    if (objGetAttrValue(w_obj,"label",POD(&ptr)) != 0) {
	       mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	       return -1;
   	    }
            memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	    snprintf(sbuf,HT_SBUF_SIZE,"    dd_additem(%s.layers.dd%dmain, '%s',", parentname, id, string);
	    htrAddScriptInit(s, sbuf);

	    if (objGetAttrValue(w_obj,"value",POD(&ptr)) != 0) {
	       mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	       return -1;
   	    }
            memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	    snprintf(sbuf,HT_SBUF_SIZE,"'%s');\n", string);
	    htrAddScriptInit(s, sbuf);
         }
         objClose(w_obj);
      }
   }
   objQueryClose(qy);

   /** HTML body <DIV> element for the layers. **/
   snprintf(sbuf,HT_SBUF_SIZE,"<DIV ID=\"dd%dmain\">\n", id);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"  <TABLE width=%d cellspacing=0 cellpadding=0 border=0 bgcolor=\"%s\"><TR><TD>\n",w,bgstr);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"  <TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w-19);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-20);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=16 width=1></TD>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD ALIGN=right></TD>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=16 width=1></TD></TR>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-20);
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"  </TABLE></TD><TD width=18><IMG SRC=/sys/images/ico15b.gif></TD></TR></TABLE>\n");
   htrAddBodyItem(s, sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"</DIV>\n");
   htrAddBodyItem(s, sbuf);

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
   strcpy(drv->Name,"DHTML Form Status Driver");
   strcpy(drv->WidgetName,"dropdown");
   drv->Render = htddRender;
   drv->Verify = htddVerify;
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
