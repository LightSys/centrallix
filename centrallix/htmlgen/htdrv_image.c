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

/**CVSDATA***************************************************************

    $Log: htdrv_image.c,v $
    Revision 1.1  2004/02/24 19:59:30  gbeeley
    - adding component-declaration widget driver
    - adding image widget driver
    - adding app-level presentation hints pseudo-widget driver

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTIMG;


/*** htimgVerify - not written yet.
 ***/
int
htimgVerify()
    {
    return 0;
    }


/*** htimgRender - generate the HTML code for the label widget.
 ***/
int
htimgRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char src[128];
    int x=-1,y=-1,w,h;
    int id;
    char* nptr;
    char *text;
    pObject sub_w_obj;
    pObjQuery qy;

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTTBL","Netscape DOM support or W3C DOM Level 1 HTML required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTIMG.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTIMG","Image widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'height' property");
	    return -1;
	    }

	if(objGetAttrValue(w_obj,"text",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    text=nmMalloc(strlen(ptr)+1);
	    strcpy(text,ptr);
	    }
	else
	    {
	    text=nmMalloc(1);
	    text[0]='\0';
	    }

	/** image source **/
	if (objGetAttrValue(w_obj,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'source' property");
	    return -1;
	    }
	snprintf(src,sizeof(src),"%s",ptr);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#img%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Event Handlers **/
	htrAddEventHandler(s, "document","MOUSEUP", "img", 
	    "\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'Click');\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "img", 
	    "\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "img", 
	    "\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'MouseOver');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEOUT", "img", 
	    "\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'MouseOut');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEMOVE", "img", 
	    "\n"
	    "    if (ly.kind == 'img') cn_activate(ly, 'MouseMove');\n"
	    "\n");

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItemLayer_va(s, 0, "img%d", id, 
	    "\n<img width=%d height=%d src=\"%s\">\n",w,h,src);

	/** Check for more sub-widgets **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
		{
		htrRenderWidget(s, sub_w_obj, z+1, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	nmFree(text,strlen(text)+1);

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
	drv->Verify = htimgVerify;

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
