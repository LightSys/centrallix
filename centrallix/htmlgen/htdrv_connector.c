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
/* Module: 	htdrv_connector.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 24, 1998 					*/
/* Description:	HTML Widget driver for a 'connector', which joins an 	*/
/*		event on the parent widget with an action on another	*/
/*		specified widget.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_connector.c,v 1.1 2001/08/13 18:00:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_connector.c,v $

    $Log: htdrv_connector.c,v $
    Revision 1.1  2001/08/13 18:00:49  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCONN;


/*** htconnVerify - not written yet.
 ***/
int
htconnVerify()
    {
    return 0;
    }


/*** htconnRender - generate the HTML code for the page.
 ***/
int
htconnRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
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
    int id;
    char* nptr;
    XString xs;

    	/** Get an id for this. **/
	id = (HTCONN.idcnt++);

	/** Get the event linkage information **/
	if (objGetAttrValue(w_obj,"event",POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTCONN","Connector must have an 'event' property");
	    return -1;
	    }
	strcpy(event,ptr);
	if (objGetAttrValue(w_obj,"target",POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have a 'target' property");
	    return -1;
	    }
	strcpy(target,ptr);
	if (objGetAttrValue(w_obj,"action",POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have an 'action' property");
	    return -1;
	    }
	strcpy(action,ptr);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Add globals for event param handling **/
	/*htrAddScriptGlobal(s, "aparam", "null", 0);
	htrAddScriptGlobal(s, "eparam", "null", 0);*/

	/** Add a script init to install the connector **/
	sprintf(sbuf,"    %s = new cn_init(%s);\n    %s.RunEvent = cn_%d;\n", nptr, parentobj, nptr, id);
	htrAddScriptInit(s, sbuf);
	sprintf(sbuf,"    %s.Event%s = %s.RunEvent;\n", parentobj, event, nptr);
	htrAddScriptInit(s, sbuf);

	/** Add function to instantiate objects **/
	htrAddScriptFunction(s, "cn_init", "\n"
		"function cn_init(p)\n"
		"    {\n"
		"    this.type = 'cn';\n"
		"    this.LSParent = p;\n"
		"    }\n", 0);

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
		    sprintf(sbuf, "    aparam.%s = %f;\n",ptr,vdbl);
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
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+2, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** htconnInitialize - register with the ht_render module.
 ***/
int
htconnInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Event-Action Connector Driver");
	strcpy(drv->WidgetName,"connector");
	drv->Render = htconnRender;
	drv->Verify = htconnVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Register. **/
	htrRegisterDriver(drv);

	HTCONN.idcnt = 0;

    return 0;
    }
