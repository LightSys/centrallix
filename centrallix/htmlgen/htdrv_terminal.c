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
#define MAX_COLORS 8
#define MAX_COLOR_LEN 32
    char colors[MAX_COLORS][MAX_COLOR_LEN]={"black","red","green","yellow","blue","purple","aqua","white"};
    XString source;
    int rows, cols, fontsize, x, y;

    	/** Get an id for this. **/
	const unsigned id = (HTTERM.idcnt++);

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTTERM", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** read the color specs, leaving the defaults if they don't exist **/
	for (unsigned int i = 0u; i < MAX_COLORS; i++)
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
	    goto err;
	    }

	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) 
	    {
	    mssError(0,"TERM","y is required");
	    goto err;
	    }

	if (wgtrGetPropertyValue(tree,"rows",DATA_T_INTEGER,POD(&rows)) != 0) 
	    {
	    mssError(0,"TERM","rows is required");
	    goto err;
	    }

	if (wgtrGetPropertyValue(tree,"cols",DATA_T_INTEGER,POD(&cols)) != 0) 
	    {
	    mssError(0,"TERM","cols is required");
	    goto err;
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
	    goto err;
	    }
	
	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) goto err;
	strtcpy(name,ptr,sizeof(name));

	/** Script include to add functions **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_terminal.js", 0) != 0) goto err;
	
	/** Link DOM node to widget data. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "term%POSbase", id) != 0) goto err;

	/** HTML body <IFRAME> element to use as the base **/
	if (htrAddBodyItem_va(s,
	    "<div id='term%POSbase'></div>"
	    "<iframe id='term%POSreader'></iframe>"
	    "<iframe id='term%POSwriter'></iframe>\n",
	    id, id, id
	) != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write HTML base, reader, and writer tags.");
	    goto err;
	    }

	/** Write CSS. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#term%POSbase { "
		"position:absolute; "
		"visibility:inherit; "
		"left:%INTpx; "
		"top:%INTpx; "
		"width:%POSpx; "
		"height:%POS; "
		"z-index:%POS; "
	    "}\n",
	    id, x, y,
	    cols * fontsize,
	    rows * fontsize,
	    z
	) != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write base CSS.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s,
	    "\t\t#term%POSreader, #term%POSwriter { "
		"position:absolute; "
		"visibility:hidden; "
		"left:0px; "
		"top:0px; "
		"width:1px; "
		"height:1; "
		"z-index:-20; "
	    "}\n",
	    id
	) != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write reader/writer CSS.");
	    goto err;
	    }
	if (htrAddStylesheetItem_va(s, "\t\t.fixed%POS { font-family: fixed; }\n",id) != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write fixed font CSS.");
	    goto err;
	    }

	/** Write script initialization. **/
	if (htrAddScriptInit_va(s,
	    "\tterminal_init({ "
		"layer:wgtrGetNodeRef(ns, '%STR&SYM'), "
		"rdr:'term%POSreader', "
		"wtr:'term%POSwriter', "
		"fxd:'fixed%POS', "
		"source:'%STR&JSSTR', "
		"rows:%INT, "
		"cols:%INT, "
		"colors:new Array(",
	    name, id, id, id,
	    source.String, rows, cols
	) != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write JS init call start.");
	    goto err;
	    }
	for (unsigned int i = 0u; i < MAX_COLORS; i++)
	    {
	    if (htrAddScriptInit_va(s, "'%STR&JSSTR',", colors[i]) != 0)
		{
		mssError(0, "HTTERM", "Failed to write terminal color #%d/%d: \"%s\"", i + 1, MAX_COLORS, colors[i]);
		goto err;
		}
	    }
	if (htrAddScriptInit(s, ")});\n") != 0)
	    {
	    mssError(0, "HTTERM", "Failed to write JS init call end.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z + 1) != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTTERM",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, id
	);
	return -1;
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
