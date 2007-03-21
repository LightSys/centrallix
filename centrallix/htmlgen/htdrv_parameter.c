#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "wgtr.h"
#include "stparse_ne.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_parameter.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 15, 2007    					*/
/* Description:	HTML Widget driver for a 'parameter', which is used to	*/
/*		pass information to/from an application or component.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_parameter.c,v 1.1 2007/03/21 04:48:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_parameter.c,v $

    $Log: htdrv_parameter.c,v $
    Revision 1.1  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTPARAM;


/*** htparamRender - generate the HTML code for the page.
 ***/
int
htparamRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char type[32];
    int id;
    int i;
    XString xs;
    pObjPresentationHints hints;
    pStruct find_inf;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML )
	    {
	    mssError(1,"HTPARAM","Netscape DOM or W3C DOM1HTML support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTPARAM.idcnt++);

	/** Get name and type **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0)
	    return -1;
	strtcpy(name,ptr,sizeof(name));
	if (wgtrGetPropertyValue(tree,"type",DATA_T_STRING,POD(&ptr)) != 0)
	    ptr = "string";
	strtcpy(type,ptr,sizeof(type));
	if (objTypeID(type) < 0)
	    {
	    mssError(1,"HTPARAM","Invalid parameter data type for '%s'", name);
	    return -1;
	    }

	/** JavaScript include file **/
	htrAddScriptInclude(s, "/sys/js/htdrv_parameter.js", 0);

	/** Value supplied? **/
	find_inf = stLookup_ne(s->Params, name);

	/** Script init **/
	htrAddScriptInit_va(s, "    pa_init(nodes[\"%s\"], {type:'%s', val:%s", 
		name, type, find_inf?"\"":"null");
	if (find_inf)
	    {
	    for(i=0;i<strlen(find_inf->StrVal);i++)
		{
		htrAddScriptInit_va(s, "%2.2x", find_inf->StrVal[i]);
		}
	    htrAddScriptInit(s, "\"");
	    }
	htrAddScriptInit(s, "});\n");

	/** Parameter has presentation-hints components to it.  Set those. **/
	hints = wgtrWgtToHints(tree);
	if (!hints)
	    {
	    mssError(0, "HTPARAM", "Error in parameter data");
	    return -1;
	    }
	xsInit(&xs);
	hntEncodeHints(hints, &xs);
	htrAddScriptInit_va(s, "    cx_set_hints(nodes[\"%s\"], '%s', 'app');\n",
		name, xs.String);
	xsDeInit(&xs);
	htrAddScriptInit_va(s, "    nodes[\"%s\"].Verify();\n", name);

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Check for more sub-widgets within the conn entity.... connectors only **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
    }


/*** htparamInitialize - register with the ht_render module.
 ***/
int
htparamInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Parameter Driver");
	strcpy(drv->WidgetName,"parameter");
	drv->Render = htparamRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTPARAM.idcnt = 0;

    return 0;
    }
