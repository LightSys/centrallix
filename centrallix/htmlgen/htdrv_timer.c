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
/* Module: 	htdrv_timer.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 1, 2001 					*/
/* Description:	HTML Widget driver for a timer nonvisual widget.  The	*/
/*		timer fires off an Event every so often that can cause	*/
/*		other actions on the page to occur.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_timer.c,v 1.8 2003/06/21 23:07:26 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_timer.c,v $

    $Log: htdrv_timer.c,v $
    Revision 1.8  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.7  2002/12/04 00:19:12  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.6  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.5  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.4  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 05:55:17  gbeeley
    Fixed html problem in the timer.  Doesn't need a closing DIV when
    it never had an opening DIV tag.  Will only mess things up, it will.

    Revision 1.1  2001/11/03 05:48:33  gbeeley
    Ok.  Really added the timer file this time.  Forgot last time.  Of
    course, the silly cvs log won't reflect my forgotten time in the
    timer widget file, because wasn't in cvs when it wasn't checked in
    yet.  Duh.  Obvious.  It's getting late....  I think my 'awake'
    timer has hit its 'expire' event ;)


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTM;


/*** httmVerify - not written yet.
 ***/
int
httmVerify()
    {
    return 0;
    }


/*** httmRender - generate the HTML code for the timer nonvisual widget.
 ***/
int
httmRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[200];
    char sbuf2[160];
    int id;
    char* nptr;
    int msec;
    int auto_reset = 0;
    int auto_start = 1;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTTM","Netscape DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTM.idcnt++);

	/** Get msec for timer countdown **/
	if (objGetAttrValue(w_obj,"msec",DATA_T_INTEGER,POD(&msec)) != 0)
	    {
	    mssError(1,"HTTM","Timer widget must have 'msec' time specified");
	    return -1;
	    }

	/** Get auto reset and auto start settings **/
	if (objGetAttrValue(w_obj,"auto_reset",DATA_T_INTEGER,POD(&auto_reset)) != 0) auto_reset = 0;
	if (objGetAttrValue(w_obj,"auto_start",DATA_T_INTEGER,POD(&auto_start)) != 0) auto_start = 1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_timer.js", 0);

	/** Script initialization call. **/
	snprintf(sbuf,200,"    %s = tm_init(%d, %d, %d);\n", nptr, msec, auto_reset, auto_start);
	htrAddScriptInit(s, sbuf);

	/** Check for objects within the timer. **/
	snprintf(sbuf,200,"%s.document",nptr);
	snprintf(sbuf2,160,"%s",nptr);
	htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);

    return 0;
    }


/*** httmInitialize - register with the ht_render module.
 ***/
int
httmInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Nonvisual Timer Widget");
	strcpy(drv->WidgetName,"timer");
	drv->Render = httmRender;
	drv->Verify = httmVerify;

	/** Add an 'expired' event **/
	htrAddEvent(drv,"Expire");

	/** Add a 'set timer' action **/
	htrAddAction(drv,"SetTimer");
	htrAddParam(drv,"SetTimer","Time",DATA_T_INTEGER);
	htrAddParam(drv,"SetTimer","AutoReset",DATA_T_INTEGER);

	/** Add a 'cancel timer' action **/
	htrAddAction(drv,"CancelTimer");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTM.idcnt = 0;

    return 0;
    }
