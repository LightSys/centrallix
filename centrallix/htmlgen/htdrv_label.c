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
#include "cxlib/qprintf.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_label.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line label.		*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTLBL;


/*** htlblRender - generate the HTML code for the label widget.
 ***/
int
htlblRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char align[64];
    char valign[64];
    char main_bg[128];
    char fgcolor[64];
    char pfgcolor[64];
    char cfgcolor[64];
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int x=-1,y=-1,w,h;
    int id;
    /*int fontsize;*/
    int font_size;
    char *text;
    char* tooltip;
    char stylestr[128];
    int is_bold = 0;
    int is_link = 0;
    int is_italic = 0;
    int allow_break = 0;
    int overflow_ellipsis = 0;
    pExpression code;
    int n;
    int auto_height=0;

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTTBL","Netscape DOM support or W3C DOM Level 1 HTML required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTLBL.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTLBL","Label widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTLBL","Label widget must have a 'height' property");
	    return -1;
	    }

	/** auto height? **/
	if (wgtrGetPropertyValue(tree,"r_height",DATA_T_INTEGER,POD(&n)) == 0 && n == -1)
	    auto_height = 1;

	if (wgtrGetPropertyType(tree,"value") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"value",DATA_T_CODE,POD(&code));
	    text = nmSysStrdup("");
	    htrAddExpression(s, name, "value", code);
	    }
	else if (wgtrGetPropertyValue(tree,"value",DATA_T_STRING,POD(&ptr)) == 0)
	    text=nmSysStrdup(ptr);
	else if (wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) == 0)
	    text=nmSysStrdup(ptr);
	else
	    text=nmSysStrdup("");

	if(wgtrGetPropertyValue(tree,"tooltip",DATA_T_STRING,POD(&ptr)) == 0)
	    tooltip=nmSysStrdup(ptr);
	else
	    tooltip=nmSysStrdup("");

	/** label text color **/
	if (wgtrGetPropertyValue(tree,"fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    /*qpfPrintf(NULL, fgcolor,sizeof(fgcolor)," color=\"%STR&HTE\"",ptr);*/
	    strtcpy(fgcolor, ptr, sizeof(fgcolor));
	else
	    fgcolor[0] = '\0';

	/** label text color - when pointed to **/
	if (wgtrGetPropertyValue(tree,"point_fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(pfgcolor, ptr, sizeof(pfgcolor));
	    is_link = 1;
	    }
	else
	    pfgcolor[0] = '\0';

	/** label text color - when clicked **/
	if (wgtrGetPropertyValue(tree,"click_fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(cfgcolor, ptr, sizeof(cfgcolor));
	    is_link = 1;
	    }
	else
	    cfgcolor[0] = '\0';

	/** font size in points **/
	if (wgtrGetPropertyValue(tree,"font_size",DATA_T_INTEGER,POD(&font_size)) != 0 || font_size < 5 || font_size > 100)
	    font_size = -1;

	/** bold? **/
	if (wgtrGetPropertyValue(tree, "style", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr,"bold"))
	    is_bold = 1;

	/** Italic? **/
	if (wgtrGetPropertyValue(tree, "style", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr,"italic"))
	    is_italic = 1;

	/** Allow text break/wrap? **/
	allow_break = htrGetBoolean(tree, "allow_break", 1);
	overflow_ellipsis = htrGetBoolean(tree, "overflow_ellipsis", 0);

	/** alignment **/
	align[0]='\0';
	if(wgtrGetPropertyValue(tree,"align",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(align,ptr,sizeof(align));
	else
	    strcpy(align,"left");
	
	if(wgtrGetPropertyValue(tree,"valign",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(valign,ptr,sizeof(valign));
	else
	    strcpy(valign,"top");
	
	/** Background color/image? **/
	htrGetBackground(tree, NULL, 0, main_bg, sizeof(main_bg));

	/** Field name **/
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#lbl%POS { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; %[HEIGHT:%POSpx; %]WIDTH:Calc(100%% - %POSpx); Z-INDEX:%POS; cursor:default; %[font-weight:bold; %]%[color:%STR&CSSVAL; %]%[font-size:%POSpx; %]text-align:%STR&CSSVAL; vertical-align:%STR&CSSVAL; %[white-space:nowrap; %]%[text-overflow:ellipsis; overflow:hidden; %]%[font-style:italic; %]}\n",
		id,x,y,
		!auto_height, h,
		x,z, 
		is_bold, *fgcolor, fgcolor, font_size > 0, font_size, align, valign,
		!allow_break, overflow_ellipsis, is_italic);
	if (is_link)
	    htrAddStylesheetItem_va(s,"\t#lbl%POS:hover { %[color:%STR&CSSVAL; %]text-decoration:underline; cursor:pointer; }\n", id, *pfgcolor, pfgcolor);
	if (is_link && *cfgcolor)
	    htrAddStylesheetItem_va(s,"\t#lbl%POS:active { color:%STR&CSSVAL; text-decoration:underline; cursor:pointer; }\n", id, cfgcolor);
	htrAddStylesheetItem_va(s,"\t#lbl%POS p { text-align:%STR&CSSVAL; %[position:relative; top:50%%; transform:translateY(-50%%); %]padding:0px; margin:0px; border-spacing:0px; width:100%%; }\n", id, align, !strcmp(valign, "middle"), w);

	htrAddWgtrObjLinkage_va(s, tree, "lbl%POS",id);
	stylestr[0] = '\0';
	htrAddScriptInit_va(s, "    lbl_init(wgtrGetNodeRef(ns,'%STR&SYM'), {field:'%STR&JSSTR', form:'%STR&JSSTR', text:'%STR&JSSTR', style:'%STR&JSSTR', tooltip:'%STR&JSSTR', link:%POS, pfg:'%STR&JSSTR'});\n",
		name, fieldname, form, text, stylestr, tooltip, is_link, pfgcolor);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_label.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document","MOUSEUP", "lbl", "lbl_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "lbl", "lbl_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "lbl", "lbl_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "lbl", "lbl_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "lbl", "lbl_mousemove");

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItemLayer_va(s, 0, "lbl%POS", id, NULL, "<p><span>%STR&HTENLBR</span></p>", text);

	/** Check for more sub-widgets **/
	htrRenderSubwidgets(s, tree, z+1);

	nmSysFree(text);
	nmSysFree(tooltip);

    return 0;
    }


/*** htlblInitialize - register with the ht_render module.
 ***/
int
htlblInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Label Driver");
	strcpy(drv->WidgetName,"label");
	drv->Render = htlblRender;

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTLBL.idcnt = 0;

    return 0;
    }
