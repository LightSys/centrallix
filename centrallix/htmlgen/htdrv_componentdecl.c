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
#include "hints.h"
#include "cxlib/cxsec.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_componentdecl.c             			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 11, 2003					*/
/* Description:	HTML Widget driver for component widget definition,	*/
/*		which is used to declare a component, its structure,	*/
/*		and its interface/params.  Use the normal component	*/
/*		widget (htdrv_component.c) to insert a component into	*/
/*		an application.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_componentdecl.c,v 1.9 2007/04/03 15:50:04 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_componentdecl.c,v $

    $Log: htdrv_componentdecl.c,v $
    Revision 1.9  2007/04/03 15:50:04  gbeeley
    - (feature) adding capability to pass a widget to a component as a
      parameter (by reference).
    - (bugfix) changed the layout logic slightly in the apos module to better
      handle ratios of flexibility and size when resizing.

    Revision 1.8  2007/03/10 02:57:41  gbeeley
    - (bugfix) setup graft point for static components as well as dynamically
      loaded ones, and allow nested components by saving and restoring previous
      graft points.

    Revision 1.7  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.6  2006/10/19 21:53:23  gbeeley
    - (feature) First cut at the component-based client side development
      system.  Only rendering of the components works right now; interaction
      with the components and their containers is not yet functional.  For
      an example, see "debugwin.cmp" and "window_test.app" in the samples
      directory of centrallix-os.

    Revision 1.5  2006/10/16 18:34:33  gbeeley
    - (feature) ported all widgets to use widget-tree (wgtr) alone to resolve
      references on client side.  removed all named globals for widgets on
      client.  This is in preparation for component widget (static and dynamic)
      features.
    - (bugfix) changed many snprintf(%s) and strncpy(), and some sprintf(%.<n>s)
      to use strtcpy().  Also converted memccpy() to strtcpy().  A few,
      especially strncpy(), could have caused crashes before.
    - (change) eliminated need for 'parentobj' and 'parentname' parameters to
      Render functions.
    - (change) wgtr port allowed for cleanup of some code, especially the
      ScriptInit calls.
    - (feature) ported scrollbar widget to Mozilla.
    - (bugfix) fixed a couple of memory leaks in allocated data in widget
      drivers.
    - (change) modified deployment of widget tree to client to be more
      declarative (the build_wgtr function).
    - (bugfix) removed wgtdrv_templatefile.c from the build.  It is a template,
      not an actual module.

    Revision 1.4  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.3  2004/08/02 14:09:34  mmcgill
    Restructured the rendering process, in anticipation of new deployment methods
    being added in the future. The wgtr module is now the main widget-related
    module, responsible for all non-deployment-specific widget functionality.
    For example, Verifying a widget tree is non-deployment-specific, so the verify
    functions have been moved out of htmlgen and into the wgtr module.
    Changes include:
    *   Creating a new folder, wgtr/, to contain the wgtr module, including all
        wgtr drivers.
    *   Adding wgtr drivers to the widget tree module.
    *   Moving the xxxVerify() functions to the wgtr drivers in the wgtr module.
    *   Requiring all deployment methods (currently only DHTML) to register a
        Render() function with the wgtr module.
    *   Adding wgtrRender(), to abstract the details of the rendering process
        from the caller. Given a widget tree, a string representing the deployment
        method to use ("DHTML" for now), and the additional args for the rendering
        function, wgtrRender() looks up the appropriate function for the specified
        deployment method and calls it.
    *   Added xxxNew() functions to each wgtr driver, to be called when a new node
        is being created. This is primarily to allow widget drivers to declare
        the interfaces their widgets support when they are instantiated, but other
        initialization tasks can go there as well.

    Also in this commit:
    *   Fixed a typo in the inclusion guard for iface.h (most embarrasing)
    *   Fixed an overflow in objCopyData() in obj_datatypes.c that stomped on
        other stack variables.
    *   Updated net_http.c to call wgtrRender instead of htrRender(). Net drivers
        can now be completely insulated from the deployment method by the wgtr
        module.

    Revision 1.2  2004/07/19 15:30:39  mmcgill
    The DHTML generation system has been updated from the 2-step process to
    a three-step process:
        1)	Upon request for an application, a widget-tree is built from the
    	app file requested.
        2)	The tree is Verified (not actually implemented yet, since none of
    	the widget drivers have proper Verify() functions - but it's only
    	a matter of a function call in net_http.c)
        3)	The widget drivers are called on their respective parts of the
    	tree structure to generate the DHTML code, which is then sent to
    	the user.

    To support widget tree generation the WGTR module has been added. This
    module allows OSML objects to be parsed into widget-trees. The module
    also provides an API for building widget-trees from scratch, and for
    manipulating existing widget-trees.

    The Render functions of all widget drivers have been updated to make their
    calls to the WGTR module, rather than the OSML, and to take a pWgtrNode
    instead of a pObject as a parameter.

    net_internal_GET() in net_http.c has been updated to call
    wgtrParseOpenObject() to make a tree, pass that tree to htrRender(), and
    then free it.

    htrRender() in ht_render.c has been updated to take a pWgtrNode instead of
    a pObject parameter, and to make calls through the WGTR module instead of
    the OSML where appropriate. htrRenderWidget(), htrRenderSubwidgets(),
    htrGetBoolean(), etc. have also been modified appropriately.

    I have assumed in each widget driver that w_obj->Session is equivelent to
    s->ObjSession; in other words, that the object being passed in to the
    Render() function was opened via the session being passed in with the
    HtSession parameter. To my understanding this is a valid assumption.

    While I did run through the test apps and all appears to be well, it is
    possible that some bugs were introduced as a result of the modifications to
    all 30 widget drivers. If you find at any point that things are acting
    funny, that would be a good place to check.

    Revision 1.1  2004/02/24 19:59:30  gbeeley
    - adding component-declaration widget driver
    - adding image widget driver
    - adding app-level presentation hints pseudo-widget driver


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCMPD;


/** structure defining a param to the component **/
typedef struct
    {
    pObjPresentationHints	Hints;
    TObjData			TypedObjData;
    char*			StrVal;
    char*			Name;
    }
    HTCmpdParam, *pHTCmpdParam;


