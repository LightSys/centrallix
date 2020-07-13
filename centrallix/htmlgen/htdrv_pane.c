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
    HTPN;


/*** htpnRender - generate the HTML code for the page.
 ***/
int
htpnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char bdr[64];
    int x=-1,y=-1;
    int minW, minH; //Minimum width and height
    int preH, treePreH, flexH, treeFlexH; //Widget's baseline width, tree's baseline width, widget's lateral flexibility, tree's lateral flexibility
    int preW, treePreW, flexW, treeFlexW; /* Some variables to facilitate dynamic resizing - preW replaces the variable w */
    int id;
    int style = 1; /* 0 = lowered, 1 = raised, 2 = none, 3 = bordered */
    char* c1;
    char* c2;
    int box_offset;
    int border_radius;
    int shadow_offset, shadow_radius;
    char shadow_color[128];

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTPN","Netscape DOM or W3C DOM1 HTML and CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTPN.idcnt++);

    	/** Get x,y,preW,flexW,preH,flexW,flexH,minW,minH of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&preW)) != 0) 
	    {
	    mssError(1,"HTPN","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&preH)) != 0)
	    {
	    mssError(1,"HTPN","Pane widget must have a 'height' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"fl_width",DATA_T_INTEGER,POD(&flexW)) != 0) flexW=1;
	if (wgtrGetPropertyValue(tree,"fl_height",DATA_T_INTEGER,POD(&flexH)) != 0) flexH=1;
	
	if (wgtrGetPropertyValue(tree,"min_width",DATA_T_INTEGER,POD(&minW)) != 0)	 //If there's no min-width value to load in...
	{
		if (wgtrGetPropertyValue(tree,"r_width",DATA_T_INTEGER,POD(&minW)) != 0) //And no r-width value to load in...
		{
			minW = preW / 2;						 //Try half the baseline width
		}
	}
	
	if (wgtrGetPropertyValue(tree,"min_height",DATA_T_INTEGER,POD(&minH)) != 0)
	{
		if (wgtrGetPropertyValue(tree,"r_height",DATA_T_INTEGER,POD(&minH)) != 0)
		{
			minH = preH / 2;
		}
	}
	
	//Now we've got to figure a way to get the total width and fl_width of the tree...
	//How about this: we don't want to lose our place in the tree, so let's see if there's a way to duplicate it
	//Then we can just pass wgtrGetRoot(dupeTree) into the wgtrGetPropertyValue() function in lieu of tree!
	//But how do we pull off a deep copy of a pWgtrNode object? It's a pointer to a memory location, but we want
	//to leave the memory location alone and just swipe the contents...
	
	pWgtrNode theWholeShebang = wgtrGetRoot(tree);
	
	if (wgtrGetPropertyValue(theWholeShebang,"height",DATA_T_INTEGER,POD(&treePreH)) != 0) treePreH=0;
	if (wgtrGetPropertyValue(theWholeShebang,"fl_height",DATA_T_INTEGER,POD(&treeFlexH)) != 0) treeFlexH=1;
	
	if (wgtrGetPropertyValue(theWholeShebang,"width",DATA_T_INTEGER,POD(&treePreW)) != 0) treePreW=0;
	if (wgtrGetPropertyValue(theWholeShebang,"fl_width",DATA_T_INTEGER,POD(&treeFlexW)) != 0) treeFlexW=1;
	
	
	
	/** Border radius, for raised/lowered/bordered panes **/
	if (wgtrGetPropertyValue(tree,"border_radius",DATA_T_INTEGER,POD(&border_radius)) != 0)
	    border_radius=0;

	/** Background color/image? **/
	htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_bg,sizeof(main_bg));

	/** figure out box offset fudge factor... stupid box model... **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Drop shadow **/
	shadow_offset=0;
	if (wgtrGetPropertyValue(tree, "shadow_offset", DATA_T_INTEGER, POD(&shadow_offset)) == 0 && shadow_offset > 0)
	    shadow_radius = shadow_offset+1;
	else
	    shadow_radius = 0;
	wgtrGetPropertyValue(tree, "shadow_radius", DATA_T_INTEGER, POD(&shadow_radius));
	if (shadow_radius > 0)
	    {
	    if (wgtrGetPropertyValue(tree, "shadow_color", DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(shadow_color, ptr, sizeof(shadow_color));
	    else
		strcpy(shadow_color, "black");
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	htrCheckAddExpression(s, tree, name, "enabled");
	htrCheckAddExpression(s, tree, name, "background");
	htrCheckAddExpression(s, tree, name, "bgcolor");

	/** Style of pane - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"lowered")) style = 0;
	    if (!strcmp(ptr,"raised")) style = 1;
	    if (!strcmp(ptr,"flat")) style = 2;
	    if (!strcmp(ptr,"bordered")) style = 3;
	    }
	if (style == 1) /* raised */
	    {
	    c1 = "white";
	    c2 = "gray";
	    }
	else if (style == 0) /* lowered */
	    {
	    c1 = "gray";
	    c2 = "white";
	    }
	else if (style == 3) /* bordered */
	    {
	    if (wgtrGetPropertyValue(tree,"border_color",DATA_T_STRING,POD(&ptr)) != 0)
		strcpy(bdr, "black");
	    else
		strtcpy(bdr,ptr,sizeof(bdr));
	    }

	if (treeFlexW == 0)
	{
		treeFlexW = 1; //Prevent division by zero
	}
	else if (treeFlexW < 0)
	{
		treeFlexW = -1 * treeFlexW; //Prevent wonky widths
	}
	
	if (treeFlexH == 0)
	{
		treeFlexH = 1; //Prevent division by zero
	}
	else if (treeFlexH < 0)
	{
		treeFlexH = -1 * treeFlexH; //Prevent wonky widths
	}
	
	/** Ok, write the style header items. **/
	if (style == 2) /* flat */
	    {
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; LEFT:%INTpx; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (1/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,x,y,minW,preW,treePreW,minH,preH,treePreH,flexH,treeFlexH,z);
		if(x > 12) 
		{
			htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; RIGHT:0px; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (2/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,y,minW,preW,treePreW,minH,preH,treePreH,flexH,treeFlexH,z);
		}
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { border-radius: %INTpx; %STR}\n",id,border_radius,main_bg);
	    }
	else if (style == 0 || style == 1) /* lowered or raised */
	    {
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow: hidden; LEFT:%INTpx; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (1/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,x,y,minW,preW-2*box_offset,treePreW,minH,preH-2*box_offset,treePreH,flexH,treeFlexH,z);
		if(x > 12) 
		{
			htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; RIGHT:0px; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (2/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,y,minW,preW,treePreW,minH,preH,treePreH,flexH,treeFlexH,z);
		}
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { border-style: solid; border-width: 1px; border-color: %STR %STR %STR %STR; border-radius: %INTpx; %STR}\n",id,c1,c2,c2,c1,border_radius,main_bg);
	    }
	else if (style == 3) /* bordered */
	    {																													//If minW turns out to be too large, consider subtracting 2*box_offset - and of course, the same goes for minH
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow: hidden; LEFT:%INTpx; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (1/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,x,y,minW,preW-2*box_offset,treePreW,minH,preH-2*box_offset,treePreH,flexH,treeFlexH,z);
		if(x > 12) 
		{
			htrAddStylesheetItem_va(s,"\t#pn%POSmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; RIGHT:0px; TOP:%INTpx; MIN-WIDTH:%POSpx; WIDTH:calc(%POSpx + (100%% - %POSpx) * (2/3)); MIN-HEIGHT:%POSpx; HEIGHT:calc(%POSpx + (100%% - %POSpx) * (%POS / %POS)); Z-INDEX:%POS;}\n",id,y,minW,preW,treePreW,minH,preH,treePreH,flexH,treeFlexH,z);
		}
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { border-style: solid; border-width: 1px; border-color:%STR&CSSVAL; border-radius: %INTpx; %STR}\n",id,bdr,border_radius,main_bg);
	    }
	if (shadow_radius > 0)
	    {
	    htrAddStylesheetItem_va(s,"\t#pn%POSmain { box-shadow: %POSpx %POSpx %POSpx %STR&CSSVAL; }\n", id, shadow_offset, shadow_offset, shadow_radius, shadow_color);
	    }

	/** DOM linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "pn%POSmain",id);

	/** Script include call **/
	htrAddScriptInclude(s, "/sys/js/htdrv_pane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document","MOUSEUP", "pn", "pn_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "pn", "pn_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "pn", "pn_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "pn", "pn_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "pn", "pn_mousemove");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    pn_init({mainlayer:wgtrGetNodeRef(ns,\"%STR&SYM\"), layer:wgtrGetNodeRef(ns,\"%STR&SYM\")});\n",
		name, name);

	/** HTML body <DIV> element for the base layer. **/
	//htrAddBodyItem_va(s,"<DIV ID=\"pn%POSmain\"><table width=%POS height=%POS cellspacing=0 cellpadding=0 border=0><tr><td></td></tr></table>\n",id, w-2, h-2);
	htrAddBodyItem_va(s,"<DIV ID=\"pn%POSmain\">\n",id, preW-2, preH-2);

	/** Check for objects within the pane. **/
	htrRenderSubwidgets(s, tree, z+2);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htpnInitialize - register with the ht_render module.
 ***/
int
htpnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"pane");
	drv->Render = htpnRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTPN.idcnt = 0;

    return 0;
    }
