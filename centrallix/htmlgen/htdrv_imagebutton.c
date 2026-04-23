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
    int rval = -1;
    char* tooltip = NULL;

	/** Get an id for this. **/
	const int id = (HTIBTN.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTIMG", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto end;
	    }
	
    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'x' property");
	    goto end;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'y' property");
	    goto end;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'width' property");
	    goto end;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto end;
	strtcpy(name,ptr,sizeof(name));

	/** Get normal, point, and click images **/
	if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'image' property");
	    goto end;
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
	if (htrAddStylesheetItem_va(s,
	    "\t\t#ib%POSpane { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"z-index:%POS; "
	    "}\n",
	    id,
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    z
	) != 0)
	    {
	    mssError(0, "HTIBTN", "Failed to write main CSS.");
	    goto end;
	    }

	/** Setup JS. **/
	if (htrAddScriptGlobal(s, "ib_cur_img", "null", 0) != 0) goto end;
	if (htrAddWgtrObjLinkage_va(s, tree, "ib%POSpane", id) != 0) goto end;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_imagebutton.js", 0) != 0) goto end;

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    pExpression code;
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Write script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\tib_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"name:'%STR&SYM', "
		"n:'%STR&JSSTR', "
		"p:'%STR&JSSTR', "
		"c:'%STR&JSSTR', "
		"d:'%STR&JSSTR', "
		"width:%INT, "
		"height:%INT, "
		"enable:%INT, "
		"tooltip:'%STR&JSSTR', "
		"repeat:%INT, "
	    "});\n",
	    name, name,
	    n_img, p_img, c_img, d_img,
	    w, h,
	    is_enabled, tooltip, button_repeat
	) != 0)
	    {
	    mssError(0, "HTIBTN", "Failed to write JS init call.");
	    goto end;
	    }

	/** Write HTML. **/
	if (htrAddBodyItem_va(s,
	    "<div id='ib%POSpane'>"
		"<img src='%STR&HTE' border='0' %[width='%POS' height='%POS'%]>"
	    "</div>\n",
	    id, (is_enabled) ? n_img : d_img, (h >= 0), w, h
	) != 0)
	    {
	    mssError(0, "HTBTN",
		"Failed to write HTML for %sabled image button.",
		(is_enabled) ? "en" : "dis"
	    );
	    goto end;
	    }

	/** Add the event handling scripts **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "ib", "ib_mousedown") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "ib", "ib_mousemove") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "ib", "ib_mouseout")  != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "ib", "ib_mouseover") != 0) goto end;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "ib", "ib_mouseup")   != 0) goto end;

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto end;

	/** Success. **/
	rval = 0;

    end:
	if (rval != 0)
	    {
	    mssError(0, "HTIBTN",
		"Failed to render \"%s\":\"%s\" (id: %d).",
		tree->Name, tree->Type, id
	    );
	    }
	
	/** Clean up. **/
	if (tooltip != NULL) nmSysFree(tooltip);
	
	return rval;
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
