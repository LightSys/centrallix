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

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_hints.c						*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 22, 2004 					*/
/* Description:	HTML Widget driver for a 'hints', which is a pseudo-	*/
/*		widget allowing the association of presentation hints	*/
/*		with a form element widget				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_hints.c,v 1.7 2007/04/19 21:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_hints.c,v $

    $Log: htdrv_hints.c,v $
    Revision 1.7  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.6  2006/10/16 18:34:33  gbeeley
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

    Revision 1.5  2005/09/26 06:24:04  gbeeley
    - (major feature) Added templating mechanism via the wgtr module.
      To use a widget template on an app, specify widget_template= in the
      app.  See the objcanvas_test sample for an example of this.
    - (bugfix) Fixed a null value issue in the sybase driver
    - (bugfix) Fixed a wgtr issue on handling presentation hints settings.

    Revision 1.4  2005/02/26 06:42:37  gbeeley
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

    Revision 1.2  2004/07/19 15:30:40  mmcgill
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
    HTHINT;


/*** hthintRender - generate the HTML code for the page.
 ***/
int
hthintRender(pHtSession s, pWgtrNode tree, int z)
    {
    int id;
    pObjPresentationHints hints;
    XString xs;
    char* nptr;

    	/** Get an id for this. **/
	id = (HTHINT.idcnt++);

	/** Convert the object data into hints data **/
	hints = wgtrWgtToHints(tree);
	if (!hints)
	    {
	    mssError(0, "HTHINT", "Error in presentation hints data");
	    return -1;
	    }

	/** Serialize the hints data and add the script init for it **/
	xsInit(&xs);
	hntEncodeHints(hints, &xs);
	wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&nptr));
	htrAddScriptInit_va(s, "    cx_set_hints(wgtrGetParent(nodes[\"%STR&SYM\"]), '%STR&ESCQ', 'app');\n",
		nptr, xs.String);
	xsDeInit(&xs);

	/** mark this node as not being associated with a DHTML object **/
	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Release the hints data structure **/
	objFreeHints(hints);

    return 0;
    }


/*** hthintInitialize - register with the ht_render module.
 ***/
int
hthintInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Presentation Hints pseudo-widget Driver");
	strcpy(drv->WidgetName,"hints");
	drv->Render = hthintRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTHINT.idcnt = 0;

    return 0;
    }
