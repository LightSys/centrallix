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

    $Id: htdrv_connector.c,v 1.3 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_connector.c,v $

    $Log: htdrv_connector.c,v $
    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

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
    char sbuf[HT_SBUF_SIZE];
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
	memccpy(event,ptr,0,31);
	event[31]=0;
	if (objGetAttrValue(w_obj,"target",POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have a 'target' property");
	    return -1;
	    }
	memccpy(target,ptr,0,31);
	target[31]=0;
	if (objGetAttrValue(w_obj,"action",POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have an 'action' property");
	    return -1;
	    }
	memccpy(action,ptr,0,31);
	action[31]=0;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Add globals for event param handling **/
	/*htrAddScriptGlobal(s, "aparam", "null", 0);
	htrAddScriptGlobal(s, "eparam", "null", 0);*/

	/** Add a script init to install the connector **/
	snprintf(sbuf,HT_SBUF_SIZE,"    %s = new cn_init(%s,cn_%d);\n", nptr, parentobj, id);
	htrAddScriptInit(s, sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"    %s.Add(%s,'%s');\n", nptr, parentobj, event);
	htrAddScriptInit(s, sbuf);

	/** Connector function to activate an action **/
	htrAddScriptFunction(s, "cn_activate", "\n"
		"function cn_activate(t,f,eparam)\n"
		"    {\n"
		"    if (t['Event' + f].constructor == Array)\n"
		"        {\n"
		"        for(var fn in t['Event' + f])\n"
		"            x = t['Event' + f][fn](eparam);\n"
		"        return x;\n"
		"        }\n"
		"    else\n"
		"        return t['Event' + f](eparam);\n"
		"    }\n", 0);

	/** Function to add a connector to a widget's event **/
	htrAddScriptFunction(s, "cn_add", "\n"
		"function cn_add(w,e)\n"
		"    {\n"
		"    if (w['Event' + e] == null)\n"
		"        w['Event' + e] = new Array();\n"
		"    w['Event' + e][w['Event' + e].length] = this.RunEvent;\n"
		"    }\n", 0);

	/** Add function to instantiate objects **/
	htrAddScriptFunction(s, "cn_init", "\n"
		"function cn_init(p,f)\n"
		"    {\n"
		"    this.Add = cn_add;\n"
		"    this.type = 'cn';\n"
		"    this.LSParent = p;\n"
		"    this.RunEvent = f;\n"
		"    }\n", 0);

	/** Add the connector function **/
	xsInit(&xs);
	xsConcatPrintf(&xs, "\n"
		     	"function cn_%d(eparam)\n"
		     	"    {\n" ,id);
	xsConcatenate(&xs,"    aparam = new Object();\n",-1);
	for(ptr = objGetFirstAttr(w_obj); ptr; ptr = objGetNextAttr(w_obj))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action")) continue;
	    switch(objGetAttrType(w_obj, ptr))
	        {
		case DATA_T_INTEGER:
	    	    objGetAttrValue(w_obj, ptr, POD(&vint));
		    snprintf(sbuf, HT_SBUF_SIZE, "    aparam.%s = %d;\n",ptr,vint);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_DOUBLE:
		    objGetAttrValue(w_obj, ptr, POD(&vdbl));
		    snprintf(sbuf, HT_SBUF_SIZE, "    aparam.%s = %f;\n",ptr,vdbl);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_STRING:
	    	    objGetAttrValue(w_obj, ptr, POD(&vstr));
		    if (!strpbrk(vstr," !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|"))
		        {
			snprintf(sbuf, HT_SBUF_SIZE, "    aparam.%s = eparam.%s\n", ptr, vstr);
			xsConcatenate(&xs,sbuf,-1);
			}
		    else
		        {
			snprintf(sbuf, HT_SBUF_SIZE, "    aparam.%s = ", ptr);
			xsConcatenate(&xs,sbuf,-1);
			xsConcatenate(&xs,vstr,-1);
			xsConcatenate(&xs,";\n",2);
			}
		    break;
		}
	    }
	xsConcatPrintf(&xs,"    %s.Action%s(aparam);\n", target, action);
	xsConcatenate(&xs,"    delete aparam;\n",-1);
	xsConcatenate(&xs,"    }\n\n",7);
	snprintf(fnname, HT_SBUF_SIZE, "cn_%d",id);
	fnbuf = (char*)nmMalloc(strlen(xs.String)+1);
	strcpy(fnbuf,xs.String);
	fnnamebuf = (char*)nmMalloc(strlen(fnname)+1);
	strcpy(fnnamebuf, fnname);
	htrAddScriptFunction(s, fnnamebuf, fnbuf, HTR_F_NAMEALLOC | HTR_F_VALUEALLOC);
	xsDeInit(&xs);

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
