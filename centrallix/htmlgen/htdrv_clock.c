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
/* Module: 	htdrv_clock.c         					*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	August 7, 2002						*/
/* Description:	HTML Widget driver for a local time clock.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: htdrv_clock.c,v 1.12 2004/08/02 14:09:34 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_clock.c,v $

    $Log: htdrv_clock.c,v $
    Revision 1.12  2004/08/02 14:09:34  mmcgill
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

    Revision 1.11  2004/07/19 15:30:39  mmcgill
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

    Revision 1.10  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.9  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.8  2003/06/03 19:27:09  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.7  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.6  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.5  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.4  2002/08/14 23:07:46  lkehresman
    Fixed some font size issues with the clock widget

    Revision 1.3  2002/08/09 19:25:22  pfinley
    added missing line for version logs of clock widget


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCL;


/*** htclRender - generate the HTML code for the clock widget.
 ***/
int
htclRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char fgcolor1[16];
    char fgcolor2[16] = "";
    int shadowed = 0;
    int shadowx = 0;
    int shadowy = 0;
    int size = 0;
    int moveable = 0;
    int bold = 0;
    int showsecs = 1;
    int showampm = 1;
    int miltime = 0;
    int x=-1,y=-1,w,h;
    int id, i;
    char* nptr;
    char fieldname[HT_FIELDNAME_SIZE];

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTCL","Netscape DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTCL","Clock widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTCL","Clock widget must have a 'height' property");
	    return -1;
	    }
	
	/** Background color/image? **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor=%.40s",ptr);
	else if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Military Time? **/
	if (wgtrGetPropertyValue(tree,"hrtype",DATA_T_INTEGER,POD(&ptr)) == 0 && (int)ptr == 24)
	    {
	    miltime = 1;
	    showampm = 0;
	    }
	else if (wgtrGetPropertyValue(tree,"ampm",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")))
	    {
	    showampm = 0;
	    }

	/** Get text color **/
	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	   sprintf(fgcolor1,"%.40s",ptr);
	else
	   strcpy(fgcolor1,"black");

	/* Shadowed text? */
	if (wgtrGetPropertyValue(tree,"shadowed",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"true") || !strcasecmp(ptr,"yes")))
	    {
	    shadowed = 1;
	    if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
		strcpy(fgcolor2,ptr);
	    else
	        strcpy(fgcolor2,"#777777");
	    if (wgtrGetPropertyValue(tree,"shadowx",DATA_T_INTEGER,POD(&ptr)) == 0)
	        shadowx = (int)ptr;
	    else
	        shadowx = 1;
	    if (wgtrGetPropertyValue(tree,"shadowy",DATA_T_INTEGER,POD(&ptr)) == 0)
	        shadowy = (int)ptr;
	    else
	        shadowy = 1;
	    }

	/** Bold text? **/
	if (wgtrGetPropertyValue(tree,"bold",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    bold = 1;

	/** Get text size **/
	if (wgtrGetPropertyValue(tree,"size",DATA_T_INTEGER,POD(&ptr)) == 0)
	    size = (int)ptr;

	/** Movable? **/
	if (wgtrGetPropertyValue(tree,"moveable",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    moveable = 1;

	/** Show Seconds **/
	if (wgtrGetPropertyValue(tree,"seconds",DATA_T_STRING,POD(&ptr)) == 0 && (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")))
	    showsecs = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get fieldname **/
	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	else 
	    fieldname[0]='\0';

	/** Write Style header items. **/
	htrAddStylesheetItem_va(s,"\t#cl%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddStylesheetItem_va(s,"\t#cl%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,0,0,w,z+2);
	htrAddStylesheetItem_va(s,"\t#cl%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,0,0,w,z+2);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Other global variables **/
	htrAddScriptGlobal(s, "cl_move", "false", 0);
	htrAddScriptGlobal(s, "cl_xOffset", "null", 0);
	htrAddScriptGlobal(s, "cl_yOffset", "null", 0);

	/** Javascript include files **/
	htrAddScriptInclude(s, "/sys/js/htdrv_clock.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Event Handlers **/
	htrAddEventHandler(s, "document","MOUSEUP", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseUp');\n"
	    "        if (ly.mainlayer.moveable) cl_move = false;\n"
	    "        }\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "cl",
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseDown');\n"
	    "        if (ly.mainlayer.moveable)\n"
	    "            {\n"
	    "            cl_move = true;\n"
	    "            cl_xOffset = e.pageX - ly.mainlayer.pageX;\n"
	    "            cl_yOffset = e.pageY - ly.mainlayer.pageY;\n"
	    "            }\n"
	    "        }\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl') cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOUT", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl') cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEMOVE", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseMove');\n"
	    "        if (ly.mainlayer.moveable && cl_move) ly.mainlayer.moveToAbsolute(e.pageX-cl_xOffset, e.pageY-cl_yOffset);\n"
	    "        }\n"
	    "\n");

	/** Set object parent **/
	htrAddScriptInit_va(s, "    htr_set_parent(%s.cxSubElement('cl%dbase'), \"%s\", %s);\n",
		parentname, id, nptr, parentobj);
	    
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = cl_init(%s.layers.cl%dbase, %s.layers.cl%dbase.document.layers.cl%dcon1, %s.layers.cl%dbase.document.layers.cl%dcon2,\"%s\",\"%s\",%d,\"%s\",\"%s\",%d,%d,%d,%d,%d,%d,%d,%d);\n",
		nptr, parentname, id,
		parentname, id, id,
		parentname, id, id,
		fieldname, main_bg, shadowed,
		fgcolor1, fgcolor2,
		size, moveable, bold,
		shadowx, shadowy,
		showsecs, showampm, miltime);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"cl%dbase\">\n",id);
	htrAddBodyItem_va(s, "    <BODY %s><TABLE width=%d height=%d border=0 cellpadding=0 cellspacing=0><TR><TD></TD></TR></TABLE></BODY>\n",main_bg,w,h);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s,    "</DIV>\n");

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1, parentname, nptr);

    return 0;
    }


/*** htclInitialize - register with the ht_render module.
 ***/
int
htclInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Local Time Clock Driver");
	strcpy(drv->WidgetName,"clock");
	drv->Render = htclRender;

	/** Events **/ 
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTCL.idcnt = 0;

    return 0;
    }
