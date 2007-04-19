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
#include "stparse_ne.h"
#include "cxlib/qprintf.h"

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

    $Id: htdrv_component.c,v 1.7 2007/04/19 21:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_component.c,v $

    $Log: htdrv_component.c,v $
    Revision 1.7  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.6  2007/04/03 15:50:04  gbeeley
    - (feature) adding capability to pass a widget to a component as a
      parameter (by reference).
    - (bugfix) changed the layout logic slightly in the apos module to better
      handle ratios of flexibility and size when resizing.

    Revision 1.5  2007/03/21 04:48:09  gbeeley
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

    Revision 1.4  2007/03/10 02:57:40  gbeeley
    - (bugfix) setup graft point for static components as well as dynamically
      loaded ones, and allow nested components by saving and restoring previous
      graft points.

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


/*** htcmp_internal_CreateParams() - scan the instantiation for
 *** parameters, and build a pStruct parameter tree to pass to
 *** the wgtr rendering.
 ***/
pStruct
htcmp_internal_CreateParams(pWgtrNode tree)
    {
    pStruct params = NULL;
    pStruct attr_inf;
    char* attrname;
    char* reserved_attrs[] = {"x", "y", "width", "height", "name", "inner_type", "outer_type",
	    "annotation", "content_type", "path", "mode", NULL};
    int i;
    int found;
    int t;
    ObjData od;

	/** Create the struct **/
	params = stCreateStruct_ne("parameters");

	/** Scan and add attributes **/
	attrname = wgtrFirstPropertyName(tree);
	while(attrname)
	    {
	    found=0;
	    for(i=0;reserved_attrs[i];i++)
		{
		if (!strcmp(attrname, reserved_attrs[i]))
		    {
		    /** Reserved attr.  Don't add it **/
		    found = 1;
		    break;
		    }
		}
	    if (!found && cxsecVerifySymbol(attrname) >= 0)
		{
		/** Not a reserved attr.  Add it if we can. **/
		t = wgtrGetPropertyType(tree, attrname);
		if (t >= 0)
		    {
		    if (wgtrGetPropertyValue(tree, attrname, t, &od) == 0)
			{
			attr_inf = stAddAttr_ne(params, attrname);
			switch(t)
			    {
			    case DATA_T_INTEGER:
			    case DATA_T_DOUBLE:
				stAddValue_ne(attr_inf, objDataToStringTmp(t, &od, 0));
				break;
			    case DATA_T_STRING:
				stAddValue_ne(attr_inf, objDataToStringTmp(t, od.String, 0));
				break;
			    }
			}
		    }
		}
	    attrname = wgtrNextPropertyName(tree);
	    }

    return params;
    }


/*** htcmp_internal_CheckReferences() - go through the top level of
 *** the widget tree for the component, and compare against the parameters
 *** inside 'params', and add the namespace reference as needed.
 ***/
