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
#include "wgtr.h"
#include "cxlib/qprintf.h"

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
/* Module: 	htdrv_datetime.c        				*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	June 26, 2002						*/
/* Description:	HTML driver for a 'date time' widget			*/
/************************************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTDT;


/*** htdtRender - generate the HTML code for the page.
 ***/
int
htdtRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char *sql;
    char *str;
    char *attr;
    char name[64];
    char initialdate[64];
    char fgcolor[64];
    char bgcolor[128];
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int type;
    int x,y,w,h,w2=184,h2=190;
    int rval;
    int search_by_range;
    int date_only = 0;
    char default_time[32];
    DateTime dt;
    ObjData od;
    pObjQuery qy;
    pObject qy_obj;

	/** Get an id for this. **/
	const int id = (HTDT.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTDT", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'height' property");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else 
	    fieldname[0]='\0';

	/** Is this a date-only control rather than date-time combined? **/
	date_only = htrGetBoolean(tree, "date_only", 0);
	if (date_only == 1) h2 = 156;

	/** If no time is entered (or if date-only), what is the default time? **/
	if (wgtrGetPropertyValue(tree,"default_time",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(default_time, ptr, sizeof(default_time));
	else
	    strcpy(default_time, "");

	/** When searching, do we use a start/end date range? **/
	search_by_range = htrGetBoolean(tree, "search_by_range", 1);
	
	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Get initial date **/
	if (wgtrGetPropertyValue(tree, "sql", DATA_T_STRING,POD(&sql)) == 0) 
	    {
	    if ((qy = objMultiQuery(s->ObjSession, sql, NULL, 0))) 
		{
		while ((qy_obj = objQueryFetch(qy, O_RDONLY)))
		    {
		    attr = objGetFirstAttr(qy_obj);
		    if (!attr)
			{
			objClose(qy_obj);
			objQueryClose(qy);
			mssError(1, "HTDT", "There was an error getting date from your SQL query");
			return -1;
			}
		    type = objGetAttrType(qy_obj, attr);
		    rval = objGetAttrValue(qy_obj, attr, type,&od);
		    if (rval == 0)
			{
			if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			    str = objDataToStringTmp(type, (void*)(&od), 0);
			else
			    str = objDataToStringTmp(type, (void*)(od.String), 0);
			strtcpy(initialdate, str, sizeof(initialdate));
			}
		    objClose(qy_obj);
		    }
		objQueryClose(qy);
		}
	    }
	else if (wgtrGetPropertyValue(tree,"initialdate",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(initialdate, ptr, sizeof(initialdate));
	    }
	else
	    {
	    initialdate[0]='\0';
	    }
	if (strlen(initialdate))
	    {
	    snprintf(
		initialdate, sizeof(initialdate),
		"%s %d %d, %d:%d%d",
		obj_short_months[dt.Part.Month], 
		dt.Part.Day + 1,
		dt.Part.Year + 1900,
		dt.Part.Hour,
		dt.Part.Minute,
		dt.Part.Second
	    );
	    }
	

	/** Get colors **/
	htrGetBackground(tree, NULL, 1, bgcolor, sizeof(bgcolor));
	if (!*bgcolor) strcpy(bgcolor,"background-color:white;");
	
	if (wgtrGetPropertyValue(tree,"fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor,ptr,sizeof(fgcolor));
	else
	    strcpy(fgcolor,"black");

	/** Write style headers. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#dt%POSbtn { "
		"position:absolute; "
		"visibility:inherit; "
		"overflow:hidden; "
		"cursor:pointer; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
		"border:1px outset #e0e0e0; "
		"%STR "
	    "}\n",
	    id,
	    ht_flex_x(x, tree),
	    ht_flex_y(y, tree),
	    ht_flex_w(w, tree),
	    ht_flex_h(h, tree),
	    z,
	    bgcolor
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write datetime button CSS.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t.dt%POScon { "
		"position:absolute; "
		"overflow:hidden; "
		"left:1px; "
		"top:1px; "
		"width:calc(100%% - 20px); "
		"height:calc(100%% - 2px); "
		"z-index:%POS; "
	    "}\n",
	    id,
	    z + 1
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write datetime con CSS.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t.dt_dropdown { "
		"cursor:default; "
	    "}\n",
	    id
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write datetime dropdown CSS.");
	    goto err;
	    }

 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "dt%POSbtn", id) != 0)
	    {
	    mssError(0, "HTDT", "Failed to add object linkage.");
	    goto err;
	    }

	/** Write named globals. **/
	if (htrAddScriptGlobal(s, "dt_current", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "dt_img_y", "0", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "dt_timeout_fn", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "dt_timeout", "null", 0) != 0) goto err;

	/** Script includes **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_date.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_datetime.js", 0) != 0) goto err;

	/** Add the event handling scripts. **/
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "dt","dt_mousedown") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "dt","dt_mousemove") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",  "dt","dt_mouseout")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "dt","dt_mouseover") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",   "dt","dt_mouseup")   != 0) goto err;

	/** Write the initialization call in its own scope. **/
	if (htrAddScriptInit_va(s, "\t{ "
	    "const layer = wgtrGetNodeRef(ns, '%STR&SYM'); "
	    "dt_init({ "
	        "layer, "
		"c1:htr_subel(layer, 'dt%POScon1'), "
		"c2:htr_subel(layer, 'dt%POScon2'), "
		"id:'%STR&JSSTR', "
		"background:'%STR&JSSTR', "
		"foreground:'%STR&JSSTR', "
		"fieldname:'%STR&JSSTR', "
		"form:'%STR&JSSTR', "
		"width:%INT, "
		"height:%INT, "
		"width2:%INT, "
		"height2:%INT, "
		"sbr:%INT, "
		"donly:%INT, "
		"dtime:'%STR&JSSTR', "
	    "}); }\n",
	    name, id, id,
	    initialdate,
	    bgcolor, fgcolor, fieldname, form,
	    w - 20, h, w2, h2,
	    search_by_range,
	    date_only, default_time
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to render child widgets.");
	    goto err;
	    }

	/** Write HTML. **/
	if (htrAddBodyItem_va(s,
	    "<div id='dt%POSbtn'>"
		"<img src='/sys/images/ico17.gif' alt='icon' style='float:right;'>"
		"<div id='dt%POScon1' class='dt%POScon' style='visibility:inherit;'></div>"
		"<div id='dt%POScon2' class='dt%POScon' style='visibility:hidden;'></div>"
	    "</div>\n",
	    id,
	    id, id,
	    id, id
	) != 0)
	    {
	    mssError(0, "HTDT", "Failed to write datetime HTML.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTDT",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
    }


/*** htdtInitialize - register with the ht_render module.
 ***/
int
htdtInitialize()
    {
    pHtDriver drv;

	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Date/Time Widget Driver");
	strcpy(drv->WidgetName,"datetime");
	drv->Render = htdtRender;

	/** Register events **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"GetFocus");
	htrAddEvent(drv,"LoseFocus");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTDT.idcnt = 0;

    return 0;
    }
