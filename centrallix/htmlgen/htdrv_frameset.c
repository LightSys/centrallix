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

    $Id: htdrv_frameset.c,v 1.6 2002/09/27 22:26:05 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_frameset.c,v $

    $Log: htdrv_frameset.c,v $
    Revision 1.6  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.5  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.4  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

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

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

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
    pObject sub_w_obj;
    pObjQuery qy;
    char geom_str[64] = "";
    int t,n,bdr=0,direc=0;
    char nbuf[16];

    	/** Check for a title. **/
	if (objGetAttrValue(w_obj,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s,"    <TITLE>%s</TITLE>\n",ptr);
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
		    snprintf(nbuf,16,"*");
		    }
		else if (t == DATA_T_INTEGER)
		    {
		    objGetAttrValue(sub_w_obj, "framesize", DATA_T_INTEGER,POD(&n));
		    snprintf(nbuf,16,"%d",n);
		    }
		else if (t == DATA_T_STRING)
		    {
		    objGetAttrValue(sub_w_obj, "framesize", DATA_T_STRING,POD(&ptr));
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
	if (objGetAttrValue(w_obj,"direction",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    if (!strcmp(ptr,"rows")) direc=1;
	    else if (!strcmp(ptr,"columns")) direc=0;
	    }
	if (objGetAttrValue(w_obj,"borderwidth",DATA_T_INTEGER,POD(&n)) != 0)
	    { 
	    bdr = n;
	    }

	/** Build the frameset tag. **/
	htrAddBodyItem_va(s, "<FRAMESET %s=%s border=%d>\n", direc?"rows":"cols", geom_str, bdr);

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj,"name",DATA_T_STRING,POD(&ptr));
		if (objGetAttrValue(sub_w_obj,"marginwidth",DATA_T_INTEGER,POD(&n)) != 0)
		    htrAddBodyItem_va(s,"    <FRAME SRC=./%s>\n",ptr);
		else
		    htrAddBodyItem_va(s,"    <FRAME SRC=./%s MARGINWIDTH=%d>\n",ptr,n);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** End the framset. **/
	htrAddBodyItem_va(s, "</FRAMESET>\n");
    return 0;
    }


/*** htsetInitialize - register with the ht_render module.
 ***/
int
htsetInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Frameset Driver");
	strcpy(drv->WidgetName,"frameset");
	drv->Render = htsetRender;
	drv->Verify = htsetVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

    return 0;
    }
