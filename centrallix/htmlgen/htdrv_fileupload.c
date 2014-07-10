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
    char* dptr;
	char name[64];
    int id, i;
	int multiselect;
	char target[512];
	char fieldname[HT_FIELDNAME_SIZE];
	char form[64];

	/** Get an id for this. **/
	id = (HTFU.idcnt++);
	
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
	
	/** Write named global **/
	htrAddWgtrObjLinkage_va(s, tree, "document.getElementById(\"fu%POSbase\")",id);
	
	htrAddEventHandlerFunction(s, "document","CHANGE", "fu", "fu_change");
	
	/** Include the javascript code for the file uploader **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_fileupload.js", 0);
	
	htrAddScriptInit_va(s, "    fu_init({layer:wgtrGetNodeRef(ns,'%STR&SYM'), pane:document.getElementById(\"fu%POSform\"), input:document.getElementById(\"fu%POSinput\"), iframe:document.getElementById(\"fu%POSiframe\"), target:\"%STR&JSSTR\"});\n", name, id, id, id, target);
	
	/** style header items **/
	htrAddStylesheetItem_va(s,"#fu%POSbase { POSITION:absolute; VISIBILITY:hidden; }\n", id);
	htrAddBodyItem_va(s,"<DIV ID=\"fu%POSbase\"><FORM ID=\"fu%POSform\" METHOD=\"post\" ENCTYPE=\"multipart/form-data\" TARGET=\"fu%POSiframe\"><iframe ID=\"fu%POSiframe\" NAME=\"fu%POSiframe\"></iframe><INPUT ID=\"fu%POSinput\" TYPE=\"file\" NAME=\"fu%POSinput\" %STR/></FORM></DIV>", id, id, id, id, id, id, id, multiselect?"MULTIPLE":"");
	
	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z);
	
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