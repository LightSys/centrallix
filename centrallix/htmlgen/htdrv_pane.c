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
/* Module: 	htdrv_tab.c             				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 28, 1998 					*/
/* Description:	HTML Widget driver for a tab control.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_pane.c,v 1.32 2006/10/16 18:34:34 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_pane.c,v $

    $Log: htdrv_pane.c,v $
    Revision 1.32  2006/10/16 18:34:34  gbeeley
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

    Revision 1.31  2006/04/07 06:51:41  gbeeley
    - (bugfix) make sure events get activated on the main object rather than
      on sublayers.

    Revision 1.30  2005/06/23 22:07:59  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.29  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.28  2004/08/15 03:55:38  gbeeley
    - set overflow to hidden on pane div's.  Can someone PLEASE explain to me
      why on earth IE changes its box model for a div when you set overflow to
      hidden?  Does W3C call for that???

    Revision 1.27  2004/08/04 20:03:09  mmcgill
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

    Revision 1.26  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.25  2004/08/02 14:09:34  mmcgill
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

    Revision 1.24  2004/07/19 15:30:40  mmcgill
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

    Revision 1.23  2004/04/29 16:26:42  gbeeley
    - Fixes to get FourTabs.app working again in NS4/Moz, and in IE5.5/IE6.
    - Added inline-include feature to help with debugging in IE, which does
      not specify the correct file in its errors.  To use it, just append
      "?ls__collapse_includes=yes" to your .app URL.

    Revision 1.22  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.21  2003/07/28 22:26:20  gbeeley
    Minor geometry fixes to the pane widget.

    Revision 1.20  2003/07/28 22:05:25  gbeeley
    Added 'flat' pane style which does not have a raised/lowered border.

    Revision 1.19  2003/07/27 03:24:54  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

    Revision 1.18  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.17  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.16  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.15  2002/08/13 04:09:09  anoncvs_obe
    Pane widget was adding several pixels' worth of offset as a container,
    causing static content (such as a static widget/html) to be shifted
    right and down a noticeable amount.  Problem was a table without the
    needed border/spacing information set.

    Revision 1.14  2002/08/01 18:27:16  pfinley
    The border images of the pane widget are now tagged with .kind, .layer, &
    .mainlayer properties using the utils tag function.

    Revision 1.13  2002/08/01 14:25:15  lkehresman
    Removed closing body tag.  There was no opening body tag, so Netscape was
    thinking that this was the close of the HTML body.  Thus, it wasn't properly
    rendering widgets after any pane widget.  This was an interesting one to
    try and track down.  ;)

    Revision 1.12  2002/07/29 21:34:40  lkehresman
    fixed typo by peter

    Revision 1.11  2002/07/26 20:23:06  pfinley
    Added standard event handlers: Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove

    Revision 1.10  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.9  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.8  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.7  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.6  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.5  2002/03/13 19:48:46  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTPN;


/*** htpnRender - generate the HTML code for the page.
 ***/
