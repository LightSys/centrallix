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
    Revision 1.2  2004/07/19 15:30:40  mmcgill
    The DHTML generation system has been updated from the 2-step process to
    a three-step process:
        1)	Upon request for an application, a widget-tree is built from the
    	app file requested.
        2)	The tree is Verified (not actually implemented yet, since none of
    	the widget drivers have proper Verify() functions - but it's only
    	a matter of a function call in net_http.c)
        3)	The widget drivers are called on their respective parts of the
    	tree structure to generate the DHTML code, which is then sent to
    	the user.

    To support widget tree generation the WGTR module has been added. This
    module allows OSML objects to be parsed into widget-trees. The module
    also provides an API for building widget-trees from scratch, and for
    manipulating existing widget-trees.

    The Render functions of all widget drivers have been updated to make their
    calls to the WGTR module, rather than the OSML, and to take a pWgtrNode
    instead of a pObject as a parameter.

    net_internal_GET() in net_http.c has been updated to call
    wgtrParseOpenObject() to make a tree, pass that tree to htrRender(), and
    then free it.

    htrRender() in ht_render.c has been updated to take a pWgtrNode instead of
    a pObject parameter, and to make calls through the WGTR module instead of
    the OSML where appropriate. htrRenderWidget(), htrRenderSubwidgets(),
    htrGetBoolean(), etc. have also been modified appropriately.

    I have assumed in each widget driver that w_obj->Session is equivelent to
    s->ObjSession; in other words, that the object being passed in to the
    Render() function was opened via the session being passed in with the
    HtSession parameter. To my understanding this is a valid assumption.

    While I did run through the test apps and all appears to be well, it is
    possible that some bugs were introduced as a result of the modifications to
    all 30 widget drivers. If you find at any point that things are acting
    funny, that would be a good place to check.

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
htimgRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char src[128];
    int x=-1,y=-1,w,h;
    int id, i;
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
	    text=nmMalloc(strlen(ptr)+1);
	    strcpy(text,ptr);
	    }
	else
	    {
	    text=nmMalloc(1);
	    text[0]='\0';
	    }

	/** image source **/
	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'source' property");
	    return -1;
	    }
	snprintf(src,sizeof(src),"%s",ptr);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
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
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1, parentname, nptr);

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
