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
/* Module: 	htdrv_variable.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 6th, 1999 						*/
/* Description:	HTML Widget driver for a 'variable', which simply 	*/
/*		provides a place to store a value.			*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTVBL;


/*** htvblRender - generate the HTML code for the page.
 ***/
int
htvblRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int t;
    int id, i;
    int n = 0;
    char* vptr = NULL;
    int is_null = 1;
    pExpression code;

    	/** Get an id for this. **/
	id = (HTVBL.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Field name **/
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	/** Value of label **/
	t = wgtrGetPropertyType(tree,"value");
	if (t < 0 || t >= OBJ_TYPE_NAMES_CNT)
	    {
	    t = DATA_T_ANY;
	    }
	else if (t == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"value",DATA_T_CODE,POD(&code));
	    htrAddExpression(s, name, "value", code);
	    }
	else if (t == DATA_T_STRING && wgtrGetPropertyValue(tree,"value",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    is_null = 0;
	    vptr=nmSysStrdup(ptr);
	    }
	else if (t == DATA_T_INTEGER)
	    {
	    if (wgtrGetPropertyValue(tree,"value",DATA_T_INTEGER,POD(&n)) == 0)
		is_null = 0;
	    }

	/** widget init **/
	htrAddScriptInit_va(s, "    vbl_init(wgtrGetNodeRef(ns,\"%STR&SYM\"), {type:\"%STR&JSSTR\", value:%[null%]%[\"%STR&JSSTR\"%]%[%INT%], field:\"%STR&JSSTR\", form:\"%STR&JSSTR\"} );\n",
		name,
		obj_type_names[t],
		is_null,
		(!is_null) && t == DATA_T_STRING, vptr,
		(!is_null) && t == DATA_T_INTEGER, n,
		fieldname, form);

	/** JavaScript include file **/
	htrAddScriptInclude(s, "/sys/js/htdrv_variable.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	/** object linkages **/
	htrAddWgtrCtrLinkage(s, tree, "_parentctr");

	/** Check for more sub-widgets within the vbl entity. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2);

	if (vptr)
	    nmSysFree(vptr);

    return 0;
    }


/*** htvblInitialize - register with the ht_render module.
 ***/
int
htvblInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Variable Object Driver");
	strcpy(drv->WidgetName,"variable");
	drv->Render = htvblRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTVBL.idcnt = 0;

    return 0;
    }
