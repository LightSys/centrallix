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
/* Module: 	htdrv_frameset.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 17, 1999   					*/
/* Description:	HTML Widget driver for a frameset; use instead of a	*/
/*		widget/page item at the top level.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_frameset.c,v 1.1 2001/08/13 18:00:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_frameset.c,v $

    $Log: htdrv_frameset.c,v $
    Revision 1.1  2001/08/13 18:00:49  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** htsetVerify - not written yet.
 ***/
int
htsetVerify()
    {
    return 0;
    }


/*** htsetRender - generate the HTML code for the page.
 ***/
int
htsetRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char sbuf[128];
    pObject sub_w_obj;
    pObjQuery qy;
    char geom_str[64] = "";
    int t,n,bdr=0,direc=0;
    char nbuf[16];

    	/** Check for a title. **/
	if (objGetAttrValue(w_obj,"title",POD(&ptr)) == 0)
	    {
	    sprintf(sbuf,"    <TITLE>%s</TITLE>\n",ptr);
	    htrAddHeaderItem(s, sbuf);
	    }

	/** Loop through the frames (widget/page items) for geometry data **/
	htrDisableBody(s);
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		t = objGetAttrType(sub_w_obj, "framesize");
		if (t < 0) 
		    {
		    sprintf(nbuf,"*");
		    }
		else if (t == DATA_T_INTEGER)
		    {
		    objGetAttrValue(sub_w_obj, "framesize", POD(&n));
		    sprintf(nbuf,"%d",n);
		    }
		else if (t == DATA_T_STRING)
		    {
		    objGetAttrValue(sub_w_obj, "framesize", POD(&ptr));
		    memccpy(nbuf, ptr, 0, 15);
		    nbuf[15] = 0;
		    }
		if (geom_str[0] != '\0')
		    {
		    strcat(geom_str,",");
		    }
		strcat(geom_str,nbuf);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** Check for some optional params **/
	if (objGetAttrValue(w_obj,"direction",POD(&ptr)) != 0)
	    {
	    if (!strcmp(ptr,"rows")) direc=1;
	    else if (!strcmp(ptr,"columns")) direc=0;
	    }
	if (objGetAttrValue(w_obj,"borderwidth",POD(&n)) != 0)
	    { 
	    bdr = n;
	    }

	/** Build the frameset tag. **/
	sprintf(sbuf, "<FRAMESET %s=%s border=%d>\n", direc?"rows":"cols", geom_str, bdr);
	htrAddBodyItem(s, sbuf);

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj,"name",POD(&ptr));
		if (objGetAttrValue(sub_w_obj,"marginwidth",POD(&n)) != 0)
		    sprintf(sbuf,"    <FRAME SRC=./%s>\n",ptr);
		else
		    sprintf(sbuf,"    <FRAME SRC=./%s MARGINWIDTH=%d>\n",ptr,n);
		htrAddBodyItem(s,sbuf);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** End the framset. **/
	sprintf(sbuf, "</FRAMESET>\n");
	htrAddBodyItem(s, sbuf);

    return 0;
    }


/*** htsetInitialize - register with the ht_render module.
 ***/
int
htsetInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Frameset Driver");
	strcpy(drv->WidgetName,"frameset");
	drv->Render = htsetRender;
	drv->Verify = htsetVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Register. **/
	htrRegisterDriver(drv);

    return 0;
    }
