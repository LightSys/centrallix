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
/* Module: 	htdrv_image.c						*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 2004 					*/
/* Description:	HTML Widget driver for an image.           		*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTIMG;


/*** htimgRender - generate the HTML code for the label widget.
 ***/
int
htimgRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char src[256];
    int x=-1,y=-1,w,h;
    int id, i;
    char *text;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTTBL","Netscape DOM support or W3C DOM Level 1 HTML required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTIMG.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTIMG","Image widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'height' property");
	    return -1;
	    }

	if(wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    text=nmSysStrdup(ptr);
	    }
	else
	    {
	    text=nmSysStrdup("");
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** image source **/
	/*if (!htrCheckAddExpression(s, tree, name, "source") && wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'source' property");
	    nmSysFree(text);
	    return -1;
	    }*/
	ptr = "";
	htrCheckAddExpression(s, tree, name, "source");
	wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr));
	strtcpy(src, ptr, sizeof(src));

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
	htrAddStylesheetItem_va(s,"\t#img%POS { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);

	/** Init image widget (?) **/
	htrAddWgtrObjLinkage_va(s, tree, "img%POS",id);
	htrAddScriptInit_va(s, "    im_init(wgtrGetNodeRef(ns,'%STR&SYM'), {field:'%STR&JSSTR', form:'%STR&JSSTR'});\n", 
		name, fieldname, form);
	htrAddScriptInclude(s, "/sys/js/htdrv_image.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document","MOUSEUP", "img", "im_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "img", "im_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "img", "im_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "img", "im_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "img", "im_mousemove");

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItemLayer_va(s, 0, "img%POS", id, 
	    "\n<img id=im%POS width=%POS height=%POS src=\"%STR&HTE\" style=\"display:block;\">\n",id,w,h,src);

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	nmSysFree(text);

    return 0;
    }


/*** htimgInitialize - register with the ht_render module.
 ***/
int
htimgInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Image Widget");
	strcpy(drv->WidgetName,"image");
	drv->Render = htimgRender;

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

	HTIMG.idcnt = 0;

    return 0;
    }
