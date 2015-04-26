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
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_textbutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 5, 1998  					*/
/* Description:	HTML Widget driver for a 'text button', which frames a	*/
/*		text string in a 3d-like box that simulates the 3d	*/
/*		clicking action when the user points and clicks.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBTN;


/*** httbtnRender - generate the HTML code for the page.
 ***/
int
httbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgstyle[128];
    char disable_color[64];
    int x,y,w,h;
    int id, i;
    int is_ts = 1;
    int is_enabled = 1;
    pExpression code;
    int box_offset;
    int clip_offset;
    int border_radius;
    char border_style[32];
    char border_color[64];
    char image_position[16]; /* top, left, right, bottom */
    char image[OBJSYS_MAX_PATH];
    char h_align[16];
    int image_width=0, image_height=0, image_margin=0;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTBTN","Netscape DOM or (W3C DOM1 HTML and W3C DOM2 CSS) support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Border radius, color, and style.  For style, we only support outset,
	 ** solid, and none here.
	 **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;
	if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_color, "#c0c0c0");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree,"border_style",DATA_T_STRING,POD(&ptr)) != 0 || (strcmp(ptr,"outset") && strcmp(ptr,"solid") && strcmp(ptr,"none")))
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));

	/** Alignment **/
	if (wgtrGetPropertyValue(tree,"align",DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"left") || !strcmp(ptr,"right") || !strcmp(ptr,"center")))
	    strtcpy(h_align, ptr, sizeof(h_align));
	else
	    strcpy(h_align, "center");

	/** Image location **/
	if (wgtrGetPropertyValue(tree,"image_position",DATA_T_STRING,POD(&ptr)) != 0 || (strcmp(ptr,"top") && strcmp(ptr,"right") && strcmp(ptr,"bottom") && strcmp(ptr, "left")))
	    strcpy(image_position, "top");
	else
	    strtcpy(image_position, ptr, sizeof(image_position));

	/** Image source **/
	if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(image, "");
	else
	    strtcpy(image, ptr, sizeof(image));

	/** Image sizing **/
	if (wgtrGetPropertyValue(tree,"image_width",DATA_T_INTEGER, POD(&image_width)) != 0)
	    image_width = 0;
	if (wgtrGetPropertyValue(tree,"image_height",DATA_T_INTEGER, POD(&image_height)) != 0)
	    image_height = 0;
	if (wgtrGetPropertyValue(tree,"image_margin",DATA_T_INTEGER, POD(&image_margin)) != 0)
	    image_margin = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** box adjustment... arrgh **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;
	clip_offset = s->Capabilities.CSSClip?1:0;

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Threestate button or twostate? **/
	if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	ptr = "-";
	if (!htrCheckAddExpression(s, tree, name, "text") && wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	strtcpy(text,ptr,sizeof(text));

	/** Get fgnd colors 1,2, and background color **/
	htrGetBackground(tree, NULL, 1, bgstyle, sizeof(bgstyle));

	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
	else
	    strcpy(fgcolor1,"white");
	if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
	else
	    strcpy(fgcolor2,"black");
	if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(disable_color,ptr,sizeof(disable_color));
	else
	    strcpy(disable_color,"#808080");

	htrAddScriptGlobal(s, "tb_current", "null", 0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "tb%POSpane",id);

	/** Include the javascript code for the textbutton **/
	htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Initial CSS styles **/
	htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; %[HEIGHT:%POSpx; %]WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; display:table; }\n",
		id,
		x, y, h>=0, h-1-2*box_offset, w-1-2*box_offset, z
		);
	htrAddStylesheetItem_va(s, "\t#tb%POSpane .cell { height:100%%; width:100%%; vertical-align:middle; display:table-cell; padding:1px; font-weight:bold; cursor:default; text-align:%STR; border-width:1px; border-style:%STR&CSSVAL; border-color:%STR&CSSVAL; border-radius:%INTpx; color:%STR&CSSVAL; %[text-shadow:1px 1px %STR&CSSVAL; %]%STR }\n",
		/* clipping no longer needed:  0, w-1-2*box_offset+2*clip_offset, h-1-2*box_offset+2*clip_offset, 0, */
		id,
		h_align,
		border_style, border_color, border_radius,
		is_enabled?fgcolor1:disable_color, is_enabled, fgcolor2,
		bgstyle
		);

	/** CSS for image on the button **/
	if (image[0] && (image_width || image_height || image_margin))
	    {
	    htrAddStylesheetItem_va(s, "\t#tb%POSpane img { %[height:%POSpx; %]%[width:%POSpx; %]%[margin:%POSpx;%] }\n",
		    id,
		    image_height, image_height,
		    image_width, image_width,
		    image_margin, image_margin);
	    }

