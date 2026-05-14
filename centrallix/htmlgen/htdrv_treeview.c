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
/* Module: 	htdrv_treeview.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 1, 1998 					*/
/* Description:	HTML Widget driver for a treeview.			*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTREE;


/*int
httree_internal_AddRule(pHtSession s, pWgtrNode parent, char* name, pWgtrNode child)
    {
    char* ruletype;
    }*/


/*** httreeRender - generate the HTML code for the page.
 ***/
int
httreeRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char src[128];
    char icon[128];
    char fgcolor[64];
    char hfgcolor[64];
    char selected_bg[128];
    int x,y,w;
    int show_root = 1;
    int show_branches = 1;
    int show_root_branch = 1;
    int use_3d_lines;
    int order_desc = 0;

	/** Get an id for this. **/
	const int id = (HTTREE.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTTERM", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTREE","TreeView widget must have an 'x' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'y' property");
	    goto err;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'width' property");
	    goto err;
	    }

	/** Are we showing root of tree or the trunk? **/
	show_root = htrGetBoolean(tree, "show_root", 1);
	if (show_root < 0) goto err;

	/** How about branches? (branch decorations, etc.) **/
	show_branches = htrGetBoolean(tree, "show_branches", 1);

	/** If not showing root, do we show the root branch? **/
	show_root_branch = htrGetBoolean(tree, "show_root_branch", show_root);

	/** 3-D lines or simple? **/
	use_3d_lines = htrGetBoolean(tree, "use_3d_lines", 1);

	/** descending order **/
	if (wgtrGetPropertyValue(tree, "order", DATA_T_STRING, POD(&ptr)) == 0 && !strcasecmp(ptr, "desc"))
	    order_desc = 1;

	/** Compensate hidden root position if not shown **/
	if (!show_root)
	    {
	    if (use_3d_lines)
		{
		if (!show_branches && !show_root_branch) x -= 20;
		y -= 20;
		}
	    else
		{
		if (!show_branches && !show_root_branch) x -= 16;
		y -= 16;
		}
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));

	/** Selected item background color **/
	strcpy(selected_bg,"");
	htrGetBackground(tree, "highlight", s->Capabilities.CSS2?1:0, selected_bg, sizeof(selected_bg));

	/** Get foreground (text) color **/
	if (wgtrGetPropertyValue(tree,"fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor,ptr,sizeof(fgcolor));
	else
	    fgcolor[0] = '\0';

	/** Get highlight foreground (text) color **/
	if (wgtrGetPropertyValue(tree,"highlight_fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(hfgcolor,ptr,sizeof(hfgcolor));
	else
	    hfgcolor[0] = '\0';

	/** Get icon to use for treeview objects **/
	if (wgtrGetPropertyValue(tree,"icon",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(icon,ptr,sizeof(icon));
	else
	    icon[0] = '\0';

	/** Get source directory tree **/
	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'source' property");
	    goto err;
	    }
	strtcpy(src,ptr,sizeof(src));

	/** Write CSS. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tv%POSload { "
		"position:absolute; "
		"visibility:hidden; "
		"overflow:hidden; "
		"left:0px; "
		"top:0px; "
		"width:0px; "
		"height:0px; "
		"z-index:0; "
	    "}\n",
	    id
	) != 0)
	    {
	    mssError(0, "HTTREE", "Failed to write treeview loader CSS.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\tdiv.tv%POS  a { %[color:%STR&CSSVAL;%] }\n"
	    "\t\tdiv.tv%POSh a { %[color:%STR&CSSVAL;%] }\n"
	    "\t\t.tv%POS { cursor:pointer; }\n",
	    id, (*fgcolor),  fgcolor,
	    id, (*hfgcolor), hfgcolor,
	    id
	) != 0)
	    {
	    mssError(0, "HTTREE", "Failed to write treeview entry CSS.");
	    goto err;
	    }

 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "tv%POSroot", id) != 0) goto err;

	/** Write globals for internal use **/
	if (htrAddScriptGlobal(s, "tv_alloc_cnt",   "0",    0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tv_cache_cnt",   "0",    0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tv_layer_cache", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tv_target_img",  "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tv_tgt_layer",   "null", 0) != 0) goto err;

	/** Write JS script includes. **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_info.js",   0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_treeview.js",  0) != 0) goto err;

	/** Add the event handling scripts **/
	if (htrAddEventHandlerFunction(s, "document", "CLICK",     "tv", "tv_click")     != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tv", "tv_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tv", "tv_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "tv", "tv_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tv", "tv_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "tv", "tv_mouseup")   != 0) goto err;

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "\t{ "
	    "const layer = wgtrGetNodeRef(ns, '%STR&SYM'); "
	    "tv_init({ "
		"layer, "
		"loader:htr_subel(wgtrGetParentContainer(layer),'tv%POSload'), "
		"divclass:'tv%POS', "
		"fname:'%STR&JSSTR', "
		"width:%INT, "
		"newroot:null, "
		"branches:%INT, "
		"use3d:%INT, "
		"showrb:%INT, "
		"icon:'%STR&JSSTR', "
		"sbg:'%STR&JSSTR', "
		"desc:%INT, "
	    "}); }\n",
	    name, id, id, src, w,
	    show_branches, use_3d_lines, show_root_branch,
	    icon, selected_bg, order_desc
	);

	/** Write HTML. **/
	if (htrAddBodyItem_va(s,
	    "<div "
		"class='tv%POS' "
		"id='tv%POSroot' "
		"style='"
		    "position:absolute; "
		    "visibility:%STR; "
		    "left:"ht_flex_format"; "
		    "top:"ht_flex_format"; "
		    "width:"ht_flex_format"; "
		    "z-index:%POS; "
		"'"
	    ">"
		"<img src='%STR&HTE' alt='folder'>"
		"&nbsp;%STR&HTE"
	    "</div>\n",
	    id, /* class */
	    id, /* id */
	    (show_root) ? "inherit" : "hidden",
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    z,
	    (*icon) ? icon : "/sys/images/ico02b.gif", src
	) != 0) 
	    {
	    mssError(0, "HTTREE", "Failed to write HTML for treeview root.");
	    goto err;
	    }
	if (htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "tv%POSload", id, NULL, "") != 0)
	    {
	    mssError(0, "HTTREE", "Failed to write HTML for treeview loader.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 2) != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTTM",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
    }


/*** httreeInitialize - register with the ht_render module.
 ***/
int
httreeInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Treeview Widget Driver");
	strcpy(drv->WidgetName,"treeview");
	drv->Render = httreeRender;

	/** Add the 'click item' event **/
	htrAddEvent(drv,"ClickItem");
	htrAddParam(drv,"ClickItem","Pathname",DATA_T_STRING);
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Add the 'rightclick item' event **/
	htrAddEvent(drv,"RightClickItem");
	htrAddParam(drv,"RightClickItem","Pathname",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTREE.idcnt = 0;

    return 0;
    }
