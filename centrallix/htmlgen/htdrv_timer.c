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
/* Module: 	htdrv_timer.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 1, 2001 					*/
/* Description:	HTML Widget driver for a timer nonvisual widget.  The	*/
/*		timer fires off an Event every so often that can cause	*/
/*		other actions on the page to occur.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_timer.c,v 1.11 2004/08/04 01:58:57 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_timer.c,v $

    $Log: htdrv_timer.c,v $
    Revision 1.11  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.10  2004/08/02 14:09:35  mmcgill
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

    Revision 1.9  2004/07/19 15:30:41  mmcgill
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

    Revision 1.7  2002/12/04 00:19:12  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.6  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.5  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.4  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 05:55:17  gbeeley
    Fixed html problem in the timer.  Doesn't need a closing DIV when
    it never had an opening DIV tag.  Will only mess things up, it will.

    Revision 1.1  2001/11/03 05:48:33  gbeeley
    Ok.  Really added the timer file this time.  Forgot last time.  Of
    course, the silly cvs log won't reflect my forgotten time in the
    timer widget file, because wasn't in cvs when it wasn't checked in
    yet.  Duh.  Obvious.  It's getting late....  I think my 'awake'
    timer has hit its 'expire' event ;)


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTM;



/*** httmRender - generate the HTML code for the timer nonvisual widget.
 ***/
int
httmRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[200];
    char sbuf2[160];
    int id;
    char* nptr;
    int msec;
    int auto_reset = 0;
    int auto_start = 1;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTTM","Netscape DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTM.idcnt++);

	/** Get msec for timer countdown **/
	if (wgtrGetPropertyValue(tree,"msec",DATA_T_INTEGER,POD(&msec)) != 0)
	    {
	    mssError(1,"HTTM","Timer widget must have 'msec' time specified");
	    return -1;
	    }

	/** Get auto reset and auto start settings **/
	if (wgtrGetPropertyValue(tree,"auto_reset",DATA_T_INTEGER,POD(&auto_reset)) != 0) auto_reset = 0;
	if (wgtrGetPropertyValue(tree,"auto_start",DATA_T_INTEGER,POD(&auto_start)) != 0) auto_start = 1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_timer.js", 0);

	/** Script initialization call. **/
	snprintf(sbuf,200,"    %s = tm_init(%d, %d, %d);\n", nptr, msec, auto_reset, auto_start);
	htrAddScriptInit(s, sbuf);

	/** Check for objects within the timer. **/
	snprintf(sbuf,200,"%s.document",nptr);
	snprintf(sbuf2,160,"%s",nptr);

    htrAddScriptWgtr(s, "    // htdrv_timer.c\n");
    /** Add this node to the widget tree **/
    htrAddScriptWgtr_va(s, "    child_node = new WgtrNode('%s', '%s', %s, false)\n", tree->Name, tree->Type, nptr);
    htrAddScriptWgtr_va(s, "    wgtrAddChild(curr_node[0], child_node);\n");

    /** make ourself the current node for our children **/
    htrAddScriptWgtr(s, "    curr_node.unshift(child_node);\n\n");


	htrRenderSubwidgets(s, tree, sbuf, sbuf2, z+2);

    /** make our parent the current node again **/
    htrAddScriptWgtr(s, "    curr_node.shift();\n\n");

    return 0;
    }


/*** httmInitialize - register with the ht_render module.
 ***/
int
httmInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Nonvisual Timer Widget");
	strcpy(drv->WidgetName,"timer");
	drv->Render = httmRender;

	/** Add an 'expired' event **/
	htrAddEvent(drv,"Expire");

	/** Add a 'set timer' action **/
	htrAddAction(drv,"SetTimer");
	htrAddParam(drv,"SetTimer","Time",DATA_T_INTEGER);
	htrAddParam(drv,"SetTimer","AutoReset",DATA_T_INTEGER);

	/** Add a 'cancel timer' action **/
	htrAddAction(drv,"CancelTimer");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTM.idcnt = 0;

    return 0;
    }
