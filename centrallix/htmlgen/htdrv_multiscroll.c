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
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_multiscroll.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	August 31, 2007  					*/
/* Description:	HTML Widget driver for the multi-scroll widget.  This	*/
/*		widget provides a scrollable region with multiple areas	*/
/*		within it, each having potentially different properties	*/
/*		when it comes to scrolling.				*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTMS;


/*** htmsRender - generate the HTML code for the page.
 ***/
int
htmsRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h,ch,total_h,top_h,total_h_accum,cy;
    int id;
    int i, cnt;
    int box_offset;
    int always_visible;
    pWgtrNode childlist[32];
    pWgtrNode child;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTMS","Netscape DOM or W3C DOM1 HTML and CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTMS.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTMS","MultiScroll widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTMS","MultiScroll widget must have a 'height' property");
	    return -1;
	    }

	/** Background color/image? **/
	htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_bg,sizeof(main_bg));

	/** figure out box offset fudge factor... stupid box model... **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Ok, write the style header items. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,"\t#ms%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,h,z);
	    }
	else if(s->Capabilities.CSS1)
	    {
	    htrAddStylesheetItem_va(s,"\t#ms%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,h,z);
	    htrAddStylesheetItem_va(s,"\t#ms%POSmain { %STR}\n",id,main_bg);
	    }
	else
	    {
	    mssError(1,"HTMS","Cannot render - environment unsupported");
	    }

	/** DOM linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "ms%POSmain",id);

	/** Script include call **/
	htrAddScriptInclude(s, "/sys/js/htdrv_multiscroll.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document","MOUSEUP", "ms", "ms_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "ms", "ms_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "ms", "ms_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "ms", "ms_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "ms", "ms_mousemove");

	/** Script init **/
	htrAddScriptInit_va(s, "    ms_init(wgtrGetNodeRef(ns,'%STR&SYM'), {});\n", 
		name);

	/** div for the main body of this widget **/
	htrAddBodyItem_va(s,"<DIV ID=\"ms%POSmain\">\n",id);

	/** Check for objects within the widget. **/
	cnt = wgtrGetMatchingChildList(tree, "widget/multiscrollpart", childlist, sizeof(childlist)/sizeof(pWgtrNode));
	top_h = total_h = 0;
	for(i=0;i<cnt;i++)
	    {
	    child = childlist[i];
	    if (wgtrGetPropertyValue(child,"height",DATA_T_INTEGER,POD(&ch)) != 0 || ch < 0) ch = 0;
	    total_h += ch;
	    if (ch && !top_h) top_h = ch;
	    }
	total_h_accum = 0;
	for(i=0;i<cnt;i++)
	    {
	    child = childlist[i];
	    htrAddWgtrObjLinkage_va(s, child, "ms%POSpart%POS",id,i);
	    htrAddBodyItem_va(s,"<DIV ID=\"ms%POSpart%POS\">\n",id,i);
	    always_visible = htrGetBoolean(child, "always_visible", 0);
	    if (wgtrGetPropertyValue(child,"height",DATA_T_INTEGER,POD(&ch)) != 0 || ch < 0) ch = 0;
	    if (top_h >= total_h_accum + ch)
		cy = total_h_accum;
	    else
		cy = h - total_h + total_h_accum;
	    total_h_accum += ch;
	    htrAddStylesheetItem_va(s,"\t#ms%POSpart%POS { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; LEFT:0px; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%INTpx,%INTpx,0px);}\n",id,i,cy,w,ch,z+1,w,ch);
	    if (wgtrGetPropertyValue(child,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	    if (always_visible && ch == 0)
		{
		mssError(1, "HTMS", "MultiScroll part '%s' of '%s' set to always_visible=true must have a height", ptr, name);
		return -1;
		}
	    htrAddScriptInit_va(s, "    wgtrGetNodeRef(ns,'%STR&SYM').addPart(wgtrGetNodeRef('%STR&SYM','%STR&SYM'), {av:%INT, h:%INT, y:%INT});\n",
		    name, wgtrGetNamespace(child), ptr, always_visible, ch, cy);
	    htrRenderSubwidgets(s, child, z+2);
	    htrAddBodyItem(s, "</DIV>\n");
	    }
	htrRenderSubwidgets(s, tree, z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htmsRender_part - renderer for the multiscroll part subwidget.
 ***/
int
htmsRender_part(pHtSession s, pWgtrNode tree, int z)
    {
    return 0;
    }


/*** htmsInitialize - register with the ht_render module.
 ***/
int
htmsInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML MultiScroll Driver");
	strcpy(drv->WidgetName,"multiscroll");
	drv->Render = htmsRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTMS.idcnt = 0;

	drv = htrAllocDriver();
	if (!drv) return -1;
	strcpy(drv->Name,"DHTML MultiScroll Part Driver");
	strcpy(drv->WidgetName,"multiscrollpart");
	drv->Render = htmsRender_part;
	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

    return 0;
    }