int
htpnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int style = 1; /* 0 = lowered, 1 = raised, 2 = none */
    char* c1;
    char* c2;
    int box_offset;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTPN","Netscape DOM or W3C DOM1 HTML and CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTPN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTPN","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTPN","Pane widget must have a 'height' property");
	    return -1;
	    }

	/** Background color/image? **/
	htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_bg,sizeof(main_bg));

	/** figure out box offset fudge factor... stupid box model... **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Style of pane - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"lowered")) style = 0;
	    if (!strcmp(ptr,"raised")) style = 1;
	    if (!strcmp(ptr,"flat")) style = 2;
	    }
	if (style == 1) /* raised */
	    {
	    if(s->Capabilities.Dom0NS /*|| s->Capabilities.Dom0IE*/)
		{
		c1 = "white_1x1.png";
		c2 = "dkgrey_1x1.png";
		}
	    else if(s->Capabilities.CSS1)
		{
		c1 = "white";
		c2 = "gray";
		}
	    else
		{
		mssError(0,"HTPN","Cannot render");
		}
	    }
	else if (style == 0) /* lowered */
	    {
	    if(s->Capabilities.Dom0NS /*|| s->Capabilities.Dom0IE*/)
		{
		c1 = "dkgrey_1x1.png";
		c2 = "white_1x1.png";
		}
	    else if(s->Capabilities.CSS1)
		{
		c1 = "gray";
		c2 = "white";
		}
	    else
		{
		mssError(0,"HTPN","Cannot render");
		}
	    }

	/** Ok, write the style header items. **/
	if(s->Capabilities.Dom0NS /*|| s->Capabilities.Dom0IE*/)
	    {
	    htrAddStylesheetItem_va(s,"\t#pn%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	    htrAddStylesheetItem_va(s,"\t#pn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,1,1,w-2,h-2,z+1);
	    }
	else if(s->Capabilities.CSS1)
	    {
	    if (style == 2) /* flat */
		{
		htrAddStylesheetItem_va(s,"\t#pn%dmain { POSITION:absolute; VISIBILITY:inherit; overflow:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,x,y,w,h,z);
		htrAddStylesheetItem_va(s,"\t#pn%dmain { %s}\n",id,main_bg);
		}
	    else /* lowered or raised */
		{
		htrAddStylesheetItem_va(s,"\t#pn%dmain { POSITION:absolute; VISIBILITY:inherit; overflow: hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,x,y,w-2*box_offset,h-2*box_offset,z);
		htrAddStylesheetItem_va(s,"\t#pn%dmain { border-style: solid; border-width: 1px; border-color: %s %s %s %s; %s}\n",id,c1,c2,c2,c1,main_bg);
		}
	    }
	else
	    {
	    mssError(0,"HTPN","Cannot render");
	    }

	/** DOM linkages **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddWgtrObjLinkage_va(s, tree, 
		    "htr_subel(_parentctr, \"pn%dbase\")",id);
	    htrAddWgtrCtrLinkage_va(s, tree, 
		    "htr_subel(_obj, \"pn%dmain\")",id);
	    }
	else
	    htrAddWgtrObjLinkage_va(s, tree, 
		    "htr_subel(_parentctr, \"pn%dmain\")",id);

	/** Script include call **/
	htrAddScriptInclude(s, "/sys/js/htdrv_pane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event Handlers **/
	htrAddEventHandler(s, "document","MOUSEUP", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'Click');\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEOUT", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'MouseOut');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEMOVE", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly.mainlayer, 'MouseMove');\n"
	    "\n");

	if(s->Capabilities.Dom0NS || (s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    /** Script initialization call. **/
	    if (s->Capabilities.Dom0NS)
		htrAddScriptInit_va(s, "    pn_init({mainlayer:nodes['%s'], layer:htr_subel(nodes['%s'], \"pn%dmain\")});\n", 
			name, name, id);
	    else
		htrAddScriptInit_va(s, "    pn_init({mainlayer:nodes[\"%s\"], layer:nodes[\"%s\"]});\n",
			name, name);

	    /** HTML body <DIV> element for the base layer. **/
	    if (s->Capabilities.Dom0NS)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"pn%dbase\">\n",id);
		htrAddBodyItem_va(s,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s height=%d>\n",w,main_bg,h);
		if (style == 2) /* flat */
		    {
		    htrAddBodyItem_va(s,"        <TR><TD><img src=/sys/images/trans_1.gif></TD></TR>\n    </TABLE>\n\n");
		    }
		else /* lowered or raised */
		    {
		    htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
		    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
		    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
		    htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
		    htrAddBodyItem_va(s,"            <TD><img src=/sys/images/trans_1.gif></TD>\n");
		    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
		    htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
		    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
		    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
		    }
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"pn%dmain\"><table width=%d height=%d cellspacing=0 cellpadding=0 border=0><tr><td>\n",id, w-2, h-2);

	    /** Check for objects within the pane. **/
	    htrRenderSubwidgets(s, tree, z+2);

	    /** End the containing layer. **/
	    htrAddBodyItem(s, "</td></tr></table></DIV>\n");
	    if (s->Capabilities.Dom0NS) htrAddBodyItem(s, "</DIV>\n");
	    }
	else
	    {
	    mssError(0,"HTPN","Cannot render");
	    }

    return 0;
    }


/*** htpnInitialize - register with the ht_render module.
 ***/
int
htpnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"pane");
	drv->Render = htpnRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTPN.idcnt = 0;

    return 0;
    }
