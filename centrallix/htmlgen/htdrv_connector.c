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
/* Module: 	htdrv_connector.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 24, 1998 					*/
/* Description:	HTML Widget driver for a 'connector', which joins an 	*/
/*		event on the parent widget with an action on another	*/
/*		specified widget.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_connector.c,v 1.23 2006/10/16 18:34:33 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_connector.c,v $

    $Log: htdrv_connector.c,v $
    Revision 1.23  2006/10/16 18:34:33  gbeeley
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

    Revision 1.22  2006/06/22 00:16:00  gbeeley
    - fix connector issue when attached to the page object

    Revision 1.21  2006/04/07 06:28:25  gbeeley
    - (change) convert widgets to new inter-widget interface mechanism rather
      than direct callbacks for Events and Actions
    - (change) get rid of cn_123() type functions, using a much cleaner way
      of handling the passing of values from events to actions.

    Revision 1.20  2005/10/09 07:45:36  gbeeley
    - (change) don't pollute the globals.
    - (change) return value that the action function returned.

    Revision 1.19  2005/06/23 22:07:58  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.18  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.17  2004/08/04 20:03:07  mmcgill
    Major change in the way the client-side widget tree works/is built.
    Instead of overlaying a tree structure on top of the global widget objects,
    the tree is built *out of* those objects.
    *   Removed the now-unnecessary tree-building code in the ht drivers
    *   added htr_internal_BuildClientTree(), which keeps just about all the
        client-side tree-building code in one spot
    *   Added RenderFlags to the WgtrNode struct, for use by any rendering
        module in whatever way that module sees fit
    *   Added the HT_WGTF_NOOBJECT flag in ht_render, which is set by ht
        drivers that deal with widgets for which a corresponding DHTML object
        is not created - for example, a radiobuttonpanel widget has
        radiobutton child widgets - but in the client-side code there are no
        corresponding DHTML objects for those child widgets. So the
        radiobuttonpanel ht driver sets the HT_WGTF_NOOBJECT RenderFlag on
        each of those child nodes, and when the client-side widget tree is
        being built, no attempt is made to add them to the client-side tree.
    *   Tweaked the connector widget a bit - it doesn't appear that the Add
        member function needs to take an object as a parameter, since each
        connector is associated with its parent object in cn_init.
    *   *cough* Er, fixed the, um....giant unclosable unmovable textarea that
        I had been using for debug messages, so that it doesn't appear unless
        WGTR_DBG_WINDOW is defined in ht_render.c. Heh heh. Sorry about that.

    Revision 1.16  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.15  2004/08/02 14:09:34  mmcgill
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

    Revision 1.14  2004/07/19 15:30:39  mmcgill
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

    Revision 1.13  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.12  2004/02/24 20:21:56  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.11  2003/07/27 03:24:53  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

    Revision 1.10  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.9  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.8  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.7  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.6  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.5  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.4  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCONN;


/*** htconnRender - generate the HTML code for the page.
 ***/
int
htconnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char* vstr;
    int vint;
    double vdbl;
    char name[64];
    char sbuf[HT_SBUF_SIZE];
    char event[32];
    char target[32];
    char action[32];
    int id, i;
    XString xs;
    pExpression code;
    int first;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML )
	    {
	    mssError(1,"HTCONN","Netscape DOM or W3C DOM1HTML support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCONN.idcnt++);

	/** Get the event linkage information **/
	if (wgtrGetPropertyValue(tree,"event",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTCONN","Connector must have an 'event' property");
	    return -1;
	    }
	strtcpy(event,ptr,sizeof(event));
	if (wgtrGetPropertyValue(tree,"target",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have a 'target' property");
	    return -1;
	    }
	strtcpy(target,ptr,sizeof(target));
	if (wgtrGetPropertyValue(tree,"action",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCONN","Connector must have an 'action' property");
	    return -1;
	    }
	strtcpy(action,ptr,sizeof(action));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Build the param list **/
	xsInit(&xs);
	first = 1;
	for(ptr = wgtrFirstPropertyName(tree); ptr; ptr = wgtrNextPropertyName(tree))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action")) continue;
	    if (!first) xsConcatenate(&xs, ",", 1);
	    first = 0;
	    switch(wgtrGetPropertyType(tree, ptr))
	        {
		case DATA_T_CODE:
		    wgtrGetPropertyValue(tree, ptr, DATA_T_CODE, POD(&code));
		    xsConcatPrintf(&xs,"%s:{type:'exp', value:'", ptr);
		    expGenerateText(code, NULL, xsWrite, &xs, '\'', "javascript");
		    xsConcatenate(&xs,"'}",2);
		    break;
		case DATA_T_INTEGER:
	    	    wgtrGetPropertyValue(tree, ptr, DATA_T_INTEGER,POD(&vint));
		    xsConcatPrintf(&xs,"%s:{type:'int', value:%d}",ptr,vint);
		    break;
		case DATA_T_DOUBLE:
		    wgtrGetPropertyValue(tree, ptr, DATA_T_DOUBLE,POD(&vdbl));
		    snprintf(sbuf, HT_SBUF_SIZE, "%s:{type:'dbl', value:%f}",ptr,vdbl);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_STRING:
	    	    wgtrGetPropertyValue(tree, ptr, DATA_T_STRING,POD(&vstr));
		    if (!strpbrk(vstr," !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|"))
		        {
			xsConcatPrintf(&xs, "%s:{type:'sym', value:'%s'}", ptr, vstr);
			}
		    else
		        {
			xsConcatPrintf(&xs, "%s:{type:'str', value:'%s'}", ptr, vstr);
			}
		    break;
		}
	    }

	/** Add a script init to install the connector **/
	htrAddScriptInit_va(s, "    wgtrGetParent(nodes[\"%s\"]).ifcProbe(ifEvent).Connect('%s', '%s', '%s', {%s});\n",
		name, event, target, action, xs.String);
	xsDeInit(&xs);

	htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0);

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Check for more sub-widgets within the conn entity. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
    }


/*** htconnInitialize - register with the ht_render module.
 ***/
int
htconnInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Event-Action Connector Driver");
	strcpy(drv->WidgetName,"connector");
	drv->Render = htconnRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTCONN.idcnt = 0;

    return 0;
    }
