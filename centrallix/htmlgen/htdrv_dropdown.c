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

    $Id: htdrv_dropdown.c,v 1.20 2002/07/09 14:09:04 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
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

   /** Get Value function **/
   htrAddScriptFunction(s, "dd_getvalue", "\n"
	"function dd_getvalue() {\n"
	"   return this.labelLayer.value;"
	"}\n", 0);
   
   /** Set Value function **/
   htrAddScriptFunction(s, "dd_setvalue", "\n"
	"function dd_setvalue(v) {\n"
	"   for (i=0; i < this.values.length; i++) {\n"
	"      if (this.values[i] == v) {\n"
	"         dd_write_item(this.labelLayer, this.labels[i], this.values[i], this);\n"
	"         return true;\n"
	"      }\n"
	"   }\n"
	"   return false;\n"
	"}\n", 0);
   
   /** Clear Value function **/
   htrAddScriptFunction(s, "dd_clearvalue", "\n"
	"function dd_clearvalue() {\n"
	"   dd_write_item(this.labelLayer, '', '');\n"
	"}\n", 0);
   
   /** Reset Value function **/
   htrAddScriptFunction(s, "dd_resetvalue", "\n"
	"function dd_resetvalue() {\n"
	"   this.clearvalue();\n"
	"}\n", 0);
   
   /** Enable function **/
   htrAddScriptFunction(s, "dd_enable", "\n"
	"function dd_enable() {\n"
	"   this.topLayer.document.images[8].src = '/sys/images/ico15b.gif';\n"
	"   this.enabled = 'full';\n"
	"}\n", 0);
   
   /** Read-Only function **/
   htrAddScriptFunction(s, "dd_readonly", "\n"
	"function dd_readonly() {\n"
	"   this.enabled = 'readonly';\n"
	"}\n", 0);
   
   /** Disable function **/
   htrAddScriptFunction(s, "dd_disable", "\n"
	"function dd_disable() {\n"
	"   this.topLayer.document.images[8].src = '/sys/images/ico15a.gif';\n"
	"   this.enabled = 'disabled';\n"
	"}\n", 0);

   /** Keyboard Handler function **/
   htrAddScriptFunction(s, "dd_keyhandler", "\n"
	"function dd_keyhandler(l,e,k) {\n"
	"    if (!dd_current) return;\n"
	"    if (dd_current.enabled != 'full') return 1;\n"
	"    if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122))\n"
	"        {\n"
	"        if (k < 97)\n"
	"            {\n"
	"            k_lower = k + 32;\n"
	"            k_upper = k;\n"
	"            k = k + 32;\n"
	"            }\n"
	"        else\n"
	"            {\n"
	"            k_lower = k;\n"
	"            k_upper = k - 32;\n"
	"            }\n"
	"        if (!dd_lastkey || dd_lastkey != k)\n"
	"            {\n"
	"            for (i=0; i < this.labels.length; i++)\n"
	"                {\n"
	"                if (this.labels[i].substring(0, 1) == String.fromCharCode(k_upper) ||\n"
	"                    this.labels[i].substring(0, 1) == String.fromCharCode(k_lower))\n"
	"                   {\n"
	"                   dd_hilight_item(this.itemLayers[i]);\n"
	"                   i=this.labels.length;\n"
	"                   }\n"
	"                }\n"
	"            }\n"
	"        else\n"
	"            {\n"
	"            var first = -1;\n"
	"            var last = -1;\n"
	"            var next = -1;\n"
	"            for (i=0; i < this.labels.length; i++)\n"
	"                {\n"
	"                if (this.labels[i].substring(0, 1) == String.fromCharCode(k_upper) ||\n"
	"                    this.labels[i].substring(0, 1) == String.fromCharCode(k_lower))\n"
	"                   {\n"
	"                   if (first < 0) { first = i; last = i; }\n"
	"                   for (var j=i; j < this.labels.length && \n"
	"                         (this.itemLayers[j].label.substring(0, 1) == String.fromCharCode(k_upper) ||\n"
	"                          this.itemLayers[j].label.substring(0, 1) == String.fromCharCode(k_lower)); j++)\n"
	"                       {\n"
	"                       if (this.itemLayers[j] == this.selectedItem)\n"
	"                           {\n"
	"                           next = j + 1;\n"
	"                           }\n"
	"                       last = j;\n"
	"                       }\n"
	"                   if (next <= last)\n"
	"                       dd_hilight_item(this.itemLayers[next]);\n"
	"                   else\n"
	"                       dd_hilight_item(this.itemLayers[first]);\n"
	"                   i=this.labels.length;\n"
	"                   }\n"
	"                }\n"
	"            }\n"
	"        }\n"
	"    else if (k == 13 && dd_lastkey != 13)\n"
	"        {\n"
	"        dd_select_item(this.selectedItem);\n"
	"        dd_unhilight_item(this.selectedItem);\n"
	"        }\n"
	"    dd_lastkey = k;\n"
	"    return false;\n"
	"}\n", 0);

   htrAddScriptFunction(s, "dd_hilight_item",
	"function dd_hilight_item(l) {\n"
	"   if (l.topLayer.selectedItem)\n"
	"      dd_unhilight_item(l.topLayer.selectedItem);\n"
	"   l.topLayer.selectedItem = l;\n"
	"   l.bgColor = dd_current.colorHilight;\n"
	"   range = l.topLayer.scrollPanelLayer.viewRangeBottom - l.topLayer.scrollPanelLayer.viewRangeTop;\n" 
	"   if (l.topLayer.scrollPanelLayer.viewRangeBottom < l.y+16)\n"
	"      {\n"
	"      l.topLayer.scrollPanelLayer.viewRangeTop = l.y - range + 16;\n"
	"      l.topLayer.scrollPanelLayer.viewRangeBottom = l.topLayer.scrollPanelLayer.viewRangeTop + range;\n"
	"      }\n"
	"   else if (l.topLayer.scrollPanelLayer.viewRangeTop > l.y)\n"
	"      {\n"
	"      l.topLayer.scrollPanelLayer.viewRangeTop = l.y;\n"
	"      l.topLayer.scrollPanelLayer.viewRangeBottom = l.y + range;\n"
	"      }\n"
	"   l.topLayer.scrollPanelLayer.y = -l.topLayer.scrollPanelLayer.viewRangeTop;\n"
	"}\n", 0);

   htrAddScriptFunction(s, "dd_unhilight_item",
	"function dd_unhilight_item(l) {\n"
	"   l.bgColor = l.topLayer.colorBack;\n"
	"   l.topLayer.selectedItem = null;\n"
	"}\n", 0);

   htrAddScriptFunction(s, "dd_select_item",
	"function dd_select_item(l) {\n"
	"   dd_current.document.images[8].src = '/sys/images/ico15b.gif';\n"
	"   dd_current.ddLayer.visibility = 'hide';\n"
	"   if (l.subkind == 'dropdown_item' && dd_current.enabled == 'full') {\n"
	"      dd_write_item(dd_current.labelLayer, l.label, l.value, dd_current);\n"
	"      if(dd_current.form)\n"
	"         dd_current.form.DataNotify(dd_current);\n"
	"   }\n"
	"   dd_current = null;\n"
	"   dd_lastkey = null;\n"
	"}\n", 0);
   
   /** Lose Keyboard Focus Handler function **/
   htrAddScriptFunction(s, "dd_losefocus", "\n"
	"function dd_losefocus() {\n"
	"    return true;\n"
	"}\n", 0);
   
   /** Write Item function **/
   htrAddScriptFunction(s, "dd_write_item", "\n"
	"function dd_write_item(itemLayer, label, value) {\n"
	"   itemLayer.document.write('<table cellpadding=2 cellspacing=0 height=16 border=0><tr><td valign=middle>'+label+'</td></tr></table>');\n"
	"   itemLayer.document.close();\n"
	"   itemLayer.label = label;\n"
	"   itemLayer.value = value;\n"
	"   itemLayer.kind = 'dropdown';\n"
	"}\n", 0);

   /** Adds an item to the dropdown layer l **/
   htrAddScriptFunction(s, "dd_additem", "\n"
	"function dd_additem(l, label, value) {\n"
	"   l.labels.push(label);\n"
	"   l.values.push(value);\n"
	"   var tmpLayer = new Layer(1024, l.scrollPanelLayer);\n"
	"   tmpLayer.kind = 'dropdown';\n"
	"   tmpLayer.subkind = 'dropdown_item';\n"
	"   tmpLayer.topLayer = l;\n"
	"   tmpLayer.document.layer = tmpLayer;\n"
	"   tmpLayer.bgColor = l.colorBack;\n"
	"   tmpLayer.label = label;\n"
	"   tmpLayer.value = value;\n"
	"   tmpLayer.x = 0;\n"
	"   tmpLayer.y = ((l.numItems) * 16);\n"
	"   tmpLayer.clip.width = l.clip.width - 20;\n"
	"   tmpLayer.clip.height = 16;\n"
	"   tmpLayer.visibility = 'inherit';\n"
	"   l.itemLayers.push(tmpLayer);\n"
	"   dd_write_item(tmpLayer, label, value);\n"
	"   l.numItems++;\n"
	"   l.scrollPanelLayer.clip.height = l.numItems * 16;\n" 
	"   if (l.numItems > l.numDispElements && !l.dispScrollLayer) {\n"
	"      l.dispScrollLayer = true;\n"
	"      l.ddLayer.clip.width = l.clip.width;\n"
	"      l.scrollLayer = new Layer(1024, l.ddLayer);\n"
	"      l.scrollLayer.bgColor = l.colorBack;\n"
	"      l.scrollLayer.visibility = 'inherit';\n"
	"      l.scrollLayer.kind = 'dropdown';\n"
	"      l.scrollLayer.subkind = 'dropdown_scroll';\n"
	"      l.scrollLayer.x = l.ddLayer.clip.width - 18;\n"
	"      l.scrollLayer.document.write('<table height='+(l.numDispElements*16)+' border=0 cellspacing=0 cellpadding=0 width=18>');\n"
	"      l.scrollLayer.document.write('<tr><td align=right><img src=/sys/images/ico13b.gif name=up></td></tr><tr><td align=right>');\n"
	"      l.scrollLayer.document.write('<img src=/sys/images/trans_1.gif height='+((l.numDispElements*16)-34)+' width=18 name=thumb>');\n"
	"      l.scrollLayer.document.write('</td></tr><tr><td align=right><img src=/sys/images/ico12b.gif name=down></td></tr></table>');\n"
	"      l.scrollLayer.document.close();\n"
	"      l.scrollLayer.document.images[0].topLayer = l;\n"
	"      l.scrollLayer.document.images[1].topLayer = l;\n"
	"      l.scrollLayer.document.images[2].topLayer = l;\n"
	"      l.scrollLayer.document.images[0].kind = 'dropdown';\n"
	"      l.scrollLayer.document.images[1].kind = 'dropdown';\n"
	"      l.scrollLayer.document.images[2].kind = 'dropdown';\n"
	"      l.scrollLayer.document.images[0].subkind = 'dropdown_scroll';\n"
	"      l.scrollLayer.document.images[1].subkind = 'dropdown_scroll';\n"
	"      l.scrollLayer.document.images[2].subkind = 'dropdown_scroll';\n"
	"   } else if (!l.dispScrollLayer) {\n"
	"      l.scrollPanelLayer.viewRangeBottom = (l.numItems * 16);\n"
	"      l.ddLayer.clip.height = ((l.numItems) * 16) + 2;\n"
	"      l.bg1Layer.clip.height = l.ddLayer.clip.height - 1;\n"
	"      l.bg2Layer.clip.height = l.bg1Layer.clip.height - 1;\n"
	"   }\n"
	"}\n", 0);
   

   /** Form Status initializer **/
   htrAddScriptFunction(s, "dd_init", "\n"
	"function dd_init(l, clr_b, clr_h, fn, disp) {\n"
	"   l.numItems = 0;\n"
	"   l.numDispElements = disp;\n"
	"   if (l.numDispElements < 4) l.numDispElements = 4;"
	"   l.fieldname = fn;\n"
	"   l.colorBack = clr_b;\n"
	"   l.colorHilight = clr_h;\n"
	"   l.enabled = 'full';\n"
	"   l.form = fm_current;\n"
	"   l.topLayer = l;\n"
	"   l.dispScrollLayer = false;\n"
	"   l.document.layer = l;\n"
	"   l.kind = 'dropdown';\n"
	"   l.subkind = 'dropdown_top';\n"
	"   l.itemLayers = new Array();\n"
	"   l.labels = new Array();\n"
	"   l.values = new Array();\n"
	"   for (var i=0; i < l.document.images.length; i++) {\n"
	"      l.document.images[i].kind = 'dropdown';\n"
	"      l.document.images[i].topLayer = l;\n"
	"      l.document.images[i].layer = l;\n"
	"   }\n"

	"   l.ddLayer = new Layer(1024);\n"
	"   l.ddLayer.layer = l.ddLayer;\n"
	"   l.ddLayer.kind = 'dropdown';\n"
	"   l.ddLayer.subkind = 'dropdown_bottom';\n"
	"   l.ddLayer.topLayer = l;\n"
	"   l.ddLayer.document.layer = l.ddLayer;\n"
	"   l.ddLayer.clip.width = l.clip.width - 18;\n"
	"   l.ddLayer.bgColor = '#ffffff';\n"

	"   l.labelLayer = new Layer(1024, l);\n"
	"   l.labelLayer.document.layer = l;\n"
	"   l.labelLayer.kind = 'dropdown';\n"
	"   l.labelLayer.topLayer = l;\n"
	"   l.labelLayer.mainlayer = l;\n"
	"   l.labelLayer.clip.width = l.clip.width - 20;\n"
	"   l.labelLayer.clip.height = 16;\n"
	"   l.labelLayer.x = 1;\n"
	"   l.labelLayer.y = 1;\n"
	"   l.labelLayer.visibility = 'inherit';\n"

	"   l.bg1Layer = new Layer(1024, l.ddLayer);\n"
	"   l.bg1Layer.layer = l.bg1Layer;\n"
	"   l.bg1Layer.kind = 'dropdown';\n"
	"   l.bg1Layer.bgColor = '#888888';\n"
	"   l.bg1Layer.visibility = 'inherit';\n"
	"   l.bg1Layer.top = 1;\n"
	"   l.bg1Layer.x = 1;\n"
	"   l.bg1Layer.clip.width = l.ddLayer.clip.width;\n"

	"   l.bg2Layer = new Layer(1024, l.bg1Layer);\n"
	"   l.bg2Layer.visibility = 'inherit';\n"
	"   l.bg2Layer.bgColor = '#cc0000';\n"
	"   l.bg2Layer.clip.left = 0;\n"
	"   l.bg2Layer.clip.top = 0;\n"
	"   l.bg2Layer.clip.width = l.bg1Layer.clip.width - 2;\n"

	"   l.scrollPanelLayer = new Layer(1024, l.bg2Layer);\n"
	"   l.scrollPanelLayer.layer = l.scrollPanelLayer;\n"
	"   l.scrollPanelLayer.topLayer = l;\n"
	"   l.scrollPanelLayer.bgColor = '#cc0000';\n"
	"   l.scrollPanelLayer.kind = 'dropdown';\n"
	"   l.scrollPanelLayer.visibility = 'inherit';\n"
	"   l.scrollPanelLayer.clip.left = 0;\n"
	"   l.scrollPanelLayer.clip.top = 0;\n"
	"   l.scrollPanelLayer.clip.width = l.bg2Layer.clip.width;\n"
	"   l.scrollPanelLayer.viewRangeTop = 0;\n"
	"   l.scrollPanelLayer.viewRangeBottom = 0;\n"

	"   l.setvalue = dd_setvalue;\n"
	"   l.getvalue = dd_getvalue;\n"
	"   l.enable = dd_enable;\n"
	"   l.readonly = dd_readonly;\n"
	"   l.disable = dd_disable;\n"
	"   l.clearvalue = dd_clearvalue;\n"
	"   l.resetvalue = dd_resetvalue;\n"
	"   l.keyhandler = dd_keyhandler;\n"
	"   l.losefocushandler = dd_losefocus;\n"
	"   pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dropdown', 'dropdown', 1);\n"
	"   if (fm_current) fm_current.Register(l);\n"
	"}\n", 0);

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
