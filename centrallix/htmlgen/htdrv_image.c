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
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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


/*** htimgSetup - do overall setup work to cover all image widgets.
 ***/
int
htimgSetup(pHtSession s)
    {

	/** Global style code for all image widget "img" tags **/
	htrAddStylesheetItem(s,
	    "\t\timg.wimage { "
		"display:block; "
		"position:relative; "
		"left:0px; "
		"top:0px; "
	    "}\n"
	);

	/** Global style code for all image widget "div" containers **/
	htrAddStylesheetItem(s,
	    "\t\tdiv.wimage { "
		"visibility:inherit; "
		"position:absolute; "
		"overflow:hidden; "
	    "}\n"
	);

    return 0;
    }


/*** htimgRender - generate the HTML code for the label widget.
 ***/
int
htimgRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char src[256];
    int x=-1,y=-1,w,h;
    int rval = -1;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    char* alt_text = NULL;
    char* aspect = NULL;
    char* default_alt_text = "";
    char* default_aspect = "stretch";

	/** Get an id for this. **/
	const int id = (HTIMG.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTIMG", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto end;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTIMG","Image widget must have a 'width' property");
	    goto end;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTIMG","Image widget must have a 'height' property");
	    goto end;
	    }

	if(wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) == 0)
	    alt_text=nmSysStrdup(ptr);
	if (alt_text == NULL) alt_text = default_alt_text;

	/** Image aspect scaling: stretch or preserve **/
	if(wgtrGetPropertyValue(tree,"aspect",DATA_T_STRING,POD(&ptr)) == 0)
	    aspect=nmSysStrdup(ptr);
	if (aspect == NULL) aspect = default_aspect;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto end;
	strtcpy(name,ptr,sizeof(name));

	/** image source **/
	ptr = "";
	htrCheckAddExpression(s, tree, name, "source");
	wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr));
	strtcpy(src, ptr, sizeof(src));

	htrCheckAddExpression(s, tree, name, "scale");
	htrCheckAddExpression(s, tree, name, "xoffset");
	htrCheckAddExpression(s, tree, name, "yoffset");

	/** Field name **/
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "img%POS", id) != 0) goto end;
	
	/** Initialize image scripts. **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_image.js", 0) != 0) goto end;
	if (htrAddScriptInit_va(s,
	    "\tim_init(wgtrGetNodeRef(ns, '%STR&SYM'), { "
		"field:'%STR&JSSTR', "
		"form:'%STR&JSSTR', "
	    "});\n", 
	    name,
	    fieldname, form
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write JS init call.");
	    goto end;
	    }

	/** Event Handlers **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "img", "im_mousedown") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "img", "im_mousemove") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "img", "im_mouseout") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "img", "im_mouseover") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "img", "im_mouseup") != 0) goto end;
	
	/** Write the style for the image container div. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#img%POS { "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
		"text-align:center; "
	    "}\n",
	    id,
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    ht_flex_h(h, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write image CSS.");
	    goto end;
	    }

	/** Use a style that honors the aspect ratio attribute. **/
	char* style = (strcmp(aspect, "stretch") == 0)
	    ? /* "stretch" */
	    "width:100%; "
	    "height:100%; "
	    : /* "preserve" */
	    "width:100%; "
	    "height:auto; "
	    "max-width:fit-content; "
	    "max-height:fit-content; "
	    "display:inline; ";
	
	/** Write image HTML, including the containing div. **/
	if (htrAddBodyItemLayer_va(s, 0,
	    "img%POS", id, "wimage",
	    "\n<img "
		"class='wimage' "
		"id='im%POS' "
		"width='%POS' "
		"height='%POS' "
		"style='%STR' "
		"src='%STR&HTE' "
		"alt='%STR&HTE' "
	    ">\n",
	    id,
	    w,
	    h,
	    style,
	    src,
	    alt_text
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write image HTML.");
	    goto end;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto end;

	/** Success. **/
	rval = 0;

    end:
	if (rval != 0)
	    {
	    mssError(0, "HTIMG",
		"Failed to render \"%s\":\"%s\" (id: %d).",
		tree->Name, tree->Type, id
	    );
	    }
	
	/** Clean up. **/
	if (alt_text != NULL && alt_text != default_alt_text) nmSysFree(alt_text);
	if (aspect != NULL && aspect != default_aspect) nmSysFree(aspect);
	
	return rval;
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
	drv->Setup = htimgSetup;
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