int
htcmp_internal_CheckReferences(pWgtrNode tree, pStruct params, char* ns)
    {
    int i, cnt, j;
    char* str;
    char* name;
    pWgtrNode param_node;
    pStruct one_param;
    char refname[128];

	/** search for 'object' parameters **/
	cnt = xaCount(&tree->Children);
	for(i=0;i<cnt;i++)
	    {
	    param_node = (pWgtrNode)xaGetItem(&tree->Children, i);
	    if (param_node && !strcmp(param_node->Type, "widget/parameter"))
		{
		if (wgtrGetPropertyValue(param_node, "type", DATA_T_STRING, POD(&str)) == 0 && !strcmp(str,"object"))
		    {
		    /** Found one - see if there is a corresponding ref
		     ** in the pStruct params list provided by the component
		     ** instantiation
		     **/
		    wgtrGetPropertyValue(param_node, "name", DATA_T_STRING, POD(&name));
		    for(j=0;j<params->nSubInf;j++)
			{
			one_param = params->SubInf[j];
			if (!strcmp(one_param->Name, name))
			    {
			    qpfPrintf(NULL, refname, sizeof(refname), "%STR&SYM:%STR&SYM", ns, one_param->StrVal);
			    stAddValue_ne(one_param, refname);
			    break;
			    }
			}
		    }
		}
	    }

    return 0;
    }


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
    int allow_multi, auto_destroy;
    char* old_graft = NULL;
    char sbuf[128];
    pStruct params = NULL;
    pStruct old_params = NULL;
    int i;

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

	/** multiple-instantiation? **/
	allow_multi = htrGetBoolean(tree, "multiple_instantiation", 0);
	if (allow_multi < 0) return -1;

	/** multiple-instantiation? **/
	auto_destroy = htrGetBoolean(tree, "auto_destroy", 1);
	if (auto_destroy < 0) return -1;

	/** Include the js module **/
	htrAddScriptInclude(s, "/sys/js/htdrv_component.js", 0);

	/** Get list of parameters **/
	params = htcmp_internal_CreateParams(tree);

	/** If static mode, load the component **/
	if (is_static)
	    {
	    /** Save the current graft point and render parameters **/
	    old_graft = s->GraftPoint;
	    qpfPrintf(NULL, sbuf, sizeof(sbuf), "%STR&SYM:%STR&SYM", wgtrGetRootDName(tree), name);
	    s->GraftPoint = nmSysStrdup(sbuf);
	    old_params = s->Params;
	    s->Params = params;

	    /** Init component **/
	    htrAddScriptInit_va(s, 
		    "    cmp_init({node:nodes[\"%STR&SYM\"], is_static:true, allow_multi:false, auto_destroy:false});\n",
		    name);

	    /** Open and parse the component **/
	    cmp_obj = objOpen(s->ObjSession, cmp_path, O_RDONLY, 0600, "system/structure");
	    if (!cmp_obj)
		{
		mssError(0,"HTCMP","Could not open component for widget '%s'",name);
		goto out;
		}
	    cmp_tree = wgtrParseOpenObject(cmp_obj, params);
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

	    /** Check param references **/
	    htcmp_internal_CheckReferences(cmp_tree, params, s->Namespace->DName);

	    /** Switch namespaces **/
	    htrAddNamespace(s, tree, wgtrGetRootDName(cmp_tree));

	    /** Generate the component **/
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    htrRenderWidget(s, cmp_tree, z+10);
	    htrBuildClientWgtr(s, cmp_tree);

	    /** Switch the namespace back **/
	    htrLeaveNamespace(s);

	    /** Restore original graft point and parameters **/
	    s->Params = old_params;
	    old_params = NULL;
	    nmSysFree(s->GraftPoint);
	    s->GraftPoint = old_graft;
	    old_graft = NULL;
	    }
	else
	    {
	    /** Init component **/
	    htrAddScriptInit_va(s, 
		    "    cmp_init({node:nodes[\"%STR&SYM\"], is_static:false, allow_multi:%POS, auto_destroy:%POS, path:\"%STR&ESCQ\", loader:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])), \"cmp%POS\")});\n",
		    name, allow_multi, auto_destroy, cmp_path, name, id);

	    /** Set Params **/
	    if (params)
		{
		for(i=0;i<params->nSubInf;i++)
		    {
		    htrAddScriptInit_va(s, "    nodes[\"%STR&SYM\"].AddParam(\"%STR&SYM\",%[null%]%[\"%STR&HEX\"%]);\n",
			name, params->SubInf[i]->Name, !params->SubInf[i]->StrVal, params->SubInf[i]->StrVal,
			params->SubInf[i]->StrVal);
		    }
		}

	    /** Dynamic mode -- load from client **/
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "cmp%POS", id, "");
	    htrAddStylesheetItem_va(s,"\t#cmp%POS { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:0px; HEIGHT:0px; Z-INDEX:0;}\n", id);
	    }

	rval = 0;

    out:
	/** Clean up **/
	if (params)
	    stFreeInf_ne(params);
	if (s->GraftPoint && old_graft)
	    {
	    nmSysFree(s->GraftPoint);
	    s->GraftPoint = old_graft;
	    old_graft = NULL;
	    }
	if (s->Params && old_params)
	    {
	    s->Params = old_params;
	    old_params = NULL;
	    }
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

