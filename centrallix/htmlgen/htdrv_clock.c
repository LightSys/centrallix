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
/* Module: 	htdrv_clock.c         					*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	August 7, 2002						*/
/* Description:	HTML Widget driver for a local time clock.		*/
/************************************************************************/



/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCL;


/*** htclRender - generate the HTML code for the clock widget.
 ***/
int
htclRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char fgcolor1[16];
    char fgcolor2[16] = "";
    int shadowed = 0;
    int shadowx = 0;
    int shadowy = 0;
    int size = 0;
    int moveable = 0;
    int bold = 0;
    int showsecs = 1;
    int showampm = 1;
    int miltime = 0;
    int x=-1,y=-1,w,h;
    int id, i;
    char fieldname[HT_FIELDNAME_SIZE];

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTCL","Netscape 4 or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTCL","Clock widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTCL","Clock widget must have a 'height' property");
	    return -1;
	    }
	
	/** Background color/image? **/
	htrGetBackground(tree, NULL, 0, main_bg, sizeof(main_bg));

	/** Military Time? **/
	if (wgtrGetPropertyValue(tree,"hrtype",DATA_T_INTEGER,POD(&ptr)) == 0 && (intptr_t)ptr == 24)
	    {
	    miltime = 1;
	    showampm = 0;
	    }
	else if (wgtrGetPropertyValue(tree,"ampm",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")))
	    {
	    showampm = 0;
	    }

	/** Get text color **/
	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	   strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
	else
	   strcpy(fgcolor1,"black");

	/* Shadowed text? */
	if (wgtrGetPropertyValue(tree,"shadowed",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"true") || !strcasecmp(ptr,"yes")))
	    {
	    shadowed = 1;
	    if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
	    else
	        strcpy(fgcolor2,"#777777");
	    if (wgtrGetPropertyValue(tree,"shadowx",DATA_T_INTEGER,POD(&ptr)) == 0)
	        shadowx = (intptr_t)ptr;
	    else
	        shadowx = 1;
	    if (wgtrGetPropertyValue(tree,"shadowy",DATA_T_INTEGER,POD(&ptr)) == 0)
	        shadowy = (intptr_t)ptr;
	    else
	        shadowy = 1;
	    }

	/** Bold text? **/
	if (wgtrGetPropertyValue(tree,"bold",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    bold = 1;

	/** Get text size **/
	if (wgtrGetPropertyValue(tree,"size",DATA_T_INTEGER,POD(&ptr)) == 0)
	    size = (intptr_t)ptr;

	/** Movable? **/
	if (wgtrGetPropertyValue(tree,"moveable",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    moveable = 1;

	/** Show Seconds **/
	if (wgtrGetPropertyValue(tree,"seconds",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")))
	    showsecs = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Get fieldname **/
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    strtcpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	else 
	    fieldname[0]='\0';

	/** Write Style header items. **/
	htrAddStylesheetItem_va(s,"\t#cl%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);
	htrAddStylesheetItem_va(s,"\t#cl%POScon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,0,0,w,z+2);
	htrAddStylesheetItem_va(s,"\t#cl%POScon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,0,0,w,z+2);

	/** Write named global **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr,\"cl%POSbase\")",id);

	/** Other global variables **/
	htrAddScriptGlobal(s, "cl_move", "false", 0);
	htrAddScriptGlobal(s, "cl_xOffset", "null", 0);
	htrAddScriptGlobal(s, "cl_yOffset", "null", 0);

	/** Javascript include files **/
	htrAddScriptInclude(s, "/sys/js/htdrv_clock.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document","MOUSEUP", "cl", "cl_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "cl", "cl_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "cl", "cl_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "cl", "cl_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "cl", "cl_mousemove");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    cl_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), c1:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"),\"cl%POScon1\"), c2:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"),\"cl%POScon2\"), fieldname:\"%STR&JSSTR\", background:\"%STR&JSSTR\", shadowed:%POS, foreground1:\"%STR&JSSTR\", foreground2:\"%STR&JSSTR\", fontsize:%INT, moveable:%INT, bold:%INT, sox:%INT, soy:%INT, showSecs:%INT, showAmPm:%INT, milTime:%INT});\n",
	    name,
	    name, id,
	    name, id,
	    fieldname, main_bg, shadowed,
	    fgcolor1, fgcolor2,
	    size, moveable, bold,
	    shadowx, shadowy,
	    showsecs, showampm, miltime);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"cl%POSbase\">\n",id);
	htrAddBodyItem_va(s, "    <BODY %STR><TABLE width=%POS height=%POS border=0 cellpadding=0 cellspacing=0><TR><TD></TD></TR></TABLE></BODY>\n",main_bg,w,h);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%POScon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%POScon2\"></DIV>\n",id);
	htrAddBodyItem(s,    "</DIV>\n");

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
    }


/*** htclInitialize - register with the ht_render module.
 ***/
int
htclInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Local Time Clock Driver");
	strcpy(drv->WidgetName,"clock");
	drv->Render = htclRender;

	/** Events **/ 
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTCL.idcnt = 0;

    return 0;
    }
