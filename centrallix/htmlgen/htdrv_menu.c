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
/* Module:      htdrv_menu.c                                            */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a drop down list                 */
/************************************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTMN;


/* 
   htmnVerify - not written yet.
*/
int htmnVerify() {
   return 0;
}


/* 
   htmenuRender - generate the HTML code for the menu widget.
*/
int htmenuRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char bgstr[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char name[64];
   char *ptr, *nptr;
   int flag=0;
   int x,y,w,h;
   int id;
   pObjQuery qy;
   XString xs;

   if(!s->Capabilities.Dom0NS)
       {
       mssError(1,"HTMENU","Netscape DOM support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTMN.idcnt++);

   /** Get x,y,height,& width of this object **/
   if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0) h=20;
   if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0) {
	mssError(1,"HTMN","Menu widget must have a 'width' property");
	return -1;
   }


   if (objGetAttrValue(w_obj,"hilight",DATA_T_STRING,POD(&ptr)) == 0) {
	snprintf(hilight,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTMN","Menu widget must have a 'hilight' property");
	return -1;
   }

   if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	snprintf(bgstr,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTMN","Menu widget must have a 'bgcolor' property");
	return -1;
   }


    /** Get name **/
    if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
    memccpy(name,ptr,0,63);
    name[63] = 0;
    nptr = (char*)nmMalloc(strlen(name)+1);
    strcpy(nptr,name);

    /** Ok, write the style header items. **/
    htrAddStylesheetItem_va(s,"\t#mn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:18; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);


    htrAddScriptGlobal(s, "mn_current", "null", 0);
    htrAddScriptGlobal(s, "mn_lastkey", "null", 0);
    htrAddScriptGlobal(s, "mn_target_img", "null", 0);
    htrAddScriptGlobal(s, "mn_thum_y","0",0);
    htrAddScriptGlobal(s, "mn_timeout","null",0);
    htrAddScriptGlobal(s, "mn_click_x","0",0);
    htrAddScriptGlobal(s, "mn_click_y","0",0);
    htrAddScriptGlobal(s, "mn_incr","0",0);
    htrAddScriptGlobal(s, "mn_cur_mainlayer","null",0);
    htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
    htrAddScriptInclude(s, "/sys/js/htdrv_menu.js", 0);

    htrAddEventHandler(s, "document","MOUSEMOVE", "mn", 
	"\n"
	// I think has to do with scrolling...
	"    ti=mn_target_img;\n"
	"    if (ti != null && ti.name == 't' && mn_current && mn_current.enabled!='disabled')\n"
	"        {\n"
	"        var pl=ti.mainlayer.PaneLayer;\n"
	"        v=pl.clip.height-(3*18)-4;\n"
	"        new_y=mn_thum_y+(e.pageY-mn_click_y)\n"
	"        if (new_y > pl.pageY+20+v) new_y=pl.pageY+20+v;\n"
	"        if (new_y < pl.pageY+20) new_y=pl.pageY+20;\n"
	"        ti.thum.pageY=new_y;\n"
	"        h=mn_current.PaneLayer.h;\n"
	"        d=h-pl.clip.height+4;\n"
	"        if (d<0) d=0;\n"
	"        mn_incr = (((ti.thum.y-22)/(v-4))*-d)-mn_current.PaneLayer.ScrLayer.y;\n"
	"        mn_scroll(0);\n"
	"        return false;\n"
	"        }\n"
	"    if (ly.mainlayer && ly.mainlayer.kind != 'mn')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseMove');\n"
	"        mn_cur_mainlayer = null;\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEOVER", "mn", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'mn')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseOver');\n"
	"        mn_cur_mainlayer = ly.mainlayer;\n"
	"        }\n"
	"    if (ly.kind == 'mn_itm')\n" 
	"        {\n"
	"        mn_lastkey = null;\n"
	"        mn_toggle_item(mn_current, ly.index);\n"
	"        }\n"
	"    if (ly.kind == 'mn_itm' && mn_current && mn_current.enabled=='full')\n"
	"        {\n"
	"        mn_lastkey = null;\n"
	"        mn_hilight_item(mn_current, ly.index);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEUP", "mn", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'mn')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseUp');\n"
	"        }\n"
	"    if (mn_timeout != null)\n"
	"        {\n"
	"        clearTimeout(mn_timeout);\n"
	"        mn_timeout = null;\n"
	"        mn_incr = 0;\n"
	"        }\n"
	"    if (mn_target_img != null)\n"
	"        {\n"
	"        if (mn_target_img.name != 'b' && mn_target_img.src && mn_target_img.kind.substr(0,2) == 'mn')\n"
	"            mn_target_img.src = htutil_subst_last(mn_target_img.src,\"b.gif\");\n"
	"        mn_target_img = null;\n"
	"        }\n"
	"    if (ly.kind == 'mn' && ly.enabled != 'disabled')\n"
	"        {\n"
	"        mn_toggle(ly);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEDOWN", "mn", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'mn') cn_activate(ly.mainlayer, 'MouseDown');\n"
	"    if (ly.kind == 'mn_itm')\n"
	"        {\n"
	"        mn_show_menu(ly.mainlayer,ly.index);\n"
	"        }\n"
	"    mn_target_img = e.target;\n"
	"    if (ly.kind == 'mn' && ly.enabled != 'disabled')\n"
	"        {\n"
	"        if (mn_current)\n"
	"            {\n"
	"            mn_current.PaneLayer.visibility = 'hide';\n"
	"            mn_current = null;\n"
	"            }\n"
	"        else\n"
	"            {\n"
	"            ly.PaneLayer.pageX = ly.pageX;\n"
	"            ly.PaneLayer.pageY = ly.pageY+20;\n"
	"            ly.PaneLayer.visibility = 'inherit';\n"
	"            if(ly.form)\n"
	"                ly.form.FocusNotify(ly);\n"
	"            mn_current = ly;\n"
	"            }\n"
	"            mn_toggle(ly);\n"
	"        }\n"
	"    else if (ly.kind == 'mn_itm' && mn_current && mn_current.enabled == 'full')\n"
	"        {\n"
	"        mn_select_item(mn_current, ly.index);\n"
	"        }\n"
	"    else if (ly.kind == 'mn_sc')\n"
	"        {\n"
	"        switch(ly.name)\n"
	"            {\n"
	"            case 'u':\n"
	"                ly.src = '/sys/images/ico13c.gif';\n"
	"                mn_incr = 8;\n"
	"                mn_scroll();\n"
	"                mn_timeout = setTimeout(mn_scroll_tm,300);\n"
	"                break;\n"
	"            case 'd':\n"
	"                ly.src = '/sys/images/ico12c.gif';\n"
	"                mn_incr = -8;\n"
	"                mn_scroll();\n"
	"                mn_timeout = setTimeout(mn_scroll_tm,300);\n"
	"                break;\n"
	"            case 'b':\n"
	"                mn_incr = mn_target_img.height+36;\n"
	"                if (e.pageY > mn_target_img.thum.pageY+9) mn_incr = -mn_incr;\n"
	"                mn_scroll();\n"
	"                mn_timeout = setTimeout(mn_scroll_tm,300);\n"
	"                break;\n"
	"            case 't':\n"
	"                mn_click_x = e.pageX;\n"
	"                mn_click_y = e.pageY;\n"
	"                mn_thum_y = mn_target_img.thum.pageY;\n"
	"                break;\n"
	"            }\n"
	"        }\n"
	"    if (mn_current && mn_current != ly && mn_current.PaneLayer != ly && (!ly.mainlayer || mn_current != ly.mainlayer))\n"
	"        {\n"
	"        mn_current.PaneLayer.visibility = 'hide';\n"
	"        mn_current = null;\n"
	"        }\n"
	"\n");

    /** Get the mode (default to 1, dynamicpage) **/

    /** Script initialization call. **/

    htrAddScriptInit_va(s,"    %s = mn_init(%s.layers.mn%dmain,'%s','%s',%d,%d);\n", nptr, parentname, id, bgstr, hilight, w, h);

    /** HTML body <DIV> element for the layers. **/
    htrAddBodyItem_va(s,"<DIV ID=\"mn%dmain\"><BODY bgcolor=\"%s\">\n", id,bgstr);
    htrAddBodyItem_va(s,"<TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
    htrAddBodyItem_va(s,"       <TD></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"</TABLE>\n");
    htrAddBodyItem_va(s,"</BODY></DIV>\n");
    
    /* Read and initialize the menu items */

    //Note: w_obj here needs to be a sub object variable...
    if ((qy = objOpenQuery(w_obj,"",NULL,NULL,NULL))) {
	flag=0;
	while((w_obj = objQueryFetch(qy, O_RDONLY))) {
	   objGetAttrValue(w_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	   if (!strcmp(ptr,"widget/menuitem")) {
		    if (objGetAttrValue(w_obj,"label",DATA_T_STRING,POD(&ptr)) != 0) {
		      mssError(1,"HTMN","Menu Item  widget must have a 'label' property");
		      return -1;
		    }
		    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
		    if (flag) { //create mn_add_top_layer function call...
		        xsConcatPrintf(&xs, ",");
		    } else {
		        xsInit(&xs);
		        xsConcatPrintf(&xs, "    mn_add_top_layer_items(%s.layers.mn%dmain, Array(", parentname, id);
		        flag=1;
		    }
		    xsConcatPrintf(&xs,"Array('%s',", string); //fill in the menu items parameters for the function...
    
		    if (objGetAttrValue(w_obj,"value",DATA_T_STRING,POD(&ptr)) != 0) {
		        mssError(1,"HTMN","Menu Item widget must have a 'value' property");
		        return -1;
		    }
		    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
		    xsConcatPrintf(&xs,"'%s',", string);
    
		    if (objGetAttrValue(w_obj,"width",DATA_T_STRING,POD(&ptr)) != 0) {
		        mssError(1,"HTMN","Menu Item widget must have a 'width' property");
		        return -1;
		    }
		    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
		    xsConcatPrintf(&xs,"%s)", string);

	    } else {
	        htrRenderWidget(s, w_obj, z+1, parentname, nptr);
	    }
	    objClose(w_obj);
	}
	if (flag) {
	    xsConcatPrintf(&xs, "));\n");
	    htrAddScriptInit(s,xs.String);
	    xsDeInit(&xs);
	}
	objQueryClose(qy);
    }

    return 0;
}


/* 
   htmenuInitialize - register with the ht_render module.
*/
int htmenuInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Menu Widget Driver");
   strcpy(drv->WidgetName,"menu");
   drv->Render = htmenuRender;
   drv->Verify = htmnVerify;

   /** Register events **/
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");
   htrAddEvent(drv,"GetFocus");
   htrAddEvent(drv,"LoseFocus");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTMN.idcnt = 0;

   return 0;
}


