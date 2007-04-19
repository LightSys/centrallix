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
/* Module: 	htdrv_window.c      					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 16, 1999					*/
/* Description:	HTML Widget driver for a window -- a DHTML layer that	*/
/*		can be dragged around the screen and appears to have	*/
/*		a 'titlebar' with a close (X) button on it.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_window.c,v 1.49 2007/04/19 21:26:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_window.c,v $

    $Log: htdrv_window.c,v $
    Revision 1.49  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.48  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.47  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.46  2006/10/16 18:34:34  gbeeley
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

    Revision 1.45  2005/10/01 00:23:46  gbeeley
    - (change) renamed 'htmlwindow' to 'childwindow' to remove the terminology
      dependence on the dhtml/http app delivery mechanism

    Revision 1.44  2005/06/23 22:08:01  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.43  2005/02/26 06:42:38  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.42  2004/12/31 04:42:03  gbeeley
    - global variable pollution fixes for dropdown widget
    - use dd_collapse() for dropdown widget event script
    - fix to background image for windows

    Revision 1.41  2004/08/30 03:20:19  gbeeley
    - updates for widgets
    - bugfix for htrRender() handling of event handler function return values

    Revision 1.40  2004/08/15 02:08:38  gbeeley
    - fixing lots of geometry and functionality issues for IE and Moz for the
      window widget.

    Revision 1.39  2004/08/04 20:03:11  mmcgill
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

    Revision 1.38  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.37  2004/08/02 14:09:35  mmcgill
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

    Revision 1.36  2004/07/19 15:30:42  mmcgill
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

    Revision 1.35  2003/11/22 16:37:18  jorupp
     * add support for moving event handler scripts to the .js code
     	note: the underlying implimentation in ht_render.c_will_ change, this was
    	just to get opinions on the API and output
     * moved event handlers for htdrv_window from the .c to the .js

    Revision 1.34  2003/08/02 22:12:06  jorupp
     * got treeview pretty much working (a bit slow though)
    	* I split up several of the functions so that the Mozilla debugger's profiler could help me out more
     * scrollpane displays, doesn't scroll

    Revision 1.33  2003/07/20 03:41:17  jorupp
     * got window mostly working in Mozilla

    Revision 1.32  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.31  2002/12/04 00:19:12  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.30  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.29  2002/08/23 17:31:05  lkehresman
    moved window_current global to the page widget so it is always defined
    even if it is null.  This prevents javascript errors from the objectsource
    widget when no windows exist in the app.

    Revision 1.28  2002/08/15 14:08:47  pfinley
    fixed a bit masking issue with the page closetype flag.

    Revision 1.27  2002/08/15 13:58:16  pfinley
    Made graphical window closing and shading properties of the window widget,
    rather than globally of the page.

    Revision 1.26  2002/08/14 20:16:38  pfinley
    Added some visual effets for the window:
     - graphical window shading (enable by setting gshade="true")
     - added 3 new closing types (enable by setting closetype="shrink1","shrink2", or "shrink3")

    These are gloabl changes, and can only be set on the page widget... these
    will become part of theming once it is implemented (i think).

    Revision 1.25  2002/08/12 17:51:16  pfinley
    - added an attract option to the page widget. if this is set, centrallix
      windows will attract to the edges of the browser window. set to how many
      pixels from border to attract.
    - also fixed a mainlayer issue with the window widget which allowed for
      the contents of a window to be draged and shaded (very interesting :)

    Revision 1.24  2002/08/01 19:22:25  lkehresman
    Renamed what was previously known as mainlayer (aka ml) to be ContentLayer
    so that it is more descriptive and doesn't conflict with other mainlayer
    properties.

    Revision 1.23  2002/07/30 12:45:44  lkehresman
    * Added standard events to window
    * Reworked the window layer properties a bit so they are standard
        (x.document.layer points to itself, x.mainlayer points to top layer)
    * Added a bugfix to the connector (it was missing brackets around a
        conditional statement
    * Changed the subwidget parsing to pass different parameters if the
        subwidget is a connector.

    Revision 1.22  2002/07/20 20:16:52  lkehresman
    Added ToggleVisibility event connector to the window widget

    Revision 1.21  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.20  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.19  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.18  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.17  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.16  2002/06/18 15:41:52  lkehresman
    Fixed a bug that made window header backgrounds transparent if the color
    wasn't specifically set.  Now it grabs the normal background color/image
    by default but the default can be overwritten by "hdr_bgcolor" and
    "hdr_background".

    Revision 1.15  2002/06/17 21:35:56  jorupp
     * allowed for window inside of window (same basic method used with form and osrc)

    Revision 1.14  2002/06/09 23:44:47  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.13  2002/06/06 17:12:23  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.12  2002/06/01 19:49:30  lkehresman
    A couple changes that provide visual enhancements to the window widget

    Revision 1.11  2002/05/31 01:26:41  lkehresman
    * modified the window header HTML to make it look nicer
    * fixed a truncation problem with the image button

    Revision 1.10  2002/03/13 19:48:45  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.9  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.8  2002/02/13 19:35:55  lkehresman
    Fixed another bug I introduced.  ly.document isn't even always there.

    Revision 1.7  2002/02/13 19:30:48  lkehresman
    Fixed a bug I introduced with my last commit.  ly.document.images[6] doesn't always exist.

    Revision 1.6  2002/02/13 19:20:40  lkehresman
    Fixed a minor bug that wouldn't reset the "X" image if the close button
    was clicked, but the mouse moved out of the image border.

    Revision 1.5  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.4  2001/10/09 01:14:52  lkehresman
    Made a few modifications to the behavior of windowshading.  It now forgets
    clicks that do other things such as raising windows--it won't count that as
    the first click of a double-click.

    Revision 1.3  2001/10/08 04:17:14  lkehresman
     * Cleaned up the generated code for windowshading (Beely-standard Complient)
     * Testing out emailing CVS commits

    Revision 1.2  2001/10/08 03:59:54  lkehresman
    Added window shading support

    Revision 1.1.1.1  2001/08/13 18:00:52  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:56  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTWIN;


/*** htwinRender - generate the HTML code for the page.
 ***/
