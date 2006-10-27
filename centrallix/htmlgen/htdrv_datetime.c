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
/* Module: 	htdrv_datetime.c        				*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	June 26, 2002						*/
/* Description:	HTML driver for a 'date time' widget			*/
/************************************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTDT;


/*** htdtRender - generate the HTML code for the page.
 ***/
int
htdtRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char *sql;
    char *str;
    char *attr;
    char name[64];
    char initialdate[64];
    char fgcolor[64];
    char bgcolor[128];
    char fieldname[30];
    int type;
    int x,y,w,h,w2=184,h2=190;
    int id, i;
    DateTime dt;
    ObjData od;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTDT","Netscape or W3C DOM support required");
	    return -1;
	    }

	/** Get an id for this. **/
	id = (HTDT.idcnt++);

	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTDT","Date/Time widget must have a 'height' property");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else 
	    fieldname[0]='\0';
	
	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Get initial date **/
	if (wgtrGetPropertyValue(tree, "sql", DATA_T_STRING,POD(&sql)) == 0) 
	    {
	    for (i=0;i<xaCount(&(tree->Children));i++)
		{
		attr = wgtrFirstPropertyName(tree);
		if (!attr)
		    {
		    mssError(1, "HTDT", "There was an error getting date from your SQL query");
		    return -1;
		    }
		type = wgtrGetPropertyType(tree, attr);
		wgtrGetPropertyValue(tree, attr, type, &od);
		if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
		    str = objDataToStringTmp(type, (void*)(&od), 0);
		else
		    str = objDataToStringTmp(type, (void*)(od.String), 0);
		strtcpy(initialdate, str, sizeof(initialdate));
		}
/*
	    if ((qy = objMultiQuery(w_obj->Session, sql))) 
		{
		while ((qy_obj = objQueryFetch(qy, O_RDONLY)))
		    {
		    attr = objGetFirstAttr(qy_obj);
		    if (!attr)
			{
			objClose(qy_obj);
			objQueryClose(qy);
			mssError(1, "HTDT", "There was an error getting date from your SQL query");
			return -1;
			}
		    type = objGetAttrType(qy_obj, attr);
		    rval = objGetAttrValue(qy_obj, attr, type,&od);
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    snprintf(initialdate, 64, "%s", str);
		    objClose(qy_obj);
		    }
		objQueryClose(qy);
		}
*/
	    }
	else if (wgtrGetPropertyValue(tree,"initialdate",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(initialdate, ptr, sizeof(initialdate));
	    }
	else
	    {
	    initialdate[0]='\0';
	    }
	if (strlen(initialdate))
	    {
	    objDataToDateTime(DATA_T_STRING, initialdate, &dt, NULL);
	    snprintf(initialdate, sizeof(initialdate), "%s %d %d, %d:%d:%d", obj_short_months[dt.Part.Month], 
	                          dt.Part.Day+1,
	                          dt.Part.Year+1900,
	                          dt.Part.Hour,
	                          dt.Part.Minute,
	                          dt.Part.Second);
	    }
	

	/** Get colors **/
	htrGetBackground(tree, NULL, 0, bgcolor, sizeof(bgcolor));
	if (!*bgcolor) strcpy(bgcolor,"bgcolor=#c0c0c0");
	//else strcpy(bgcolor, "bgcolor=green");

///////////////////////////////////////
//	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
//	    sprintf(bgcolor,"%.40s",ptr);
//	else {
	    //mssError(1,"HTDD","Date Time widget must have a 'bgcolor' property");
	    //return -1;
	    //strcpy(bgcolor,"blue");
//	}
	
	if (wgtrGetPropertyValue(tree,"fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor,ptr,sizeof(fgcolor));
	else
	    strcpy(fgcolor,"black");

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#dt%dbtn  { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	htrAddStylesheetItem_va(s,"\t#dt%dcon1 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:1px; TOP:1px; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,w-20,h-2,z+1);
	htrAddStylesheetItem_va(s,"\t#dt%dcon2 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:hidden; LEFT:1px; TOP:1px; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,w-20,h-2,z+1);

	/** Write named global **/
	htrAddScriptGlobal(s, "dt_current", "null", 0);
	htrAddScriptGlobal(s, "dt_timeout", "null", 0);
	htrAddScriptGlobal(s, "dt_timeout_fn", "null", 0);
	htrAddScriptGlobal(s, "dt_img_y", "0", 0);

	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"dt%dbtn\")",id);
	htrAddWgtrCtrLinkage(s, tree, "_obj");

	/** Script includes **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_date.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_datetime.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    dt_init({layer:nodes[\"%s\"],c1:htr_subel(nodes[\"%s\"],\"dt%dcon1\"),c2:htr_subel(nodes[\"%s\"],\"dt%dcon2\"),id:\"%s\", background:\"%s\", foreground:\"%s\", fieldname:\"%s\", width:%d, height:%d, width2:%d, height2:%d})\n",
	    name,
	    name,id, 
	    name,id, 
	    initialdate, bgcolor, fgcolor, fieldname, w-20, h, w2,h2);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dbtn\">\n", id);
	htrAddBodyItem_va(s,"<TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w, bgcolor);
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
	htrAddBodyItem_va(s,"       <TD align=right valign=middle><IMG SRC=/sys/images/ico17.gif></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
	htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"</TABLE>\n");
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s,"<DIV ID=\"dt%dcon2\"></DIV>\n",id);
	htrAddBodyItem_va(s,"</DIV>\n");

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","dt","dt_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","dt","dt_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","dt","dt_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER","dt","dt_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT","dt","dt_mouseout");

	/** Check for more sub-widgets within the datetime. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
    }


/*** htdtInitialize - register with the ht_render module.
 ***/
int
htdtInitialize()
    {
    pHtDriver drv;

	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Date/Time Widget Driver");
	strcpy(drv->WidgetName,"datetime");
	drv->Render = htdtRender;

	/** Register events **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"GetFocus");
	htrAddEvent(drv,"LoseFocus");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTDT.idcnt = 0;

    return 0;
    }

/**CVSDATA***************************************************************

    $Id: htdrv_datetime.c,v 1.36 2006/10/27 05:57:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_datetime.c,v $

    $Log: htdrv_datetime.c,v $
    Revision 1.36  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.35  2006/10/16 18:34:33  gbeeley
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

    Revision 1.34  2006/08/07 15:14:20  darkwing0o0rama
    changed for mozilla compatibility

    Revision 1.33  2005/06/23 22:07:58  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.32  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.31  2004/08/04 20:03:07  mmcgill
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

    Revision 1.30  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.29  2004/08/02 14:09:34  mmcgill
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

    Revision 1.28  2004/07/19 15:30:39  mmcgill
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

    Revision 1.27  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.26  2004/05/06 01:23:00  gbeeley
    - various enhancements/updates to datetime widget

    Revision 1.25  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.24  2003/06/03 20:29:11  gbeeley
    Fix to CSV driver due to uninitialized memory causing a segfault when
    opening CSV files from time to time.

    Revision 1.23  2003/06/03 19:27:09  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.22  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.21  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.20  2002/07/31 14:06:05  lkehresman
    Moved CVS log messages to bottom

    Revision 1.19  2002/07/25 20:26:57  lkehresman
    Changed the "Change" event to our standardized "DataChange" event, and
    actually added the connector call for it (forgot to do that earlier).

    Revision 1.18  2002/07/25 18:46:35  lkehresman
    Standardized event connectors for datetime widget.  It now has:
    MouseUp,MouseDown,MouseOver,MouseOut,MouseMove,Change,GetFocus,LoseFocus

    Revision 1.17  2002/07/25 17:06:45  lkehresman
    Added the width parameter to datetime drawing function.

    Revision 1.16  2002/07/22 15:24:54  lkehresman
    Fixed datetime widget so the dropdown calendar wouldn't be hidden by window
    borders.  The dropdown calendar now is just a floating layer that is always
    above the other layers so it can extend beyond a window border.

    Revision 1.15  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.14  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.13  2002/07/17 20:20:43  lkehresman
    Overhaul of the datetime widget (c file)

 **END-CVSDATA***********************************************************/
