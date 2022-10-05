#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
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
/* Copyright (C) 1999-2014 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_window.c      					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 16, 1999					*/
/* Description:	HTML Widget driver for a window -- a DHTML layer that	*/
/*		can be dragged around the screen and appears to have	*/
/*		a 'titlebar' with a close (X) button on it.		*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTWIN;


/*** htwinRender - generate the HTML code for the page.
 ***/
int
htwinRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    pWgtrNode sub_tree;
    int x,y,w,h;
    int tbw,tbh,bx,by,bw,bh;
    int id, i;
    int visible = 1;
    char bgnd_style[128] = "";
    char hdr_bgnd_style[128] = "";
    char txtcolor[64] = "";
    int has_titlebar = 1;
    char title[128];
    int is_dialog_style = 0;
    int gshade = 0;
    int closetype = 0;
    int box_offset = 1;
    int is_toplevel = 0;
    int is_modal = 0;
    char icon[128];
    int shadow_offset,shadow_radius,shadow_angle;
    char shadow_color[128];
    int border_radius;
    char border_color[128];
    char border_style[32];
    int border_width;

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTWIN","Netscape DOM support or W3C DOM Level 1 support required");
	    return -1;
	    }

	/** IE puts css box borders inside box width/height **/
	if (!s->Capabilities.CSSBox)
	    {
	    box_offset = 0;
	    }

    	/** Get an id for this. **/
	id = (HTWIN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x = 0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y = 0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'height' property");
	    return -1;
	    }

	/** Border radius, color, and style. **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;
	if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_color, "#ffffff");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree,"border_style",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));
	if (!strcmp(border_style, "none") || !strcmp(border_style, "hidden"))
	    border_width=0;
	else
	    border_width=1;

	/** Drop shadow **/
	shadow_offset=0;
	if (wgtrGetPropertyValue(tree, "shadow_offset", DATA_T_INTEGER, POD(&shadow_offset)) == 0 && shadow_offset > 0)
	    shadow_radius = shadow_offset+1;
	else
	    shadow_radius = 0;
	wgtrGetPropertyValue(tree, "shadow_radius", DATA_T_INTEGER, POD(&shadow_radius));
	if (shadow_radius > 0)
	    {
	    if (wgtrGetPropertyValue(tree, "shadow_color", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(shadow_color, ptr, sizeof(shadow_color));
	    else
		strcpy(shadow_color, "black");
	    }
	if (wgtrGetPropertyValue(tree, "shadow_angle", DATA_T_INTEGER, POD(&shadow_angle)) != 0)
	    shadow_angle = 135;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Is this a toplevel window? **/
	is_toplevel = htrGetBoolean(tree, "toplevel", 0);

	/** Is this a modal window? **/
	is_modal = htrGetBoolean(tree, "modal", 0);

	/** Check background color **/
	htrGetBackground(tree, NULL, 1, bgnd_style, sizeof(bgnd_style));

	/** Check header background color/image **/
	if (htrGetBackground(tree, "hdr", 1, hdr_bgnd_style, sizeof(hdr_bgnd_style)) < 0)
	    strcpy(hdr_bgnd_style, bgnd_style);

	/** Check title text color. **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(txtcolor,ptr,sizeof(txtcolor));
	else
	    strcpy(txtcolor,"black");

	/** Check window title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(title,ptr, sizeof(title));
	else
	    strcpy(title,name);

	/** Marked not visible? **/
	visible = htrGetBoolean(tree, "visible", 1);

	/** No titlebar? **/
	if (wgtrGetPropertyValue(tree,"titlebar",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no"))
	    has_titlebar = 0;

	/** Dialog or window style? **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
	    is_dialog_style = 1;

	/** Graphical window shading? **/
	if (wgtrGetPropertyValue(tree,"gshade",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    gshade = 1;

	/** Graphical window close? **/
	if (wgtrGetPropertyValue(tree,"closetype",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"shrink1")) closetype = 1;
	    else if (!strcmp(ptr,"shrink2")) closetype = 2;
	    else if (!strcmp(ptr,"shrink3")) closetype = 1 | 2;
	    }

	/** Window icon? **/
	if (wgtrGetPropertyValue(tree, "icon", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(icon,ptr,sizeof(icon));
	    }
	else
	    {
	    strcpy(icon, "/sys/images/centrallix_18x18.gif");
	    }

	/** Compute titlebar width & height - includes edge below titlebar. **/
	if (has_titlebar)
	    {
	    tbw = w-2;
	    if (is_dialog_style || !s->Capabilities.Dom0NS)
	        tbh = 24;
	    else
	        tbh = 23;
	    }
	else
	    {
	    tbw = w-2;
	    tbh = 0;
	    }

	/** Compute window body geometry **/
	if (is_dialog_style)
	    {
	    bx = 1;
	    by = 1+tbh;
	    bw = w-2;
	    bh = h-tbh-2;
	    }
	else
	    {
	    bx = 2;
	    bw = w-4;
	    if (has_titlebar)
		{
		by = 1+tbh;
		bh = h-tbh-3;
		}
	    else
		{
		by = 2;
		bh = h-4;
		}
	    }

	/** Draw the main window layer and outer edge. **/
	/*htrAddStylesheetItem_va(s,"\t#wn%POSbase { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS;}\n",
		id,visible?"inherit":"hidden",x,y,w-2*box_offset,h-2*box_offset, w, h, z+100);*/
	htrAddStylesheetItem_va(s,"\t#wn%POSbase { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; overflow: hidden; Z-INDEX:%POS;}\n",
		id,visible?"inherit":"hidden",x,y,w-2*box_offset,h-2*box_offset, z+100);
	htrAddStylesheetItem_va(s,"\t#wn%POSbase { border-style: %STR&CSSVAL; border-width: %INTpx; border-color: %STR&CSSVAL; border-radius: %INTpx; }\n", 
		id, border_style, border_width, border_color, border_radius);
	if (shadow_radius > 0)
	    {
	    htrAddStylesheetItem_va(s,"\t#wn%POSbase { box-shadow: %DBLpx %DBLpx %POSpx %STR&CSSVAL; }\n", id,
			    sin(shadow_angle*M_PI/180)*shadow_offset, cos(shadow_angle*M_PI/180)*(-shadow_offset), shadow_radius, shadow_color);
	    }

	/** draw titlebar div **/
	if (has_titlebar)
	    {
	    htrAddStylesheetItem_va(s,"\t#wn%POStitlebar { POSITION: absolute; VISIBILITY: inherit; LEFT: 0px; TOP: 0px; HEIGHT: %POSpx; WIDTH: 100%%; overflow: hidden; Z-INDEX: %POS; color:%STR&CSSVAL; cursor:default; %STR}\n", id, tbh-1-box_offset, z+1, txtcolor, hdr_bgnd_style);
	    htrAddStylesheetItem_va(s,"\t#wn%POStitlebar { border-style: solid; border-width: 0px 0px 1px 0px; border-color: gray; }\n", id);
	    }

	/** inner structure depends on dialog vs. window style **/
	if (is_dialog_style)
	    {
	    /** window inner container -- dialog **/
	    htrAddStylesheetItem_va(s,"\t#wn%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:%INTpx; WIDTH: %POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS; %STR}\n",
		    id, tbh?(tbh-1):0, w-2, h-tbh-1, w, h-tbh+1, z+1, bgnd_style);
	    htrAddStylesheetItem_va(s,"\t#wn%POSmain { border-style: solid; border-width: %POSpx 0px 0px 0px; border-color: white; }\n", id, has_titlebar?1:0);
	    }
	else
	    {
	    /** window inner container -- window **/
	    htrAddStylesheetItem_va(s,"\t#wn%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:%INTpx; WIDTH: %POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS; %STR}\n",
		    id, tbh?(tbh-1):0, w-2-2*box_offset, h-tbh-(has_titlebar?1:2)-(has_titlebar?1:2)*box_offset, w, h-tbh+(has_titlebar?1:0)-2*box_offset, z+1, bgnd_style);
	    htrAddStylesheetItem_va(s,"\t#wn%POSmain { border-style: solid; border-width: %POSpx 1px 1px 1px; border-color: gray white white gray; }\n", id, has_titlebar?0:1);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "wn_top_z","10000",0);
	htrAddScriptGlobal(s, "wn_list","[]",0);
	htrAddScriptGlobal(s, "wn_current","null",0);
	htrAddScriptGlobal(s, "wn_newx","null",0);
	htrAddScriptGlobal(s, "wn_newy","null",0);
	htrAddScriptGlobal(s, "wn_topwin","null",0);
	htrAddScriptGlobal(s, "wn_msx","null",0);
	htrAddScriptGlobal(s, "wn_msy","null",0);
	htrAddScriptGlobal(s, "wn_moved","0",0);
	htrAddScriptGlobal(s, "wn_clicked","0",0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "wn%POSbase",id);
	htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, \"wn%POSmain\")",id);

	htrAddScriptInclude(s, "/sys/js/htdrv_window.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event handler for mousedown/up/click/etc **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "wn", "wn_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "wn", "wn_mouseup");
	htrAddEventHandlerFunction(s, "document", "DBLCLICK", "wn", "wn_dblclick");

	/** Mouse move event handler -- when user drags the window **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "wn", "wn_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "wn", "wn_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "wn", "wn_mouseout");

	/** Script initialization call. **/
	if (has_titlebar)
	    {
	    htrAddScriptInit_va(s,"    wn_init({mainlayer:wgtrGetNodeRef(ns,'%STR&SYM'), clayer:wgtrGetContainer(wgtrGetNodeRef(ns,'%STR&SYM')), gshade:%INT, closetype:%INT, toplevel:%INT, modal:%INT, titlebar:htr_subel(wgtrGetNodeRef(ns,'%STR&SYM'),'wn%POStitlebar')});\n", 
		    name,name,gshade,closetype, is_toplevel, is_modal, name, id);
	    }
	else
	    {
	    htrAddScriptInit_va(s,"    wn_init({mainlayer:wgtrGetNodeRef(ns,'%STR&SYM'), clayer:wgtrGetNodeRef(ns,'%STR&SYM'), gshade:%INT, closetype:%INT, toplevel:%INT, modal:%INT, titlebar:null});\n", 
		    name,name,gshade,closetype, is_toplevel, is_modal);
	    }

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"wn%POSbase\" CLASS=\"wnbase\">\n",id);
	if (has_titlebar)
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%POStitlebar\" class=\"wntitlebar\" >\n",id);
	    htrAddBodyItem_va(s,"<table border=0 cellspacing=0 cellpadding=0 height=%POS><tr><td><table cellspacing=0 cellpadding=0 border=0 width=%POS><tr><td align=left><TABLE cellspacing=0 cellpadding=0 border=\"0\"><TR><td width=26 align=center><img width=18 height=18 src=\"%STR&HTE\" name=\"icon\"></td><TD valign=\"middle\" nobreak><FONT COLOR='%STR&HTE'>&nbsp;<b>%STR&HTE</b></FONT></TD></TR></TABLE></td><td align=right><IMG src=\"/sys/images/01bigclose.gif\" name=\"close\" align=\"right\"></td></tr></table></td></tr></table>\n", 
		    tbh-1, tbw-2, icon, txtcolor, title);
	    htrAddBodyItem(s,   "</DIV>\n");
	    }
	htrAddBodyItem_va(s,"<DIV class=\"wnborder\"><DIV ID=\"wn%POSmain\">\n",id);

	/** Check for more sub-widgets within the page. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    htrRenderWidget(s, sub_tree, z+2);
	    }

	htrAddBodyItem(s,"</DIV></DIV></DIV>\n");

    return 0;
    }


/*** htwinInitialize - register with the ht_render module.
 ***/
int
htwinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Child Window Widget Driver");
	strcpy(drv->WidgetName,"childwindow");
	drv->Render = htwinRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Add the 'set visibility' action **/
	htrAddAction(drv,"ToggleVisibility");
	htrAddAction(drv,"SetVisibility");
	htrAddParam(drv,"SetVisibility","IsVisible",DATA_T_INTEGER);
	htrAddParam(drv,"SetVisibility","NoInit",DATA_T_INTEGER);

	/** Add the 'window closed' event **/
	htrAddEvent(drv,"Close");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTWIN.idcnt = 0;

    return 0;
    }
