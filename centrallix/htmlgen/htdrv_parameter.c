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

    $Id: htdrv_parameter.c,v 1.10 2010/09/09 01:04:17 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_parameter.c,v $

    $Log: htdrv_parameter.c,v $
    Revision 1.10  2010/09/09 01:04:17  gbeeley
    - (bugfix) allow client to specify what scripts (cx__scripts) have already
      been deployed to the app, to avoid having multiple copies of JS scripts
      loaded on the client.
    - (feature) component parameters may contain expressions
    - (feature) presentation hints may be placed on a component, which can
      specify what widget in the component those apply to

    Revision 1.9  2009/12/15 21:55:50  gbeeley
    - (change) allow parameters to have an operational name different from the
      widget name assigned to them.

    Revision 1.8  2009/06/25 21:04:46  gbeeley
    - (change) deploy all non-component-params to the client regardless of
      param type.  Needed esp. for widget/osrc parameters.

    Revision 1.7  2008/03/28 05:48:11  gbeeley
    - (bugfix) parameter widget requires hints functionality.

    Revision 1.6  2008/03/04 01:10:57  gbeeley
    - (security) changing from ESCQ to JSSTR in numerous places where
      building JavaScript strings, to avoid such things as </script>
      in the string from having special meaning.  Also began using the
      new CSSVAL and CSSURL in places (see qprintf).
    - (performance) allow the omission of certain widgets from the rendered
      page.  In particular, omitting most widget/parameter's significantly
      reduces the total widget count.
    - (performance) omit double-buffering in edit boxes for Firefox/Mozilla,
      which reduces the <div> count for the page significantly.
    - (bugfix) allow setting text color on tabs in mozilla/firefox.

    Revision 1.5  2007/12/05 18:51:54  gbeeley
    - (change) parameters on a static component should not be automatically
      deployed to the client; adding deploy_to_client boolean on parameters
      to cause the old behavior.

    Revision 1.4  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.3  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.2  2007/04/03 15:50:04  gbeeley
    - (feature) adding capability to pass a widget to a component as a
      parameter (by reference).
    - (bugfix) changed the layout logic slightly in the apos module to better
      handle ratios of flexibility and size when resizing.

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
    char paramname[64];
    char type[32];
    char findcontainer[64];
    int id;
    int i;
    XString xs;
    pObjPresentationHints hints;
    pStruct find_inf;
    int deploy_to_client;

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
	if (objTypeID(type) < 0 && strcmp(type,"object"))
	    {
	    mssError(1,"HTPARAM","Invalid parameter data type for '%s'", name);
	    return -1;
	    }

	/** Specify name of param (in case param widget name isn't the desired param name) **/
	if (wgtrGetPropertyValue(tree,"param_name",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(paramname, ptr, sizeof(paramname));
	else
	    strtcpy(paramname, name, sizeof(paramname));

	/** Make param available for client expressions? **/
	deploy_to_client = htrGetBoolean(tree, "deploy_to_client", s->IsDynamic || !strcmp(type, "object") || (tree->Parent && strcmp(tree->Parent->Type, "widget/component-decl") != 0));

	if (!strcmp(type, "object") && deploy_to_client && 
		wgtrGetPropertyValue(tree,"find_container",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(findcontainer, ptr, sizeof(findcontainer));
	else
	    findcontainer[0] = '\0';

	/** JavaScript include file **/
	htrAddScriptInclude(s, "/sys/js/htdrv_parameter.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	/** Value supplied? **/
	if (deploy_to_client)
	    find_inf = stLookup_ne(s->Params, name);
	else
	    find_inf = NULL;

	/** Parameter has presentation-hints components to it.  Set those. **/
	if (deploy_to_client)
	    {
	    /** Script init **/
	    htrAddScriptInit_va(s, "    pa_init(nodes[\"%STR&SYM\"], {name:'%STR&JSSTR', type:'%STR&JSSTR', findc:'%STR&JSSTR', val:%[null%]%[\"%STR&HEX\"%]});\n", 
		    name, paramname, type, findcontainer, !find_inf || !find_inf->StrVal, find_inf && find_inf->StrVal, find_inf?find_inf->StrVal:"");

	    hints = wgtrWgtToHints(tree);
	    if (!hints)
		{
		mssError(0, "HTPARAM", "Error in parameter data");
		return -1;
		}
	    xsInit(&xs);
	    hntEncodeHints(hints, &xs);
	    htrAddScriptInit_va(s, "    cx_set_hints(nodes[\"%STR&SYM\"], '%STR&JSSTR', 'app');\n",
		    name, xs.String);
	    xsDeInit(&xs);
	    objFreeHints(hints);

	    htrAddScriptInit_va(s, "    nodes[\"%STR&SYM\"].Verify();\n", name);
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NORENDER;
	    }

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Check for more sub-widgets within the param **/
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
