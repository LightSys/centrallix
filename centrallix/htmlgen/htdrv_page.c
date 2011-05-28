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
#include "centrallix.h"
#include "wgtr.h"
#include "iface.h"
#include "stparse.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_page.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Widget driver for the overall HTML page.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_page.c,v 1.87 2010/09/09 01:11:34 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_page.c,v $

    $Log: htdrv_page.c,v $
    Revision 1.87  2010/09/09 01:11:34  gbeeley
    - (feature) adding application and session global variables, and
      intelligent linking of windows which opened each other so that they
      can share data/widgets in the session/application global namespaces

    Revision 1.86  2009/06/25 19:55:30  gbeeley
    - (change) adding a linkcolor setting on the page.
    - (change) swap the highlight colors used for mouse and keyboard focus
      so kbd focus is a black rectange and mouse focus is a raised border.
    - (bugfix) prevent scrollbars from appearing on the main document.

    Revision 1.85  2008/08/16 00:31:37  thr4wn
    I made some more modification of documentation and begun logic for
    caching generated WgtrNode instances (see centrallix-sysdoc/misc.txt)

    Revision 1.84  2008/07/16 00:34:57  thr4wn
    Added a bunch of documentation in different README files. Also added documentation in certain parts of the code itself.

    Revision 1.83  2008/03/04 01:10:57  gbeeley
    - (security) changing from ESCQ to JSSTR in numerous places where
      building JavaScript strings, to avoid such things as </script>
      in the string from having special meaning.  Also began using the
      new CSSVAL and CSSURL in places (see qprintf).
    - (performance) allow the omission of certain widgets from the rendered
      page.  In particular, omitting most widget/parameter's significantly
      reduces the total widget count.
    - (performance) omit double-buffering in edit boxes for Firefox/Mozilla,
      which reduces the <div> count for the page significantly.
    - (bugfix) allow setting text color on tabs in mozilla/firefox.

    Revision 1.82  2007/09/18 17:42:54  gbeeley
    - (change) allow font size to be specified on page and label, and do font
      sizing in CSS px instead of using the old 1...7 HTML approach.

    Revision 1.81  2007/07/25 16:55:57  gbeeley
    - (bugfix) try to keep keystrokes from getting jumbled up by deleting any
      scheduled key repeat callback as needed.

    Revision 1.80  2007/06/12 15:05:35  gbeeley
    - (feature) if cx__obscure=yes is included in the URL as a param, the
      system will automatically randomize alphanumeric text in editboxes,
      tables, and textareas, so that a running system with confidential
      data can be demo'd

    Revision 1.79  2007/06/06 15:20:09  gbeeley
    - (feature) pass templates on to components, etc.

    Revision 1.78  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.77  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.76  2007/03/06 16:12:04  gbeeley
    - (feature) tooltip capability.

    Revision 1.75  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.74  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.73  2006/10/16 18:34:34  gbeeley
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

    Revision 1.72  2006/04/07 06:34:13  gbeeley
    - (change) adding show_diagnostics attribute to page widget to allow
      wgtr et al warning messages to be un-suppressed

    Revision 1.71  2005/10/09 07:48:54  gbeeley
    - (change) allow contextmenu event to trigger RightClick
    - fix image-in-table problem on Moz

    Revision 1.70  2005/03/01 07:08:26  gbeeley
    - don't activate the serialized loading until after startup() finishes
      running.

    Revision 1.69  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.68  2004/08/18 04:54:25  gbeeley
    - proper keyboard input in IE6 and Moz (including handling the keydown vs.
      keypress event issue).
    - editbox appearance/functionality now relatively consistent across IE6,
      Moz, and NS4.

    Revision 1.67  2004/08/04 20:03:09  mmcgill
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

    Revision 1.66  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.65  2004/08/02 14:09:34  mmcgill
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

    Revision 1.64  2004/07/19 15:30:40  mmcgill
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

    Revision 1.63  2004/06/12 03:57:56  gbeeley
    - mechanism to receive control messages from the server.  For NS4, it has
      to use a polled approach.  Not sure if Moz or IE will be better in this
      area.

    Revision 1.62  2004/03/11 23:12:53  jasonyip

    Added IE browser check.

    Revision 1.61  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.60  2004/02/24 20:21:57  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.59  2003/11/30 02:09:40  gbeeley
    - adding autoquery modes to OSRC (never, onload, onfirstreveal, or
      oneachreveal)
    - adding serialized loader queue for preventing communcations with the
      server from interfering with each other (netscape bug)
    - pg_debug() writes to a "debug:" dynamic html widget via AddText()
    - obscure/reveal subsystem initial implementation

    Revision 1.58  2003/11/12 22:15:56  gbeeley
    Formal Launch action declaration

    Revision 1.57  2003/07/20 03:41:17  jorupp
     * got window mostly working in Mozilla

    Revision 1.56  2003/07/15 01:57:51  gbeeley
    Adding an independent DHTML scrollbar widget that will be used to
    control scrolling/etc on other widgets.

    Revision 1.55  2003/06/21 23:54:41  jorupp
     * fixex up a few problems I found with the version I committed (like compilation...)
     * removed some code that was commented out

    Revision 1.54  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.53  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.52  2002/12/24 09:41:07  jorupp
     * move output of cn_browser to ht_render, also moving up above the first place where it is needed

    Revision 1.51  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.50  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.49  2002/08/23 17:31:05  lkehresman
    moved window_current global to the page widget so it is always defined
    even if it is null.  This prevents javascript errors from the objectsource
    widget when no windows exist in the app.

    Revision 1.48  2002/08/15 13:58:16  pfinley
    Made graphical window closing and shading properties of the window widget,
    rather than globally of the page.

    Revision 1.47  2002/08/14 20:16:38  pfinley
    Added some visual effets for the window:
     - graphical window shading (enable by setting gshade="true")
     - added 3 new closing types (enable by setting closetype="shrink1","shrink2", or "shrink3")

    These are gloabl changes, and can only be set on the page widget... these
    will become part of theming once it is implemented (i think).

    Revision 1.46  2002/08/13 05:23:25  gbeeley
    I should have documented this one from the beginning.  The sense of the
    'flags' parameter ended up being changed, breaking the static mode table.
    Changed the implementation back to using the original sense of this
    param (bitmask 1 = allow mouse focus, 2 = allow kbd focus).  Tested on
    static table, dropdown, datetime, textarea, editbox.

    Revision 1.45  2002/08/12 17:51:16  pfinley
    - added an attract option to the page widget. if this is set, centrallix
      windows will attract to the edges of the browser window. set to how many
      pixels from border to attract.
    - also fixed a mainlayer issue with the window widget which allowed for
      the contents of a window to be draged and shaded (very interesting :)

    Revision 1.44  2002/08/08 16:23:07  lkehresman
    Added backwards compatible page area handling because the datetime widget
    needs to do some funky stuff with page areas (areas inside of areas) that
    worked with our previous model, but not with the mainlayer stuff.  Just
    added a few checks to see if mainlayer existed or not.

    Revision 1.43  2002/08/05 21:06:29  pfinley
    created a property that can optionally be added to layers that will keep
    the current keyboard focus if it is clicked on. I added this property so
    that keyboard focus would not be lost when you move a window. add the
    property '.keep_kbd_focus = true' to any layer that you want to keep
    the keyboard focus on (any layer without this property will act normally).

    Revision 1.42  2002/08/05 19:20:08  lkehresman
    * Revamped the GUI for the DropDown to make it look cleaner
    * Added the function pg_resize_area() so page areas can be resized.  This
      allows the dropdown and datetime widgets to contain focus on their layer
      that extends down.  Previously it was very kludgy, now it works nicely
      by just extending the page area for that widget.
    * Reworked the dropdown widget to take advantage of the resize function
    * Added .mainlayer attributes to the editbox widget (new page functionaly
      requires .mainlayer properties soon to be standard in all widgets).

    Revision 1.41  2002/07/31 13:35:58  lkehresman
    * Made x.mainlayer always point to the top layer in dropdown
    * Fixed a netscape crash bug with the event stuff from the last revision of dropdown
    * Added a check to the page event stuff to make sure that pg_curkbdlayer is set
        before accessing the pg_curkbdlayer.getfocushandler() function. (was causing
        javascript errors before because of the special case of the dropdown widget)

    Revision 1.40  2002/07/30 16:09:05  pfinley
    Added Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove events to the
    radiobutton widget.

    Note: MouseOut does not use the MOUSEOUT event.  It had to be done manually
          since MouseOut's on the whole widget can not be distingushed if a widget
          has more than one layer visible at one time. For this I added a global
          variable: util_cur_mainlayer which is set to the mainlayer of the widget
          (the layer that the event connector functions are attached to) on a
          MouseOver event.  In order to use this in other widgets, each layer in
          the widget must:  1) have a .kind property, 2) have a .mainlayer property,
          3) set util_cur_mainlayer to the mainlayer upon a MouseOver event on the
          widget, and 4) set util_cur_mainlayer back to null upon a "MouseOut"
          event. (See code for example)

    I also:
    - changed the enabled/disabled to be properties of the mainlayer rather
      than of each layer in the widget.
    - changed toggle function to only toggle if selecting a new option (eliminated
      flicker and was needed for DataChange event).
    - removed the .parentPane pointer and replaced it with mainlayer.
    [ how about that for a log message :) ]

    Revision 1.39  2002/07/30 12:56:29  lkehresman
    Renamed the "Load" action and the "Page" parameter to be "LoadPage" and
    "Source", which is what is used in the "html" widget.  We should probably
    keep actions as similar and standard as possible.

    Revision 1.38  2002/07/29 18:21:52  pfinley
    Changed a static while loop conditional to be an if statement.  This was
    causing an infinite loop when e.target pointed to an object.

    Revision 1.37  2002/07/26 16:57:44  lkehresman
    a couple bugfixes to the scrollbar detection

    Revision 1.36  2002/07/26 14:42:04  lkehresman
    Added detection for scrollbars.  If they exist, set a timeout function
    to watch for scrolling activity.  If scroling happens, move the textarea
    layer (that we use to gain keypress input).

    Revision 1.35  2002/07/24 21:26:35  pfinley
    this is needed incase a page does not have any connectors.

    Revision 1.34  2002/07/23 13:45:32  lkehresman
    Added the "Load" action with the "Page" parameter to the page widget.  This
    enables us to load different applications with button clicks or other events.
    This will be useful for a program that spans several different app structure
    files. (i.e. Kardia)

    Revision 1.33  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.32  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.31  2002/07/19 14:54:22  pfinley
    - Modified the page mousedown & mouseover handlers so that the cursor ibeam
    "can't" be clicked on (only supports the global cursor).
    - Modified the editbox & textarea files to support the new global cursor.

    Revision 1.30  2002/07/18 20:12:40  lkehresman
    Added support for a loadstatus icon to be displayed, hiding the drawing
    of the visible windows.  This looks MUCH nicer when loading Kardia or
    any other large apps.  It is completely optional part of the page widget.
    To take advantage of it, put the parameter "loadstatus" equal to "true"
    in the page widget.

    Revision 1.29  2002/07/18 14:27:25  pfinley
    fixed another bug i created yesterday. cleaned up code a bit.

    Revision 1.28  2002/07/17 20:09:12  lkehresman
    Two changes to htdrv_page event handlers
      *  getfocushandler() function now is passed a reference to the current
         pg_area object as the 5th parameter.
      *  losefocushandler() is now an optional function (like the
         getfocushandler function is).  I think it was supposed to work like
         this originally, but someone forgot to put the check in there.

    Revision 1.27  2002/07/17 19:45:44  pfinley
    fixed a javascript error that i created

    Revision 1.26  2002/07/17 18:59:57  pfinley
    The flag parameter (f) of pg_addarea() was not being used, so I put it to use. If
    the flag is set to 0, it will not draw the box around the area when it has mouse
    focus; any other value and it will.

    Revision 1.25  2002/07/17 15:32:17  pfinley
    Changed losefocushandler so that it is only called when another layer other than itself is clicked (getfocushandler is still called).

    Revision 1.24  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.23  2002/07/16 16:12:07  pfinley
    added a script init that accidentally got deleted when converting to a js file.

    Revision 1.22  2002/07/15 22:41:02  lkehresman
    Whoops!  Got copy-happy and removed an event with all the functions.

    Revision 1.21  2002/07/15 21:58:02  lkehresman
    Split the page out into include scripts

    Revision 1.20  2002/07/08 23:21:38  jorupp
     * added a global object, cn_browser with two boolean properties -- netscape47 and mozilla
        The corresponding one will be set to true by the page
     * made one minor change to the form to get around the one .layers reference in the form (no .document references)
        It _should_ work, however I don't have a _simple_ form test to try it on, so it'll have to wait

    Revision 1.19  2002/07/07 00:21:46  jorupp
     * added Mozilla support for the page
       * BARELY WORKS -- hardly any events checked
       * ping works (after several days of trying)

    Revision 1.18  2002/06/24 19:45:35  pfinley
    eliminated the flicker in the border of an edit box when it is clicked when it already has keyboard focus..

    Revision 1.17  2002/06/19 21:21:50  lkehresman
    Bumped up the zIndex values so the hilights wouldn't be hidden.

    Revision 1.16  2002/06/19 18:35:25  pfinley
    fixed bug in edit box which didn't remove the focus border when clicking a place other than another edit box.

    Revision 1.15  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.14  2002/06/03 05:31:39  lkehresman
    Fixed some global variables that should have been local.

    Revision 1.13  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.12  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.11  2002/05/08 00:46:30  jorupp
     * added client-side support for the session watchdog.  The client will ping every timer/2 seconds (just to be safe)

    Revision 1.10  2002/04/28 03:19:53  gbeeley
    Fixed a bit of a bug in ht_render where it did not properly set the
    length on the StrValue structures when adding script functions.  This
    was basically causing some substantial heap corruption.

    Revision 1.9  2002/03/23 01:18:09  lkehresman
    Fixed focus detection and form notification on editbox and anything that
    uses keyboard input.

    Revision 1.8  2002/03/16 06:53:34  gbeeley
    Added modal-layer function at the page level.  Calling pg_setmodal(l)
    causes all mouse activity outside of layer l to be ignored.  Useful
    when you need to require the user to act on a certain window/etc.
    Call pg_setmodal(null) to clear the modal status.  Note: keep the
    modal layer simple!  The algorithm does quite a bit of poking around
    to figure out whether the activity is targeted within the given modal
    layer or not.  NOTE:  for modal windows, if you want to keep the user
    from clicking the 'x' on the window, set the window's mainLayer to be
    modal instead of the window itself, although the user will not be able
    to even drag the window in that case, which might be desirable.

    Revision 1.7  2002/03/15 22:40:47  gbeeley
    Modified key input logic in the page widget to improve key debouncing
    and make key repeat rate and delay a bit more natural and more
    consistent across different machines and platforms.

    Revision 1.6  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.5  2002/02/27 01:38:51  jheth
    Initial commit of object source

    Revision 1.4  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


