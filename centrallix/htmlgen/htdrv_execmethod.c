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
/* Module: 	htdrv_execmethod.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 4, 2001 					*/
/* Description:	HTML Widget driver for an exec-method nonvisual object.	*/
/*		This widget basically executes a method in the object	*/
/*		system.  Later, this should be incorporated into a more	*/
/*		general purpose osml widget.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_execmethod.c,v 1.2 2002/02/07 03:48:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_execmethod.c,v $

    $Log: htdrv_execmethod.c,v $
    Revision 1.2  2002/02/07 03:48:53  gbeeley
    I think I fixed a bug where the param wasn't defaulting to the widget's
    supplied Parameter property during an ExecuteMethod action.

    Revision 1.1  2001/11/12 20:43:44  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

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
    HTEX;


/*** htexVerify - not written yet.
 ***/
int
htexVerify()
    {
    return 0;
    }


/*** htexRender - generate the HTML code for the timer nonvisual widget.
 ***/
int
htexRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[200];
    char sbuf2[160];
    int id;
    char* nptr;
    char* objname;
    char* methodname;
    char* methodparam;

    	/** Get an id for this. **/
	id = (HTEX.idcnt++);

	/** Get params. **/
	if (objGetAttrValue(w_obj,"object",POD(&objname)) != 0) objname="";
	if (objGetAttrValue(w_obj,"method",POD(&objname)) != 0) methodname="";
	if (objGetAttrValue(w_obj,"parameter",POD(&objname)) != 0) methodparam="";

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Set timer **/
	htrAddScriptFunction(s, "ex_action_domethod", "\n"
		"function ex_action_domethod(aparam)\n"
		"    {\n"
		"    o = aparam.Objname?aparam.Objname:this.Objname;\n"
		"    m = aparam.Method?aparam.Method:this.Methodname;\n"
		"    p = aparam.Parameter?aparam.Parameter:this.Methodparam;\n"
		"    if (!o || !m || !p) return false;\n"
		"    this.HiddenLayer.load(o + '?ls__mode=execmethod&ls__methodname=' + (escape(m).replace(/\\//g,'%2f')) + '&ls__methodparam=' + (escape(p).replace(/\\//g,'%2f')), 64);\n"
		"    return true;\n"
		"    }\n", 0);

	/** Timer initializer **/
	htrAddScriptFunction(s, "ex_init", "\n"
		"function ex_init(o,m,p)\n"
		"    {\n"
		"    ex = new Object();\n"
		"    ex.Objname = o;\n"
		"    ex.Methodname = m;\n"
		"    ex.Methodparam = p;\n"
		"    ex.ActionExecuteMethod = ex_action_domethod;\n"
		"    ex.HiddenLayer = new Layer(64);\n"
		"    return ex;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	sprintf(sbuf,"    %s = ex_init('%s', '%s', '%s');\n", nptr, objname, methodname, methodparam);
	htrAddScriptInit(s, sbuf);

	/** Check for objects within the timer. **/
	sprintf(sbuf,"%s.document",nptr);
	sprintf(sbuf2,"%s",nptr);
	htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);

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
	drv->Verify = htexVerify;

	/** Add a 'executemethod' action **/
	htrAddAction(drv,"ExecuteMethod");
	htrAddParam(drv,"ExecuteMethod","Objname",DATA_T_STRING);
	htrAddParam(drv,"ExecuteMethod","Method",DATA_T_STRING);
	htrAddParam(drv,"ExecuteMethod","Parameter",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	HTEX.idcnt = 0;

    return 0;
    }
