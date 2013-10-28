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
/* Module: 	htdrv_execmethod.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 4, 2001 					*/
/* Description:	HTML Widget driver for an exec-method nonvisual object.	*/
/*		This widget basically executes a method in the object	*/
/*		system.  Later, this should be incorporated into a more	*/
/*		general purpose osml widget.				*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTEX;


/*** htexRender - generate the HTML code for the timer nonvisual widget.
 ***/
int
htexRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int id;
    char* objname;
    char* methodname = NULL;
    char* methodparam = NULL;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTTEX","Netscape DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTEX.idcnt++);

	/** Get params. **/
	if (wgtrGetPropertyValue(tree,"object",DATA_T_STRING,POD(&objname)) != 0) objname="";
	if (wgtrGetPropertyValue(tree,"method",DATA_T_STRING,POD(&methodname)) != 0) methodname="";
	if (wgtrGetPropertyValue(tree,"parameter",DATA_T_STRING,POD(&methodparam)) != 0) methodparam="";

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name, ptr, sizeof(name));

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    ex_init({node:wgtrGetNodeRef(ns,\"%STR&SYM\"), objname:'%STR&SYM', methname:'%STR&SYM', methparam:'%STR&JSSTR'});\n", 
		name, objname, methodname, methodparam);

	/** Check for objects within the exec method object. **/
	htrRenderSubwidgets(s, tree, z+2);

	htrAddScriptInclude(s,"/sys/js/htdrv_execmethod.js",0);

    return 0;
    }


/*** htexInitialize - register with the ht_render module.
 ***/
int
htexInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Method-Execution Widget");
	strcpy(drv->WidgetName,"execmethod");
	drv->Render = htexRender;

	/** Add a 'executemethod' action **/
	htrAddAction(drv,"ExecuteMethod");
	htrAddParam(drv,"ExecuteMethod","Objname",DATA_T_STRING);
	htrAddParam(drv,"ExecuteMethod","Method",DATA_T_STRING);
	htrAddParam(drv,"ExecuteMethod","Parameter",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTEX.idcnt = 0;

    return 0;
    }