int
htpageRender(pHtSession s, pWgtrNode tree, int z)
    {
    char *ptr;
    char name[64];
    int attract = 0;
    int watchdogtimer;
    char bgstr[128];
    int show, i, count;
    char kbfocus1[64];	/* kb focus = 3d raised */
    char kbfocus2[64];
    char msfocus1[64];	/* ms focus = black rectangle */
    char msfocus2[64];
    char dtfocus1[64];	/* dt focus = navyblue rectangle */
    char dtfocus2[64];
    int font_size = 12;
    char font_name[128];
    int show_diag = 0;
    int w,h;
    char* path;
    pStruct c_param;

	if(!((s->Capabilities.Dom0NS || s->Capabilities.Dom0IE || (s->Capabilities.Dom1HTML && s->Capabilities.Dom2Events)) && s->Capabilities.CSS1) )
	    {
	    mssError(1,"HTPAGE","CSS Level 1 Support and (Netscape DOM support or (W3C Level 1 DOM support and W3C Level 2 Events support required))");
	    return -1;
	    }

	strcpy(msfocus1,"#ffffff");	/* ms focus = 3d raised */
	strcpy(msfocus2,"#7a7a7a");
	strcpy(kbfocus1,"#000000");	/* kb focus = black rectangle */
	strcpy(kbfocus2,"#000000");
	strcpy(dtfocus1,"#000080");	/* dt focus = navyblue rectangle */
	strcpy(dtfocus2,"#000080");

    	/** If not at top-level, don't render the page. **/
	/** Z is set to 10 for the top-level page. **/
	if (z != 10) return 0;

	/** These are always set for a page widget **/
	wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w));
	wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h));

    	/** Check for a title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s, "    <TITLE>%STR&HTE</TITLE>\n",ptr);
	    }

    	/** Check for page load status **/
	show = htrGetBoolean(tree, "loadstatus", 0);

	/** Initialize the html-related interface stuff **/
	if (ifcHtmlInit(s, tree) < 0)
	    {
	    mssError(0, "HTR", "Error Initializing Html Interface code...continuing, but things might not work for client");
	    }
	
	/** Auto-call startup and cleanup **/
	htrAddBodyParam_va(s, " onLoad=\"startup_%STR&SYM();\" onUnload=\"cleanup();\"",
		s->Namespace->DName);

	/** Check for bgcolor. **/
	if (htrGetBackground(tree, NULL, 0, bgstr, sizeof(bgstr)) == 0)
	    {
	    htrAddBodyParam_va(s, " %STR",bgstr);
	    }

	/** Check for text color **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " TEXT=\"%STR&HTE\"",ptr);
	    }

	/** Check for link color **/
	if (wgtrGetPropertyValue(tree,"linkcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " LINK=\"%STR&HTE\"",ptr);
	    }

	/** Keyboard Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"kbdfocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(kbfocus1,ptr,sizeof(kbfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"kbdfocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(kbfocus2,ptr,sizeof(kbfocus2));
	    }

	/** Mouse Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"mousefocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(msfocus1,ptr,sizeof(msfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"mousefocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(msfocus2,ptr,sizeof(msfocus2));
	    }

	/** Data Focus Indicator colors 1 and 2 **/
	if (wgtrGetPropertyValue(tree,"datafocus1",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(dtfocus1,ptr,sizeof(dtfocus1));
	    }
	if (wgtrGetPropertyValue(tree,"datafocus2",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(dtfocus2,ptr,sizeof(dtfocus2));
	    }

	/** Cx windows attract to browser edges? if so, by how much **/
	if (wgtrGetPropertyValue(tree,"attract",DATA_T_INTEGER,POD(&ptr)) == 0)
	    attract = (intptr_t)ptr;

	show_diag = htrGetBoolean(tree, "show_diagnostics", 0);

	wgtrGetPropertyValue(tree, "font_size", DATA_T_INTEGER, POD(&font_size));
	if (font_size < 5 || font_size > 100) font_size = 12;

	if (wgtrGetPropertyValue(tree, "font_name", DATA_T_STRING, POD(&ptr)) ==  0 && ptr)
	    strtcpy(font_name, ptr, sizeof(font_name));
	else
	    strcpy(font_name, "");

	/** Add global for page metadata **/
	htrAddScriptGlobal(s, "page", "new Object()", 0);

	/** Add a list of highlightable areas **/
	/** These are javascript global variables**/
	htrAddScriptGlobal(s, "pg_arealist", "[]", 0);
	htrAddScriptGlobal(s, "pg_keylist", "[]", 0);
	htrAddScriptGlobal(s, "pg_curarea", "null", 0);
	htrAddScriptGlobal(s, "pg_curlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdarea", "null", 0);
	htrAddScriptGlobal(s, "pg_lastkey", "-1", 0);
	htrAddScriptGlobal(s, "pg_lastmodifiers", "null", 0);
	htrAddScriptGlobal(s, "pg_keytimeoutid", "null", 0);
	htrAddScriptGlobal(s, "pg_keyschedid", "0", 0);
	htrAddScriptGlobal(s, "pg_modallayer", "null", 0);
	htrAddScriptGlobal(s, "pg_key_ie_shifted", "false", 0);
	htrAddScriptGlobal(s, "pg_attract", "null", 0);
	htrAddScriptGlobal(s, "pg_gshade", "null", 0);
	htrAddScriptGlobal(s, "pg_closetype", "null", 0);
	htrAddScriptGlobal(s, "pg_explist", "[]", 0);
	htrAddScriptGlobal(s, "pg_schedtimeout", "null", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutlist", "[]", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutid", "0", 0);
	htrAddScriptGlobal(s, "pg_schedtimeoutstamp", "0", 0);
	htrAddScriptGlobal(s, "pg_insame", "false", 0);
	htrAddScriptGlobal(s, "cn_browser", "null", 0);
	htrAddScriptGlobal(s, "ibeam_current", "null", 0);
	htrAddScriptGlobal(s, "util_cur_mainlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_loadqueue", "[]", 0);
	htrAddScriptGlobal(s, "pg_loadqueue_busy", "true", 0);
	htrAddScriptGlobal(s, "pg_debug_log", "null", 0);
	htrAddScriptGlobal(s, "pg_isloaded", "false", 0);
	htrAddScriptGlobal(s, "pg_username", "null", 0);
	htrAddScriptGlobal(s, "pg_msg_handlers", "[]", 0);
	htrAddScriptGlobal(s, "pg_msg_layer", "null", 0);
	htrAddScriptGlobal(s, "pg_msg_timeout", "null", 0);
	htrAddScriptGlobal(s, "pg_diag", show_diag?"true":"false", 0); /* causes pop-up boxes for certain non-fatel warnings */
	htrAddScriptGlobal(s, "pg_width", "0", 0);
	htrAddScriptGlobal(s, "pg_height", "0", 0);
	htrAddScriptGlobal(s, "pg_charw", "0", 0);
	htrAddScriptGlobal(s, "pg_charh", "0", 0);
	htrAddScriptGlobal(s, "pg_parah", "0", 0);
	htrAddScriptGlobal(s, "pg_namespaces", "{}", 0);
	htrAddScriptGlobal(s, "pg_handlertimeout", "null", 0); /* this is used by htr_mousemovehandler */
	htrAddScriptGlobal(s, "pg_mousemoveevents", "[]", 0);
	htrAddScriptGlobal(s, "pg_handlers", "[]", 0); /* keeps track of handlers for basic events tied to document (click, mousemove, keypress, etc) */
	htrAddScriptGlobal(s, "pg_capturedevents", "0", 0); /* is the binary OR of all event flags that the document currently has registered */
	htrAddScriptGlobal(s, "pg_tiplayer", "null", 0);
	htrAddScriptGlobal(s, "pg_tipindex", "0", 0);
	htrAddScriptGlobal(s, "pg_tiptmout", "null", 0);
	htrAddScriptGlobal(s, "pg_waitlyr", "null", 0);
	htrAddScriptGlobal(s, "pg_appglobals", "[]", 0);
	htrAddScriptGlobal(s, "pg_sessglobals", "[]", 0);
	htrAddScriptGlobal(s, "pg_scripts", "[]", 0);

	/** Add script include to get function declarations **/
	if(s->Capabilities.JS15 && s->Capabilities.Dom1HTML)
	    {
	    /*htrAddScriptInclude(s, "/sys/js/htdrv_page_js15.js", 0);*/
	    }
	htrAddScriptInclude(s, "/sys/js/htdrv_page.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Write named global **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	htrAddWgtrObjLinkage(s, tree, "window");
	htrAddWgtrCtrLinkage(s, tree, "document");

	/** Set the application key **/
	htrAddScriptInit_va(s, "    if (typeof window.akey == 'undefined') window.akey = '%STR&JSSTR';\n", s->ClientInfo->AKey);

	/** Page init **/
	htrAddScriptInit(s,    "    if(typeof(pg_status_init)=='function')pg_status_init();\n");
	htrAddScriptInit_va(s, "    pg_init(nodes['%STR&SYM'],%INT);\n", name, attract);
	htrAddScriptInit_va(s, "    pg_username = '%STR&JSSTR';\n", mssUserName());
	htrAddScriptInit_va(s, "    pg_width = %INT;\n", w);
	htrAddScriptInit_va(s, "    pg_height = %INT;\n", h);
	htrAddScriptInit_va(s, "    pg_charw = %INT;\n", s->ClientInfo->CharWidth);
	htrAddScriptInit_va(s, "    pg_charh = %INT;\n", s->ClientInfo->CharHeight);
	htrAddScriptInit_va(s, "    pg_parah = %INT;\n", s->ClientInfo->ParagraphHeight);

	c_param = stLookup_ne(s->Params, "cx__obscure");
	if (c_param && !strcasecmp(c_param->StrVal,"yes"))
	    htrAddScriptInit(s, "    obscure_data = true;\n");
	else
	    htrAddScriptInit(s, "    obscure_data = false;\n");

	/** Add template paths **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    {
	    if ((path = wgtrGetTemplatePath(tree, i)) != NULL)
		htrAddScriptInit_va(s, "    nodes['%STR&SYM'].templates.push('%STR&JSSTR');\n",
		    name, path);
	    }

	/** Shutdown **/
	htrAddScriptCleanup_va(s, "    pg_cleanup();\n");

	if(s->Capabilities.HTML40)
	    {
	    /** Add focus box **/
	    htrAddStylesheetItem(s,"\ttd img { display: block; }\n");
	    htrAddStylesheetItem(s,"\t#pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:-1000px;TOP:0px;WIDTH:1152px;HEIGHT:1px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:-1000px;TOP:0px;WIDTH:1152px;HEIGHT:1px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0px;TOP:-1000px;WIDTH:1px;HEIGHT:864px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0px;TOP:-1000px;WIDTH:1px;HEIGHT:864px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0px;TOP:0px;WIDTH:1px;HEIGHT:1px; Z-INDEX:0; }\n");
	    htrAddStylesheetItem(s,"\t#pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:-1000px;TOP:0px;WIDTH:1152px;HEIGHT:1px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:-1000px;TOP:0px;WIDTH:1152px;HEIGHT:1px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0px;TOP:-1000px;WIDTH:1px;HEIGHT:864px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0px;TOP:-1000px;WIDTH:1px;HEIGHT:864px; clip:rect(0px,0px,0px,0px); Z-INDEX:1000; overflow:hidden;}\n");
	    htrAddStylesheetItem(s,"\t#pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:20px; Z-INDEX:20; }\n");
	    htrAddStylesheetItem(s,"\t#pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:0px; HEIGHT:0px; Z-INDEX:0;}\n");
	    htrAddStylesheetItem(s,"\t#pgmsg { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:0px; HEIGHT:0px; Z-INDEX:0;}\n");
	    }
	else
	    {
	    /** Add focus box **/
	    htrAddStylesheetItem(s,
		    "\t#pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:1; Z-INDEX:0; }\n"
		    "\t#pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		    "\t#pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:20; Z-INDEX:20; }\n"
		    "\t#pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; Z-INDEX:0; }\n"
		    "\t#pgmsg { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; Z-INDEX:0; }\n");
	    }

	if (show == 1)
	    {
	    htrAddStylesheetItem(s, "\t#pgstat { POSITION:absolute; VISIBILITY:visible; LEFT:0;TOP:0;WIDTH:100%;HEIGHT:99%; Z-INDEX:100000;}\n");
	    htrAddBodyItemLayerStart(s,0,"pgstat",0);
	    htrAddBodyItem_va(s, "<BODY %STR>", bgstr);
	    htrAddBodyItem   (s, "<TABLE width=\"100\%\" height=\"100\%\" cellpadding=20><TR><TD valign=top><IMG src=\"/sys/images/loading.gif\"></TD></TR></TABLE></BODY>\n");
	    htrAddBodyItemLayerEnd(s,0);
	    }

	htrAddStylesheetItem_va(s, "\tbody { overflow:hidden; %[font-size:%POSpx; %]%[font-family:%STR&CSSVAL; %]}\n",
		font_size > 0, font_size, *font_name, font_name);
	htrAddStylesheetItem(s, "\tpre { font-size:90%; }\n");

	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s, "\ttd { %[font-size:%POSpx; %]%[font-family:%STR&CSSVAL; %]}\n",
		font_size > 0, font_size, *font_name, font_name);
	    htrAddStylesheetItem_va(s, "\tfont { %[font-size:%POSpx; %]%[font-family:%STR&CSSVAL; %]}\n",
		font_size > 0, font_size, *font_name, font_name);
	    }

	htrAddBodyItem(s, "<DIV ID=\"pgtop\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1152\" height=\"1\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgbtm\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1152\" height=\"1\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgrgt\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1\" height=\"864\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pglft\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1\" height=\"864\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgtvl\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgktop\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1152\" height=\"1\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkbtm\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1152\" height=\"1\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgkrgt\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1\" height=\"864\"></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgklft\"><IMG src=\"/sys/images/trans_1.gif\" width=\"1\" height=\"864\"></DIV>\n");

	htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"pgping",0);
	htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
	htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"pgmsg",0);
	htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
	htrAddBodyItem(s, "\n");

	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	htrAddScriptInit_va(s,"    pg_ping_init(htr_subel(nodes[\"%STR&SYM\"],\"pgping\"),%INT);\n",name,watchdogtimer/2*1000);

	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "pg", "pg_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "pg", "pg_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "pg", "pg_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "pg", "pg_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "pg", "pg_mouseup");
	if (s->Capabilities.Dom1HTML)
	    htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "pg", "pg_contextmenu");

	/** W3C DOM Level 2 Event model doesn't require a textbox to get keystrokes **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");
	    htrAddEventHandlerFunction(s, "document", "MOUSEUP", "pg2", "pg_mouseup_ns4");
	    }

	/** Set colors for the focus layers **/
	htrAddScriptInit_va(s, "    page.kbcolor1 = '%STR&JSSTR';\n    page.kbcolor2 = '%STR&JSSTR';\n",kbfocus1,kbfocus2);
	htrAddScriptInit_va(s, "    page.mscolor1 = '%STR&JSSTR';\n    page.mscolor2 = '%STR&JSSTR';\n",msfocus1,msfocus2);
	htrAddScriptInit_va(s, "    page.dtcolor1 = '%STR&JSSTR';\n    page.dtcolor2 = '%STR&JSSTR';\n",dtfocus1,dtfocus2);
	/*htrAddScriptInit(s, "    document.LSParent = null;\n");*/

	htrAddScriptInit(s, "    pg_togglecursor();\n");


	htrAddEventHandlerFunction(s, "document", "KEYDOWN", "pg", "pg_keydown");
	htrAddEventHandlerFunction(s, "document", "KEYUP", "pg", "pg_keyup");
	htrAddEventHandlerFunction(s, "document", "KEYPRESS", "pg", "pg_keypress");

	/** create the root node of the wgtr **/
	count = xaCount(&(tree->Children));
	for (i=0;i<count;i++)
	    {
	    htrRenderWidget(s, xaGetItem(&(tree->Children),i), z+1);
	    }

	/** keyboard input for NS4 **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit(s,
		    "    setTimeout(pg_mvpginpt, 1, document.layers.pginpt);\n"
		    "    document.layers.pginpt.moveTo(window.innerWidth-2, 20);\n"
		    "    document.layers.pginpt.visibility = 'inherit';\n");
	    htrAddScriptInit(s,"    document.layers.pginpt.document.tmpform.x.focus();\n");
	    }

	htrAddScriptInit(s, "    if(typeof(pg_status_close)=='function')pg_status_close();\n");
	htrAddScriptInit(s, "    pg_loadqueue_busy = false;\n");
	htrAddScriptInit(s, "    pg_serialized_load_doone();\n");
	
	return 0;
    }

/*** htpageInitialize - register with the ht_render module.
 ***/
int
htpageInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Page Driver");
	strcpy(drv->WidgetName,"page");
	drv->Render = htpageRender;
	/** Actions **/
	htrAddAction(drv, "LoadPage");
	htrAddParam(drv, "LoadPage", "Source", DATA_T_STRING);
	htrAddAction(drv, "Launch");
	htrAddParam(drv, "Launch", "Source", DATA_T_STRING);
	htrAddParam(drv, "Launch", "Width", DATA_T_INTEGER);
	htrAddParam(drv, "Launch", "Height", DATA_T_INTEGER);
	htrAddParam(drv, "Launch", "Name", DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");


    return 0;
    }
