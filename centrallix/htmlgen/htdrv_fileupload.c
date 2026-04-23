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
#include "wgtr.h"

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
/* Module: 	htdrv_fileupload.c         				*/
/* Author:	Brady Steed (BLS)					*/
/* Creation:	May 29, 2014					*/
/* Description:	HTML Widget driver for a file uploader.		*/
/************************************************************************/

/** globals **/
static struct
	{
	int idcnt;
	}
	HTFU;

/*** htfuRender - generate the HTML code for the fileupload widget
***/
int
htfuRender(pHtSession s, pWgtrNode tree, int z)
	{
	char* ptr;
	char name[64];
	int multiselect;
	char target[512];
	char fieldname[HT_FIELDNAME_SIZE];
	char form[64];

	/** Get an id for this. **/
	const int id = (HTFU.idcnt++);
	
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
		strtcpy(name,ptr,sizeof(name));
		
	if (wgtrGetPropertyValue(tree,"target",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(target,ptr,sizeof(target));
	
	multiselect = htrGetBoolean(tree, "multiselect", 0); //default = 0;
	
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';
		
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';
	
 	/** Link the widget to the DOM node. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "fu%POSbase", id) != 0)
	    {
	    mssError(0, "HTFU", "Failed to add object linkage.");
	    goto err;
	    }
	
	if (htrAddEventHandlerFunction(s, "document", "CHANGE", "fu", "fu_change") != 0) goto err;
	
	/** Include the javascript code for the file uploader **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_fileupload.js", 0) != 0) goto err;
	
	if (htrAddScriptInit_va(s,
	    "\tfu_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"pane:document.getElementById('fu%POSform'), "
		"input:document.getElementById('fu%POSinput'), "
		"iframe:document.getElementById('fu%POSiframe'), "
		"target:'%STR&JSSTR', "
	    "});\n",
	    name, id, id, id, target
	) != 0)
	    {
	    mssError(0, "HTFU", "Failed to write JS init call.");
	    goto err;
	    }
	
	/** style header items **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#fu%POSbase { "
		"position:absolute; "
		"visibility:hidden; "
	    "}\n",
	    id
	) != 0)
	    {
	    mssError(0, "HTFU", "Failed to write base CSS.");
	    goto err;
	    }
	if (htrAddBodyItem_va(s,
	    "<div id='fu%POSbase'><form id='fu%POSform' method='post' enctype='multipart/form-data' target='fu%POSiframe'>"
		"<iframe id='fu%POSiframe' name='fu%POSiframe'></iframe>"
		"<input id='fu%POSinput' type='file' name='fu%POSinput' %[MULTIPLE%]/>"
	    "</form></div>",
	    id, id, id,
	    id, id,
	    id, id,
	    (multiselect)
	)!= 0)
	    {
	    mssError(0, "HTFU", "Failed to write base HTML.");
	    goto err;
	    }
	
	/** Check for more sub-widgets **/
	if (htrRenderSubwidgets(s, tree, z) != 0) goto err;

	return 0;

    err:
	mssError(0, "HTFU",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
	}//end htfuRender
	
	

/*** htfuInitialize - register with the ht_render module.
 ***/
int
htfuInitialize()
	{
	pHtDriver drv;
	
    /** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML File Upload Driver");
	strcpy(drv->WidgetName,"fileupload");
	drv->Render = htfuRender;
	
	/** Add actions **/
	htrAddAction(drv,"Reset");
	htrAddAction(drv,"Prompt");
	htrAddAction(drv,"Submit");
	
	/** Add event **/
	htrAddEvent(drv,"DataChange");
	htrAddParam(drv,"DataChange","NewValue",DATA_T_STRING);
	htrAddParam(drv,"DataChange","OldValue",DATA_T_STRING);
    
	htrAddEvent(drv,"Success");
	htrAddParam(drv,"Success","data",DATA_T_ARRAY);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTFU.idcnt = 0;

    return 0;
	}//end htfuInitialize
