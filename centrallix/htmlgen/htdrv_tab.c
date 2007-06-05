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

    $Id: htdrv_tab.c,v 1.34 2007/06/05 22:07:32 dkasper Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_tab.c,v $

    $Log: htdrv_tab.c,v $
    Revision 1.34  2007/06/05 22:07:32  dkasper
    - Added the visible property so that it can be watched for changes allowing
      dynamic tab hiding/showing

    Revision 1.33  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.32  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.31  2006/10/16 18:34:34  gbeeley
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

    Revision 1.30  2005/10/09 07:49:30  gbeeley
    - (feature) allow tab control to be configured to not display the tabs
      at all.

    Revision 1.29  2005/06/23 22:08:00  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.28  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.27  2004/08/13 18:46:13  mmcgill
    *   Differentiated between non-visual widgets and widgets without associated
        objects during the rendering process. Widgets without associated objects
        on the client-side (no layers) have an object created for them and are
        included in the tree. This was not previously the case (for example,
        widget/table-columns were not previously included in the client-side tree.
        Now, they are.)
    *   Added code in ht_render to initiate the process of including interface
        information on the client-side.
    *   Modified htdrivers to flag widgets as HT_WGTF_NOOBJECT when appropriate.
    *   Modified wgtdrivers to flag widgets as WGTR_F_NONVISUAL when appropriate.
    *   Fixed bug in tab widget
    *   Added 'fieldname' property to widget/table-column (SEE NOTE BELOW)
    *   Added support for sending interface definitions to the client dynamically,
        and for including them statically in an application at render time
    *   Added a parameter to wgtrNewNode, and added wgtrImplementsInterface()
    *   Unique widget names are now *required* within an application (SEE NOTE)

    NOTE: THIS UPDATE WILL BREAK YOUR APPLICATIONS.

    The applications in the
    centrallix-os package have been updated to work with the noted changes. Any
    applications you may have written that aren't in that module are probably
    broken now for one of two reasons:
        1) Not all widgets are uniquely named within an application
        2) 'fieldname' is not specified for a widget/table-column
    These are now requirements. Update your applications accordingly. Also note
    that each widget will now receive a global variable named after that widget
    on the client side - don't pick widget names that might collide with already-
    existing globals.

    Revision 1.26  2004/08/04 20:03:10  mmcgill
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

    Revision 1.25  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.24  2004/08/02 14:09:34  mmcgill
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

    Revision 1.23  2004/07/20 21:28:52  mmcgill
    *   ht_render
        -   Added code to perform verification of widget-tree prior to
            rendering.
        -   Added concept of 'pseudo-types' for widget-drivers, e.g. the
            table driver getting called for 'table-column' widgets. This is
            necessary now since the 'table-column' entry in an app file will
            actually get put into its own widget node. Pseudo-type names
            are stored in an XArray in the driver struct during the
            xxxInitialize() function of the driver, and BEFORE ANY CALLS TO
            htrAddSupport().
        -   Added htrLookupDriver() to encapsulate the process of looking up
            a driver given an HtSession and widget type
        -   Added 'pWgtrVerifySession VerifySession' to HtSession.
            WgtrVerifySession represents a 'verification context' to be used
            by the xxxVerify functions in the widget drivers to schedule new
            widgets for verification, and otherwise interact with the
            verification system.
    *   xxxVerify() functions now take a pHtSession parameter.
    *   Updated the dropdown, tab, and table widgets to register their
        pseudo-types
    *   Moved the ObjProperty out of obj.h and into wgtr.c to internalize it,
        in anticipation of converting the Wgtr module to use PTODs instead.
    *   Fixed some Wgtr module memory-leak issues
    *   Added functions wgtrScheduleVerify() and wgtrCancelVerify(). They are
        to be used in the xxxVerify() functions when a node has been
        dynamically added to the widget tree during tree verification.
    *   Added the formbar widget driver, as a demonstration of how to modify
        the widget-tree during the verification process. The formbar widget
        doesn't actually do anything during the rendering process excpet
        call htrRenderWidget on its subwidgets, but during Verify it adds
        all the widgets necessary to reproduce the 'form control pane' from
        ors.app. This will eventually be done even more efficiently with
        component widgets - this serves as a tech test.

    Revision 1.22  2004/07/19 15:30:40  mmcgill
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

    Revision 1.21  2004/04/29 16:26:43  gbeeley
    - Fixes to get FourTabs.app working again in NS4/Moz, and in IE5.5/IE6.
    - Added inline-include feature to help with debugging in IE, which does
      not specify the correct file in its errors.  To use it, just append
      "?ls__collapse_includes=yes" to your .app URL.

    Revision 1.20  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.19  2003/12/01 19:04:40  gbeeley
    - fixed error in drawing tabs which was causing a minor visual issue
      when tabs are on the righthand side of the tab control.

    Revision 1.18  2003/11/30 02:09:40  gbeeley
    - adding autoquery modes to OSRC (never, onload, onfirstreveal, or
      oneachreveal)
    - adding serialized loader queue for preventing communcations with the
      server from interfering with each other (netscape bug)
    - pg_debug() writes to a "debug:" dynamic html widget via AddText()
    - obscure/reveal subsystem initial implementation

    Revision 1.17  2003/11/18 05:59:40  gbeeley
    - mozilla support
    - inactive tab background image/colors
    - 'hot properties' selected and selected_index for changing current tab

    Revision 1.16  2003/11/15 19:59:57  gbeeley
    Adding support for tabs on any of 4 sides of tab control

    Revision 1.15  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.14  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.13  2002/08/13 01:43:56  gbeeley
    Updating htdrv_tab to use the new OSML API for objGetAttrValue().

    Revision 1.12  2002/07/30 19:04:45  lkehresman
    * Added standard events to tab widget
    * Converted tab widget to use standard mainlayer and layer properties

    Revision 1.11  2002/07/23 15:22:39  mcancel
    Changing htrAddHeaderItem to htrAddStylesheetItem for a couple of files
    that missed the API change - for adding style sheet definitions.

    Revision 1.10  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.9  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.8  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.7  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2001/10/23 02:21:09  gbeeley
    Fixed incorrect auto-setting of tabpage clip height.

    Revision 1.2  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

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
    HTTAB;


