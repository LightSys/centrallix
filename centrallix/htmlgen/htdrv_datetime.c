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

    $Id: htdrv_datetime.c,v 1.15 2002/07/20 19:44:25 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_datetime.c,v $

    $Log: htdrv_datetime.c,v $
    Revision 1.15  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.14  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.13  2002/07/17 20:20:43  lkehresman
    Overhaul of the datetime widget (c file)

    Revision 1.12  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

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
    int x,y,w,h,w2=184,h2=190;
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
	htrAddStylesheetItem_va(s,"\t#dt%dbtn  { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	htrAddStylesheetItem_va(s,"\t#dt%dpane { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y+h,w2,h2,z+1);
	htrAddStylesheetItem_va(s,"\t#dt%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+1);
	htrAddStylesheetItem_va(s,"\t#dt%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:1; TOP:1; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w-20,h-2,z+1);
	htrAddStylesheetItem_va(s,"\t#dt%dcon3 { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:44; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w2-2,100,z+2);
	htrAddStylesheetItem_va(s,"\t#dt%dcon4 { POSITION:absolute; VISIBILITY:hidden; LEFT:1; TOP:44; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w2-2,100,z+2);
	htrAddStylesheetItem_va(s,"\t#dt%dcon5 { POSITION:absolute; VISIBILITY:inherit; LEFT:36; TOP:2; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w2-72,20,z+3);
	htrAddStylesheetItem_va(s,"\t#dt%dcon6 { POSITION:absolute; VISIBILITY:hidden; LEFT:36; TOP:2; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,w2-72,20,z+3);

	/** Write named global **/
	htrAddScriptGlobal(s, "dt_current", "null", 0);

	/** Script includes **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_date.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_datetime.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    dt_init("
	                       "%s.layers.dt%dbtn, "
	                       "%s.layers.dt%dpane, "
	                       "%s.layers.dt%dbtn.document.layers.dt%dcon1, "
	                       "%s.layers.dt%dbtn.document.layers.dt%dcon2, "
	                       "%s.layers.dt%dpane.document.layers.dt%dcon3, "
	                       "%s.layers.dt%dpane.document.layers.dt%dcon4, "
	                       "%s.layers.dt%dpane.document.layers.dt%dcon5, "
	                       "%s.layers.dt%dpane.document.layers.dt%dcon6, "
	                       "\"%s\",\"%s\",\"%s\",\"%s\", %d)\n",
			parentname,id, 
			parentname,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			parentname,id,id, 
			initialdate, bgcolor, fgcolor, fieldname, w-20);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dbtn\"><BODY %s>\n", id,bgcolor);
	htrAddBodyItem_va(s,"<TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n");
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
	htrAddBodyItem_va(s,"       <TD align=right valign=middle><IMG SRC=/sys/images/ico17.gif></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"</TABLE>\n");
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dcon2\"></DIV>\n",id);
	htrAddBodyItem_va(s,"</BODY></DIV>\n");
		
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dpane\"><BODY %s>\n",id,bgcolor);
	htrAddBodyItem_va(s,"<TABLE border=0 cellpadding=0 cellspacing=0 width=%d height=%d>\n",w2,h2);
	htrAddBodyItem_va(s,"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>\n");
	htrAddBodyItem_va(s,"    <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w2-2);
	htrAddBodyItem_va(s,"    <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>\n");
	htrAddBodyItem_va(s,"<TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h2-2);
	htrAddBodyItem_va(s,"    <TD valign=top>\n");
	htrAddBodyItem_va(s,"    <TABLE height=25 cellpadding=0 cellspacing=0 border=0>\n");
	htrAddBodyItem_va(s,"    <TR><TD width=18><IMG SRC=/sys/images/ico16aa.gif></TD>\n");
	htrAddBodyItem_va(s,"        <TD width=18><IMG SRC=/sys/images/ico16ba.gif></TD>\n");
	htrAddBodyItem_va(s,"        <TD width=%d></TD>\n",w2-72);
	htrAddBodyItem_va(s,"        <TD width=18><IMG SRC=/sys/images/ico16ca.gif></TD>\n");
	htrAddBodyItem_va(s,"        <TD width=18><IMG SRC=/sys/images/ico16da.gif></TD></TR>\n");
	htrAddBodyItem_va(s,"    </TABLE>\n");
	htrAddBodyItem_va(s,"    <TABLE width=%d cellpadding=0 cellspacing=0 border=0>\n",w2);
	htrAddBodyItem_va(s,"    <TR><TD align=center><B>S</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>M</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>T</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>W</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>T</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>F</B></TD>\n");
	htrAddBodyItem_va(s,"        <TD align=center><B>S</B></TD></TR>\n");
	htrAddBodyItem_va(s,"    </TABLE>\n");
	htrAddBodyItem_va(s,"    <DIV ID=\"dt%dcon3\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <DIV ID=\"dt%dcon4\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <DIV ID=\"dt%dcon5\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <DIV ID=\"dt%dcon6\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    </TD>\n");
	htrAddBodyItem_va(s,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h2-2);
	htrAddBodyItem_va(s,"<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>\n");
	htrAddBodyItem_va(s,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w2-2);
	htrAddBodyItem_va(s,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>\n");
	htrAddBodyItem_va(s,"</TABLE>\n");
	htrAddBodyItem_va(s,"</BODY></DIV>\n");

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","dt",
		"    if (e.target.kind && e.target.kind.substr(0, 5) == 'dtimg') {\n"
		"        eval('dt_'+e.target.kind.substr(6, 4)+'()');\n"
		"    } else {\n"
		"        if (ly.kind && ly.kind.substr(0, 2) == 'dt') {\n"
		"            dt_mousedown(ly);\n"
		"        } else if (dt_current && dt_current != ly) {\n"
		"            dt_current.PaneLayer.visibility = 'hide';\n"
		"            dt_current = null;\n"
		"        }\n"
		"    }\n");
	htrAddEventHandler(s, "document","MOUSEUP","dt",
		"    if (ly.kind && ly.kind.substr(0, 2) == 'dt') {\n"
		"        dt_mouseup(ly);\n"
		"    }\n");



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
	drv->Verify = htdtVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTDT.idcnt = 0;

    return 0;
    }
