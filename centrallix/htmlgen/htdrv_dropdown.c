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

    $Id: htdrv_dropdown.c,v 1.26 2002/07/25 15:06:47 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
    Revision 1.26  2002/07/25 15:06:47  lkehresman
    * Fixed bug where dropdown wasn't going away
    * Added enable/disable/readonly support

    Revision 1.25  2002/07/24 20:33:15  lkehresman
    Complete reworking of the dropdown widget.  Much more functionality
    (including, FINALLY, a working scrollbar).  Better interface.  More
    bugs (still working out some of the kinks).  This also has a shell
    for client-side dynamic population of the dropdown, which was the
    main reason for the restructure/rewrite.

    Revision 1.24  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.23  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.22  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

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
   int type, rval, mode, count=0;
   int x,y,w,h;
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
   if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) h=20;
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
    htrAddStylesheetItem_va(s,"\t#dd%dbtn { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:18; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
    htrAddStylesheetItem_va(s,"\t#dd%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:1024; HEIGHT:%d; Z-INDEX:%d; }\n",id,h-2,z+1);
    htrAddStylesheetItem_va(s,"\t#dd%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:1; TOP:1; WIDTH:1024; HEIGHT:%d; Z-INDEX:%d; }\n",id,h-2,z+1);

    htrAddScriptGlobal(s, "dd_current", "null", 0);
    htrAddScriptGlobal(s, "dd_lastkey", "null", 0);
    htrAddScriptGlobal(s, "dd_target_img", "null", 0);
    htrAddScriptGlobal(s, "dd_thum_y","0",0);
    htrAddScriptGlobal(s, "dd_timeout","null",0);
    htrAddScriptGlobal(s, "dd_click_x","0",0);
    htrAddScriptGlobal(s, "dd_click_y","0",0);
    htrAddScriptGlobal(s, "dd_incr","0",0);

    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js", 0);

    htrAddEventHandler(s, "document","MOUSEMOVE", "dd", 
	"\n"
	"    ti=dd_target_img;\n"
	"    if (ly.kind == 'dd_sc' && ti != null && ti.name == 't' && dd_current && dd_current.enabled!='disabled')\n"
	"        {\n"
	"        var pl=ti.mainlayer.PaneLayer;\n"
	"        v=pl.clip.height-(3*18)-4;\n"
	"        new_y=dd_thum_y+(e.pageY-dd_click_y)\n"
	"        if (new_y > pl.pageY+20+v) new_y=pl.pageY+20+v;\n"
	"        if (new_y < pl.pageY+20) new_y=pl.pageY+20;\n"
	"        ti.thum.pageY=new_y;\n"
	"        h=dd_current.PaneLayer.h;\n"
	"        d=h-pl.clip.height-4;\n"
	"        if (d<0) d=0;\n"
	"        dd_incr = (((ti.thum.y-22)/(v-4))*-d)-dd_current.PaneLayer.ScrLayer.y;\n"
	"        dd_scroll(0);\n"
	"        return false;\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEOVER", "dd", 
	"\n"
	"    if (ly.kind == 'dd_itm' && dd_current && dd_current.enabled=='full')\n"
	"        {\n"
	"        dd_lastkey = null;\n"
	"        dd_hilight_item(dd_current, ly.index);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEUP", "dd", 
	"\n"
	"    if (dd_timeout != null)\n"
	"        {\n"
	"        clearTimeout(dd_timeout);\n"
	"        dd_timeout = null;\n"
	"        dd_incr = 0;\n"
	"        }\n"
	"    if (dd_target_img != null)\n"
	"        {\n"
	"        if (ly.name == 'u') ly.src = '/sys/images/ico13b.gif';\n"
	"        else if (ly.name == 'd') ly.src = '/sys/images/ico12b.gif';\n"
	"        dd_target_img = null;\n"
	"        }\n"
	"    if (ly.kind == 'dd' && ly.enabled != 'disabled')\n"
	"        {\n"
	"        dd_toggle(ly);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEDOWN", "dd", 
	"\n"
	"    dd_target_img = e.target;\n"
	"    if (ly.kind == 'dd' && ly.enabled != 'disabled')\n"
	"        {\n"
	"        if (dd_current)\n"
	"            {\n"
	"            dd_current.PaneLayer.visibility = 'hide';\n"
	"            dd_current = null;\n"
	"            }\n"
	"        else\n"
	"            {\n"
	"            ly.PaneLayer.pageX = ly.pageX;\n"
	"            ly.PaneLayer.pageY = ly.pageY+20;\n"
	"            ly.PaneLayer.visibility = 'inherit';\n"
	"            if(ly.form)\n"
	"                ly.form.FocusNotify(ly);\n"
	"            dd_current = ly;\n"
	"            }\n"
	"            dd_toggle(ly);\n"
	"        }\n"
	"    else if (ly.kind == 'dd_itm' && dd_current && dd_current.enabled == 'full')\n"
	"        {\n"
	"        dd_select_item(dd_current, ly.index);\n"
	"        }\n"
	"    else if (ly.kind == 'dd_sc')\n"
	"        {\n"
	"        switch(ly.name)\n"
	"            {\n"
	"            case 'u':\n"
	"                dd_timeout = setTimeout(dd_scroll_tm,300);\n"
	"                ly.src = '/sys/images/ico13c.gif';\n"
	"                dd_incr = 8;\n"
	"                dd_scroll();\n"
	"                break;\n"
	"            case 'd':\n"
	"                dd_timeout = setTimeout(dd_scroll_tm,300);\n"
	"                ly.src = '/sys/images/ico12c.gif';\n"
	"                dd_incr = -8;\n"
	"                dd_scroll();\n"
	"                break;\n"
	"            case 't':\n"
	"                dd_click_x = e.pageX;\n"
	"                dd_click_y = e.pageY;\n"
	"                dd_thum_y = dd_target_img.thum.pageY;\n"
	"                break;\n"
	"            }\n"
	"        }\n"
	"    if (dd_current && dd_current != ly && dd_current.PaneLayer != ly && (!ly.mainlayer || dd_current != ly.mainlayer))\n"
	"        {\n"
	"        dd_current.PaneLayer.visibility = 'hide';\n"
	"        dd_current = null;\n"
	"        }\n"
	"\n");

    /** Get the mode (default to 1, dynamicpage) **/
    mode = 1;
    if (objGetAttrValue(w_obj,"mode",POD(&ptr)) == 0) {
	if (!strcmp(ptr,"static")) mode = 0;
	else if (!strcmp(ptr,"dynamic_server")) mode = 1;
	else if (!strcmp(ptr,"dynamic")) mode = 2;
	else if (!strcmp(ptr,"dynamic_client")) mode = 2;
	else {
	    mssError(1,"HTDD","Dropdown widget has not specified a valid mode.");
	    return -1;
	}
    }

    sql = 0;
    if (objGetAttrValue(w_obj,"sql",POD(&sql)) != 0 && mode != 0) {
	mssError(1, "HTDD", "SQL parameter was not specified for dropdown widget");
	return -1;
    }

    /* Read and initialize the dropdown items */
    if (mode == 1) {
	if ((qy = objMultiQuery(w_obj->Session, sql))) {
	    count=0;
	    htrAddScriptInit_va(s,"    dd_add_items(%s.layers.dd%dbtn, Array(",parentname,id);
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
		if (count) htrAddScriptInit(s,",");
		htrAddScriptInit_va(s,"Array('%s',",str);
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
		htrAddScriptInit_va(s,"'%s')", str);
		objClose(qy_obj);
		count++;
	    }
	    htrAddScriptInit_va(s,"));\n", str);
	    objQueryClose(qy);
	}
    } else if (mode == 0) {
	if ((qy = objOpenQuery(w_obj,"",NULL,NULL,NULL))) {
	    count=0;
	    htrAddScriptInit_va(s, "    dd_add_items(%s.layers.dd%dbtn, Array(", parentname, id);
	    while((w_obj = objQueryFetch(qy, O_RDONLY))) {
		objGetAttrValue(w_obj,"outer_type",POD(&ptr));
		if (!strcmp(ptr,"widget/dropdownitem")) {
		    if (objGetAttrValue(w_obj,"label",POD(&ptr)) != 0) {
			mssError(1,"HTDD","Drop Down widget must have a 'width' property");
			return -1;
		    }
		    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
		    if (count) htrAddScriptInit_va(s, ",");
		    htrAddScriptInit_va(s,"Array('%s',", string);
    
		    if (objGetAttrValue(w_obj,"value",POD(&ptr)) != 0) {
			mssError(1,"HTDD","Drop Down widget must have a 'width' property");
			return -1;
		    }
		    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
		    htrAddScriptInit_va(s,"'%s')", string);
	        }
		count++;
		objClose(w_obj);
	    }
	    htrAddScriptInit_va(s, "));\n");
	    objQueryClose(qy);
	}
    }

    /** Script initialization call. **/
    htrAddScriptInit_va(s,"    dd_init(%s.layers.dd%dbtn,%s.layers.dd%dbtn.document.layers.dd%dcon1,%s.layers.dd%dbtn.document.layers.dd%dcon2,'%s','%s','%s',%d,%d,'%s',%d,%d);\n", parentname, id, parentname, id, id, parentname, id, id, bgstr, hilight, fieldname, num_disp, mode, sql, w, h);

    /** HTML body <DIV> element for the layers. **/
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dbtn\"><BODY bgcolor=\"%s\">\n", id,bgstr);
    htrAddBodyItem_va(s,"<TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
    htrAddBodyItem_va(s,"       <TD align=right valign=middle><IMG SRC=/sys/images/ico15b.gif></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"</TABLE>\n");
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dcon1\"></DIV>\n",id);
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dcon2\"></DIV>\n",id);
    htrAddBodyItem_va(s,"</BODY></DIV>\n");
    
    return 0;
}


/* 
   htddInitialize - register with the ht_render module.
*/
int htddInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Drop Down Widget Driver");
   strcpy(drv->WidgetName,"dropdown");
   drv->Render = htddRender;
   drv->Verify = htddVerify;
   strcpy(drv->Target, "Netscape47x:default");

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
