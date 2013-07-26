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
    int id, i;
    int show_root = 1;
    int show_branches = 1;
    int show_root_branch = 1;
    int use_3d_lines;
    int order_desc = 0;
    pWgtrNode sub_tree;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTREE","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTREE.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTREE","TreeView widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'width' property");
	    return -1;
	    }

	/** Are we showing root of tree or the trunk? **/
	show_root = htrGetBoolean(tree, "show_root", 1);
	if (show_root < 0) return -1;

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
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
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
	    return -1;
	    }
	strtcpy(src,ptr,sizeof(src));

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,"\t#tv%POSroot { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,show_root?"inherit":"hidden",x,y,w,z);
	    }
	htrAddStylesheetItem_va(s,"\t#tv%POSload { POSITION:absolute; VISIBILITY:hidden; OVERFLOW:hidden; LEFT:0px; TOP:0px; WIDTH:0px; HEIGHT:0px; clip:rect(0px,0px,0px,0px); Z-INDEX:0; }\n",id);
	htrAddStylesheetItem_va(s,"\tdiv.tv%POS a { %[color:%STR&CSSVAL;%] }\n", id, *fgcolor, fgcolor);
	htrAddStylesheetItem_va(s,"\tdiv.tv%POSh a { %[color:%STR&CSSVAL;%] }\n", id, *hfgcolor, hfgcolor);

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "tv_tgt_layer", "null", 0);
	htrAddScriptGlobal(s, "tv_target_img","null",0);
	htrAddScriptGlobal(s, "tv_layer_cache","null",0);
	htrAddScriptGlobal(s, "tv_alloc_cnt","0",0);
	htrAddScriptGlobal(s, "tv_cache_cnt","0",0);

	/** DOM Linkage on client **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tv%POSroot\")",id);

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    tv_init({layer:nodes[\"%STR&SYM\"], fname:\"%STR&JSSTR\", loader:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"tv%POSload\"), width:%INT, newroot:null, branches:%INT, use3d:%INT, showrb:%INT, icon:\"%STR&JSSTR\", divclass:\"tv%POS\", sbg:\"%STR&JSSTR\", desc:%INT});\n",
		name, src, name, id, w, show_branches, use_3d_lines, show_root_branch, icon, id, selected_bg, order_desc);

	/** Script includes **/
	htrAddScriptInclude(s, "/sys/js/htdrv_treeview.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_info.js", 0);

	/** HTML body <DIV> elements for the layers. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem_va(s, "<DIV class=\"tv%POS\" ID=\"tv%POSroot\"><IMG SRC=\"%STR&HTE\" align=left>&nbsp;%STR&HTE</DIV>\n", id, id, (*icon)?icon:"/sys/images/ico02b.gif", src);
	    htrAddBodyItem_va(s, "<DIV ID=\"tv%POSload\"></DIV>\n",id);
	    }
	else
	    {
	    htrAddBodyItem_va(s, "<DIV class=\"tv%POS\" ID=\"tv%POSroot\" style=\"POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS;\"><IMG SRC=\"%STR&HTE\" align=left>&nbsp;%STR&HTE</DIV>\n",id,id,show_root?"inherit":"hidden",x,y,w,z,(*icon)?icon:"/sys/images/ico02b.gif", src);
	    htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "tv%POSload", id, "");
	    /*htrAddBodyItem_va(s, "<DIV ID=\"tv%dload\" style=\"POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; clip:rect(0px,0px,0px,0px); Z-INDEX:0;\"></DIV>\n",id);*/
	    }

	/** Event handler for click-on-url **/
	htrAddEventHandlerFunction(s, "document","CLICK","tv","tv_click");

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","tv","tv_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tv","tv_mouseup");
	htrAddEventHandlerFunction(s,"document","MOUSEOVER","tv","tv_mouseover");
	htrAddEventHandlerFunction(s,"document","MOUSEMOVE","tv","tv_mousemove");
	htrAddEventHandlerFunction(s,"document","MOUSEOUT","tv", "tv_mouseout");

	/** Check for more sub-widgets within the treeview. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    /*if (wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "widget/osrc-rule"))
		{
		httree_internal_AddRule(s, tree, name, sub_tree);
		}
	    else
		{*/
		htrRenderWidget(s, sub_tree, z+2);
		/*}*/
	    }

    return 0;
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
