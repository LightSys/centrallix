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
/* Module: 	htdrv_imagebutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 4, 1998  					*/
/* Description:	HTML Widget driver for an 'image button', or a button	*/
/*		comprised of a set of three images - one the default,	*/
/*		second the image when pointed to, and third the image	*/
/*		when clicked.						*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTIBTN;


/*** htibtnRender - generate the HTML code for the page.
 ***/
int
htibtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char n_img[128];
    char p_img[128];
    char c_img[128];
    char d_img[128];
    int is_enabled = 1;
    int button_repeat = 0;
    int x,y,w,h;
    int id, i;
    pExpression code;
    char* tooltip = NULL;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTIBTN","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTIBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Get normal, point, and click images **/
	if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'image' property");
	    return -1;
	    }
	strtcpy(n_img,ptr,sizeof(n_img));

	if (wgtrGetPropertyValue(tree,"pointimage",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(p_img,ptr,sizeof(p_img));
	else
	    strcpy(p_img, n_img);

	if (wgtrGetPropertyValue(tree,"clickimage",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(c_img,ptr,sizeof(c_img));
	else
	    strcpy(c_img, p_img);

	if (wgtrGetPropertyValue(tree,"disabledimage",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(d_img,ptr,sizeof(d_img));
	else
	    strcpy(d_img, n_img);

	if(wgtrGetPropertyValue(tree,"tooltip",DATA_T_STRING,POD(&ptr)) == 0)
	    tooltip=nmSysStrdup(ptr);
	else
	    tooltip=nmSysStrdup("");

	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	button_repeat = htrGetBoolean(tree, "repeat", 0);

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ib%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; cursor:pointer; }\n",id,x,y,w,z);

	htrAddScriptGlobal(s, "ib_cur_img", "null", 0);
	htrAddWgtrObjLinkage_va(s, tree, "ib%POSpane", id);

	htrAddScriptInclude(s, "/sys/js/htdrv_imagebutton.js", 0);

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	htrAddScriptInit_va(s,"    ib_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), n:'%STR&JSSTR', p:'%STR&JSSTR', c:'%STR&JSSTR', d:'%STR&JSSTR', width:%INT, height:%INT, name:'%STR&SYM', enable:%INT, tooltip:'%STR&JSSTR', repeat:%INT});\n",
	        name, n_img, p_img, c_img, d_img, w, h, name,is_enabled, tooltip, button_repeat);

	/** HTML body <DIV> elements for the layers. **/
	if (h < 0)
	    if(is_enabled)
		htrAddBodyItem_va(s,"<DIV ID=\"ib%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\"></DIV>\n",id,n_img);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\"></DIV>\n",id,d_img);
	else
	    if(is_enabled)
		htrAddBodyItem_va(s,"<DIV ID=\"ib%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\" width=\"%POS\" height=\"%POS\"></DIV>\n",id,n_img,w,h);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%POSpane\"><IMG SRC=\"%STR&HTE\" border=\"0\" width=\"%POS\" height=\"%POS\"></DIV>\n",id,d_img,w,h);

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "ib", "ib_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "ib", "ib_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "ib", "ib_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "ib", "ib_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "ib", "ib_mousemove");

	/** Check for more sub-widgets within the imagebutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	if (tooltip) nmSysFree(tooltip);

    return 0;
    }


/*** htibtnInitialize - register with the ht_render module.
 ***/
int
htibtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ImageButton Widget Driver");
	strcpy(drv->WidgetName,"imagebutton");
	drv->Render = htibtnRender;

	htrAddAction(drv,"Enable");
	htrAddAction(drv,"Disable");
	
	/** Add the 'click' event **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTIBTN.idcnt = 0;

    return 0;
    }
