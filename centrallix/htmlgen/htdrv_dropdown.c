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
/* Copyright (C) 2000-2026 LightSys Technology Services, Inc.		*/
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

/** Dropdown modes. **/
#define HTDD_STATIC 0
#define HTDD_DYNAMIC_SERVER 1
#define HTDD_DYNAMIC 2
#define HTDD_DYNAMIC_CLIENT HTDD_DYNAMIC
#define HTDD_OBJECTSOURCE 3
#define HTDD_EASTER_EGG_4 4

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
   int num_disp;
   int query_multiselect;
   int invalid_select_default;
   int pop_w;
   ObjData od;
   XString xs;
   pXString items_xs = NULL;
   pObjQuery qy = NULL;
   pObject qy_obj = NULL;
   pWgtrNode subtree;
   int ret = -1;

    /** Get an id for this. **/
    const int id = (HTDD.idcnt++);

    /** Verify browser capabilities. **/
    if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	{
	mssError(1, "HTDD", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	goto end;
	}

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h=0;
   if (h < s->ClientInfo->ParagraphHeight+2)
	h = s->ClientInfo->ParagraphHeight+2;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	goto end;
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
	goto end;
   }

   if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(bgstr,ptr,sizeof(bgstr));
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'bgcolor' property");
	goto end;
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
    if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto end;
    strtcpy(name,ptr,sizeof(name));

    /** Write basic element CSS. **/
    if (htrAddStylesheetItem_va(s,
	"\t\t#dd%POSbtn { "
	    "position:absolute; "
	    "visibility:inherit; "
	    "overflow:hidden; "
	    "cursor:pointer; "
	    "left:"ht_flex_format"; "
	    "top:"ht_flex_format"; "
	    "width:"ht_flex_format"; "
	    "height:"ht_flex_format"; "
	    "z-index:%POS; "
	    "background-color: %STR&CSSVAL; "
	    "border:1px outset #e0e0e0; "
	"}\n",
	id,
	ht_flex_x(x, tree),
	ht_flex_y(y, tree),
	ht_flex_w(w, tree),
	ht_flex_h(h, tree),
	z,
	bgstr
    ) != 0)
	{
	mssError(0, "HTDD", "Failed to write base btn CSS.");
	goto end;
	}
    if (*textcolor)
        {
	if (htrAddStylesheetItem_va(s,
	    "\t\t#dd%POSbtn { "
		"color:%STR&CSSVAL; "
	    "}\n",
	    id,
	    textcolor
        ) != 0)
	    {
	    mssError(0, "HTDD", "Failed to write btn CSS text color.");
	    goto end;
	    }
        }
    if (htrAddStylesheetItem_va(s,
	"\t\t.dd%POScon { "
	    "position:absolute; "
	    "overflow:hidden; "
	    "left:1px; "
	    "top:1px; "
	    "width:1024px; "
	    "height:%POSpx; "
	    "z-index:%POS; "
	"}\n",
	id,
	h - 2,
	z + 1
    ) != 0)
	{
	mssError(0, "HTDD", "Failed to write con CSS.");
	goto end;
	}
    
    /** Link the widget to the DOM node. **/
    if (htrAddWgtrObjLinkage_va(s, tree, "dd%POSbtn", id) != 0)
	{
	mssError(0, "HTDD", "Failed to add object linkage.");
	goto end;
	}

    /** Write JS globals. **/
    if (htrAddScriptGlobal(s, "dd_click_x",       "0",    0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_click_y",       "0",    0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_cur_mainlayer", "null", 0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_current",       "null", 0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_incr",          "0",    0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_lastkey",       "null", 0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_target_img",    "null", 0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_thum_y",        "0",    0) != 0) goto end;
    if (htrAddScriptGlobal(s, "dd_timeout",       "null", 0) != 0) goto end;

    /** Write JS script includes. **/
    if (htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js",  0) != 0) goto end;
    if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto end;
    if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto end;
    if (htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js",  0) != 0) goto end;

    /** Register JS event handlers. **/
    if (htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "dd", "dd_contextmenu") != 0) goto end;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN",   "dd", "dd_mousedown")   != 0) goto end;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE",   "dd", "dd_mousemove")   != 0) goto end;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",    "dd", "dd_mouseout")    != 0) goto end;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER",   "dd", "dd_mouseover")   != 0) goto end;
    if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",     "dd", "dd_mouseup")     != 0) goto end;


    /** Get the mode (default to 1, dynamicpage) **/
    mode = HTDD_STATIC;
    if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	{
	if (strcmp(ptr, "static") == 0)              mode = HTDD_STATIC;
	else if (strcmp(ptr, "dynamic_server") == 0) mode = HTDD_DYNAMIC_SERVER;
	else if (strcmp(ptr, "dynamic") == 0)        mode = HTDD_DYNAMIC;
	else if (strcmp(ptr, "dynamic_client") == 0) mode = HTDD_DYNAMIC_CLIENT;
	else if (strcmp(ptr, "objectsource") == 0)   mode = HTDD_OBJECTSOURCE;
	else
	    {
	    mssError(1, "HTDD", "Invalid dropdown widget 'mode' value: \"%s\"", ptr);
	    goto end;
	    }
	}

    sql = NULL;
    if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0 && mode != HTDD_STATIC && mode != HTDD_OBJECTSOURCE)
	{
	mssError(1, "HTDD", "SQL parameter was not specified for dropdown widget");
	goto end;
	}
    htrCheckAddExpression(s,tree,name,"sql");

    /** Write the initialization call in its own scope. **/
    if (htrAddScriptInit_va(s, "\t{ "
	"const layer = wgtrGetNodeRef(ns, '%STR&SYM'); "
	"dd_init({ "
	    "layer, "
	    "c1:htr_subel(layer, 'dd%POScon1'), "
	    "c2:htr_subel(layer, 'dd%POScon2'), "
	    "background:'%STR&JSSTR', "
	    "highlight:'%STR&JSSTR', "
	    "fieldname:'%STR&JSSTR', "
	    "numDisplay:%INT, "
	    "mode:%INT, "
	    "sql:'%STR&JSSTR', "
	    "form:'%STR&JSSTR', "
	    "osrc:'%STR&JSSTR', "
	    "qms:%INT, "
	    "ivs:%INT, "
	    "width:%INT, "
	    "height:%INT, "
	    "popup_width:%INT, "
	"}); }\n",
	name, id, id,
	bgstr, hilight,
	fieldname, num_disp, mode,
	(sql != NULL) ? sql : "",
	form, osrc, query_multiselect,
	invalid_select_default,
	w, h, pop_w
    ) != 0)
	{
	mssError(0, "HTDD", "Failed to write JS init call.");
	goto end;
	}

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
    htrAddBodyItem_va(s,"<DIV ID='dd%POScon1' CLASS='dd%POScon' style='visibility:inherit;'></DIV>\n", id, id);
    htrAddBodyItem_va(s,"<DIV ID='dd%POScon2' CLASS='dd%POScon' style='visibility:hidden;'></DIV>\n", id, id);
    htrAddBodyItem(s,   "</DIV>\n");
    
    /* Read and initialize the dropdown items */
    if (mode == HTDD_DYNAMIC_SERVER) {
	/** The result set from this SQL query can take two forms: positional or named.
	 ** For Positional, the params are: label, value, selected, group, hidden.
	 ** For Named, the above names can appear in any order.
	 ** label and value are required.
	 **/
	if ((qy = objMultiQuery(s->ObjSession, sql, NULL, 0))) {
	    flag=0;
	    htrAddScriptInit_va(s, "\tdd_add_items(wgtrGetNodeRef(ns, '%STR&SYM'), [",name);
	    while ((qy_obj = objQueryFetch(qy, O_RDONLY))) {
		// Label
		attr = objGetFirstAttr(qy_obj);
		if (!attr) {
		    mssError(1, "HTDD", "SQL query must have at least two attributes: label and value.");
		    goto end;
		}
		type = objGetAttrType(qy_obj, attr);
		rval = objGetAttrValue(qy_obj, attr, type,&od);

                // Check if anything was returned
                if(rval != 0){
                    objClose(qy_obj);
		    qy_obj = NULL;
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
		    mssError(1, "HTDD", "SQL query must have at least two attributes: label and value.");
		    goto end;
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
		qy_obj = NULL;
		flag=1;
	    }
	    htrAddScriptInit(s,"]);\n");
	    objQueryClose(qy);
	    qy = NULL;
	}
    }
    else if (mode == HTDD_OBJECTSOURCE) {
	/* get objects from form */
    }


    const int n_children = xaCount(&(tree->Children));
    for (int i = 0; i < n_children; i++)
	{
	subtree = xaGetItem(&(tree->Children), i);
	if (!strcmp(subtree->Type, "widget/dropdownitem")) 
	    subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	
	/** Write JS to render dropdown items in static mode. **/
	if (!strcmp(subtree->Type,"widget/dropdownitem") && mode == HTDD_STATIC) 
	    {
	    if (wgtrGetPropertyValue(subtree,"label",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'width' property");
		goto end;
		}
	    strtcpy(string, ptr, sizeof(string));
	    if (items_xs != NULL)
		{
		xsConcatenate(&xs, ",", 1);
		}
	    else
		{
		if (xsInit(&xs) < 0) goto end;
		items_xs = &xs;
		xsConcatQPrintf(&xs, "    dd_add_items(wgtrGetNodeRef(ns,\"%STR&SYM\"), [", name);
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
		goto end;
		}
	    strtcpy(string,ptr, sizeof(string));
	    xsConcatQPrintf(&xs,"value:'%STR&JSSTR'}", string);
	    } 
	else 
	    {
	    if (htrRenderWidget(s, subtree, z + 1) != 0) goto end;
	    }
	}
    if (items_xs != NULL)
	{
	xsConcatenate(&xs, "]);\n", 4);
	htrAddScriptInit(s,xs.String);
	}

    /** Success. **/
    ret = 0;

    end:
    if (ret != 0)
	{
	mssError(0, "HTDD",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	}
    if (qy_obj != NULL) objClose(qy_obj);
    if (qy != NULL) objQueryClose(qy);
    if (items_xs != NULL) xsDeInit(items_xs);
    return ret;
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
