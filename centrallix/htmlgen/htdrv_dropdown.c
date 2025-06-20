#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "wgtr.h"

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

/** globals **/
static struct {
   int     idcnt;
} HTDD;


/* 
   htddRender - generate the HTML code for the page.
*/
int htddRender(pHtSession s, pWgtrNode tree, int z) {
   char bgstr[HT_SBUF_SIZE];
   char textcolor[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char fieldname[30];
   char form[64];
   char osrc[64];
   char name[64];
   char *ptr;
   char *sql;
   char *str;
   char *attr;
   int type, rval, mode, flag=0;
   int x,y,w,h;
   int id, i;
   int num_disp;
   int query_multiselect;
   int invalid_select_default;
   int pop_w;
   ObjData od;
   XString xs;
   pObjQuery qy;
   pObject qy_obj;
   pWgtrNode subtree;

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTDD","Netscape or W3C DOM support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTDD.idcnt++);

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h=0;
   if (h < s->ClientInfo->ParagraphHeight+2)
	h = s->ClientInfo->ParagraphHeight+2;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	return -1;
   }
   pop_w = w;

   /** Width of popup dropdown list **/
   wgtrGetPropertyValue(tree,"popup_width", DATA_T_INTEGER, POD(&pop_w));

   query_multiselect = htrGetBoolean(tree, "query_multiselect", 0);
   invalid_select_default = htrGetBoolean(tree, "invalid_select_default", 0);

   if (wgtrGetPropertyValue(tree,"numdisplay",DATA_T_INTEGER,POD(&num_disp)) != 0) num_disp=3;

   if (wgtrGetPropertyValue(tree,"hilight",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(hilight,ptr,sizeof(hilight));
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'hilight' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(bgstr,ptr,sizeof(bgstr));
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'bgcolor' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(textcolor,ptr,sizeof(textcolor));
   } else {
	strcpy(textcolor, "");
   }

   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(fieldname,ptr,sizeof(fieldname));
   } else {
	fieldname[0]='\0';
   }

   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	strtcpy(form,ptr,sizeof(form));
   else
	form[0]='\0';
   if (wgtrGetPropertyValue(tree,"objectsource",DATA_T_STRING,POD(&ptr)) == 0)
	strtcpy(osrc,ptr,sizeof(osrc));
   else
	osrc[0]='\0';

    /** Get name **/
    if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
    strtcpy(name,ptr,sizeof(name));

    /** Ok, write the style header items. **/
    htrAddStylesheetItem_va(s,"\t#dd%POSbtn { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; HEIGHT:%POSpx; WIDTH:%POSpx; Z-INDEX:%POS; cursor:default; background-color: %STR&CSSVAL; border:1px outset #e0e0e0;}\n",id,x,y,h,w,z,bgstr);
    if (*textcolor) {
	htrAddStylesheetItem_va(s,"\t#dd%POSbtn { color: %STR&CSSVAL; }\n",id,textcolor);
    }
    htrAddStylesheetItem_va(s,"\t#dd%POScon1 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,h-2,z+1);
    htrAddStylesheetItem_va(s,"\t#dd%POScon2 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:hidden; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,h-2,z+1);

    htrAddScriptGlobal(s, "dd_current", "null", 0);
    htrAddScriptGlobal(s, "dd_lastkey", "null", 0);
    htrAddScriptGlobal(s, "dd_target_img", "null", 0);
    htrAddScriptGlobal(s, "dd_thum_y","0",0);
    htrAddScriptGlobal(s, "dd_timeout","null",0);
    htrAddScriptGlobal(s, "dd_click_x","0",0);
    htrAddScriptGlobal(s, "dd_click_y","0",0);
    htrAddScriptGlobal(s, "dd_incr","0",0);
    htrAddScriptGlobal(s, "dd_cur_mainlayer","null",0);
    htrAddWgtrObjLinkage_va(s, tree, "dd%POSbtn", id);

    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);
    htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js", 0);

    htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "dd", "dd_mousemove");
    htrAddEventHandlerFunction(s, "document","MOUSEOVER", "dd", "dd_mouseover");
    htrAddEventHandlerFunction(s, "document","MOUSEUP", "dd", "dd_mouseup");
    htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "dd", "dd_mousedown");
    htrAddEventHandlerFunction(s, "document","MOUSEOUT", "dd", "dd_mouseout");
    if (s->Capabilities.Dom1HTML)
       htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "dd", "dd_contextmenu");


    /** Get the mode (default to 1, dynamicpage) **/
    mode = 0;
    if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0) {
	if (!strcmp(ptr,"static")) mode = 0;
	else if (!strcmp(ptr,"dynamic_server")) mode = 1;
	else if (!strcmp(ptr,"dynamic")) mode = 2;
	else if (!strcmp(ptr,"dynamic_client")) mode = 2;
	else if (!strcmp(ptr,"objectsource")) mode = 3;
	else {
	    mssError(1,"HTDD","Dropdown widget has not specified a valid mode.");
	    return -1;
	}
    }

    sql = 0;
    if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0 && mode != 0 && mode != 3) {
	mssError(1, "HTDD", "SQL parameter was not specified for dropdown widget");
	return -1;
    }
    htrCheckAddExpression(s,tree,name,"sql");

    /** Script initialization call. **/
    htrAddScriptInit_va(s,"    dd_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), c1:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"), \"dd%POScon1\"), c2:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"), \"dd%POScon2\"), background:'%STR&JSSTR', highlight:'%STR&JSSTR', fieldname:'%STR&JSSTR', numDisplay:%INT, mode:%INT, sql:'%STR&JSSTR', width:%INT, height:%INT, form:'%STR&JSSTR', osrc:'%STR&JSSTR', qms:%INT, ivs:%INT, popup_width:%INT});\n", name, name, id, name, id, bgstr, hilight, fieldname, num_disp, mode, sql?sql:"", w, h, form, osrc, query_multiselect, invalid_select_default, pop_w);

    /** HTML body <DIV> element for the layers. **/
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POSbtn\">\n"
			"<IMG SRC=\"/sys/images/ico15b.gif\" style=\"float:right;\">\n", id);
    /*htrAddBodyItem_va(s,"<TABLE width=%POS cellspacing=0 cellpadding=0 border=0>\n",w);
    htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%POS></TD>\n",w-2);
    htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%POS width=1></TD>\n",h-2);
    htrAddBodyItem(s,   "       <TD align=right valign=middle><IMG SRC=/sys/images/ico15b.gif></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%POS width=1></TD></TR>\n",h-2);
    htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%POS></TD>\n",w-2);
    htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
    htrAddBodyItem(s,   "</TABLE>\n");*/
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POScon1\"></DIV>\n",id);
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POScon2\"></DIV>\n",id);
    htrAddBodyItem(s,   "</DIV>\n");
    
    /* Read and initialize the dropdown items */
    if (mode == 1) {
	/** The result set from this SQL query can take two forms: positional or named.
	 ** For Positional, the params are: label, value, selected, group, hidden.
	 ** For Named, the above names can appear in any order.
	 ** label and value are required.
	 **/
	if ((qy = objMultiQuery(s->ObjSession, sql, NULL, 0))) {
	    flag=0;
	    htrAddScriptInit_va(s,"    dd_add_items(wgtrGetNodeRef(ns,\"%STR&SYM\"), [",name);
	    while ((qy_obj = objQueryFetch(qy, O_RDONLY))) {
		// Label
		attr = objGetFirstAttr(qy_obj);
		if (!attr) {
		    objClose(qy_obj);
		    objQueryClose(qy);
		    mssError(1, "HTDD", "SQL query must have at least two attributes: label and value.");
		    return -1;
		}
		type = objGetAttrType(qy_obj, attr);
		rval = objGetAttrValue(qy_obj, attr, type,&od);

                // Check if anything was returned
                if(rval != 0){
                    objClose(qy_obj);
                    continue;
                }
                
		if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
		    str = objDataToStringTmp(type, (void*)(&od), DATA_F_QUOTED);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), DATA_F_QUOTED);
		}
		if (flag) htrAddScriptInit(s,",");
		htrAddScriptInit_va(s,"{wname:null, label:%STR,",str);
		// Value
		attr = objGetNextAttr(qy_obj);
		if (!attr) {
		    objClose(qy_obj);
		    objQueryClose(qy);
		    mssError(1, "HTDD", "SQL query must have at least two attributes: label and value.");
		    return -1;
		}

		type = objGetAttrType(qy_obj, attr);
		rval = objGetAttrValue(qy_obj, attr, type,&od);
		if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
		    str = objDataToStringTmp(type, (void*)(&od), DATA_F_QUOTED);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), DATA_F_QUOTED);
		}
		htrAddScriptInit_va(s,"value:%STR", str);

		/** is selected **/
		attr = objGetNextAttr(qy_obj);
		if (attr) {
		    type = objGetAttrType(qy_obj, attr);
		    rval = objGetAttrValue(qy_obj, attr, type, &od);
		    htrAddScriptInit_va(s, ",sel:%INT", type == DATA_T_INTEGER && rval == 0 && od.Integer != 0);

		    /** grouping **/
		    attr = objGetNextAttr(qy_obj);
		    if (attr) {
			type = objGetAttrType(qy_obj, attr);
			rval = objGetAttrValue(qy_obj, attr, type, &od);
			if (rval == 0 && type == DATA_T_INTEGER)
			    htrAddScriptInit_va(s, ",grp:%INT", od.Integer);
			else if (rval == 0 && type == DATA_T_STRING)
			    htrAddScriptInit_va(s, ",grp:\"%STR&JSSTR\"", od.String);

			/** hidden **/
			attr = objGetNextAttr(qy_obj);
			if (attr) {
			    type = objGetAttrType(qy_obj, attr);
			    rval = objGetAttrValue(qy_obj, attr, type, &od);
			    htrAddScriptInit_va(s, ",hide:%INT}", type == DATA_T_INTEGER && rval == 0 && od.Integer != 0);
			} else {
			    htrAddScriptInit(s, "}");
			}
		    } else {
			htrAddScriptInit(s, "}");
		    }
		} else {
		    htrAddScriptInit(s, "}");
		}

		objClose(qy_obj);
		flag=1;
	    }
	    htrAddScriptInit(s,"]);\n");
	    objQueryClose(qy);
	}
    }
    else if(mode==3) {
	/* get objects from form */
    }


    flag=0;
    for (i=0;i<xaCount(&(tree->Children));i++)
	{
	subtree = xaGetItem(&(tree->Children), i);
	if (!strcmp(subtree->Type, "widget/dropdownitem")) 
	    subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	if (!strcmp(subtree->Type,"widget/dropdownitem") && mode == 0) 
	    {
	    if (wgtrGetPropertyValue(subtree,"label",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'width' property");
		return -1;
		}
	    strtcpy(string, ptr, sizeof(string));
	    if (flag) 
		{
		xsConcatenate(&xs, ",", 1);
		}
	    else 
		{
		xsInit(&xs);
		xsConcatQPrintf(&xs, "    dd_add_items(wgtrGetNodeRef(ns,\"%STR&SYM\"), [", name);
		flag=1;
		}
	    wgtrGetPropertyValue(subtree,"name",DATA_T_STRING,POD(&ptr));
	    xsConcatQPrintf(&xs,"{wname:'%STR&SYM', label:'%STR&JSSTR',", ptr, string);

	    if (htrGetBoolean(subtree, "selected", 0) == 1)
		{
		xsConcatenate(&xs,"sel:1,",6);
		}

	    if (wgtrGetPropertyValue(subtree,"value",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'value' property");
		return -1;
		}
	    strtcpy(string,ptr, sizeof(string));
	    xsConcatQPrintf(&xs,"value:'%STR&JSSTR'}", string);
	    } 
	else 
	    {
	    htrRenderWidget(s, subtree, z+1);
	    }
	}
    if (flag) 
	{
	xsConcatenate(&xs, "]);\n", 4);
	htrAddScriptInit(s,xs.String);
	xsDeInit(&xs);
	}


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
   xaAddItem(&(drv->PseudoTypes), "dropdownitem");

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

   HTDD.idcnt = 0;

   return 0;
}