#if 00
	if(h >=0 )
	    {
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; HEIGHT:%POSpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx %INTpx %INTpx)}\n", id, x, y, h-1-2*box_offset, w-1-2*box_offset, z, 0, w-1-2*box_offset+2*clip_offset, h-1-2*box_offset+2*clip_offset, 0);
	    }
	else
	    {
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx auto %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,0);
	    }
	htrAddStylesheetItem_va(s,"\t#tb%POSpane { cursor:default; text-align: center; %STR border-width: 1px; border-style: solid; border-color: white gray gray white; border-radius: %INTpx; }\n",id,bgstyle, border_radius);
	if (is_enabled)
	    htrAddStylesheetItem_va(s,"\t#tb%POSspan { color:%STR&CSSVAL; text-shadow:1px 1px %STR&CSSVAL; }\n", id, fgcolor1, fgcolor2);
	else
	    htrAddStylesheetItem_va(s,"\t#tb%POSspan { color:%STR&CSSVAL; }\n", id, disable_color);
#endif

	//htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><span id=\"tb%POSspan\"><b>%STR&HTE</b></span></td></tr></table></center>\n", id, h-3, id, text);
	//htrAddBodyItem(s,   "</DIV>");

	/** We need two DIVs here because of a long-outstanding Firefox bug :( **/
	htrAddBodyItem_va(s,"<div id=\"tb%POSpane\"><div class=\"cell\">%[<img border=\"0\" src=\"%STR&HTE\"/><br>%]%[<img border=\"0\" src=\"%STR&HTE\" style=\"vertical-align:middle;\"/>%]<span>%STR&HTE</span>%[<img border=\"0\" src=\"%STR&HTE\" style=\"vertical-align:middle;\"/>%]%[<br><img border=\"0\" src=\"%STR&HTE\"/>%]</div></div>", 
		id,
		image[0] && !strcmp(image_position, "top"), image,
		image[0] && !strcmp(image_position, "left"), image,
		text,
		image[0] && !strcmp(image_position, "right"), image,
		image[0] && !strcmp(image_position, "bottom"), image
		);

	/** Script initialization call. **/
	//htrAddScriptInit_va(s, "    tb_init({layer:wgtrGetNodeRef(ns,'%STR&SYM'), span:document.getElementById(\"tb%POSspan\"), ena:%INT, c1:\"%STR&JSSTR\", c2:\"%STR&JSSTR\", dc1:\"%STR&JSSTR\", top:null, bottom:null, right:null, left:null, width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		//name, id, is_enabled, fgcolor1, fgcolor2, disable_color, w, h, is_ts, name, text);
	htrAddScriptInit_va(s, "    tb_init({layer:wgtrGetNodeRef(ns,'%STR&SYM'), ena:%INT, c1:\"%STR&JSSTR\", c2:\"%STR&JSSTR\", dc1:\"%STR&JSSTR\", top:null, bottom:null, right:null, left:null, width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		name, is_enabled, fgcolor1, fgcolor2, disable_color, w, h, is_ts, name, text);

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tb", "tb_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "tb", "tb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tb", "tb_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "tb", "tb_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tb", "tb_mousemove");

	/** IE handles dblclick strangely **/
	if (s->Capabilities.Dom0IE)
	    htrAddEventHandlerFunction(s, "document", "DBLCLICK", "tb", "tb_dblclick");

	/** Check for more sub-widgets within the textbutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

    return 0;
    }


/*** httbtnInitialize. - register with the ht_render module.
 ***/
int
httbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Text Button Widget Driver");
	strcpy(drv->WidgetName,"textbutton");
	drv->Render = httbtnRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBTN.idcnt = 0;

    return 0;
    }
