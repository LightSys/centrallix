#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "xstring.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_sys_osml.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 11, 1999 					*/
/* Description:	HTML Widget driver for an OSML session.  Allows the use	*/
/*		of the entire OSML interface from JavaScript on the	*/
/*		client side.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_sys_osml.c,v 1.5 2002/12/04 00:19:11 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/Attic/htdrv_sys_osml.c,v $

    $Log: htdrv_sys_osml.c,v $
    Revision 1.5  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.4  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.3  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.2  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTOSML;


/*** htosmlVerify - not written yet.
 ***/
int
htosmlVerify()
    {
    return 0;
    }


/*** htosmlRender - generate the HTML code for the page.
 ***/
int
htosmlRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char* vstr;
    int vint;
    double vdbl;
    char name[64];
    char sbuf[256];
    char* fnbuf;
    char fnname[16];
    char* fnnamebuf;
    char event[32];
    char target[32];
    char action[32];
    pObject sub_w_obj;
    pObjQuery qy;
    int x=-1,y=-1,w,h;
    int id,cnt;
    char* nptr;
    pObject content_obj;
    XString xs;

    	/** Get an id for this. **/
	id = (HTOSML.idcnt++);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s,"/sys/js/htdrv_sys_osml.js",0);

	/** Add a script init to install the connector **/
	sprintf(sbuf,"    %s = new cn_init(%s);\n    %s.RunEvent = cn_%d;\n", nptr, parentobj, nptr, id);
	htrAddScriptInit(s, sbuf);
	sprintf(sbuf,"    %s.Event%s = %s.RunEvent;\n", parentobj, event, nptr);
	htrAddScriptInit(s, sbuf);

	/** Add the connector function **/
	xsInit(&xs);
	sprintf(sbuf,	"\n"
		     	"function cn_%d(eparam)\n"
		     	"    {\n" ,id);
	xsConcatenate(&xs,sbuf,-1);
	xsConcatenate(&xs,"    aparam = new Object();\n",-1);
	for(ptr = objGetFirstAttr(w_obj); ptr; ptr = objGetNextAttr(w_obj))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action")) continue;
	    switch(objGetAttrType(w_obj, ptr))
	        {
		case DATA_T_INTEGER:
	    	    objGetAttrValue(w_obj, ptr, POD(&vint));
		    sprintf(sbuf, "    aparam.%s = %d;\n",ptr,vint);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_DOUBLE:
		    objGetAttrValue(w_obj, ptr, POD(&vdbl));
		    sprintf(sbuf, "    aparam.%s = %lf;\n",ptr,vdbl);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_STRING:
	    	    objGetAttrValue(w_obj, ptr, POD(&vstr));
		    if (!strpbrk(vstr," !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|"))
		        {
			sprintf(sbuf, "    aparam.%s = eparam.%s\n", ptr, vstr);
			xsConcatenate(&xs,sbuf,-1);
			}
		    else
		        {
			sprintf(sbuf, "    aparam.%s = ", ptr);
			xsConcatenate(&xs,sbuf,-1);
			xsConcatenate(&xs,vstr,-1);
			xsConcatenate(&xs,";\n",2);
			}
		    break;
		}
	    }
	sprintf(sbuf,"    %s.Action%s(aparam);\n", target, action);
	xsConcatenate(&xs,sbuf,-1);
	xsConcatenate(&xs,"    delete aparam;\n",-1);
	xsConcatenate(&xs,"    }\n\n",7);
	sprintf(fnname, "cn_%d",id);
	fnbuf = (char*)nmMalloc(strlen(xs.String)+1);
	strcpy(fnbuf,xs.String);
	fnnamebuf = (char*)nmMalloc(strlen(fnname)+1);
	strcpy(fnnamebuf, fnname);
	htrAddScriptFunction(s, fnnamebuf, fnbuf, HTR_F_NAMEALLOC | HTR_F_VALUEALLOC);
	xsDeInit(&xs);

	/** Add init for param passing structures **/
	/*htrAddScriptInit(s, "    if (eparam == null) eparam = new cn_init();\n");
	htrAddScriptInit(s, "    if (aparam == null) aparam = new cn_init();\n");*/

	/** Check for more sub-widgets within the conn entity. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while(sub_w_obj = objQueryFetch(qy, O_RDONLY))
	        {
		htrRenderWidget(s, sub_w_obj, z+2, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** htosmlInitialize - register with the ht_render module.
 ***/
int
htosmlInitialize()
    {
    pHtDriver drv;
    pHtEventAction action;
    pHtParam param;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"System:OSML Driver");
	strcpy(drv->WidgetName,"sys-osml");
	drv->Render = htosmlRender;
	drv->Verify = htosmlVerify;
	htrAddSupport(drv, HTR_UA_NETSCAPE_47);

	/** Register. **/
	htrRegisterDriver(drv);

	HTOSML.idcnt = 0;

    return 0;
    }
