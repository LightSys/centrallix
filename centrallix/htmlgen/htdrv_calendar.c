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
/* Module: 	htdrv_calendar.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	July 9, 2003     					*/
/* Description:	HTML Widget driver for calendar/event type viewing.	*/
/*		This widget provides a view of calendar/event types of	*/
/*		data that has been queried via an objectsource.  It can	*/
/*		be configured to provide year, month, week, and day	*/
/*		views of the data, and will interact with an object-	*/
/*		source to query additional data if need be.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_calendar.c,v 1.7 2004/08/04 20:03:07 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_calendar.c,v $

    $Log: htdrv_calendar.c,v $
    Revision 1.7  2004/08/04 20:03:07  mmcgill
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

    Revision 1.6  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.5  2004/08/02 14:09:33  mmcgill
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

    Revision 1.4  2004/07/19 15:30:39  mmcgill
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

    Revision 1.3  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.2  2003/11/12 22:16:51  gbeeley
    Trying to improve performance on calendar.

    Revision 1.1  2003/07/12 04:14:34  gbeeley
    Initial rough beginnings of a calendar widget.

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCA;


/*** htcaRender - generate the HTML code for the editbox widget.
 ***/
int
htcaRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char cell_bg[128];
    char textcolor[32];
    char dispmode[32] = "year";
    char eventdatefield[32];
    char eventdescfield[32] = "";
    char eventnamefield[32];
    char eventpriofield[32] = "";
    int minpriority=0;
    int x=-1,y=-1,w,h;
    int id, i;
    char* nptr;

	/** Verify user-agent's capabilities allow us to continue... **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCA","Netscape 4.x DOM support or W3C HTML DOM1/CSS1 support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCA.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTCA","Calendar widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = 0;
	
	/** Background color/image? **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"bgColor='%.40s'",ptr);
	else if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Cell background color/image? **/
	if (wgtrGetPropertyValue(tree,"cell_bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(cell_bg,"bgColor='%.40s'",ptr);
	else if (wgtrGetPropertyValue(tree,"cell_background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(cell_bg,"background='%.110s'",ptr);
	else
	    strcpy(cell_bg,"");

	/** Text color? **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(textcolor,"%.31s",ptr);
	else
	    strcpy(textcolor,"black");

	/** Data source field names **/
	if (wgtrGetPropertyValue(tree, "eventdatefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventdatefield' property");
	    return -1;
	    }
	memccpy(eventdatefield, ptr, 0, 31);
	eventdatefield[31] = 0;
	if (wgtrGetPropertyValue(tree, "eventnamefield", DATA_T_STRING, POD(&ptr)) != 0)
	    {
	    mssError(1,"HTCA","Calendar widget must have an 'eventnamefield' property");
	    return -1;
	    }
	memccpy(eventnamefield, ptr, 0, 31);
	eventnamefield[31] = 0;
	if (wgtrGetPropertyValue(tree, "eventpriofield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(eventpriofield, ptr, 0, 31);
	    eventpriofield[31] = 0;
	    }
	if (wgtrGetPropertyValue(tree, "eventdescfield", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(eventdescfield, ptr, 0, 31);
	    eventdescfield[31] = 0;
	    }

	/** display mode **/
	if (wgtrGetPropertyValue(tree, "displaymode", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    memccpy(dispmode, ptr, 0, 31);
	    dispmode[31] = 0;
	    }

	/** minimum priority **/
	wgtrGetPropertyValue(tree, "displaymode", DATA_T_STRING, POD(&minpriority));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ca%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_calendar.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddEventHandler(s, "document","MOUSEUP", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseOver');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEOUT", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseOut');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEMOVE", "ca", 
	    "\n"
	    "    if (ly.kind == 'ca') cn_activate(ly, 'MouseMove');\n"
	    "\n");

	/** Set object parent **/
	htrAddScriptInit_va(s, "    htr_set_parent(%s.layers.ca%dbase, \"%s\", %s);\n",
		parentname, id, nptr, parentobj);

	/** Script initialization call. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit_va(s, "    %s = ca_init(%s.layers.ca%dbase, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d);\n",
		nptr, parentname, id, 
		main_bg, cell_bg, textcolor, dispmode,
		eventdatefield, eventdescfield, eventnamefield, eventpriofield,
		minpriority, w, h);
	    }
	else /** W3C **/
	    {
	    htrAddScriptInit_va(s, "    %s = ca_init(document.getElementById('ca%dbase'), \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d);\n",
		nptr, id, 
		main_bg, cell_bg, textcolor, dispmode,
		eventdatefield, eventdescfield, eventnamefield, eventpriofield,
		minpriority, w, h);
	    }

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"ca%dbase\"><BODY %s text='%s'>\n",id, main_bg, textcolor);




	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1, parentname, nptr);


	/** End the containing layer. **/
	htrAddBodyItem(s, "</BODY></DIV>\n");

    return 0;
    }


/*** htcaInitialize - register with the ht_render module.
 ***/
int
htcaInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Calendar View Driver");
	strcpy(drv->WidgetName,"calendar");
	drv->Render = htcaRender;

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");
	HTCA.idcnt = 0;

    return 0;
    }
