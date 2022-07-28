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
#include "wgtr.h"
#include "cxlib/qprintf.h"

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

int htdtSetup(pHtSession s)
	{
	htrAddStylesheetItem_va(s,"\t.dtAbsHid { OVERFLOW: hidden; POSITION:absolute;}\n");
	htrAddStylesheetItem_va(s,"\t.dtBtn  { VISIBILITY:inherit; cursor:default; border:1px outset #e0e0e0;}\n");
	htrAddStylesheetItem_va(s,"\t.dtCon1 { VISIBILITY:inherit; LEFT:1px; TOP:1px; }\n");
	htrAddStylesheetItem_va(s,"\t.dtCon2 { VISIBILITY:hidden; LEFT:1px; TOP:1px; }\n");
	return 0;
	}

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
    int id, i;
    int rval;
    int search_by_range;
    int date_only = 0;
    char default_time[32];
    DateTime dt;
    ObjData od;
    pObjQuery qy;
    pObject qy_obj;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTDT","Netscape or W3C DOM support required");
	    return -1;
	    }

	/** Get an id for this. **/
	id = (HTDT.idcnt++);

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
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    strtcpy(initialdate, str, sizeof(initialdate));
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
	    objDataToDateTime(DATA_T_STRING, initialdate, &dt, NULL);
	    qpfPrintf(NULL, initialdate, sizeof(initialdate), "%STR %INT %INT, %INT:%INT:%INT", obj_short_months[dt.Part.Month], 
	                          dt.Part.Day+1,
	                          dt.Part.Year+1900,
	                          dt.Part.Hour,
	                          dt.Part.Minute,
	                          dt.Part.Second);
	    }
	

	/** Get colors **/
	htrGetBackground(tree, NULL, 1, bgcolor, sizeof(bgcolor));
	if (!*bgcolor) strcpy(bgcolor,"background-color:white;");
	//else strcpy(bgcolor, "bgcolor=green");

///////////////////////////////////////
//	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
//	    sprintf(bgcolor,"%.40s",ptr);
//	else {
	    //mssError(1,"HTDD","Date Time widget must have a 'bgcolor' property");
	    //return -1;
	    //strcpy(bgcolor,"blue");
//	}
	
	if (wgtrGetPropertyValue(tree,"fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor,ptr,sizeof(fgcolor));
	else
	    strcpy(fgcolor,"black");

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#dt%POSbtn  { LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; %STR }\n",id,x,y,w,h,z, bgcolor);
	htrAddStylesheetItem_va(s,"\t#dt%POScon1 { WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,w-20,h-2,z+1);
	htrAddStylesheetItem_va(s,"\t#dt%POScon2 { WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,w-20,h-2,z+1);

	/** Write named global **/
	htrAddScriptGlobal(s, "dt_current", "null", 0);
	htrAddScriptGlobal(s, "dt_timeout", "null", 0);
	htrAddScriptGlobal(s, "dt_timeout_fn", "null", 0);
	htrAddScriptGlobal(s, "dt_img_y", "0", 0);

	htrAddWgtrObjLinkage_va(s, tree, "dt%POSbtn",id);

	/** Script includes **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_date.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_datetime.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    dt_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"),c1:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"),\"dt%POScon1\"),c2:htr_subel(wgtrGetNodeRef(ns,\"%STR&SYM\"),\"dt%POScon2\"),id:\"%STR&JSSTR\", background:\"%STR&JSSTR\", foreground:\"%STR&JSSTR\", fieldname:\"%STR&JSSTR\", form:\"%STR&JSSTR\", width:%INT, height:%INT, width2:%INT, height2:%INT, sbr:%INT, donly:%INT, dtime:\"%STR&JSSTR\"})\n",
	    name,
	    name,id, 
	    name,id, 
	    initialdate, bgcolor, fgcolor, fieldname, form,
	    w-20, h, w2,h2,
	    search_by_range,
	    date_only, default_time);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"dt%POSbtn\" class=\"dtBtn dtAbsHid\">\n"
			    "<IMG SRC=\"/sys/images/ico17.gif\" style=\"float:right;\">\n", id);
	/*htrAddBodyItem_va(s,"<TABLE width=%POS cellspacing=0 cellpadding=0 border=0>\n",w);
	htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%POS></TD>\n",w-2);
	htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%POS width=1></TD>\n",h-2);
	htrAddBodyItem(s,   "       <TD align=right valign=middle><IMG SRC=/sys/images/ico17.gif></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%POS width=1></TD></TR>\n",h-2);
	htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%POS></TD>\n",w-2);
	htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
	htrAddBodyItem(s,   "</TABLE>\n");*/
	htrAddBodyItem_va(s,"<DIV ID=\"dt%POScon1\" class=\"dtCon1 dtAbsHid\"></DIV>\n",id);
	htrAddBodyItem_va(s,"<DIV ID=\"dt%POScon2\" class=\"dtCon2 dtAbsHid\"></DIV>\n",id);
	htrAddBodyItem(s,   "</DIV>\n");

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","dt","dt_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","dt","dt_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","dt","dt_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER","dt","dt_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT","dt","dt_mouseout");

	/** Check for more sub-widgets within the datetime. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
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
	drv->Setup = htdtSetup;

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

