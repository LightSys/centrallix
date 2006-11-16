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
#include "hints.h"
#include "cxlib/cxsec.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2006 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_component.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 18, 2006 					*/
/* Description:	HTML Widget driver for a component widget instance.	*/
/*		This can either be a component loaded in-line, or a	*/
/*		component loaded dynamically.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_component.c,v 1.3 2006/11/16 20:15:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_component.c,v $

    $Log: htdrv_component.c,v $
    Revision 1.3  2006/11/16 20:15:53  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.2  2006/10/27 05:57:22  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.1  2006/10/19 21:53:23  gbeeley
    - (feature) First cut at the component-based client side development
      system.  Only rendering of the components works right now; interaction
      with the components and their containers is not yet functional.  For
      an example, see "debugwin.cmp" and "window_test.app" in the samples
      directory of centrallix-os.


 **END-CVSDATA***********************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCMP;


/*** htcmpRender - generate the HTML code for the component.
 ***/
int
htcmpRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int id;
    char cmp_path[256];
    pObject cmp_obj = NULL;
    pWgtrNode cmp_tree = NULL;
    int w,h,x,y;
    int rval = -1;
    WgtrClientInfo wgtr_params;
    int is_static;

	/** Verify capabilities **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCMP","Either Netscape DOM or W3C DOM1 HTML and W3C CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCMP.idcnt++);

        /** Get x,y,w,h of this object **/
        if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x = 0;
        if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y = 0;
        if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    w = wgtrGetContainerWidth(tree) - x;
        if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    h = wgtrGetContainerHeight(tree) - y;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));
	if (cxsecVerifySymbol(name) < 0)
	    {
	    mssError(1,"HTCMP","Invalid name '%s' for component", name);
	    return -1;
	    }

	/** OSML path to component declaration **/
	if (wgtrGetPropertyValue(tree, "path", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCMP","Component must specify declaration location with 'path' attribute");
	    return -1;
	    }
	strtcpy(cmp_path, ptr, sizeof(cmp_path));

	/** Load now, or dynamically later on? **/
	if (wgtrGetPropertyValue(tree, "mode", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "dynamic"))
	    is_static = 0;
	else
	    is_static = 1;

	/** Include the js module **/
	htrAddScriptInclude(s, "/sys/js/htdrv_component.js", 0);

	/** If static mode, load the component **/
	if (is_static)
	    {
	    /** Init component **/
	    htrAddScriptInit_va(s, 
		    "    cmp_init({node:nodes[\"%s\"], is_static:true});\n",
		    name);

	    /** Open and parse the component **/
	    cmp_obj = objOpen(s->ObjSession, cmp_path, O_RDONLY, 0600, "system/structure");
	    if (!cmp_obj)
		{
		mssError(0,"HTCMP","Could not open component for widget '%s'",name);
		goto out;
		}
	    cmp_tree = wgtrParseOpenObject(cmp_obj, NULL);
	    if (!cmp_tree)
		{
		mssError(0,"HTCMP","Invalid component for widget '%s'",name);
		goto out;
		}

	    /** Do the layout for the component **/
	    memcpy(&wgtr_params, s->ClientInfo, sizeof(wgtr_params));
	    wgtr_params.MaxHeight = h;
	    wgtr_params.MinHeight = h;
	    wgtr_params.MaxWidth = w;
	    wgtr_params.MinWidth = w;
	    if (wgtrVerify(cmp_tree, &wgtr_params) < 0)
		{
		mssError(0,"HTCMP","Invalid component for widget '%s'",name);
		goto out;
		}
	    wgtrMoveChildren(cmp_tree, x, y);

	    /** Switch namespaces **/
	    htrAddNamespace(s, tree, wgtrGetRootDName(cmp_tree));

	    /** Generate the component **/
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    htrRenderWidget(s, cmp_tree, z+10);
	    htrBuildClientWgtr(s, cmp_tree);

	    /** Switch the namespace back **/
	    htrLeaveNamespace(s);
	    }
	else
	    {
	    /** Init component **/
	    htrAddScriptInit_va(s, 
		    "    cmp_init({node:nodes[\"%s\"], is_static:false, path:\"%s\", loader:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%s\"])), \"cmp%d\")});\n",
		    name, cmp_path, name, id);

	    /** Dynamic mode -- load from client **/
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "cmp%d", id, "");
	    htrAddStylesheetItem_va(s,"\t#cmp%d { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:0px; HEIGHT:0px; Z-INDEX:0;}\n", id);
	    }

	rval = 0;

    out:
	/** Clean up **/
	if (cmp_obj) objClose(cmp_obj);
	if (cmp_tree) wgtrFree(cmp_tree);

    return rval;
    }


/*** htcmpInitialize - register with the ht_render module.
 ***/
int
htcmpInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Component Instance Driver");
	strcpy(drv->WidgetName,"component");
	drv->Render = htcmpRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	/** Declare support for DHTML user interface class **/
	htrAddSupport(drv, "dhtml");

	HTCMP.idcnt = 0;

    return 0;
    }

