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
/* Module: 	htdrv_scrollpane.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 30, 1998					*/
/* Description:	HTML Widget driver for a scrollpane -- a css layer with	*/
/*		a scrollable layer and a scrollbar for scrolling the	*/
/*		layer.  Can contain most objects, except for framesets.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTSPANE;


/*** htspaneRender - generate the HTML code for the page.
 ***/
int
htspaneRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int x,y,w,h;
    int id, i;
    int visible = 1;
    char bcolor[64] = "";
    char bimage[64] = "";

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE &&!(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTSPANE","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTSPANE.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Check background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bcolor,ptr,sizeof(bcolor));
	    }
	if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bimage,ptr,sizeof(bimage));
	    }

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,"\t#sp%POSpane { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:calc(100%%); HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,visible?"inherit":"hidden",x,y,w,h, z);
	    htrAddStylesheetItem_va(s,"\t#sp%POSarea { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:0px; WIDTH:calc(100%% - 20px); Z-INDEX:%POS; }\n",id,w,z+1);
	    htrAddStylesheetItem_va(s,"\t#sp%POSthum { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:18px; WIDTH:calc(100%% - 18px); Z-INDEX:%POS; }\n",id,w,z+1);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "sp_target_img", "null", 0);
	htrAddScriptGlobal(s, "sp_click_x","0",0);
	htrAddScriptGlobal(s, "sp_click_y","0",0);
	htrAddScriptGlobal(s, "sp_thum_y","0",0);
	htrAddScriptGlobal(s, "sp_mv_timeout","null",0);
	htrAddScriptGlobal(s, "sp_mv_incr","0",0);
	htrAddScriptGlobal(s, "sp_cur_mainlayer","null",0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "sp%POSpane",id);
	htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, \"sp%POSarea\")",id);

	htrAddScriptInclude(s, "/sys/js/htdrv_scrollpane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddScriptInit_va(s,"    sp_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), aname:\"sp%POSarea\", tname:\"sp%POSthum\"});\n", name,id,id);

	/** HTML body <DIV> elements for the layers. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%POSpane\"><TABLE %[bgcolor=\"%STR&HTE\"%] %[background=\"%STR&HTE\"%] border='0' cellspacing='0' cellpadding='0' width='%POS'>", id, *bcolor, bcolor, *bimage, bimage, w);
	    htrAddBodyItem(s,   "<TR><TD align=right><IMG SRC='/sys/images/ico13b.gif' NAME='u'></TD></TR><TR><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC='/sys/images/trans_1.gif' height='%POSpx' width='18px' name='b'>",h-36);
	    htrAddBodyItem(s,   "</TD></TR><TR><TD align=right><IMG SRC='/sys/images/ico12b.gif' NAME='d'></TD></TR></TABLE>\n");
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%POSthum\"><IMG SRC='/sys/images/ico14b.gif' NAME='t'></DIV>\n<DIV ID=\"sp%POSarea\"><table border='0' cellpadding='0' cellspacing='0' width='%POSpx' height='%POSpx'><tr><td>",id,id,w-2,h-2);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    //htrAddStylesheetItem_va(s,"\t#sp%dpane { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(0px,%dpx,%dpx,0px); Z-INDEX:%d; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	    //htrAddStylesheetItem_va(s,"\t#sp%darea { HEIGHT: %dpx; WIDTH:%dpx; }\n",id, h, w-18);
	    //htrAddStylesheetItem_va(s,"\t#sp%dthum { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:18px; WIDTH:18px; Z-INDEX:%dpx; }\n",id,w-18,z+1);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%POSpane\" style=\"POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:calc(100%% - 20px); HEIGHT:%POSpx; Z-INDEX:%POS;\">\n",id,visible?"inherit":"hidden",x,y,w,h,z);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%POSup\" SRC='/sys/images/ico13b.gif' NAME='u'/>", id);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%POSbar\" SRC='/sys/images/trans_1.gif' NAME='b'/>", id);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%POSdown\" SRC='/sys/images/ico12b.gif' NAME='d'/>", id);
	    htrAddStylesheetItem_va(s,"\t#sp%POSup { POSITION: absolute; LEFT: calc(100%% - 18px); TOP: 0px; }\n",id, w-18);
	    htrAddStylesheetItem_va(s,"\t#sp%POSbar { POSITION: absolute; LEFT: calc(100%% - 18px); TOP: 18px; WIDTH: 18px; HEIGHT: %POSpx;}\n",id, w-18, h-36);
	    htrAddStylesheetItem_va(s,"\t#sp%POSdown { POSITION: absolute; LEFT: calc(100%% - 18px); TOP: %INTpx; }\n",id, w-18, h-18);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%POSthum\" style=\"POSITION:absolute; VISIBILITY:inherit; LEFT: calc(100%% - 18px); TOP:18px; WIDTH:18px; Z-INDEX:%POS;\"><IMG SRC='/sys/images/ico14b.gif' NAME='t'></DIV>\n", id,w-18,z+1);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%POSarea\" style=\"HEIGHT: %POSpx; POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:0px; WIDTH:calc(100%% - 20px); Z-INDEX:%POS;\">",id,h,w,z+1);
	    }
	else
	    {
	    mssError(1,"HTSPNE","Browser not supported");
	    }

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","sp","sp_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","sp","sp_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","sp","sp_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "sp","sp_mouseover");

	/** Do subwidgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2);

	/** Finish off the last <DIV> **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem(s,"</td></tr></table></DIV></DIV>\n");
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    htrAddBodyItem(s,"</DIV></DIV>\n");
	    }
	else
	    {
	    mssError(1,"HTSPNE","browser not supported");
	    }

    return 0;
    }


/*** htspaneInitialize - register with the ht_render module.
 ***/
int
htspaneInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ScrollPane Widget Driver");
	strcpy(drv->WidgetName,"scrollpane");
	drv->Render = htspaneRender;

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
	HTSPANE.idcnt = 0;

    return 0;
    }
