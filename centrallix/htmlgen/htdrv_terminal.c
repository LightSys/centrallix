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
#include "cxlib/qprintf.h"

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
/* Module: 	htdrv_terminal.c      					*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is visual widget that emulates a vt100 terminal 	*/
/*									*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTERM;


/*** httermRender - generate the HTML code for the form 'glue'
 ***/
int
httermRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int id;
    pWgtrNode sub_tree;
#define MAX_COLORS 8
#define MAX_COLOR_LEN 32
    char colors[MAX_COLORS][MAX_COLOR_LEN]={"black","red","green","yellow","blue","purple","aqua","white"};
    int i;
    XString source;
    int rows, cols, fontsize, x, y;

	/** our first Mozilla-only widget :) **/
	if(!s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTERM","W3C DOM Level 1 support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTERM.idcnt++);

	/** read the color specs, leaving the defaults if they don't exist **/
	for(i=0;i<MAX_COLORS;i++)
	    {
	    char color[32];
	    qpfPrintf(NULL, color, sizeof(color), "color%POS",i);
	    if (wgtrGetPropertyValue(tree,color,DATA_T_STRING,POD(&ptr)) == 0) 
		{
		strtcpy(colors[i],ptr,MAX_COLOR_LEN);
		}
	    }

	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(0,"TERM","x is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) 
	    {
	    mssError(0,"TERM","y is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"rows",DATA_T_INTEGER,POD(&rows)) != 0) 
	    {
	    mssError(0,"TERM","rows is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"cols",DATA_T_INTEGER,POD(&cols)) != 0) 
	    {
	    mssError(0,"TERM","cols is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"fontsize",DATA_T_INTEGER,POD(&fontsize)) != 0) 
	    {
	    fontsize = 12;
	    }

	xsInit(&source);

	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    xsCopy(&source,ptr,strlen(ptr));
	    }
	else
	    {
	    xsDeInit(&source);
	    mssError(0,"TERM","source is required");
	    return -1;
	    }
	
	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Script include to add functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_terminal.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** HTML body <IFRAME> element to use as the base **/
	htrAddBodyItem_va(s,"    <DIV ID=\"term%POSbase\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%POSreader\"></IFRAME>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%POSwriter\"></IFRAME>\n",id);
	
	/** write the stylesheet header element **/
	htrAddStylesheetItem_va(s,"        #term%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT;  WIDTH:%POS; HEIGHT:%POS; Z-INDEX:%POS; }\n",id,x,y,cols*fontsize,rows*fontsize,z);
	htrAddStylesheetItem_va(s,"        #term%POSreader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        #term%POSwriter { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        .fixed%POS {font-family: fixed; }\n",id);

	/** init line **/
	htrAddScriptInit_va(s,"    terminal_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), rdr:\"term%POSreader\", wtr:\"term%POSwriter\", fxd:\"fixed%POS\", source:'%STR&JSSTR', rows:%INT, cols:%INT, colors:new Array(",
		name,id,id,id,source.String,rows,cols);
	for(i=0;i<MAX_COLORS;i++)
	    {
	    if(i!=0)
		htrAddScriptInit(s,",");
	    htrAddScriptInit_va(s,"'%STR&JSSTR'",colors[i]);
	    }
	htrAddScriptInit(s,")});\n");

	/** Check for and render all subobjects. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    htrRenderWidget(s, sub_tree, z+1);
	    }
	
    return 0;
    }


/*** httermInitialize - register with the ht_render module.
 ***/
int
httermInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Terminal Widget");
	strcpy(drv->WidgetName,"terminal");
	drv->Render = httermRender;


	/** Add our actions **/
	htrAddAction(drv,"Disconnect");
	htrAddAction(drv,"Connect");

	/** Add our Events **/
	htrAddEvent(drv,"ConnectionOpen");
	htrAddEvent(drv,"ConnectionClose");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTERM.idcnt = 0;

    return 0;
    }
