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

/**CVSDATA***************************************************************

    $Id: htdrv_datetime.c,v 1.11 2002/07/16 17:52:00 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_datetime.c,v $

    $Log: htdrv_datetime.c,v $
    Revision 1.11  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.10  2002/07/15 22:24:15  lkehresman
    Updated datetime widget to include generic date manipulation script

    Revision 1.9  2002/07/15 21:20:21  lkehresman
    Stripped out all functions and added htrAddScriptInclude() function
    calls to include the .js files.

    Revision 1.8  2002/07/15 20:36:32  lkehresman
    Added another check for blank or invalid dates

    Revision 1.7  2002/07/15 18:16:39  lkehresman
    * Removed some flickering
    * Fixed a couple minor bugs with invalid dates

    Revision 1.6  2002/07/12 14:56:27  lkehresman
    Added a *simple* fix for invalid dates stored in the database.  This needs
    to be expanded in the future, but now it at least doesn't throw javascript
    errors any more.

    Revision 1.5  2002/07/12 14:24:54  lkehresman
    Added form interaction with the datetime widget.  Works with the Kardia demo!!

    Revision 1.4  2002/07/10 20:54:29  lkehresman
    Corrected the leapyear detection

    Revision 1.3  2002/07/10 20:18:01  lkehresman
    Added time controls

    Revision 1.2  2002/07/09 18:58:49  lkehresman
    Added double buffering to the datetime widget to reduse flickering

    Revision 1.1  2002/07/09 14:09:04  lkehresman
    Added first revision of the datetime widget.  No form interatction, and no
    time setting functionality, only date.  This has been on my laptop for a
    while and I wanted to get it into CVS for backup purposes.  More functionality
    to come soon.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTDT;


/*** htdtVerify - not written yet.
 ***/
int
htdtVerify()
    {
    return 0;
    }


/*** htdtRender - generate the HTML code for the page.
 ***/
