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
/* Module: 	htdrv_alert.c      					*/
/* Author:	Jonathan Rupp (JDR)		 			*/
/* Creation:	February 23, 2002 					*/
/* Description:	This is a very simple widget that will give the user  	*/
/*		a message.						*/
/************************************************************************/

/**CVSDATA***************************************************************
 
    $Log: htdrv_alerter.c,v $
    Revision 1.1  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTALRT;


/*** htalrtVerify - not written yet.
 ***/
int
htalrtVerify()
    {
    return 0;
    }


/*** htalrtRender - generate the HTML code for the alert -- not much..
 ***/
int
htalrtRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    //char sbuf[200];
    //char sbuf2[160];
    char *sbuf3;
    int id;
    char* nptr;
    
    	/** Get an id for this. **/
	id = (HTALRT.idcnt++);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); /* create our instance variable */

	htrAddScriptFunction(s, "alrt_action_alert", "\n"
		"function alrt_action_alert(sendthis)\n"
		"    {\n"
		"    window.alert(sendthis[\"param\"]);\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_action_confirm", "\n"
		"function alrt_action_confirm(sendthis)\n"
		"    {\n"
		"    window.confirm(sendthis[\"param\"]);\n"
		"    }\n", 0);


	/** Alert initializer **/
	htrAddScriptFunction(s, "alrt_init", "\n"
		"function alrt_init()\n"
		"    {\n"
		"    alrt = new Object();\n"
		"    alrt.ActionAlert = alrt_action_alert;\n"
		"    alrt.ActionConfirm = alrt_action_confirm;\n"
		"    return alrt;\n"
		"    }\n",0);

	sbuf3 = nmMalloc(200);
	snprintf(sbuf3,200,"    %s=alrt_init();\n",name);
	htrAddScriptInit(s,sbuf3);
	nmFree(sbuf3,200);

    return 0;
    }


/*** htalrtInitialize - register with the ht_render module.
 ***/
int
htalrtInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Alert Widget");
	strcpy(drv->WidgetName,"alerter");
	drv->Render = htalrtRender;
	drv->Verify = htalrtVerify;

	/** Add a 'executemethod' action **/
	htrAddAction(drv,"Alert");
	htrAddParam(drv,"Alert","Parameter",DATA_T_STRING);
	htrAddAction(drv,"Confirm");
	htrAddParam(drv,"Confirm","Parameter",DATA_T_STRING);


	/** Register. **/
	htrRegisterDriver(drv);

	HTALRT.idcnt = 0;

    return 0;
    }
