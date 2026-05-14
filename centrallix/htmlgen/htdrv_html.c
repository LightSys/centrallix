#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/util.h"
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
/* Module: 	htdrv_html.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 3, 1998 					*/
/* Description:	HTML Widget driver for HTML text, either given as an	*/
/*		immediate string or pulled from an ObjectSystem source	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTHTML;


/*** hthtmlRender - generate the HTML code for the page.
 ***/
int
hthtmlRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;

	/** Get an id for this. **/
	const int id = (HTHTML.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTHTML", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Get x, y, w, & h. **/
	int x = -1, y = -1, w, h;
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=-1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=-1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTHTML","HTML widget must have a 'width' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get the HTML source path. **/
	char src[256] = "";
	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (strtcpy(src, ptr, sizeof(src)) < 0)
		{
		/** Data truncated: Send warning. **/
		fprintf(stderr, "Warning! Source path truncated.");
		}
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	int mode = 0;
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	char name[64];
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    /** Only give x and y if supplied. **/
	    if (htrAddStylesheetItem_va(s,
		"\t\t#ht%POSpane, #ht%POSpane2, #ht%POSfader { "
		    "position:absolute; "
		    "visibility:inherit; "
		    "overflow:hidden; "
		    "%[left:"ht_flex_format"; "
		    "top:"ht_flex_format"; %]"
		    "width:"ht_flex_format"; "
		    "z-index:%POS; "
		"}\n",
		id, id, id,
		(x < 0 || y < 0),
		ht_flex_x(x, tree),
		ht_flex_y(y, tree),
		ht_flex_w(w, tree),
		z
	    ) != 0)
		{
		mssError(0, "HTHTML", "Failed to add CSS.");
		goto err;
		}
	    if (htrAddStylesheetItem_va(s,
		"\t\t#ht%POSloader { "
		    "position:absolute; "
		    "visibility:hidden; "
		    "overflow:hidden; "
		    "top:0px; "
		    "left:0px; "
		    "width:0px; "
		    "height:0px; "
		"}\n",
		id
	    ) != 0)
		{
		mssError(0, "HTHTML", "Failed to add loader CSS.");
		goto err;
		}

	    /** Link the widget to the DOM node. **/
	    if (htrAddWgtrObjLinkage_va(s, tree, "ht%POSpane", id) != 0) goto err;

	    /** Declare globals. **/
	    if (htrAddScriptGlobal(s, "ht_fadeobj", "null", 0) != 0) goto err;
	    
	    /** Include JS dependencies. **/
	    if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	    if (htrAddScriptInclude(s, "/sys/js/htdrv_html.js", 0) != 0) goto err;
	    
	    /** Event handler for click-on-link. **/
	    if (htrAddEventHandlerFunction(s, "document", "CLICK",     "ht", "ht_click")     != 0) goto err;
	    if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "ht", "ht_mousedown") != 0) goto err;
	    if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "ht", "ht_mousemove") != 0) goto err;
	    if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "ht", "ht_mouseout")  != 0) goto err;
	    if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "ht", "ht_mouseover") != 0) goto err;
	    if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "ht", "ht_mouseup")   != 0) goto err;
	    
	    /** Write script initialization call in its own scope. **/
	    if (htrAddScriptInit_va(s, "\t{ "
		"const layer = wgtrGetNodeRef(ns, '%STR&SYM'); "
		"const layer_container = wgtrGetParentContainer(layer); "
		"ht_init({"
		    "layer, "
		    "layer2:htr_subel(layer_container, 'ht%POSpane2'), "
		    "faderLayer:htr_subel(layer_container, 'ht%POSfader'), "
		    "source:'%STR&JSSTR', "
		    "width:%INT, "
		    "height:%INT, "
		    "%[loader:htr_subel(layer_container, 'ht%POSloader'), %]"
		"}); }\n",
		name, id, id, src, w, h,
		(s->Capabilities.Dom1HTML), id
	    ) != 0)
		{
		mssError(0, "HTHTML", "Failed to write JS init call.");
		goto err;
		}
    
            /** HTML body <DIV> element for the layer. **/
            if (htrAddBodyItem_va(s,
		"<div "
		    "id='ht%POSfader' "
		    "background='/sys/images/fade_lrwipe_01.gif' "
		"></div>",
		id
	    ) != 0)
		{
		mssError(0, "HTHTML", "Failed to write HTML background fader.");
		goto err;
		}
	    if (htrAddBodyItemLayer_va(s, 0, "ht%POSpane2", id, NULL, "") != 0)
		{
		mssError(0, "HTHTML", "Failed to write HTML opening tag.");
		goto err;
		}
	    if (htrAddBodyItemLayerStart(s, 0, "ht%POSpane", id, NULL) != 0)
		{
		mssError(0, "HTHTML", "Failed to write HTML start tag.");
		goto err;
		}
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NOOBJECT;
	    }

        /** If prefix text given, put it. **/
        if (wgtrGetPropertyValue(tree, "prologue", DATA_T_STRING,POD(&ptr)) == 0)
            {
            if (htrAddBodyItem(s, ptr) != 0)
		{
		mssError(0, "HTHTML", "Failed to write prologue HTML.");
		goto err;
		}
            }

        /** If full text given, put it. **/
        if (wgtrGetPropertyValue(tree, "content", DATA_T_STRING,POD(&ptr)) == 0)
            {
            if (htrAddBodyItem(s, ptr) != 0)
		{
		mssError(0, "HTHTML", "Failed to write provided HTML content.");
		goto err;
		}
            }

        /** If source is an objectsystem entry... **/
        if (src[0] && strncmp(src, "http:", 5) != 0 && strncmp(src, "debug:", 6) != 0)
            {
	    pObject content_obj = NULL;
	    char* page_buf = NULL;
	    bool successful = false;

	    /** Open the content object. **/
	    content_obj = objOpen(s->ObjSession, src, O_RDONLY, 0600, "text/html");
	    if (content_obj == NULL)
		{
		mssError(0, "HTHTML", "Failed to open content object.");
		goto end_reading;
		}
	    
	    /** Get object info. **/
	    pObjectInfo obj_info = objInfo(content_obj);
	    if (obj_info == NULL)
		{
		mssError(0, "HTHTML", "Failed to get content object info.");
		goto end_reading;
		}
	    
	    /** Inspect object info. **/
	    if (obj_info->Flags & OBJ_INFO_F_CANT_HAVE_CONTENT)
		{
		mssError(1, "HTHTML", "FAIL: Provided object source cannot have content!");
		goto err;
		}
	    if (obj_info->Flags & OBJ_INFO_F_NO_CONTENT)
		{
		/** Nothing to read. **/
		successful = true;
		goto end_reading;
		}

	    /** Allocate a buffer for reading HTML content. **/
	    const size_t page_buf_len = BUFSIZ;
	    page_buf = check_ptr(nmSysMalloc(page_buf_len * sizeof(char*)));
	    
	    /* read content until we run out.*/
	    int n_chars_read = 0;
	    while (1)
		{
		/** Read some data. **/
		const int max_read = page_buf_len - 1lu;
		n_chars_read = objRead(content_obj, page_buf, max_read, 0, 0);
		
		/** Edge cases. **/
		if (n_chars_read == 0) break; /* Done! */
		if (n_chars_read < 0)
		    {
		    char error_code[32] = {'\0'};
		    if (n_chars_read != -1)
			snprintf(error_code, sizeof(error_code), " (error code: %d)", n_chars_read);
		    mssError(0, "HTHTML", "Failed to read from content object%s.", error_code);
		    goto end_reading;
		    }
		if (n_chars_read > max_read)
		    {
		    mssError(1, "HTHTML",
			"Driver read too many characters (%d/%d).",
			n_chars_read, max_read
		    );
		    goto end_reading;
		    }
		
		/** Write the data to the page. **/
		page_buf[n_chars_read] = '\0';
		fprintf(stderr, "Got: \"%s\"\n", page_buf);
		if (htrAddBodyItem(s, page_buf) != 0)
		    {
		    mssError(0, "HTHTML", "Failed to write HTML chunk: \"%s\"", page_buf);
		    goto end_reading;
		    }
		}
	    
	    /** Success. **/
	    successful = true;
	    
    end_reading:
	    /** Clean up. **/
	    if (content_obj != NULL) objClose(content_obj);
	    if (page_buf != NULL) nmSysFree(page_buf);
	    
	    /** Handle failure. **/
	    if (!successful)
		{
		mssError(0, "HTHTML", "Failed to write HTML from object source: \"%s\"", src);
		goto err;
		}
            }

        /** If post text given, put it. **/
        if (wgtrGetPropertyValue(tree, "epilogue", DATA_T_STRING, POD(&ptr)) == 0)
            {
            if (htrAddBodyItem(s, ptr) != 0)
		{
		mssError(0, "HTHTML", "Failed to write epilogue HTML.");
		goto err;
		}
            }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 3) != 0) goto err;

        /** End the containing layer. **/
        if (mode == 1 && htrAddBodyItemLayerEnd(s, 0) != 0)
	    {
	    mssError(0, "HTHTML", "Failed to add HTML end.");
	    goto err;
	    }
	
	/** Success. **/
	return 0;

    err:
	mssError(0, "HTHTML",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** hthtmlInitialize - register with the ht_render module.
 ***/
int
hthtmlInitialize()
    {
    pHtDriver drv;

        /** Allocate the driver **/
        drv = htrAllocDriver();
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"HTML Textual Source Driver");
        strcpy(drv->WidgetName,"html");
        drv->Render = hthtmlRender;

	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

        /** Add the 'load page' action **/
	htrAddAction(drv,"LoadPage");
	htrAddParam(drv,"LoadPage","Mode",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Source",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Transition",DATA_T_STRING);

        /** Register. **/
        htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

        HTHTML.idcnt = 0;

    return 0;
    }
