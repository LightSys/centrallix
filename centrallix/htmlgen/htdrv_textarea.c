#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
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
/* Module: 	htdrv_textarea.c         				*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	July 9, 2002						*/
/* Description:	HTML Widget driver for a multi-line textarea.		*/
/************************************************************************/



/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTX;


/*** httxRender - generate the HTML code for the textarea widget.
 ***/
int
httxRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char elementid[16];
    //char main_bg[128];
    int x=-1,y=-1,w,h;
    int is_readonly = 0;
    int is_raised = 0;
    int mode = 0; /* 0=text, 1=html, 2=wiki */
    int max_chars;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];

	/** Get an id for this. **/
	const int id = (HTTX.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTTX", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'width' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'height' property");
	    goto err;
	    }
	
	/** Maximum characters to accept from the user.  Negative means no limit. **/
	if (wgtrGetPropertyValue(tree,"max_chars",DATA_T_INTEGER,POD(&max_chars)) != 0) max_chars = -1;

	/** Readonly flag **/
	is_readonly = htrGetBoolean(tree, "readonly", 0);

	/** Allow HTML? **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"text")) mode = 0;
	    else if (!strcasecmp(ptr,"html")) mode = 1;
	    else if (!strcasecmp(ptr,"wiki")) mode = 2;
	    else
		{
		mssError(1,"HTTX","Textarea widget 'mode' property must be either 'text','html', or 'wiki'");
		goto err;
		}
	    }

	/** Background color/image? **/
	//htrGetBackground(tree, NULL, 1, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));

	/** Style of Textarea - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"raised")) is_raised = 1;

	/** Form linkage **/
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else 
	    fieldname[0]='\0';

	/** Write Style header items. **/
	snprintf(elementid, sizeof(elementid), "#tx%dbase", id);
	if (htrFormatElement(s, tree, elementid, 0,
	    x, y, w, h, z, "",
	    (char*[]){"border_color","#e0e0e0", "border_style", (is_raised ? "outset" : "inset"), NULL},
	    "position:absolute; "
	    "overflow:hidden; "
	) != 0)
	    {
	    mssError(0, "HTTX", "Failed to write styles.");
	    goto err;
	    }

	/** Link DOM node to widget data. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "tx%POSbase", id) != 0) goto err;

	/** Write JS globals. **/
	if (htrAddScriptGlobal(s, "text_metric",      "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tx_current",       "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tx_cur_mainlayer", "null", 0) != 0) goto err;

	/** Write JS script includes. **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_textarea.js",  0) != 0) goto err;

	/** Register JS event handlers. **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tx", "tx_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tx", "tx_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tx", "tx_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "tx", "tx_mouseup")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "PASTE",     "tx", "tx_paste")     != 0) goto err;
	    
	/** Script initialization call. **/
	if (htrAddScriptInit_va(s,
	    "\ttx_init({ "
	        "layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"fieldname:'%STR&JSSTR', "
		"form:'%STR&JSSTR', "
		"isReadonly:%INT, "
		"mode:%INT, "
	    "});\n",
	    name, fieldname, form, is_readonly, mode
	) != 0)
	    {
	    mssError(0, "HTTX", "Failed to write JS init call.");
	    goto err;
	    }

	/** Write HTML container opening tag. **/
	if (htrAddBodyItem_va(s, "<div id='tx%POSbase'>", id) != 0)
	    {
	    mssError(0, "HTTX", "Failed to write HTML container opening tag.");
	    goto err;
	    }
	
	/** Write HTML text area opening tag. **/
	if (htrAddBodyItem_va(s,
	    "<textarea%[ maxlength='%POS'%] style='"
		"width:100%%; "
		"height:100%%; "
		"border:none; "
		"outline:none; "
		"font-family:inherit; "
		"font-size:inherit; "
	    "'></textarea>\n",
	    (max_chars > 0), max_chars
	) != 0)
	    {
	    mssError(0, "HTTX", "Failed to write HTML textarea tags.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Write HTML to close containers. **/
	if (htrAddBodyItem(s, "</div>") != 0)
	    {
	    mssError(0, "HTTX", "Failed to write HTML container closing tags.");
	    goto err;
	    }

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTTX",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** httxInitialize - register with the ht_render module.
 ***/
int
httxInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Multiline Textarea Driver");
	strcpy(drv->WidgetName,"textarea");
	drv->Render = httxRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTX.idcnt = 0;

    return 0;
    }
