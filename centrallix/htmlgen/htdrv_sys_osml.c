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
/* Module: 	htdrv_sys_osml.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 11, 1999 					*/
/* Description:	HTML Widget driver for an OSML session.  Allows the use	*/
/*		of the entire OSML interface from JavaScript on the	*/
/*		client side.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_sys_osml.c,v 1.10 2005/02/26 06:42:37 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/Attic/htdrv_sys_osml.c,v $

    $Log: htdrv_sys_osml.c,v $
    Revision 1.10  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.9  2004/08/04 20:03:10  mmcgill
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

    Revision 1.8  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.7  2004/08/02 14:09:34  mmcgill
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

    Revision 1.6  2004/07/19 15:30:40  mmcgill
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

    Revision 1.5  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.4  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.3  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.2  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTOSML;


/*** htosmlRender - generate the HTML code for the page.
 ***/
int
htosmlRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char* vstr;
    int vint;
    double vdbl;
    char name[64];
    char sbuf[256];
    char* fnbuf;
    char fnname[16];
    char* fnnamebuf;
    char event[32];
    char target[32];
    char action[32];
    pObject sub_w_obj;
    pObjQuery qy;
    int x=-1,y=-1,w,h;
    int id,cnt, i;
    char* nptr;
    pObject content_obj;
    XString xs;

    	/** Get an id for this. **/
	id = (HTOSML.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s,"/sys/js/htdrv_sys_osml.js",0);

	/** Add a script init to install the connector **/
	sprintf(sbuf,"    %s = new cn_init(%s);\n    %s.RunEvent = cn_%d;\n", nptr, parentobj, nptr, id);
	htrAddScriptInit(s, sbuf);
	sprintf(sbuf,"    %s.Event%s = %s.RunEvent;\n", parentobj, event, nptr);
	htrAddScriptInit(s, sbuf);

	/** Add the connector function **/
	xsInit(&xs);
	sprintf(sbuf,	"\n"
		     	"function cn_%d(eparam)\n"
		     	"    {\n" ,id);
	xsConcatenate(&xs,sbuf,-1);
	xsConcatenate(&xs,"    aparam = new Object();\n",-1);
	for(ptr = wgtrFirstProperty(tree); ptr; ptr = wgtrNextProperty(tree))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action")) continue;
	    switch(wgtrGetPropertyType(tree, ptr))
	        {
		case DATA_T_INTEGER:
	    	    wgtrGetPropertyValue(tree, ptr, POD(&vint));
		    sprintf(sbuf, "    aparam.%s = %d;\n",ptr,vint);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_DOUBLE:
		    wgtrGetPropertyValue(tree, ptr, POD(&vdbl));
		    sprintf(sbuf, "    aparam.%s = %lf;\n",ptr,vdbl);
		    xsConcatenate(&xs,sbuf,-1);
		    break;
		case DATA_T_STRING:
	    	    wgtrGetPropertyValue(w_tree, ptr, POD(&vstr));
		    if (!strpbrk(vstr," !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|"))
		        {
			sprintf(sbuf, "    aparam.%s = eparam.%s\n", ptr, vstr);
			xsConcatenate(&xs,sbuf,-1);
			}
		    else
		        {
			sprintf(sbuf, "    aparam.%s = ", ptr);
			xsConcatenate(&xs,sbuf,-1);
			xsConcatenate(&xs,vstr,-1);
			xsConcatenate(&xs,";\n",2);
			}
		    break;
		}
	    }
	sprintf(sbuf,"    %s.Action%s(aparam);\n", target, action);
	xsConcatenate(&xs,sbuf,-1);
	xsConcatenate(&xs,"    delete aparam;\n",-1);
	xsConcatenate(&xs,"    }\n\n",7);
	sprintf(fnname, "cn_%d",id);
	fnbuf = (char*)nmMalloc(strlen(xs.String)+1);
	strcpy(fnbuf,xs.String);
	fnnamebuf = (char*)nmMalloc(strlen(fnname)+1);
	strcpy(fnnamebuf, fnname);
	htrAddScriptFunction(s, fnnamebuf, fnbuf, HTR_F_NAMEALLOC | HTR_F_VALUEALLOC);
	xsDeInit(&xs);

	/** Add init for param passing structures **/
	/*htrAddScriptInit(s, "    if (eparam == null) eparam = new cn_init();\n");
	htrAddScriptInit(s, "    if (aparam == null) aparam = new cn_init();\n");*/




	/** Check for more sub-widgets within the conn entity. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2, parentname, nptr);


    return 0;
    }


/*** htosmlInitialize - register with the ht_render module.
 ***/
int
htosmlInitialize()
    {
    pHtDriver drv;
    pHtEventAction action;
    pHtParam param;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"System:OSML Driver");
	strcpy(drv->WidgetName,"sys-osml");
	drv->Render = htosmlRender;
	htrAddSupport(drv, HTR_UA_NETSCAPE_47);

	/** Register. **/
	htrRegisterDriver(drv);

	HTOSML.idcnt = 0;

    return 0;
    }