int
htwinRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    pWgtrNode sub_tree;
    int x,y,w,h;
    int tbw,tbh,bx,by,bw,bh;
    int id, i;
    int visible = 1;
    char bgnd[128] = "";	    /* these bgnd's must all be same length */
    char hdr_bgnd[128] = "";
    char bgnd_style[128] = "";
    char hdr_bgnd_style[128] = "";
    char txtcolor[64] = "";
    int has_titlebar = 1;
    char title[128];
    int is_dialog_style = 0;
    int gshade = 0;
    int closetype = 0;
    int box_offset = 1;
    char icon[128];

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTWIN","Netscape DOM support or W3C DOM Level 1 support required");
	    return -1;
	    }

	/** IE puts css box borders inside box width/height **/
	if (!s->Capabilities.CSSBox)
	    {
	    box_offset = 0;
	    }

    	/** Get an id for this. **/
	id = (HTWIN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x = 0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y = 0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Check background color **/
	htrGetBackground(tree, NULL, 1, bgnd_style, sizeof(bgnd_style));
	htrGetBackground(tree, NULL, 0, bgnd, sizeof(bgnd));

	/** Check header background color/image **/
	if (htrGetBackground(tree, "hdr", 1, hdr_bgnd_style, sizeof(hdr_bgnd_style)) < 0)
	    strcpy(hdr_bgnd_style, bgnd_style);
	if (htrGetBackground(tree, "hdr", 0, hdr_bgnd, sizeof(hdr_bgnd)) < 0)
	    strcpy(hdr_bgnd, bgnd);

	/** Check title text color. **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(txtcolor,ptr,sizeof(txtcolor));
	else
	    strcpy(txtcolor,"black");

	/** Check window title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(title,ptr, sizeof(title));
	else
	    strcpy(title,name);

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** No titlebar? **/
	if (wgtrGetPropertyValue(tree,"titlebar",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no"))
	    has_titlebar = 0;

	/** Dialog or window style? **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
	    is_dialog_style = 1;

	/** Graphical window shading? **/
	if (wgtrGetPropertyValue(tree,"gshade",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    gshade = 1;

	/** Graphical window close? **/
	if (wgtrGetPropertyValue(tree,"closetype",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"shrink1")) closetype = 1;
	    else if (!strcmp(ptr,"shrink2")) closetype = 2;
	    else if (!strcmp(ptr,"shrink3")) closetype = 1 | 2;
	    }

	/** Window icon? **/
	if (wgtrGetPropertyValue(tree, "icon", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    strtcpy(icon,ptr,sizeof(icon));
	    }
	else
	    {
	    strcpy(icon, "/sys/images/centrallix_18x18.gif");
	    }

	/** Compute titlebar width & height - includes edge below titlebar. **/
	if (has_titlebar)
	    {
	    tbw = w-2;
	    if (is_dialog_style || !s->Capabilities.Dom0NS)
	        tbh = 24;
	    else
	        tbh = 23;
	    }
	else
	    {
	    tbw = w-2;
	    tbh = 0;
	    }

	/** Compute window body geometry **/
	if (is_dialog_style)
	    {
	    bx = 1;
	    by = 1+tbh;
	    bw = w-2;
	    bh = h-tbh-2;
	    }
	else
	    {
	    bx = 2;
	    bw = w-4;
	    if (has_titlebar)
		{
		by = 1+tbh;
		bh = h-tbh-3;
		}
	    else
		{
		by = 2;
		bh = h-4;
		}
	    }

	if(s->Capabilities.HTML40 && s->Capabilities.CSS2)
	    {
	    /** Draw the main window layer and outer edge. **/
	    htrAddStylesheetItem_va(s,"\t#wn%POSbase { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS;}\n",
		    id,visible?"inherit":"hidden",x,y,w-2*box_offset,h-2*box_offset, w, h, z+100);
	    htrAddStylesheetItem_va(s,"\t#wn%POSbase { border-style: solid; border-width: 1px; border-color: white gray gray white; }\n", id);

	    /** draw titlebar div **/
	    if (has_titlebar)
		{
		htrAddStylesheetItem_va(s,"\t#wn%POStitlebar { POSITION: absolute; VISIBILITY: inherit; LEFT: 0px; TOP: 0px; HEIGHT: %POSpx; WIDTH: 100%%; overflow: hidden; Z-INDEX: %POS; color:%STR&HTE; %STR}\n", id, tbh-1-box_offset, z+1, txtcolor, hdr_bgnd_style);
		htrAddStylesheetItem_va(s,"\t#wn%POStitlebar { border-style: solid; border-width: 0px 0px 1px 0px; border-color: gray; }\n", id);
		}

	    /** inner structure depends on dialog vs. window style **/
	    if (is_dialog_style)
		{
		/** window inner container -- dialog **/
		htrAddStylesheetItem_va(s,"\t#wn%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:%INTpx; WIDTH: %POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS; %STR}\n",
			id, tbh?(tbh-1):0, w-2, h-tbh-1, w, h-tbh+1, z+1, bgnd_style);
		htrAddStylesheetItem_va(s,"\t#wn%POSmain { border-style: solid; border-width: %POSpx 0px 0px 0px; border-color: white; }\n", id, has_titlebar?1:0);
		}
	    else
		{
		/** window inner container -- window **/
		htrAddStylesheetItem_va(s,"\t#wn%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:%INTpx; WIDTH: %POSpx; HEIGHT:%POSpx; overflow: hidden; clip:rect(0px, %INTpx, %INTpx, 0px); Z-INDEX:%POS; %STR}\n",
			id, tbh?(tbh-1):0, w-2-2*box_offset, h-tbh-(has_titlebar?1:2)-(has_titlebar?1:2)*box_offset, w, h-tbh+(has_titlebar?1:0)-2*box_offset, z+1, bgnd_style);
		htrAddStylesheetItem_va(s,"\t#wn%POSmain { border-style: solid; border-width: %POSpx 1px 1px 1px; border-color: gray white white gray; }\n", id, has_titlebar?0:1);
		}
	    }
	else
	    {
	    /** Write the style header items for NS4 type browsers. **/
	    htrAddStylesheetItem_va(s,"\t#wn%POSbase { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; clip:rect(%INTpx, %INTpx); Z-INDEX:%POS; }\n",
		    id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	    htrAddStylesheetItem_va(s,"\t#wn%POSmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; clip:rect(%INTpx,%INTpx); Z-INDEX:%POS; }\n",
		    id, bx, by, bw, bh, bw, bh, z+1);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "wn_top_z","10000",0);
	htrAddScriptGlobal(s, "wn_list","new Array()",0);
	htrAddScriptGlobal(s, "wn_current","null",0);
	htrAddScriptGlobal(s, "wn_newx","null",0);
	htrAddScriptGlobal(s, "wn_newy","null",0);
	htrAddScriptGlobal(s, "wn_topwin","null",0);
	htrAddScriptGlobal(s, "wn_msx","null",0);
	htrAddScriptGlobal(s, "wn_msy","null",0);
	htrAddScriptGlobal(s, "wn_moved","0",0);
	htrAddScriptGlobal(s, "wn_clicked","0",0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"wn%POSbase\")",id);
	htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(_obj, \"wn%POSmain\")",id);

	htrAddScriptInclude(s, "/sys/js/htdrv_window.js", 0);

	/** Event handler for mousedown/up/click/etc **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "wn", "wn_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "wn", "wn_mouseup");
	htrAddEventHandlerFunction(s, "document", "DBLCLICK", "wn", "wn_dblclick");

	/** Mouse move event handler -- when user drags the window **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "wn", "wn_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "wn", "wn_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "wn", "wn_mouseout");

	if(s->Capabilities.Dom1HTML)
	    {
	    /** Script initialization call. **/
	    if (has_titlebar)
		{
		htrAddScriptInit_va(s,"    wn_init({mainlayer:nodes[\"%STR&SYM\"], clayer:wgtrGetContainer(nodes[\"%STR&SYM\"]), gshade:%INT, closetype:%INT, titlebar:htr_subel(nodes[\"%STR&SYM\"],'wn%POStitlebar')});\n", 
			name,name,gshade,closetype, name, id);
		}
	    else
		{
		htrAddScriptInit_va(s,"    wn_init({mainlayer:nodes[\"%STR&SYM\"], clayer:nodes[\"%STR&SYM\"], gshade:%INT, closetype:%INT, titlebar:null});\n", 
			name,name,gshade,closetype);
		}
	    }
	else if(s->Capabilities.Dom0NS)
	    {
	    /** Script initialization call. **/
	    htrAddScriptInit_va(s,"    wn_init({mainlayer:nodes[\"%STR&SYM\"], clayer:wgtrGetContainer(nodes[\"%STR&SYM\"]), gshade:%INT, closetype:%INT, titlebar:null});\n", 
		    name,name,gshade,closetype);
	    }

	/** HTML body <DIV> elements for the layers. **/
	if(s->Capabilities.HTML40 && s->Capabilities.CSS2) 
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%POSbase\">\n",id);
	    if (has_titlebar)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"wn%POStitlebar\">\n",id);
		htrAddBodyItem_va(s,"<table border=0 cellspacing=0 cellpadding=0 height=%POS><tr><td><table cellspacing=0 cellpadding=0 border=0 width=%POS><tr><td align=left><TABLE cellspacing=0 cellpadding=0 border=\"0\"><TR><td width=26 align=center><img width=18 height=18 src=\"%STR&HTE\" name=\"icon\"></td><TD valign=\"middle\" nobreak><FONT COLOR='%STR&HTE'>&nbsp;<b>%STR&HTE</b></FONT></TD></TR></TABLE></td><td align=right><IMG src=\"/sys/images/01bigclose.gif\" name=\"close\" align=\"right\"></td></tr></table></td></tr></table>\n", 
			tbh-1, tbw-2, icon, txtcolor, title);
		htrAddBodyItem(s,   "</DIV>\n");
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%POSmain\">\n",id);
	    }
	else
	    {
	    /** This is the top white edge of the window **/
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%POSbase\"><TABLE border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n",id);
	    htrAddBodyItem(s,   "<TR><TD><IMG src=\"/sys/images/white_1x1.png\" \"width=\"1\" height=\"1\"></TD>\n");
	    if (!is_dialog_style)
		{
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		}
	    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"%POS\" height=\"1\"></TD>\n",is_dialog_style?(tbw):(tbw-2));
	    if (!is_dialog_style)
		{
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		}
	    htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
	    
	    /** Titlebar for window, if specified. **/
	    if (has_titlebar)
		{
		htrAddBodyItem(s,   "<TR><TD width=\"1\"><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"22\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD valign=middle width=\"%POS\" %STR colspan=\"%POS\"><table cellspacing=0 cellpadding=0 border=0 width=%POS><tr><td align=left><TABLE cellspacing=0 cellpadding=0 border=\"0\"><TR><td><img width=18 height=18 src=\"%STR&HTE\" name=\"icon\" align=\"left\"></td><TD valign=\"middle\"><FONT COLOR='%STR&HTE'>&nbsp;<b>%STR&HTE</b></FONT></TD></TR></TABLE></td><td align=right><IMG src=\"/sys/images/01bigclose.gif\" name=\"close\" align=\"right\"></td></tr></table></TD>\n",
		    tbw,hdr_bgnd,is_dialog_style?1:3,tbw,icon,txtcolor,title);
		htrAddBodyItem(s,   "    <TD width=\"1\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"22\"></TD></TR>\n");
		}

	    /** This is the beveled-down edge below the top of the window **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem(s,   "<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%POS\" height=\"1\"></TD>\n",w-3);
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		}
	    else
		{
		if (has_titlebar)
		    {
		    htrAddBodyItem(s,   "<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		    htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%POS\" height=\"1\"></TD></TR>\n",w-1);
		    htrAddBodyItem_va(s,"<TR><TD colspan=\"2\"><IMG src=\"/sys/images/white_1x1.png\" width=\"%POS\" height=\"1\"></TD>\n",w-1);
		    htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		    }
		}

	    /** This is the left side of the window. **/
	    htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"%POS\"></TD>\n", bh);
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"%POS\"></TD>\n",bh);
		}

	    /** Here's where the content goes... **/
	    htrAddBodyItem(s,"    <TD></TD>\n");

	    /** Right edge of the window **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"%POS\"></TD>\n",bh);
		}
	    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"%POS\"></TD></TR>\n",bh);

	    /** And... bottom edge of the window. **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem(s,   "<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/white_1x1.png\" width=\"%POS\" height=\"1\"></TD>\n",w-3);
		htrAddBodyItem(s,   "    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		}
	    htrAddBodyItem_va(s,"<TR><TD colspan=\"5\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%POS\" height=\"1\"></TD></TR>\n",w);
	    htrAddBodyItem(s,"</TABLE>\n");

	    htrAddBodyItem_va(s,"<DIV ID=\"wn%POSmain\"><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"%POS\" height=\"%POS\" %STR><tr><td>&nbsp;</td></tr></table>\n",id,bw,bh,bgnd);
	    }

	/** Check for more sub-widgets within the page. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    htrRenderWidget(s, sub_tree, z+2);
	    }

	htrAddBodyItem(s,"</DIV></DIV>\n");

    return 0;
    }


/*** htwinInitialize - register with the ht_render module.
 ***/
int
htwinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Child Window Widget Driver");
	strcpy(drv->WidgetName,"childwindow");
	drv->Render = htwinRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Add the 'set visibility' action **/
	htrAddAction(drv,"ToggleVisibility");
	htrAddAction(drv,"SetVisibility");
	htrAddParam(drv,"SetVisibility","IsVisible",DATA_T_INTEGER);
	htrAddParam(drv,"SetVisibility","NoInit",DATA_T_INTEGER);

	/** Add the 'window closed' event **/
	htrAddEvent(drv,"Close");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTWIN.idcnt = 0;

    return 0;
    }
