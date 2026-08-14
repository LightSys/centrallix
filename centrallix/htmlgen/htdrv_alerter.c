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
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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


/*** htalrtRender - generate the HTML code for the alert -- not much..
 ***/
int
htalrtRender(pHtSession s, pWgtrNode tree, int z)
    {

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTALRT", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Script include call **/
	if (htrAddScriptInclude(s, "/sys/js/htdrv_alerter.js", 0) != 0)
	    {
	    mssError(0, "HTALRT", "Failed to include JS.");
	    goto err;
	    }

	/** Script initialization call. **/
	if (htrAddScriptInit_va(s, "\talrt_init(wgtrGetNodeRef(ns, '%STR&SYM'));\n", wgtrGetName(tree)) != 0)
	    {
	    mssError(0, "HTALRT", "Failed to write JS init call.");
	    goto err;
	    }

	return 0;

    err:
	mssError(0, "HTALRT",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
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

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");


    return 0;
    }
