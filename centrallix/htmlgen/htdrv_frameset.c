#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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
/* Module: 	htdrv_frameset.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 17, 1999   					*/
/* Description:	HTML Widget driver for a frameset; use instead of a	*/
/*		widget/page item at the top level.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_frameset.c,v 1.10 2004/08/02 14:09:34 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_frameset.c,v $

    $Log: htdrv_frameset.c,v $
    Revision 1.10  2004/08/02 14:09:34  mmcgill
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

    Revision 1.9  2004/07/19 15:30:39  mmcgill
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

    Revision 1.8  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.7  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.6  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.5  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.4  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.3  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** htsetRender - generate the HTML code for the page.
 ***/
int
htsetRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    pWgtrNode sub_tree;
    pObjQuery qy;
    char geom_str[64] = "";
    int t,n,bdr=0,direc=0, i;
    char nbuf[16];

    	/** Check for a title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s,"    <TITLE>%s</TITLE>\n",ptr);
	    }

	/** Loop through the frames (widget/page items) for geometry data **/
	htrDisableBody(s);
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    t = wgtrGetPropertyType(sub_tree, "framesize");
	    if (t < 0) 
		{
		snprintf(nbuf,16,"*");
		}
	    else if (t == DATA_T_INTEGER)
		{
		wgtrGetPropertyValue(sub_tree, "framesize", DATA_T_INTEGER,POD(&n));
		snprintf(nbuf,16,"%d",n);
		}
	    else if (t == DATA_T_STRING)
		{
		wgtrGetPropertyValue(sub_tree, "framesize", DATA_T_STRING,POD(&ptr));
		memccpy(nbuf, ptr, 0, 15);
		nbuf[15] = 0;
		}
	    if (geom_str[0] != '\0')
		{
		strcat(geom_str,",");
		}
	    strcat(geom_str,nbuf);
	    }

	/** Check for some optional params **/
	if (wgtrGetPropertyValue(tree,"direction",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    if (!strcmp(ptr,"rows")) direc=1;
	    else if (!strcmp(ptr,"columns")) direc=0;
	    }
	if (wgtrGetPropertyValue(tree,"borderwidth",DATA_T_INTEGER,POD(&n)) != 0)
	    { 
	    bdr = n;
	    }

	/** Build the frameset tag. **/
	htrAddBodyItem_va(s, "<FRAMESET %s=%s border=%d>\n", direc?"rows":"cols", geom_str, bdr);

	/** Check for more sub-widgets within the page. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    wgtrGetPropertyValue(sub_tree,"name",DATA_T_STRING,POD(&ptr));
	    if (wgtrGetPropertyValue(sub_tree,"marginwidth",DATA_T_INTEGER,POD(&n)) != 0)
		htrAddBodyItem_va(s,"    <FRAME SRC=./%s>\n",ptr);
	    else
		htrAddBodyItem_va(s,"    <FRAME SRC=./%s MARGINWIDTH=%d>\n",ptr,n);
	    }

	/** End the framset. **/
	htrAddBodyItem_va(s, "</FRAMESET>\n");
    return 0;
    }


/*** htsetInitialize - register with the ht_render module.
 ***/
int
htsetInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Frameset Driver");
	strcpy(drv->WidgetName,"frameset");
	drv->Render = htsetRender;
	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    return 0;
    }
