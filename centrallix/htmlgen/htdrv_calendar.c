#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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

/**CVSDATA***************************************************************

    $Id: htdrv_calendar.c,v 1.1 2003/07/12 04:14:34 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_calendar.c,v $

    $Log: htdrv_calendar.c,v $
    Revision 1.1  2003/07/12 04:14:34  gbeeley
    Initial rough beginnings of a calendar widget.

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCA;


/*** htcaVerify - not written yet.
 ***/
int
htcaVerify()
    {
    return 0;
    }


/*** htcaRender - generate the HTML code for the editbox widget.
 ***/
int
htcaRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
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
    int id;
    char* nptr;
    pObject sub_w_obj;
    pObjQuery qy;

	/** Verify user-agent's capabilities allow us to continue... **/
	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTCA","Netscape 4.x DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCA.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTCA","Calendar widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0) h = 0;
	
	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"bgColor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Cell background color/image? **/
	if (objGetAttrValue(w_obj,"cell_bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(cell_bg,"bgColor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"cell_background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(cell_bg,"background='%.110s'",ptr);
	else
	    strcpy(cell_bg,"");

	/** Text color? **/
	if (objGetAttrValue(w_obj,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(textcolor,"%.31s",ptr);
	else
	    strcpy(textcolor,"black");

	/** Data source field names **/
	if (objGetAttrValue(w_obj, "eventdatefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventdatefield' property");
	    return -1;
	    }
	memccpy(eventdatefield, ptr, 0, 31);
	eventdatefield[31] = 0;
	if (objGetAttrValue(w_obj, "eventnamefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventnamefield' property");
	    return -1;
	    }
	memccpy(eventnamefield, ptr, 0, 31);
	eventnamefield[31] = 0;
	if (objGetAttrValue(w_obj, "eventpriofield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(eventpriofield, ptr, 0, 31);
	    eventpriofield[31] = 0;
	    }
	if (objGetAttrValue(w_obj, "eventdescfield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(eventdescfield, ptr, 0, 31);
	    eventdescfield[31] = 0;
	    }

	/** display mode **/
	if (objGetAttrValue(w_obj, "displaymode", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(dispmode, ptr, 0, 31);
	    dispmode[31] = 0;
	    }

	/** minimum priority **/
	objGetAttrValue(w_obj, "displaymode", DATA_T_STRING, POD(&minpriority));

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ca%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_calendar.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddEventHandler(s, "document","MOUSEUP", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseOver');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEOUT", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseOut');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEMOVE", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseMove');\n"
	    "\n");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = ca_init(%s.layers.ca%dbase, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d);\n",
		nptr, parentname, id, 
		main_bg, cell_bg, textcolor, dispmode,
		eventdatefield, eventdescfield, eventnamefield, eventpriofield,
		minpriority, w);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"ca%dbase\"><BODY %s text='%s'>\n",id, main_bg, textcolor);

	/** Check for more sub-widgets **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
		{
		htrRenderWidget(s, sub_w_obj, z+1, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

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
	drv->Verify = htcaVerify;

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
