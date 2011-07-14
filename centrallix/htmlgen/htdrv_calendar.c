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
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_calendar.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	July 9, 2003     					*/
/* Description:	HTML Widget driver for calendar/event type viewing.	*/
/*		This widget provides a view of calendar/event types of	*/
/*		data that has been queried via an objectsource.  It can	*/
/*		be configured to provide year, month, week, and day	*/
/*		views of the data, and will interact with an object-	*/
/*		source to query additional data if need be.		*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCA;


/*** htcaRender - generate the HTML code for the editbox widget.
 ***/
int
htcaRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char cell_bg[128];
    char textcolor[32];
    char dispmode[32] = "year";
    char eventdatefield[32];
    char eventdescfield[32] = "";
    char eventnamefield[32];
    char eventpriofield[32] = "";
    int minpriority=0;
    int x=-1,y=-1,w,h;
    int id, i;

	/** Verify user-agent's capabilities allow us to continue... **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCA","Netscape 4.x DOM support or W3C HTML DOM1/CSS1 support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCA.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTCA","Calendar widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = 0;
	
	/** Background color/image? **/
	htrGetBackground(tree,NULL,0,main_bg,sizeof(main_bg));

	/** Cell background color/image? **/
	htrGetBackground(tree,"cell",0,cell_bg,sizeof(cell_bg));

	/** Text color? **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(textcolor,ptr,sizeof(textcolor));
	else
	    strcpy(textcolor,"black");

	/** Data source field names **/
	if (wgtrGetPropertyValue(tree, "eventdatefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventdatefield' property");
	    return -1;
	    }
	strtcpy(eventdatefield, ptr, sizeof(eventdatefield));
	if (wgtrGetPropertyValue(tree, "eventnamefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventnamefield' property");
	    return -1;
	    }
	strtcpy(eventnamefield,ptr,sizeof(eventnamefield));
	if (wgtrGetPropertyValue(tree, "eventpriofield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(eventpriofield,ptr,sizeof(eventpriofield));
	    }
	if (wgtrGetPropertyValue(tree, "eventdescfield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(eventdescfield,ptr,sizeof(eventdescfield));
	    }

	/** display mode **/
	if (wgtrGetPropertyValue(tree, "displaymode", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(dispmode,ptr,sizeof(dispmode));
	    }

	/** minimum priority **/
	wgtrGetPropertyValue(tree, "displaymode", DATA_T_STRING, POD(&minpriority));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ca%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,h,z);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_calendar.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddEventHandlerFunction(s, "document","MOUSEUP", "ca", "ca_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "ca", "ca_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "ca", "ca_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "ca", "ca_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "ca", "ca_mousemove");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    ca_init(nodes[\"%STR&SYM\"], \"%STR&JSSTR\", \"%STR&JSSTR\", \"%STR&JSSTR\", \"%STR&JSSTR\", \"%STR&SYM\", \"%STR&SYM\", \"%STR&SYM\", \"%STR&SYM\", %INT, %INT, %INT);\n",
	    name,
	    main_bg, cell_bg, textcolor, dispmode,
	    eventdatefield, eventdescfield, eventnamefield, eventpriofield,
	    minpriority, w, h);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"ca%POSbase\"><BODY %STR text='%STR&HTE'>\n",id, main_bg, textcolor);

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</BODY></DIV>\n");

    return 0;
    }


/*** htcaInitialize - register with the ht_render module.
 ***/
int
htcaInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Calendar View Driver");
	strcpy(drv->WidgetName,"calendar");
	drv->Render = htcaRender;

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
	HTCA.idcnt = 0;

    return 0;
    }
