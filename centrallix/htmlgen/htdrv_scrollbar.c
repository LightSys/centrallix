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

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_scrollbar.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	July 14, 2003    					*/
/* Description:	HTML Widget driver for a scrollbar - either horizontal	*/
/*		or vertical.						*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTSB;


/*** htsbRender - generate the HTML code for the page.
 ***/
int
htsbRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int x,y,w,h,r;
    int id, i, t;
    int visible = 1;
    char bcolor[64] = "";
    char bimage[64] = "";
    int is_horizontal = 0;
    pExpression code;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTSB","Netscape 4.x, IE, or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTSB.idcnt++);

	/** Which direction? **/
	if (wgtrGetPropertyValue(tree,"direction",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"vertical")) is_horizontal = 0;
	    else if (!strcasecmp(ptr,"horizontal")) is_horizontal = 1;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTSB","Scrollbar widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTSB","Scrollbar widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    if (is_horizontal)
		{
		mssError(1,"HTSB","Horizontal scrollbar widgets must have a 'width' property");
		return -1;
		}
	    else
		{
		w = 18;
		}
	    }
	if (is_horizontal && w <= 18*3)
	    {
	    mssError(1,"HTSB","Horizontal scrollbar width must be greater than %d", 18*3);
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    if (!is_horizontal)
		{
		mssError(1,"HTSB","Vertical scrollbar widgets must have a 'height' property");
		return -1;
		}
	    else
		{
		h = 18;
		}
	    }
	if (!is_horizontal && h <= 18*3)
	    {
	    mssError(1,"HTSB","Vertical scrollbar height must be greater than %d", 18*3);
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Range of scrollbar (static or dynamic property) **/
	if (is_horizontal)
	    r = w;
	else
	    r = h;
	if ((t = wgtrGetPropertyType(tree,"range")) == DATA_T_INTEGER)
	    {
	    wgtrGetPropertyValue(tree,"range",DATA_T_INTEGER,POD(&r));
	    }
	else if (t == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"range", DATA_T_CODE, POD(&code));
	    htrAddExpression(s, name, "range", code);
	    }

	/** Check background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bcolor,ptr,sizeof(bcolor));
	    }
	if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bimage,ptr,sizeof(bimage));
	    }

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#sb%POSpane { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; clip:rect(0px,%POSpx,%POSpx,0px); Z-INDEX:%POS; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	if (is_horizontal)
	    htrAddStylesheetItem_va(s,"\t#sb%POSthum { POSITION:absolute; VISIBILITY:inherit; LEFT:18px; TOP:0px; WIDTH:18px; Z-INDEX:%POS; }\n",id,z+1);
	else
	    htrAddStylesheetItem_va(s,"\t#sb%POSthum { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:18px; Z-INDEX:%POS; }\n",id,z+1);

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "sb_target_img", "null", 0);
	htrAddScriptGlobal(s, "sb_click_x","0",0);
	htrAddScriptGlobal(s, "sb_click_y","0",0);
	htrAddScriptGlobal(s, "sb_thum_x","0",0);
	htrAddScriptGlobal(s, "sb_thum_y","0",0);
	htrAddScriptGlobal(s, "sb_mv_timeout","null",0);
	htrAddScriptGlobal(s, "sb_mv_incr","0",0);
	htrAddScriptGlobal(s, "sb_cur_mainlayer","null",0);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"sb%POSpane\")",id);

	htrAddScriptInclude(s, "/sys/js/htdrv_scrollbar.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    sb_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), tname:\"sb%POSthum\", isHorizontal:%INT, range:%INT});\n", name, id, is_horizontal, r);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"sb%POSpane\"><TABLE %[bgcolor=\"%STR&HTE\"%] %[background=\"%STR&HTE\"%] border=0 cellspacing=0 cellpadding=0 width=%POS>", id, *bcolor, bcolor, *bimage, bimage, w);
	if (is_horizontal)
	    {
	    htrAddBodyItem(s,   "<TR><TD align=right><IMG SRC=/sys/images/ico19b.gif width=18 height=18 NAME=u></TD><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC=/sys/images/trans_1.gif height=18 width=%POS name='b'>",w-36);
	    htrAddBodyItem(s,   "</TD><TD align=right><IMG SRC=/sys/images/ico18b.gif width=18 height=18 NAME=d></TD></TR></TABLE>\n");
	    }
	else
	    {
	    htrAddBodyItem(s,   "<TR><TD align=right><IMG SRC=/sys/images/ico13b.gif width=18 height=18 NAME=u></TD></TR><TR><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC=/sys/images/trans_1.gif height=%POS width=18 name='b'>",h-36);
	    htrAddBodyItem(s,   "</TD></TR><TR><TD align=right><IMG SRC=/sys/images/ico12b.gif width=18 height=18 NAME=d></TD></TR></TABLE>\n");
	    }
	htrAddBodyItem_va(s,"<DIV ID=\"sb%POSthum\"><IMG SRC=/sys/images/ico14b.gif NAME=t></DIV>",id);

	/** Add the event handling scripts **/

	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","sb","sb_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","sb","sb_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","sb","sb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "sb","sb_mouseover");

	/** Check for more sub-widgets within the scrollbar (visual ones not allowed). **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2);

	/** Finish off the last <DIV> **/
	htrAddBodyItem(s,"</DIV>\n");

    return 0;
    }


/*** htsbInitialize - register with the ht_render module.
 ***/
int
htsbInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Scrollbar Widget Driver");
	strcpy(drv->WidgetName,"scrollbar");
	drv->Render = htsbRender;

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	htrAddAction(drv,"MoveTo");
	htrAddParam(drv,"MoveTo","Value",DATA_T_INTEGER);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");
	HTSB.idcnt = 0;

    return 0;
    }