/*** htcmpdRender - generate the HTML code for the component.
 ***/
int
htcmpdRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char subobj_name[64];
    int id;
    char* nptr;
//    pObject subobj = NULL;
    pWgtrNode sub_tree = NULL;
    pObjQuery subobj_qy = NULL;
    XArray attrs;
    pHTCmpdParam param;
    int i,t;
    int rval = 0;
    int is_visual = 1;
    char gbuf[256];
    char* gname;

	/** Verify capabilities **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCMPD","Either Netscape DOM or W3C DOM1 HTML and W3C CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCMPD.idcnt++);

	/** Is this a visual component? **/
	if ((is_visual = htrGetBoolean(tree, "visual", 1)) < 0)
	    {
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name, ptr, sizeof(name));
	if (cxsecVerifySymbol(name) < 0)
	    {
	    mssError(1,"HTCMPD","Invalid name '%s' for component", name);
	    return -1;
	    }

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	/*htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);*/

	/** Include the js module **/
	htrAddScriptInclude(s, "/sys/js/htdrv_componentdecl.js", 0);

	/** DOM Linkages **/
	if (s->GraftPoint)
	    {
	    strtcpy(gbuf, s->GraftPoint, sizeof(gbuf));
	    gname = strchr(gbuf,':');
	    if (!gname)
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto htcmpd_cleanup;
		}
	    *(gname++) = '\0';
	    if (strpbrk(gname,"'\"\\<>\r\n\t ") || strspn(gbuf,"w0123456789abcdef") != strlen(gbuf))
		{
		mssError(1,"HTCMPD", "Invalid graft point");
		goto htcmpd_cleanup;
		}
	    htrAddWgtrCtrLinkage_va(s, tree, 
		    "wgtrGetContainer(wgtrGetNode(%s,\"%s\"))", gbuf, gname);
	    htrAddBodyItem_va(s, "<a id=\"dname\" target=\"%s\" href=\".\"></a>", s->Namespace->DName);
	    }
	else
	    {
	    strcpy(gbuf,"null");
	    gname="";
	    htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	    }

	/** Init component **/
	htrAddScriptInit_va(s, "    cmpd_init(nodes[\"%s\"], {vis:%d, gns:%s, gname:'%s'});\n", 
		name, is_visual, gbuf, gname);

	/** Hunt for parameters for this component **/
	xaInit(&attrs, 16);