enum httab_locations { Top=0, Bottom=1, Left=2, Right=3, None=4 };


/*** httabRender - generate the HTML code for the page.
 ***/
int
httabRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char tab_txt[128];
    char main_bg[128];
    char inactive_bg[128];
    char sel[128];
    int sel_idx= -1;
    pWgtrNode tabpage_obj;
    int x=-1,y=-1,w,h;
    int id,tabcnt, i, j;
    char* subnptr;
    enum httab_locations tloc;
    int tab_width = 0;
    int xoffset,yoffset,xtoffset, ytoffset;
    int is_selected;
    char* bg;
    char* tabname;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE &&(!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTAB","NS4 or W3C DOM Support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTAB.idcnt++);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'height' property");
	    return -1;
	    }

	/** Which side are the tabs on? **/
	if (wgtrGetPropertyValue(tree,"tab_location",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"top")) tloc = Top;
	    else if (!strcasecmp(ptr,"bottom")) tloc = Bottom;
	    else if (!strcasecmp(ptr,"left")) tloc = Left;
	    else if (!strcasecmp(ptr,"right")) tloc = Right;
	    else if (!strcasecmp(ptr,"none")) tloc = None;
	    else
		{
		mssError(1,"HTTAB","%s: '%s' is not a valid tab_location",name,ptr);
		return -1;
		}
	    }
	else
	    {
	    tloc = Top;
	    }

	/** How wide should left/right tabs be? **/
	if (wgtrGetPropertyValue(tree,"tab_width",DATA_T_INTEGER,POD(&tab_width)) != 0)
	    {
	    if (tloc == Right || tloc == Left)
		{
		mssError(1,"HTTAB","%s: tab_width must be specified with tab_location of left or right", name);
		return -1;
		}
	    }
	else
	    {
	    if (tab_width < 0) tab_width = 0;
	    }

	/** Which tab is selected? **/
	if (wgtrGetPropertyType(tree,"selected") == DATA_T_STRING &&
		wgtrGetPropertyValue(tree,"selected",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(sel,ptr, sizeof(sel));
	    }
	else
	    {
	    strcpy(sel,"");
	    }
	if (wgtrGetPropertyValue(tree,"selected_index", DATA_T_INTEGER, POD(&sel_idx)) != 0)
	    {
	    sel_idx = -1;
	    }
	if (sel_idx != -1 && *sel != '\0')
	    {
	    mssError(1,"HTTAB","%s: cannot specify both 'selected' and 'selected_index'", name);
	    return -1;
	    }

	/** User requesting expression for selected tab? **/
	htrCheckAddExpression(s, tree, name, "selected");

	/** User requesting expression for selected tab using integer index value? **/
	htrCheckAddExpression(s, tree, name, "selected_index");

	/** Background color/image? **/
	htrGetBackground(tree, NULL, s->Capabilities.Dom2CSS, main_bg, sizeof(main_bg));

	/** Inactive tab color/image? **/
	htrGetBackground(tree, "inactive", s->Capabilities.Dom2CSS, inactive_bg, sizeof(inactive_bg));

	/** Text color? **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(tab_txt, ptr, sizeof(tab_txt));
	else
	    strcpy(tab_txt,"black");

	/** Determine offset to actual tab pages **/
	switch(tloc)
	    {
	    case Top:    xoffset = 0;         yoffset = 24; xtoffset = 0; ytoffset = 0; break;
	    case Bottom: xoffset = 0;         yoffset = 0;  xtoffset = 0; ytoffset = h; break;
	    case Right:  xoffset = 0;         yoffset = 0;  xtoffset = w; ytoffset = 0; break;
	    case Left:   xoffset = tab_width; yoffset = 0;  xtoffset = 0; ytoffset = 0; break;
	    case None:   xoffset = 0;         yoffset = 0;  xtoffset = 0; ytoffset = 0;
	    }

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
	    htrAddStylesheetItem_va(s,"\t#tc%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x+xoffset,y+yoffset,w,z+1);
	else if (s->Capabilities.Dom2CSS)
	    htrAddStylesheetItem_va(s,"\t#tc%POSbase { %STR }\n", id, main_bg);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tc%POSbase\")",id);
	htrAddWgtrCtrLinkage(s, tree, "_obj");

	/** Script include **/
	htrAddScriptInclude(s, "/sys/js/htdrv_tab.js", 0);

	/** Add a global for the master tabs listing **/
	htrAddScriptGlobal(s, "tc_tabs", "null", 0);
	htrAddScriptGlobal(s, "tc_cur_mainlayer", "null", 0);

	/** Event handler for click-on-tab **/
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","tc","tc_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tc","tc_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","tc","tc_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER","tc","tc_mouseover");

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    tc_init({layer:nodes[\"%STR&SYM\"], tloc:%INT, mainBackground:\"%STR&ESCQ\", inactiveBackground:\"%STR&ESCQ\"});\n",
		name, tloc, main_bg, inactive_bg);

	/** Check for tabpages within the tab control, to do the tabs at the top. **/
	if (tloc != None)
	    {
	    tabcnt = 0;
	    for (i=0;i<xaCount(&(tree->Children));i++)
		{
		tabpage_obj = xaGetItem(&(tree->Children), i);
		wgtrGetPropertyValue(tabpage_obj,"outer_type",DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"widget/tabpage"))
		    {
		    wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
		    tabcnt++;
		    is_selected = (tabcnt == sel_idx || (!*sel && tabcnt == 1) || !strcmp(sel,ptr));
		    bg = is_selected?main_bg:inactive_bg;
		    if (wgtrGetPropertyValue(tabpage_obj,"title",DATA_T_STRING,POD(&tabname)) != 0)
			wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&tabname));

		    /** Add stylesheet headers for the layers (tab and tabpage) **/
		    if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
			{
			htrAddStylesheetItem_va(s,"\t#tc%POStab%POS { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; Z-INDEX:%POS; }\n",
				id,tabcnt,x+xtoffset,y+ytoffset,is_selected?(z+2):z);
			}

		    /** Generate the tabs along the edge of the control **/
		    if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
			{
			htrAddBodyItem_va(s,"<DIV ID=\"tc%POStab%POS\" %STR>\n",id,tabcnt,bg);
			if (tab_width == 0)
			    htrAddBodyItem(s,   "    <TABLE cellspacing=0 cellpadding=0 border=0>\n");
			else
			    htrAddBodyItem_va(s,"    <TABLE cellspacing=0 cellpadding=0 border=0 width=%POS>\n", tab_width);
			if (tloc != Bottom)
			    htrAddBodyItem_va(s,"        <TR><TD colspan=%POS background=/sys/images/white_1x1.png><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n", (tloc == Top || tloc == Bottom)?3:2);
			htrAddBodyItem(s,   "        <TR>");
			if (tloc != Right)
			    {
			    htrAddBodyItem(s,"<TD width=6><IMG SRC=/sys/images/white_1x1.png height=24 width=1>");
			    if (is_selected)
				htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft2.gif name=tb height=24></TD>\n");
			    else
				htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft3.gif name=tb height=24></TD>\n");
			    }
			htrAddBodyItem_va(s,"            <TD valign=middle align=center><FONT COLOR=%STR&HTE><b>&nbsp;%STR&HTE&nbsp;</b></FONT></TD>\n", tab_txt, tabname);
			if (tloc != Left && tloc != Right)
			    htrAddBodyItem(s,"           <TD align=right>");
			if (tloc == Right)
			    {
			    htrAddBodyItem(s,"           <TD align=right width=6>");
			    if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
				htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft2.gif name=tb height=24>");
			    else
				htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft3.gif name=tb height=24>");
			    }
			if (tloc != Left)
			    htrAddBodyItem(s,"<IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=24></TD>");
			htrAddBodyItem(s,"</TR>\n");
			if (tloc != Top)
			    htrAddBodyItem_va(s,"        <TR><TD colspan=%POS background=/sys/images/dkgrey_1x1.png><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n", (tloc == Top || tloc == Bottom)?3:2);
			htrAddBodyItem(s,"    </TABLE>\n");
			htrAddBodyItem(s, "</DIV>\n");
			}
		    else if (s->Capabilities.Dom2CSS)
			{
			htrAddStylesheetItem_va(s, "\t#tc%POStab%POS { %STR }\n",
				id, tabcnt, bg);
			if (tab_width <= 0)
			    htrAddBodyItem_va(s, "<div id=\"tc%POStab%POS\" style=\"position:absolute; visibility:inherit; left:%INTpx; top:%INTpx; overflow:hidden; z-index:%POS; \">\n", id, tabcnt, x+xtoffset, y+ytoffset, is_selected?(z+2):z);
			else
			    htrAddBodyItem_va(s, "<div id=\"tc%POStab%POS\" style=\"position:absolute; visibility:inherit; left:%INTpx; top:%INTpx; width:%POSpx; overflow:hidden; z-index:%POS; \">\n", id, tabcnt, x+xtoffset, y+ytoffset, tab_width, is_selected?(z+2):z);
			if (tloc != Right)
			    {
			    if (tab_width <= 0)
				htrAddBodyItem_va(s, "    <table style=\"border-style:solid; border-width: %POSpx %POSpx %POSpx %POSpx; border-color: white gray gray white;\" border=0 cellspacing=0 cellpadding=0><tr><td><img align=left src=/sys/images/tab_lft%POS.gif width=5 height=24></td><td align=center><b>&nbsp;%STR&HTE&nbsp;</b></td></tr></table>\n",
				    (tloc!=Bottom)?1:0, (tloc!=Left)?1:0, (tloc!=Top)?1:0, (tloc!=Right)?1:0, is_selected?2:3, tabname);
			    else
				htrAddBodyItem_va(s, "    <table width=%POS style=\"border-style:solid; border-width: %POSpx %POSpx %POSpx %POSpx; border-color: white gray gray white;\" border=0 cellspacing=0 cellpadding=0><tr><td><img align=left src=/sys/images/tab_lft%POS.gif width=5 height=24></td><td align=center><b>&nbsp;%STR&HTE&nbsp;</b></td></tr></table>\n",
				    tab_width, (tloc!=Bottom)?1:0, (tloc!=Left)?1:0, (tloc!=Top)?1:0, (tloc!=Right)?1:0, is_selected?2:3, tabname);
			    }
			else
			    {
			    htrAddBodyItem_va(s, "    <table style=\"border-style:solid; border-width: 1px 1px 1px 0px; border-color: white gray gray white;\" width=%POS border=0 cellspacing=0 cellpadding=0><tr><td valign=middle align=center><b>&nbsp;%STR&HTE&nbsp;</b></td><td><img src=/sys/images/tab_lft%POS.gif align=right width=5 height=24></td></tr></table>\n",
				    tab_width, tabname, is_selected?2:3);
			    }
			htrAddBodyItem(s, "</div>\n");
			}
		    }
		}
	    }

	/** HTML body <DIV> element for the base layer. **/
	if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"tc%POSbase\">\n",id);
	    htrAddBodyItem_va(s,"    <TABLE width=%POS cellspacing=0 cellpadding=0 border=0 %STR>\n",w,main_bg);
	    htrAddBodyItem(s,   "        <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%POS></TD>\n",w-2);
	    htrAddBodyItem(s,   "            <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	    htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%POS width=1></TD>\n",h-2);
	    htrAddBodyItem(s,   "            <TD>&nbsp;</TD>\n");
	    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%POS width=1></TD></TR>\n",h-2);
	    htrAddBodyItem(s,   "        <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	    htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%POS></TD>\n",w-2);
	    htrAddBodyItem(s,   "            <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n    </TABLE>\n\n");
	    }
	else
	    {
	    /** h-2 and w-2 because w3c dom borders add to actual width **/
	    htrAddBodyItem_va(s,"<div id=\"tc%POSbase\" style=\"position:absolute; overflow:hidden; height:%POSpx; width:%POSpx; left:%INTpx; top:%INTpx; z-index:%POS;\" ><table cellspacing=0 cellpadding=0 style=\"border-width: 1px; border-style:solid; border-color: white gray gray white;\"><tr><td height=%POS width=%POS>&nbsp;</td></tr></table>\n",
		    id, h, w, x+xoffset, y+yoffset, z+1,
		    h-2,w-2);
	    }

	/** Check for tabpages within the tab control entity, this time to do the pages themselves **/
	tabcnt = 0;
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    tabpage_obj = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(tabpage_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/tabpage"))
		{
		/** First, render the tabpage and add stuff for it **/
		wgtrGetPropertyValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
		tabcnt++;
		is_selected = (tabcnt == sel_idx || (!*sel && tabcnt == 1) || !strcmp(sel,ptr));

		/** Add the pane **/
		if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
		    {
		    htrAddStylesheetItem_va(s,"\t#tc%POSpane%POS { POSITION:absolute; VISIBILITY:%STR; LEFT:1px; TOP:1px; WIDTH:%POSpx; Z-INDEX:%POS; }\n",
			    id,tabcnt,is_selected?"inherit":"hidden",w-2,z+2);
		    }

		/** Add script initialization to add a new tabpage **/
		if (tloc == None)
		    htrAddScriptInit_va(s,"    nodes[\"%STR&SYM\"].addTab(null,wgtrGetContainer(nodes[\"%STR&SYM\"]),nodes[\"%STR&SYM\"],'%STR&ESCQ');\n",
			name, ptr, name, ptr);
		else
		    htrAddScriptInit_va(s,"    nodes[\"%STR&SYM\"].addTab(nodes[\"%STR&SYM\"],wgtrGetContainer(nodes[\"%STR&SYM\"]),nodes[\"%STR&SYM\"],'%STR&ESCQ');\n",
			name, ptr, ptr, name, ptr);

		/** Add named global for the tabpage **/
		subnptr = nmSysStrdup(ptr);
		if (tloc == None)
		    htrAddWgtrObjLinkage_va(s, tabpage_obj, "htr_subel(_parentctr, \"tc%POSpane%POS\")", id, tabcnt);
		else
		    htrAddWgtrObjLinkage_va(s, tabpage_obj, "htr_subel(wgtrGetContainer(wgtrGetParent(_parentobj)), \"tc%POStab%POS\")", id, tabcnt);
		htrAddWgtrCtrLinkage_va(s, tabpage_obj, "htr_subel(_parentobj, \"tc%POSpane%POS\")", id, tabcnt);

		/** Add DIV section for the tabpage. **/
		if (s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
		    htrAddBodyItem_va(s,"<DIV ID=\"tc%POSpane%POS\">\n",id,tabcnt);
		else
		    htrAddBodyItem_va(s,"<div id=\"tc%POSpane%POS\" style=\"POSITION:absolute; VISIBILITY:%STR; LEFT:1px; TOP:1px; WIDTH:%POSpx; Z-INDEX:%POS;\">\n",
			    id,tabcnt,is_selected?"inherit":"hidden",w-2,z+2);

		/** Now look for sub-items within the tabpage. **/
		for (j=0;j<xaCount(&(tabpage_obj->Children));j++)
		    htrRenderWidget(s, xaGetItem(&(tabpage_obj->Children), j), z+3);

		htrAddBodyItem(s, "</DIV>\n");

		nmSysFree(subnptr);
		/** Add the visible property **/
		htrCheckAddExpression(s, tabpage_obj, ptr, "visible");
		}
	    else if (!strcmp(ptr,"widget/connector"))
		{
		htrRenderWidget(s, tabpage_obj, z+2);
		}
	    }

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** httabInitialize - register with the ht_render module.
 ***/
int
httabInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Tab Control / Tab Page Driver");
	strcpy(drv->WidgetName,"tab");
	drv->Render = httabRender;
	xaAddItem(&(drv->PseudoTypes), "tabpage");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTAB.idcnt = 0;

    return 0;
    }
