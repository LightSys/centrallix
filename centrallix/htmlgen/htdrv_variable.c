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
/* Module: 	htdrv_variable.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 6th, 1999 						*/
/* Description:	HTML Widget driver for a 'variable', which simply 	*/
/*		provides a place to store a value.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_variable.c,v 1.3 2002/06/09 23:44:46 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_variable.c,v $

    $Log: htdrv_variable.c,v $
    Revision 1.3  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.1.1.1  2001/08/13 18:00:52  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:56  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTVBL;


/*** htvblVerify - not written yet.
 ***/
int
htvblVerify()
    {
    return 0;
    }


/*** htvblRender - generate the HTML code for the page.
 ***/
int
htvblRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[HT_SBUF_SIZE];
    pObject sub_w_obj;
    pObjQuery qy;
    int t;
    int id;
    char* nptr;
    char* vptr;
    char* avptr;

    	/** Get an id for this. **/
	id = (HTVBL.idcnt++);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** Get type and value **/
	t = objGetAttrType(w_obj,"value");
	if (t == -1)
	    {
	    htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
	    }
	else if (t == DATA_T_STRING)
	    {
	    objGetAttrValue(w_obj,"value",POD(&vptr));
	    avptr = (char*)nmMalloc(strlen(vptr)+3);
	    sprintf(avptr, "\"%s\"",vptr);
	    htrAddScriptGlobal(s, nptr, avptr, HTR_F_NAMEALLOC | HTR_F_VALUEALLOC);
	    }
	else if (t == DATA_T_INTEGER)
	    {
	    objGetAttrValue(w_obj,"value",POD(&t));
	    snprintf(sbuf, HT_SBUF_SIZE, "%d", t);
	    avptr = (char*)nmMalloc(strlen(sbuf)+1);
	    strcpy(avptr,sbuf);
	    htrAddScriptGlobal(s, nptr, avptr, HTR_F_NAMEALLOC | HTR_F_VALUEALLOC);
	    }

	/** Check for more sub-widgets within the vbl entity. **/
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


/*** htvblInitialize - register with the ht_render module.
 ***/
int
htvblInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Variable Object Driver");
	strcpy(drv->WidgetName,"variable");
	drv->Render = htvblRender;
	drv->Verify = htvblVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTVBL.idcnt = 0;

    return 0;
    }