#if 0
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    /** Loop through each param we get **/
	    sub_tree = xaGetItem(&(tree->Children), i);
	    if (!strcmp(sub_tree->Type, "system/parameter"))
		{
		param = (pHTCmpdParam)nmMalloc(sizeof(HTCmpdParam));
		if (!param) break;
		xaAddItem(&attrs, param);

		/** Get component parameter name **/
		wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
		param->Name = nmSysStrdup(ptr);
		if (cxsecVerifySymbol(param->Name) < 0)
		    {
		    mssError(1,"HTCMPD","Invalid name '%s' for parameter in component '%s'", param->Name, name);
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Does the param have a value? **/
		param->StrVal = htrParamValue(s, param->Name);

		/** Get component type **/
		if (wgtrGetPropertyValue(sub_tree, "type", DATA_T_STRING, POD(&ptr)) == 0)
		    {
		    t = objTypeID(ptr);
		    if (t < 0)
			{
			mssError(1,"HTCMPD","Component '%s' parameter '%s' has unknown/invalid data type '%s'", name, param->Name, ptr);
			rval = -1;
			goto htcmpd_cleanup;
			}
		    param->TypedObjData.DataType = t;
		    }
		else
		    {
		    /** default type is string **/
		    param->TypedObjData.DataType = DATA_T_STRING;
		    }

		/** Get hints **/
		param->Hints = wgtrWgtToHints(sub_tree);
		if (!param->Hints)
		    {
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Close the object **/
		sub_tree = NULL;
		}
	    subobj_qy = NULL;
	    }
#endif

	/** Build the typed pod values for each data value **/
	for(i=0;i<attrs.nItems;i++)
	    {
	    param = (pHTCmpdParam)(attrs.Items[i]);
	    if (!param->StrVal)
		{
		param->TypedObjData.Flags = DATA_TF_NULL;
		}
	    else
		{
		param->TypedObjData.Flags = 0;
		switch(param->TypedObjData.DataType)
		    {
		    case DATA_T_STRING:
			param->TypedObjData.Data.String = param->StrVal;
			break;
		    case DATA_T_INTEGER:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to integer", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Integer = strtol(param->StrVal,&ptr,10);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to integer", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    case DATA_T_DOUBLE:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to double", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Double = strtod(param->StrVal,&ptr);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to double", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    default:
			mssError(1,"HTCMPD","Unsupported type for param '%s'", param->Name);
			rval= -1;
			goto htcmpd_cleanup;
		    }
		}

	    /** Verify the thing **/
	    if (hntVerifyHints(param->Hints, &(param->TypedObjData), &ptr, NULL) < 0)
		{
		mssError(1,"HTCMPD","Invalid value '%s' for component '%s' param '%s': %s", param->StrVal, name, param->Name, ptr);
		rval = -1;
		goto htcmpd_cleanup;
		}
	    }

	/** Add actions, events, and client properties **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;

	    /** Get component action/event/cprop name **/
	    wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
	    strtcpy(subobj_name, ptr, sizeof(subobj_name));
	    if (cxsecVerifySymbol(subobj_name) < 0)
		{
		mssError(1,"HTCMPD","Invalid name '%s' for action/event/cprop in component '%s'", subobj_name, name);
		rval = -1;
		goto htcmpd_cleanup;
		}

	    /** Get type **/
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING, POD(&ptr));
	    if (!strcmp(ptr,"widget/component-decl-action"))
		htrAddScriptInit_va(s, "    nodes[\"%s\"].addAction('%s');\n", name, subobj_name);
	    else if (!strcmp(ptr,"widget/component-decl-event"))
		htrAddScriptInit_va(s, "    nodes[\"%s\"].addEvent('%s');\n", name, subobj_name);
	    else if (!strcmp(ptr,"widget/component-decl-cprop"))
		htrAddScriptInit_va(s, "    nodes[\"%s\"].addProp('%s');\n", name, subobj_name);

	    sub_tree = NULL;
	    }

	/** End init for component **/
	htrAddScriptInit_va(s, "    cmpd_endinit(nodes[\"%s\"]);\n", name);

	/** Do subwidgets **/
	htrRenderSubwidgets(s, tree, z+2);

    htcmpd_cleanup:
//	if (subobj) objClose(subobj);
//	if (subobj_qy) objQueryClose(subobj_qy);
	for(i=0;i<attrs.nItems;i++)
	    {
	    if (attrs.Items[i])
		{
		param = (pHTCmpdParam)(attrs.Items[i]);
		if (param->Hints) objFreeHints(param->Hints);
		if (param->Name) nmSysFree(param->Name);
		nmFree(param, sizeof(HTCmpdParam));
		}
	    }
	xaDeInit(&attrs);

    return rval;
    }


/*** htcmpdInitialize - register with the ht_render module.
 ***/
int
htcmpdInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Component Declaration Driver");
	strcpy(drv->WidgetName,"component-decl");
	drv->Render = htcmpdRender;

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

	HTCMPD.idcnt = 0;

    return 0;
    }

