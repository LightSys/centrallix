#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
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
/* Module: 	htdrv_tab.c             				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 28, 1998 					*/
/* Description:	HTML Widget driver for a tab control.			*/
/************************************************************************/


/** globals **/
static struct
    {
    int		idcnt;
    }
    HTTAB;

int httabSetup(pHtSession s)
	{
	htrAddStylesheetItem_va(s,"\t.tcPosAbs {POSITION:absolute; }\n");
	htrAddStylesheetItem_va(s,"\t.tcBase { background-position: 0px -24px; OVERFLOW:hidden; border-width: 1px; }\n");
	htrAddStylesheetItem_va(s, "\t.tcTab { VISIBILITY:inherit; OVERFLOW:hidden; cursor:default; font-weight:bold; }\n");
	htrAddStylesheetItem_va(s, "\t.tcPane { LEFT:0px; TOP:0px;} \n");
	return 0;
	}

enum httab_locations { Top=0, Bottom=1, Left=2, Right=3, None=4 };


/*** httabRender - generate the HTML code for the page.
 ***/
int
httabRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char* type,*field;
    char name[64];
    char tab_txt[128];
    char main_bg[128];
    char inactive_bg[128];
    char page_type[32];
    char fieldname[128];
    char sel[128];
    int sel_idx= -1;
    pWgtrNode tabpage_obj;
    int x=-1,y=-1,w,h;
    int id,tabcnt, i, j;
    char* subnptr;
    enum httab_locations tloc;
    int tab_width = 0;
    int xoffset,yoffset,xtoffset, ytoffset;
    int is_selected;
    char* bg;
    char* tabname;
    pWgtrNode children[32];
    int border_radius;
    char border_style[32];
    char border_color[64];
    int border_width;
    int shadow_offset, shadow_radius, shadow_angle;
    char shadow_color[128];

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE &&(!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTAB","NS4 or W3C DOM Support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTAB.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'height' property");
	    return -1;
	    }

	/** Drop shadow **/
	shadow_offset=0;
	if (wgtrGetPropertyValue(tree, "shadow_offset", DATA_T_INTEGER, POD(&shadow_offset)) == 0 && shadow_offset > 0)
	    shadow_radius = shadow_offset+1;
	else
	    shadow_radius = 0;
	wgtrGetPropertyValue(tree, "shadow_radius", DATA_T_INTEGER, POD(&shadow_radius));
	strcpy(shadow_color, "black");
	if (shadow_radius > 0)
	    {
	    if (wgtrGetPropertyValue(tree, "shadow_color", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(shadow_color, ptr, sizeof(shadow_color));
	    }
	if (wgtrGetPropertyValue(tree, "shadow_angle", DATA_T_INTEGER, POD(&shadow_angle)) != 0)
	    shadow_angle = 135;

	/** Border radius, color, and style. **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;
	if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_color, "#ffffff");
	else
	    strtcpy(border_color, ptr, sizeof(border_color));
	if (wgtrGetPropertyValue(tree,"border_style",DATA_T_STRING,POD(&ptr)) != 0)
	    strcpy(border_style, "outset");
	else
	    strtcpy(border_style, ptr, sizeof(border_style));
	if (!strcmp(border_style, "none") || !strcmp(border_style, "hidden"))
	    border_width=0;
	else
	    border_width=1;

	/** Which side are the tabs on? **/
	if (wgtrGetPropertyValue(tree,"tab_location",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"top")) tloc = Top;
	    else if (!strcasecmp(ptr,"bottom")) tloc = Bottom;
	    else if (!strcasecmp(ptr,"left")) tloc = Left;
	    else if (!strcasecmp(ptr,"right")) tloc = Right;
	    else if (!strcasecmp(ptr,"none")) tloc = None;
	    else
		{
		mssError(1,"HTTAB","%s: '%s' is not a valid tab_location",name,ptr);
		return -1;
		}
	    }
	else
	    {
	    tloc = Top;
	    }

	/** How wide should left/right tabs be? **/
	if (wgtrGetPropertyValue(tree,"tab_width",DATA_T_INTEGER,POD(&tab_width)) != 0)
	    {
	    if (tloc == Right || tloc == Left)
		{
		mssError(1,"HTTAB","%s: tab_width must be specified with tab_location of left or right", name);
		return -1;
		}
	    }
	else
	    {
	    if (tab_width < 0) tab_width = 0;
	    }

	/** Which tab is selected? **/
	if (wgtrGetPropertyType(tree,"selected") == DATA_T_STRING &&
		wgtrGetPropertyValue(tree,"selected",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(sel,ptr, sizeof(sel));
	    }
	else
	    {
	    strcpy(sel,"");
	    }
	if (wgtrGetPropertyValue(tree,"selected_index", DATA_T_INTEGER, POD(&sel_idx)) != 0)
	    {
	    sel_idx = -1;
	    }
	if (sel_idx != -1 && *sel != '\0')
	    {
	    mssError(1,"HTTAB","%s: cannot specify both 'selected' and 'selected_index'", name);
	    return -1;
	    }

	/** User requesting expression for selected tab? **/
	htrCheckAddExpression(s, tree, name, "selected");

	/** User requesting expression for selected tab using integer index value? **/
	htrCheckAddExpression(s, tree, name, "selected_index");

	/** Background color/image? **/
	htrGetBackground(tree, NULL, s->Capabilities.Dom2CSS, main_bg, sizeof(main_bg));

	/** Inactive tab color/image? **/
	htrGetBackground(tree, "inactive", s->Capabilities.Dom2CSS, inactive_bg, sizeof(inactive_bg));

	/** Text color? **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(tab_txt, ptr, sizeof(tab_txt));
	else
	    strcpy(tab_txt,"black");
	if (strpbrk(tab_txt, "{};&<>\"\'"))
	    strcpy(tab_txt,"black");

	/** Determine offset to actual tab pages **/
	switch(tloc)
	    {
	    case Top:    xoffset = 0;         yoffset = 24; xtoffset = 0; ytoffset = 0; break;
	    case Bottom: xoffset = 0;         yoffset = 0;  xtoffset = 0; ytoffset = h; break;
	    case Right:  xoffset = 0;         yoffset = 0;  xtoffset = w; ytoffset = 0; break;
	    case Left:   xoffset = tab_width; yoffset = 0;  xtoffset = 0; ytoffset = 0; break;
	    case None:   xoffset = 0;         yoffset = 0;  xtoffset = 0; ytoffset = 0;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#tc%POSbase { %STR }\n", id, main_bg);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "tc%POSbase",id);

	/** Script include **/
	htrAddScriptInclude(s, "/sys/js/htdrv_tab.js", 0);

	/** Add a global for the master tabs listing **/
	htrAddScriptGlobal(s, "tc_tabs", "null", 0);
	htrAddScriptGlobal(s, "tc_cur_mainlayer", "null", 0);

	/** Event handler for click-on-tab **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","tc","tc_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tc","tc_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","tc","tc_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER","tc","tc_mouseover");

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    tc_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), tloc:%INT, mainBackground:\"%STR&JSSTR\", inactiveBackground:\"%STR&JSSTR\"});\n",
		name, tloc, main_bg, inactive_bg);

	/** Check for tabpages within the tab control, to do the tabs at the top. **/
	tabcnt = wgtrGetMatchingChildList(tree, "widget/tabpage", children, sizeof(children)/sizeof(pWgtrNode));
	if (tloc != None)
	    {
	    for (i=0;i<tabcnt;i++)
		{
		tabpage_obj = children[i];
		wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));

		if(wgtrGetPropertyValue(tabpage_obj,"type",DATA_T_STRING,POD(&type)) != 0)
		    strcpy(page_type,"static");
		else if(!strcmp(type,"static") || !strcmp(type,"dynamic"))
		    strcpy(page_type,type);
		else
		    strcpy(page_type,"static");
		is_selected = (i+1 == sel_idx || (!*sel && i == 0) || !strcmp(sel,ptr));
		bg = is_selected?main_bg:inactive_bg;
		if (wgtrGetPropertyValue(tabpage_obj,"title",DATA_T_STRING,POD(&tabname)) != 0)
		    wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&tabname));

		htrAddStylesheetItem_va(s, "\t#tc%POStab%POS { left:%INTpx; top:%INTpx; %[width:%POSpx; %] z-index:%POS; border-radius:%POSpx %POSpx %POSpx %POSpx; border-style:%STR&CSSVAL; border-width: %POSpx %POSpx %POSpx %POSpx; border-color: %STR&CSSVAL; box-shadow:%DBLpx %DBLpx %POSpx %STR&CSSVAL; text-align:%STR&CSSVAL; color:%STR&CSSVAL; %STR }\n",
			id, i+1,
			x+xtoffset, y+ytoffset,
			tab_width>0, tab_width,
			is_selected?(z+2):z,
			(tloc==Bottom || tloc==Right)?0:border_radius, (tloc==Bottom || tloc==Left)?0:border_radius, (tloc==Top || tloc==Left)?0:border_radius, (tloc==Top || tloc==Right)?0:border_radius,
			//(tloc==Top || tloc==Left)?0:border_radius, (tloc==Right)?0:border_radius, border_radius, (tloc==Bottom)?0:border_radius,
			border_style,
			(tloc!=Bottom)?1:0, (tloc!=Left)?1:0, (tloc!=Top)?1:0, (tloc!=Right)?1:0,
			border_color,
			sin(shadow_angle*M_PI/180)*shadow_offset, cos(shadow_angle*M_PI/180)*(-shadow_offset), shadow_radius, shadow_color,
			(tloc != Right)?"left":"right",
			tab_txt, bg
			);

		htrAddBodyItem_va(s, "<div id=\"tc%POStab%POS\" class=\"tcTab tcPosAbs\"><p style=\"white-space:nowrap; margin:0px; padding:0px;\">%[<span>&nbsp;%STR&HTE&nbsp;</span>%]<img src=\"/sys/images/tab_lft%POS.gif\" style=\"width:5px; height:24px; vertical-align:middle;\">%[<span>&nbsp;%STR&HTE&nbsp;</span>%]</p></div>\n",
			id, i+1,
			tloc == Right, tabname,
			is_selected?2:3,
			tloc != Right, tabname
			);
		}
	    }

	/** h-2 and w-2 because w3c dom borders add to actual width **/
	htrAddBodyItem_va(s,"<div id=\"tc%POSbase\" class=\"tcBase tcPosAbs\" style=\"height:%POSpx; width:%POSpx; left:%INTpx; top:%INTpx; z-index:%POS; border-style:%STR&CSSVAL; border-color: %STR&CSSVAL; border-radius:%POSpx %POSpx %POSpx %POSpx; box-shadow: %DBLpx %DBLpx %POSpx %STR&CSSVAL;\">\n",
		id, h-border_width*2, w-border_width*2, x+xoffset, y+yoffset, z+1,
		border_style, border_color,
		(tloc==Top || tloc==Left)?0:border_radius, (tloc==Right)?0:border_radius, border_radius, (tloc==Bottom)?0:border_radius,
		sin(shadow_angle*M_PI/180)*shadow_offset, cos(shadow_angle*M_PI/180)*(-shadow_offset), shadow_radius, shadow_color
		);

	/** Check for tabpages within the tab control entity, this time to do the pages themselves **/
	for (i=0;i<tabcnt;i++)
	    {
	    tabpage_obj = children[i];

	    htrCheckNSTransition(s, tree, tabpage_obj);

	    /** First, render the tabpage and add stuff for it **/
	    wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
	    is_selected = (i+1 == sel_idx || (!*sel && i == 0) || !strcmp(sel,ptr));
	    if(wgtrGetPropertyValue(tabpage_obj,"type",DATA_T_STRING,POD(&type)) != 0)
		strcpy(page_type,"static");
	    else if(!strcmp(type,"static") || !strcmp(type,"dynamic"))
		strcpy(page_type,type);
	    else
		strcpy(page_type,"static");
	    strcpy(fieldname,"");
	    if(!strcmp(page_type,"dynamic"))
		{
		if(wgtrGetPropertyValue(tabpage_obj,"fieldname",DATA_T_STRING,POD(&field)) == 0)
		    strtcpy(fieldname,field,sizeof(fieldname));
		}

	    /** Add script initialization to add a new tabpage **/
	    if (tloc == None)
		htrAddScriptInit_va(s,"    wgtrGetNodeRef('%STR&SYM', '%STR&SYM').addTab(null,wgtrGetContainer(wgtrGetNodeRef(\"%STR&SYM\",\"%STR&SYM\")),wgtrGetNodeRef('%STR&SYM','%STR&SYM'),'%STR&JSSTR','%STR&JSSTR','%STR&JSSTR');\n",
		    wgtrGetNamespace(tree), name,
		    wgtrGetNamespace(tabpage_obj), ptr, wgtrGetNamespace(tree), name, ptr,page_type,fieldname);
	    else
		htrAddScriptInit_va(s,"    wgtrGetNodeRef('%STR&SYM','%STR&SYM').addTab(htr_subel(wgtrGetParentContainer(wgtrGetNodeRef('%STR&SYM','%STR&SYM')),\"tc%POStab%POS\"),wgtrGetContainer(wgtrGetNodeRef(\"%STR&SYM\",\"%STR&SYM\")),wgtrGetNodeRef('%STR&SYM','%STR&SYM'),'%STR&JSSTR','%STR&JSSTR','%STR&JSSTR');\n",
		    wgtrGetNamespace(tree), name,
		    wgtrGetNamespace(tree), name,
		    id, i+1, wgtrGetNamespace(tabpage_obj), ptr, wgtrGetNamespace(tree), name, ptr,page_type,fieldname);

	    /** Add named global for the tabpage **/
	    subnptr = nmSysStrdup(ptr);
	    htrAddWgtrObjLinkage_va(s, tabpage_obj, "tc%POSpane%POS", id, i+1);
	    htrAddWgtrCtrLinkage_va(s, tabpage_obj, "htr_subel(_parentobj, \"tc%POSpane%POS\")", id, i+1);

	    /** Add DIV section for the tabpage. **/
	    htrAddBodyItem_va(s,"<div id=\"tc%POSpane%POS\" class=\"tcPane tcPosAbs\" style=\"VISIBILITY:%STR&CSSVAL; WIDTH:%POSpx; Z-INDEX:%POS;\">\n",
		    id,i+1,is_selected?"inherit":"hidden",w-2,z+2);

	    /** Now look for sub-items within the tabpage. **/
	    for (j=0;j<xaCount(&(tabpage_obj->Children));j++)
		htrRenderWidget(s, xaGetItem(&(tabpage_obj->Children), j), z+3);

	    htrAddBodyItem(s, "</DIV>\n");

	    nmSysFree(subnptr);

	    /** Add the visible property **/
	    htrCheckAddExpression(s, tabpage_obj, ptr, "visible");

	    htrCheckNSTransitionReturn(s, tree, tabpage_obj);
	    }

	/** Need to do other subwidgets (connectors, etc.) now **/
	htrRenderSubwidgets(s, tree, z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }

int 
httabRender_page(pHtSession s, pWgtrNode tabpage, int z) 
    {
    /** we already rendered subwidgets of the tabpage **/
    /*htrRenderSubwidgets(s, tabpage, z);*/
    return 0;
    }


/*** httabInitialize - register with the ht_render module.
 ***/
int
httabInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Tab Control Driver");
	strcpy(drv->WidgetName,"tab");
	drv->Render = httabRender;
	drv->Setup = httabSetup;
	/*xaAddItem(&(drv->PseudoTypes), "tabpage");*/

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	drv = htrAllocDriver();
	if (!drv) return -1;
	strcpy(drv->Name,"DHTML Tab Page Driver");
	strcpy(drv->WidgetName,"tabpage");
	drv->Render = httabRender_page;
	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

	HTTAB.idcnt = 0;

    return 0;
    }