int
htdtRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char *sql;
    char *str;
    char *attr;
    char name[128];
    char initialdate[64];
    char fgcolor[64];
    char bgcolor[128];
    char fieldname[30];
    int type, rval;
    int x,y,w,h;
    int id;
    DateTime dt;
    ObjData od;
    pObject qy_obj;
    pObjQuery qy;

	/** Get an id for this. **/
	id = (HTDT.idcnt++);

	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'height' property");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
	    strncpy(fieldname,ptr,30);
	else 
	    fieldname[0]='\0';

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get initial date **/
	if (objGetAttrValue(w_obj, "sql", POD(&sql)) == 0) 
	    {
	    if ((qy = objMultiQuery(w_obj->Session, sql))) 
		{
		while ((qy_obj = objQueryFetch(qy, O_RDONLY)))
		    {
		    attr = objGetFirstAttr(qy_obj);
		    if (!attr)
			{
			mssError(1, "HTDT", "There was an error getting date from your SQL query");
			return -1;
			}
		    type = objGetAttrType(qy_obj, attr);
		    rval = objGetAttrValue(qy_obj, attr, &od);
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    snprintf(initialdate, 64, "%s", str);
		    objClose(qy_obj);
		    }
		objQueryClose(qy);
		}
	    }
	else if (objGetAttrValue(w_obj,"initialdate",POD(&ptr)) == 0)
	    {
	    memccpy(initialdate, ptr, '\0', 63);
	    }
	else
	    {
	    initialdate[0]='\0';
	    }
	if (strlen(initialdate))
	    {
	    objDataToDateTime(DATA_T_STRING, initialdate, &dt, NULL);
	    snprintf(initialdate, 64, "%s %d %d, %d:%d:%d", obj_short_months[dt.Part.Month], 
	                          dt.Part.Day+1,
	                          dt.Part.Year+1900,
	                          dt.Part.Hour,
	                          dt.Part.Minute,
	                          dt.Part.Second);
	    }
	

	/** Get colors **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(bgcolor,"bgcolor=%.100s",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(bgcolor,"background='%.90s'",ptr);
	else
	    strcpy(bgcolor,"bgcolor=#c0c0c0");
	if (objGetAttrValue(w_obj,"fgcolor",POD(&ptr)) == 0)
	    sprintf(fgcolor,"%.63s",ptr);
	else
	    strcpy(fgcolor,"black");

	/** Ok, write the style header items. **/
	htrAddHeaderItem(s,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem_va(s,"\t#dt%dpane1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	htrAddHeaderItem_va(s,"\t#dt%dbg1   { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:0; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w,h,z+1);
	htrAddHeaderItem_va(s,"\t#dt%dbg2   { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-1,h-1,z+2);
	htrAddHeaderItem_va(s,"\t#dt%dbody  { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+3);
	htrAddHeaderItem_va(s,"\t#dt%dimg   { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:1; WIDTH:18; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+4);
	htrAddHeaderItem_va(s,"\t#dt%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:182; HEIGHT:190; Z-INDEX:%d; }\n",id,x,y+h,w,z+5);
	htrAddHeaderItem(s,"    </STYLE>\n");

	/** Write named global **/
	sprintf(name, "%s.layers.dt%dpane", parentname, id);
	htrAddScriptGlobal(s, "dt_current", "null", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_date.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_datetime.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    dt_init("
	                       "%s.layers.dt%dpane1, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbg1, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbg2, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dbody, "
	                       "%s.layers.dt%dpane1.document.layers.dt%dimg, "
	                       "%s.layers.dt%dpane2, "
	                       "%d,%d,\"%s\",\"%s\",\"%s\",\"%s\")\n",
			parentname,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id, 
			w,h, initialdate, bgcolor, fgcolor, fieldname);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s, /* Date Display Button */ 
		"<DIV ID=\"dt%dpane1\">\n"
		"<DIV ID=\"dt%dbg1\">\n"
		"  <BODY bgcolor=\"#ffffff\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD><IMG src=/sys/images/trans_1.gif height=1 width=1></TD></TR>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dbg2\">\n"
		"  <BODY bgcolor=\"#7a7a7a\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD><IMG src=/sys/images/trans_1.gif height=1 width=1></TD></TR>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dbg3\">\n"
		"  <BODY bgcolor=\"#7a7a7a\"></BODY>\n"
		"</DIV><DIV ID=\"dt%dbody\"><BODY %s>\n"
		"  <TABLE border=0 cellspacing=0 cellpadding=0 width=%d height=%d>\n"
		"  <TR><TD align=center valign=middle nowrap></TD>\n"
		"  </TABLE></BODY>\n"
		"</DIV><DIV ID=\"dt%dimg\">\n"
		"  <TABLE border=0 cellspacing=0 cellpadding=0 %s width=18 height=%d>\n"
		"  <TD valign=middle align=right><img src=/sys/images/ico17.gif></TD></TR>\n"
		"  </TABLE>\n"
		"</DIV>\n"
		"</DIV>\n", 
		id, id, w, h, id, w-1, h-1, id, id, bgcolor, w-2, h-2, id, bgcolor, h-2); 
	htrAddBodyItem_va(s, 
		"<DIV ID=\"dt%dpane2\">\n"
		"<TABLE border=0 cellpadding=0 cellspacing=0 width=182 height=142>\n"
		"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>\n"
		"    <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=182></TD>\n"
		"    <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>\n"
		"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=190 width=1></TD>\n"
		"    <TD %s>&nbsp;</TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=190 width=1></TD></TR>\n"
		"<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=182></TD>\n"
		"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>\n"
		"</TABLE>\n"
		"</DIV>\n", 
		id, bgcolor);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","dt",
		"    var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
		"    if (dt_current != null && targetLayer.layer != dt_current && targetLayer.subkind != 'dt_dropdown')\n"
		"        dt_event_mousedown1(targetLayer);\n"
		"    else if (targetLayer != null && targetLayer.kind == 'dt')\n"
		"        dt_event_mousedown2(targetLayer);\n\n");

	htrAddEventHandler(s, "document","MOUSEUP","dt",
		"    var targetLayer = (e.target.layer == null) ? e.target : e.target.layer;\n"
		"    if (dt_current != null && targetLayer.kind == 'dt' && (targetLayer.subkind == null || targetLayer.subkind == 'dt_button'))\n"
		"        dt_setmode(targetLayer,1);\n\n");



    return 0;
    }


/*** htdtInitialize - register with the ht_render module.
 ***/
int
htdtInitialize()
    {
    pHtDriver drv;

	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Date/Time Widget Driver");
	strcpy(drv->WidgetName,"datetime");
	drv->Render = htdtRender;
	drv->Verify = htdtVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTDT.idcnt = 0;

    return 0;
    }
