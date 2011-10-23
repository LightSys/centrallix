#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

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


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTM;



/*** httmRender - generate the HTML code for the timer nonvisual widget.
 ***/
int
httmRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int id;
    int msec;
    int auto_reset = 0;
    int auto_start = 1;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTM","Netscape 4.x or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTM.idcnt++);

	/** Get msec for timer countdown **/
	if (wgtrGetPropertyValue(tree,"msec",DATA_T_INTEGER,POD(&msec)) != 0)
	    {
	    mssError(1,"HTTM","Timer widget must have 'msec' time specified");
	    return -1;
	    }

	/** Get auto reset and auto start settings **/
	if (wgtrGetPropertyValue(tree,"auto_reset",DATA_T_INTEGER,POD(&auto_reset)) != 0) auto_reset = 0;
	if (wgtrGetPropertyValue(tree,"auto_start",DATA_T_INTEGER,POD(&auto_start)) != 0) auto_start = 1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	htrAddScriptInclude(s, "/sys/js/htdrv_timer.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    tm_init({node:nodes[\"%STR&SYM\"], time:%INT, autoreset:%INT, autostart:%INT});\n", name, msec, auto_reset, auto_start);

	/** Check for objects within the timer. **/
	htrRenderSubwidgets(s, tree, z+2);

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
