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

    $Id: htdrv_timer.c,v 1.4 2002/06/09 23:44:46 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_timer.c,v $

    $Log: htdrv_timer.c,v $
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

    	/** Get an id for this. **/
	id = (HTTM.idcnt++);

	/** Get msec for timer countdown **/
	if (objGetAttrValue(w_obj,"msec",POD(&msec)) != 0)
	    {
	    mssError(1,"HTTM","Timer widget must have 'msec' time specified");
	    return -1;
	    }

	/** Get auto reset and auto start settings **/
	if (objGetAttrValue(w_obj,"auto_reset",POD(&auto_reset)) != 0) auto_reset = 0;
	if (objGetAttrValue(w_obj,"auto_start",POD(&auto_start)) != 0) auto_start = 1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Timer expiration function **/
	htrAddScriptFunction(s, "tm_expire", "\n"
		"function tm_expire(tm)\n"
		"    {\n"
		"    if (tm.autoreset)\n"
		"        tm.timerid = setTimeout(tm_expire,tm.msec,tm);\n"
		"    else\n"
		"        tm.timerid = null;\n"
		"    if (tm.EventExpire != null)\n"
		"        {\n"
		"        eparam = new Object();\n"
		"        eparam.Caller = tm;\n"
		"        cn_activate(tm, 'Expire', eparam);\n"
		"        delete eparam;\n"
		"        }\n"
		"    }\n", 0);

	/** Set timer **/
	htrAddScriptFunction(s, "tm_action_settimer", "\n"
		"function tm_action_settimer(aparam)\n"
		"    {\n"
		"    if (this.timerid != null) clearTimeout(this.timerid)\n"
		"    if (aparam.AutoReset != null) this.autoreset = aparam.AutoReset;\n"
		"    this.timerid = setTimeout(tm_expire, aparam.Time, this);\n"
		"    }\n", 0);

	/** Cancel timer **/
	htrAddScriptFunction(s, "tm_action_canceltimer", "\n"
		"function tm_action_canceltimer(aparam)\n"
		"    {\n"
		"    if (this.timerid != null) clearTimeout(this.timerid)\n"
		"    this.timerid = null;\n"
		"    }\n", 0);

	/** Timer initializer **/
	htrAddScriptFunction(s, "tm_init", "\n"
		"function tm_init(t,ar,as)\n"
		"    {\n"
		"    tm = new Object();\n"
		"    tm.autostart = as;\n"
		"    tm.autoreset = ar;\n"
		"    tm.msec = t;\n"
		"    tm.ActionSetTimer = tm_action_settimer;\n"
		"    tm.ActionCancelTimer = tm_action_canceltimer;\n"
		"    if (as)\n"
		"        tm.timerid = setTimeout(tm_expire,t,tm);\n"
		"    else\n"
		"        tm.timerid = null;\n"
		"    return tm;\n"
		"    }\n", 0);

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
	strcpy(drv->Target, "Netscape47x:default");

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

	HTTM.idcnt = 0;

    return 0;
    }
