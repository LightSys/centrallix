#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "expression.h"
#include "cxlib/qprintf.h"
#include <assert.h>

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
/* Module: 	ht_render.h,ht_render.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Page rendering engine that interacts with the 	*/
/*		various widget drivers to produce a dynamic HTML page.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: ht_render.c,v 1.68 2007/04/19 21:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/ht_render.c,v $

    $Log: ht_render.c,v $
    Revision 1.68  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.67  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.66  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.65  2007/02/22 23:25:13  gbeeley
    - (feature) adding initial framework for CXSS, the security subsystem.
    - (feature) CXSS entropy pool and key generation, basic framework.
    - (feature) adding xmlhttprequest capability
    - (change) CXSS requires OpenSSL, adding that check to the build
    - (security) Adding application key to thwart request spoofing attacks.
      Once the AML is active, application keying will be more important and
      will be handled there instead of in net_http.

    Revision 1.64  2006/11/16 20:15:53  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.63  2006/10/27 19:26:00  gbeeley
    - (bugfix) use Dom2Event for event capture, if the UA supports it.  The old
      captureEvents/onthisorthatevent interface was far to unreliable in Gecko
      when new events needed to be captured after the fact.

    Revision 1.62  2006/10/27 05:57:22  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.61  2006/10/19 21:53:23  gbeeley
    - (feature) First cut at the component-based client side development
      system.  Only rendering of the components works right now; interaction
      with the components and their containers is not yet functional.  For
      an example, see "debugwin.cmp" and "window_test.app" in the samples
      directory of centrallix-os.

    Revision 1.60  2006/10/16 18:34:33  gbeeley
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

    Revision 1.59  2006/10/04 17:33:25  gbeeley
    - (bugfix) ht_render processing of user agents crashes if useragent.cfg
      is not valid.
    - (bugfix) add detection for newer versions of Gecko.  I do not know if
      the detection draws the line at the correct place regarding versions
      and such, but this is a starting point...

    Revision 1.58  2006/10/04 17:12:54  gbeeley
    - (bugfix) Newer versions of Gecko handle clipping regions differently than
      anything else out there.  Created a capability flag to handle that.
    - (bugfix) Useragent.cfg processing was sometimes ignoring sub-definitions.

    Revision 1.57  2005/03/01 07:08:25  gbeeley
    - don't activate the serialized loading until after startup() finishes
      running.

    Revision 1.56  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.55  2004/08/30 03:20:17  gbeeley
    - updates for widgets
    - bugfix for htrRender() handling of event handler function return values

    Revision 1.54  2004/08/15 03:10:48  gbeeley
    - moving client canvas size detection logic from htmlgen to net_http so
      that it can be passed to wgtrVerify(), later to be used in adjusting
      geometry of application to fit browser window.

    Revision 1.53  2004/08/15 01:57:51  gbeeley
    - adding CSSBox capability - not a standard, but IE and Moz differ in how
      they handle the box model.  IE draws borders within the width and height,
      but Moz draws them outside the width and height.  Neither compute borders
      as being a part of the content area of the DIV.

    Revision 1.52  2004/08/14 20:28:29  gbeeley
    - fix for windowsize-getter script to work with IE.

    Revision 1.51  2004/08/13 18:46:13  mmcgill
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

    Revision 1.50  2004/08/04 20:03:07  mmcgill
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

    Revision 1.49  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.48  2004/08/02 14:09:33  mmcgill
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

    Revision 1.47  2004/07/20 21:28:52  mmcgill
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

    Revision 1.46  2004/07/19 15:30:39  mmcgill
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

    Revision 1.45  2004/06/25 16:46:30  gbeeley
    - Auto-detect size of user-agent's window

    Revision 1.44  2004/04/29 16:26:41  gbeeley
    - Fixes to get FourTabs.app working again in NS4/Moz, and in IE5.5/IE6.
    - Added inline-include feature to help with debugging in IE, which does
      not specify the correct file in its errors.  To use it, just append
      "?ls__collapse_includes=yes" to your .app URL.

    Revision 1.43  2004/03/10 10:39:04  jasonyip

    I have added a JS15 CAP for javascript 1.5 Capability.

    Revision 1.42  2004/02/24 20:21:56  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.41  2003/11/22 16:37:18  jorupp
     * add support for moving event handler scripts to the .js code
     	note: the underlying implimentation in ht_render.c_will_ change, this was
    	just to get opinions on the API and output
     * moved event handlers for htdrv_window from the .c to the .js

    Revision 1.40  2003/11/18 06:01:10  gbeeley
    - adding utility method htrGetBackground to simplify bgcolor/image

    Revision 1.39  2003/08/02 22:12:06  jorupp
     * got treeview pretty much working (a bit slow though)
    	* I split up several of the functions so that the Mozilla debugger's profiler could help me out more
     * scrollpane displays, doesn't scroll

    Revision 1.38  2003/07/15 01:57:51  gbeeley
    Adding an independent DHTML scrollbar widget that will be used to
    control scrolling/etc on other widgets.

    Revision 1.37  2003/06/21 23:54:41  jorupp
     * fixex up a few problems I found with the version I committed (like compilation...)
     * removed some code that was commented out

    Revision 1.36  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.35  2003/06/03 23:31:04  gbeeley
    Adding pro forma netscape 4.8 support.

    Revision 1.34  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.33  2003/03/30 22:49:23  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.32  2003/01/05 04:18:08  lkehresman
    Added detection for Mozilla 1.2.x

    Revision 1.31  2002/12/24 09:41:07  jorupp
     * move output of cn_browser to ht_render, also moving up above the first place where it is needed

    Revision 1.30  2002/12/04 00:19:09  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.29  2002/11/22 19:29:36  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.28  2002/09/27 22:26:04  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.27  2002/09/11 00:57:08  jorupp
     * added check for Mozilla 1.1

    Revision 1.26  2002/08/12 09:14:28  mattphillips
    Use the built-in PACKAGE_VERSION instead of VERSION to get the current version
    number to be more standard.  PACKAGE_VERSION is set by autoconf, but read from
    .version when configure is generated.

    Revision 1.25  2002/08/03 02:36:34  gbeeley
    Made all hash tables the same size at 257 (a prime) entries.

    Revision 1.24  2002/08/02 19:44:20  gbeeley
    Have ht_render report the widget type when it complains about not knowing
    a widget type when generating a page.

    Revision 1.23  2002/07/18 20:12:40  lkehresman
    Added support for a loadstatus icon to be displayed, hiding the drawing
    of the visible windows.  This looks MUCH nicer when loading Kardia or
    any other large apps.  It is completely optional part of the page widget.
    To take advantage of it, put the parameter "loadstatus" equal to "true"
    in the page widget.

    Revision 1.22  2002/07/18 15:17:44  lkehresman
    Ok, I got caught being lazy.  I used snprintf and the string sbuf to
    help me count the number of characters in the string I modified.  But
    sbuf was being used elsewhere and I messed it up.  Fixed it so it isn't
    using sbuf any more.  I broke down and counted the characters.
    (how many times can we modify this line in one hour?? SHEESH!

    Revision 1.20  2002/07/18 14:31:05  lkehresman
    Whoops!  I was sending the wrong string size to fdWrite.  Fixed it.

    Revision 1.19  2002/07/18 14:26:13  lkehresman
    Added a work-around for the Netscape resizing bug.  Instead of leaving
    the page totally messed up on a resize, it will now completely reload the
    page whenever the window is resized.

    Revision 1.18  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.17  2002/07/15 21:27:02  lkehresman
    Added copyright statement at top of generated DHTML documents.  (if the
    wording needs to change, please do it or let me know)

    Revision 1.16  2002/07/07 00:17:01  jorupp
     * add support for Mozilla 1.1alpha (1.1a)

    Revision 1.15  2002/06/24 20:07:41  lkehresman
    Committing a fix for Jonathan (he doesn't have CVS access right now).
    This now detects Mozilla pre-1.0 versions.

    Revision 1.14  2002/06/20 16:22:08  gbeeley
    Wrapped the nonconstant format string warning in an ifdef WITH_SECWARN
    so it doesn't bug people other than developers.

    Revision 1.13  2002/06/19 19:57:13  gbeeley
    Added warning code if htr..._va() function is passed a format string
    from the heap or other modifiable data segments.  Half a kludge...

    Revision 1.12  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.11  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.10  2002/05/03 03:43:25  gbeeley
    Added FD_U_PACKET to the fdWrite() calls in ht_render.  It is possible
    that some data was getting dropped - fdWrite() makes no guarantee of
    writing *all* the data unless you include the FD_U_PACKET flag :)

    Revision 1.9  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.8  2002/04/28 06:00:38  jorupp
     * added htrAddScriptCleanup* stuff
     * added cleanup stuff to osrc

    Revision 1.7  2002/04/28 03:19:53  gbeeley
    Fixed a bit of a bug in ht_render where it did not properly set the
    length on the StrValue structures when adding script functions.  This
    was basically causing some substantial heap corruption.

    Revision 1.6  2002/04/25 22:54:48  gbeeley
    Set the starting tmpbuf back to 512 from the 8 bytes I was using to
    test the auto-realloc logic... ;)

    Revision 1.5  2002/04/25 22:51:29  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.4  2002/04/25 04:27:21  gbeeley
    Added new AddInclude() functionality to the html generator, so include
    javascript files can be added.  Untested.

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:48  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** GLOBALS ***/
struct
    {
    XArray	Drivers;		/* simple driver listing. */
    XHashTable	Classes;		/* classes of widget sets */
    }
    HTR;

/** a structure to store the detection code for a specific browser/set of browsers **/
typedef struct
    {
    regex_t  UAreg;
    XArray  Children;
    HtCapabilities  Capabilities;
    }
    AgentCapabilities, *pAgentCapabilities;

/**
 * Processes a single node (and it's children) of the tree of useragents
 **/
pAgentCapabilities
htr_internal_ProcessUserAgent(const pStructInf node, const pHtCapabilities parentCap)
    {
    int i;
    pStructInf entry;
    pAgentCapabilities agentCap;
    char* data;

    /** build the structure **/
    agentCap = nmMalloc(sizeof(AgentCapabilities));
    if(!agentCap)
	{
	mssError(0,"HTR","nmMalloc() failed");
	return NULL;
	}
    memset(agentCap, 0, sizeof(AgentCapabilities));

    /** if we're a top-level definition (under a class), there's no parent agentCapabilities to inherit **/
    if(parentCap)
	{
	memcpy(&(agentCap->Capabilities), parentCap, sizeof(HtCapabilities));
	}

    /** find and build the regex for the useragent detection **/
    entry = stLookup(node,"useragent");
    if(!entry)
	{
	mssError(1,"HTR","Missing useragent for %s", node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}
    if(stGetAttrValue(entry, DATA_T_STRING, POD(&data), 0)<0)
	{
	mssError(1,"HTR","Can't read useragent for %s", node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}
    if(regcomp(&(agentCap->UAreg), data, REG_EXTENDED | REG_NOSUB | REG_ICASE))
	{
	mssError(1,"HTR","Could not compile regular expression: '%s' for %s", data, node->Name);
	nmFree(agentCap,sizeof(AgentCapabilities));
	return NULL;
	}

    /**
     * process all the listed agentCapabilities
     * seriously... who wants to write this code for each attribute....
     **/
#define PROCESS_CAP_INIT(attr) \
    if((entry = stLookup(node, # attr ))) \
	{ \
	if(stGetAttrValue(entry, DATA_T_STRING, POD(&data), 0)>=0) \
	    { \
	    if(!strcmp(data, "yes")) \
		agentCap->Capabilities.attr = 1; \
	    else if(!strcmp(data, "no")) \
		agentCap->Capabilities.attr = 0; \
	    else \
		mssError(1,"HTR","%s must be yes, no, 0, or 1 in %s", # attr ,node->Name); \
	    } \
	else if(stGetAttrValue(entry, DATA_T_INTEGER, POD(&i), 0)>=0) \
	    { \
	    if(i==0) \
		agentCap->Capabilities.attr = 0; \
	    else \
		agentCap->Capabilities.attr = 1; \
	    } \
	else \
	    mssError(1,"HTR","%s must be yes, no, 0, or 1 in %s", # attr ,node->Name); \
	}

    PROCESS_CAP_INIT(Dom0NS);
    PROCESS_CAP_INIT(Dom0IE);
    PROCESS_CAP_INIT(Dom1HTML);
    PROCESS_CAP_INIT(Dom1XML);
    PROCESS_CAP_INIT(Dom2Core);
    PROCESS_CAP_INIT(Dom2HTML);
    PROCESS_CAP_INIT(Dom2XML);
    PROCESS_CAP_INIT(Dom2Views);
    PROCESS_CAP_INIT(Dom2StyleSheets);
    PROCESS_CAP_INIT(Dom2CSS);
    PROCESS_CAP_INIT(Dom2CSS2);
    PROCESS_CAP_INIT(Dom2Events);
    PROCESS_CAP_INIT(Dom2MouseEvents);
    PROCESS_CAP_INIT(Dom2HTMLEvents);
    PROCESS_CAP_INIT(Dom2MutationEvents);
    PROCESS_CAP_INIT(Dom2Range);
    PROCESS_CAP_INIT(Dom2Traversal);
    PROCESS_CAP_INIT(CSS1);
    PROCESS_CAP_INIT(CSS2);
    PROCESS_CAP_INIT(CSSBox);
    PROCESS_CAP_INIT(CSSClip);
    PROCESS_CAP_INIT(HTML40);
    PROCESS_CAP_INIT(JS15);
    PROCESS_CAP_INIT(XMLHttpRequest);

    /** now process children, passing a reference to our capabilities along **/
    xaInit(&(agentCap->Children), 4);
    for(i=0;i<node->nSubInf;i++)
	{
	pStructInf childNode;
	childNode = node->SubInf[i];
	/** UsrType is non-null if this is a sub-structure, ie. not an attribute **/
	if(childNode && childNode->UsrType)
	    {
	    pAgentCapabilities childCap = htr_internal_ProcessUserAgent(childNode, &(agentCap->Capabilities));
	    if(childCap)
		{
		xaAddItem(&(agentCap->Children), childCap);
		}
	    }
	}

    return agentCap;
    }

/***
 ***  Writes the capabilities of the browser used in the passed session to the passed pFile
 ***     as the cx__capabilities object (for javascript)
 ***/
void
htr_internal_writeCxCapabilities(pHtSession s, pFile out)
    {
    fdWrite(out,"    cx__capabilities = new Object();\n",37,0,FD_U_PACKET);
#define PROCESS_CAP_OUT(attr) \
    fdWrite(out,"    cx__capabilities.",21,0,FD_U_PACKET); \
    fdWrite(out, # attr ,strlen( # attr ),0,FD_U_PACKET); \
    fdWrite(out," = ",3,0,FD_U_PACKET); \
    fdWrite(out,(s->Capabilities.attr?"1;\n":"0;\n"),3,0,FD_U_PACKET);

    PROCESS_CAP_OUT(Dom0NS);
    PROCESS_CAP_OUT(Dom0IE);
    PROCESS_CAP_OUT(Dom1HTML);
    PROCESS_CAP_OUT(Dom1XML);
    PROCESS_CAP_OUT(Dom2Core);
    PROCESS_CAP_OUT(Dom2HTML);
    PROCESS_CAP_OUT(Dom2XML);
    PROCESS_CAP_OUT(Dom2Views);
    PROCESS_CAP_OUT(Dom2StyleSheets);
    PROCESS_CAP_OUT(Dom2CSS);
    PROCESS_CAP_OUT(Dom2CSS2);
    PROCESS_CAP_OUT(Dom2Events);
    PROCESS_CAP_OUT(Dom2MouseEvents);
    PROCESS_CAP_OUT(Dom2HTMLEvents);
    PROCESS_CAP_OUT(Dom2MutationEvents);
    PROCESS_CAP_OUT(Dom2Range);
    PROCESS_CAP_OUT(Dom2Traversal);
    PROCESS_CAP_OUT(CSS1);
    PROCESS_CAP_OUT(CSS2);
    PROCESS_CAP_OUT(CSSBox);
    PROCESS_CAP_OUT(CSSClip);
    PROCESS_CAP_OUT(HTML40);
    PROCESS_CAP_OUT(JS15);
    PROCESS_CAP_OUT(XMLHttpRequest);
    }

/**
 * Registers the tree of classes and user agents by reading the file specified in the config file
**/
int
htrRegisterUserAgents()
    {
    pStructInf uaConfigEntry;
    char *uaConfigFilename;
    pFile uaConfigFile;
    pStructInf uaConfigRoot;
    int i,j;

    /** find the name of the config file **/
    uaConfigEntry = stLookup(CxGlobals.ParsedConfig,"useragent_config");
    if(!uaConfigEntry)
	{
	mssError(1,"HTR","No configuration directive useragent_config found.  Unable to register useragents.");
	return -1;
	}
    if(stGetAttrValue(uaConfigEntry, DATA_T_STRING, POD(&uaConfigFilename), 0) <0 || !uaConfigFilename )
	{
	mssError(0,"HTR","Unable to read useragent_config's value.  Unable to register useragents.");
	return -1;
	}

    /** open and parse it **/
    uaConfigFile = fdOpen(uaConfigFilename, O_RDONLY, 0600);
    if(!uaConfigFile)
	{
	mssError(0,"HTR","Unable to open useragent_config %s", uaConfigFilename);
	return -1;
	}
    uaConfigRoot = stParseMsg(uaConfigFile, 0);
    if(!uaConfigRoot)
	{
	mssError(0,"HTR","Unable to parse useragent_config %s", uaConfigFilename);
	fdClose(uaConfigFile, 0);
	return -1;
	}

    /** iterate through the classes and create them **/
    for(i=0;i<uaConfigRoot->nSubInf;i++)
	{
	pStructInf stClass;
	stClass = uaConfigRoot->SubInf[i];
	if(stClass)
	    {
	    pHtClass class = (pHtClass)nmMalloc(sizeof(HtClass));
	    if(!class)
		{
		mssError(0,"HTR","nmMalloc() failed");
		return -1;
		}
	    memset(class, 0, sizeof(HtClass));
	    strncpy(class->ClassName, stClass->Name, 32);
	    class->ClassName[31] = '\0';

	    xaInit(&(class->Agents),4);
	    xhInit(&(class->WidgetDrivers), 257, 0);

	    for(j=0;j<stClass->nSubInf;j++)
		{
		pStructInf entry = stClass->SubInf[j];
		if(entry && entry->UsrType)
		    {
		    pAgentCapabilities cap;
		    if((cap = htr_internal_ProcessUserAgent(entry, NULL)))
			{
			xaAddItem(&(class->Agents), cap);
			}
		    }
		}
	    xhAdd(&(HTR.Classes), class->ClassName, (void*) class);
	    }
	}

    fdClose(uaConfigFile, 0);
    return 0;
    }

/**
 * This function finds the capabilities of the specified browser for the specified class
 * Both browser and class are required to not be null
 * A null return value indicates that no match was found for the browser
 * A non-null return value should not be freeed or modified in any way
 **/
pHtCapabilities
htr_internal_GetBrowserCapabilities(char *browser, pHtClass class)
    {
    pXArray list;
    pAgentCapabilities agentCap = NULL;
    pHtCapabilities cap = NULL;
    int i;
    if(!browser || !class)
	return NULL;

    list = &(class->Agents);
    for(i=0;i<xaCount(list);i++)
	{
	if ((agentCap = (pAgentCapabilities)xaGetItem(list,i)))
	    {
	    /** 0 signifies a match, REG_NOMATCH signifies the opposite **/
	    if (regexec(&(agentCap->UAreg), browser, (size_t)0, NULL, 0) == 0)
		{
		list = &(agentCap->Children);
		if(xaCount(list)>0)
		    {
		    /** remember this point in case there are no more matches **/
		    cap = &(agentCap->Capabilities);
		    /** reset to the beginning of the list **/
		    i = -1;
		    }
		else
		    {
		    /** no children -- this is a terminal node **/
		    return &(agentCap->Capabilities);
		    }
		}
	    }
	}

    /** if we found a match while walking the tree (at a non-terminal node), but
	nothing under it matched, cap will not be null, otherwise it will be
	(we don't get here if we matched a terminal node) **/
    return cap;
    }


/*** htr_internal_AddTextToArray - adds a string of text to an array of
 *** buffer blocks, allocating new blocks in the XArray if necessary.
 ***/
int
htr_internal_AddTextToArray(pXArray arr, char* txt)
    {
    int l,n,cnt;
    char* ptr;

    	/** Need new block? **/
	if (arr->nItems == 0)
	    {
	    ptr = (char*)nmMalloc(2048);
	    if (!ptr) return -1;
	    *(int*)ptr = 0;
	    l = 0;
	    xaAddItem(arr,ptr);
	    }
	else
	    {
	    ptr = (char*)(arr->Items[arr->nItems-1]);
	    l = *(int*)ptr;
	    }

	/** Copy into the blocks, allocating more as needed. **/
	n = strlen(txt);
	while(n)
	    {
	    cnt = n;
	    if (cnt > (2040-l)) cnt = 2040-l;
	    memcpy(ptr+l+8,txt,cnt);
	    n -= cnt;
	    txt += cnt;
	    l += cnt;
	    *(int*)ptr = l;
	    if (n)
	        {
		ptr = (char*)nmMalloc(2048);
		if (!ptr) return -1;
		*(int*)ptr = 0;
		l = 0;
		xaAddItem(arr,ptr);
		}
	    }

    return 0;
    }


/*** htrRenderWidget - generate a widget into the HtPage structure, given the
 *** widget's objectsystem descriptor...
 ***/
int
htrRenderWidget(pHtSession session, pWgtrNode widget, int z)
    {
    pHtDriver drv;
    pXHashTable widget_drivers = NULL;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    return -1;
	    }

	/** Find the hashtable keyed with widget names for this combination of
	 ** user-agent:style that contains pointers to the drivers to use.
	 **/
	if(!session->Class)
	    {
	    printf("Class not defined %s:%i\n",__FILE__,__LINE__);
	    return -1;
	    }
	widget_drivers = &( session->Class->WidgetDrivers);
	if (!widget_drivers)
	    {
	    htrAddBodyItem_va(session, "No widgets have been defined for your browser type and requested class combination.");
	    mssError(1, "HTR", "Invalid UserAgent:class combination");
	    return -1;
	    }

	/** Get the name of the widget.. **/
	if (strncmp(widget->Type,"widget/",7))
	    {
	    mssError(1,"HTR","Invalid content type for widget - must be widget/xxx");
	    return -1;
	    }

	/** Lookup the driver **/
	drv = (pHtDriver)xhLookup(widget_drivers,widget->Type+7);
	if (!drv)
	    {
	    mssError(1,"HTR","Unknown widget object type '%s'", widget->Type);
	    return -1;
	    }

    return drv->Render(session, widget, z);
    }


/*** htrAddStylesheetItem -- copies stylesheet definitions into the
 *** buffers that will eventually be output as HTML.
 ***/
int
htrAddStylesheetItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlStylesheet), html_text);
    }


/*** htrAddHeaderItem -- copies html text into the buffers that will
 *** eventually be output as the HTML header.
 ***/
int
htrAddHeaderItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlHeader), html_text);
    }


/*** htrAddBodyItem -- copies html text into the buffers that will
 *** eventually be output as the HTML body.
 ***/
int
htrAddBodyItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlBody), html_text);
    }


/*** htrAddExpressionItem -- copies html text into the buffers that will
 *** eventually be output as the HTML body.
 ***/
int
htrAddExpressionItem(pHtSession s, char* html_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlExpressionInit), html_text);
    }


/*** htrAddBodyParam -- copies html text into the buffers that will
 *** eventually be output as the HTML body.  These are simple html tag
 *** parameters (i.e., "BGCOLOR=white")
 ***/
int
htrAddBodyParam(pHtSession s, char* html_param)
    {
    return htr_internal_AddTextToArray(&(s->Page.HtmlBodyParams), html_param);
    }


extern int __data_start;


/*** htr_internal_GrowFn() - qPrintf grow function to resize tmp buffer on
 *** the fly.
 ***/
int
htr_internal_GrowFn(char** str, size_t* size, void* arg, int req_size)
    {
    pHtSession s = (pHtSession)arg;
    char* new_buf;
    int new_buf_size;

	if (*size >= req_size) return 1;

	assert(*str == s->Tmpbuf);
	assert(*size == s->TmpbufSize);
	new_buf_size = s->TmpbufSize * 2;
	while(new_buf_size < req_size) new_buf_size *= 2;
	new_buf = nmSysRealloc(s->Tmpbuf, new_buf_size);
	if (!new_buf)
	    return 0;
	*str = s->Tmpbuf = new_buf;
	*size = s->TmpbufSize = new_buf_size;

    return 1; /* OK */
    }


/*** htr_internal_QPAddText() - same as below function, but uses qprintf
 *** instead of snprintf.
 ***/
int
htr_internal_QPAddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    int rval;

	/** Print a warning if we think the format string isn't a constant.
	 ** We'll need to upgrade this once htdrivers start being loaded as
	 ** modules, since their text segments will have different addresses
	 ** and we'll then have to read /proc/self/maps manually.
	 **/
#ifdef WITH_SECWARN
	if ((unsigned int)fmt > (unsigned int)(&__data_start))
	    {
	    printf("***WARNING*** htrXxxYyy_va() format string '%s' at address 0x%X > 0x%X may not be a constant.\n",fmt,(unsigned int)fmt,(unsigned int)(&__data_start));
	    }
#endif

	rval = qpfPrintf_va_internal(NULL, &(s->Tmpbuf), &(s->TmpbufSize), htr_internal_GrowFn, (void*)s, fmt, va);
	if (rval < 0)
	    {
	    printf("WARNING:  QPAddText() failed for format: %s\n", fmt);
	    }
	if (rval < 0 || rval > (s->TmpbufSize - 1))
	    return -1;

	/** Ok, now add the tmpbuf normally. **/
	fn(s, s->Tmpbuf);

    return 0;
    }


/*** htr_internal_AddText() - use vararg mechanism to add text using one of
 *** the standard add routines.
 ***/
int
htr_internal_AddText(pHtSession s, int (*fn)(), char* fmt, va_list va)
    {
    va_list orig_va;
    int rval;
    char* new_buf;
    int new_buf_size;

	/** Print a warning if we think the format string isn't a constant.
	 ** We'll need to upgrade this once htdrivers start being loaded as
	 ** modules, since their text segments will have different addresses
	 ** and we'll then have to read /proc/self/maps manually.
	 **/
#ifdef WITH_SECWARN
	if ((unsigned int)fmt > (unsigned int)(&__data_start))
	    {
	    printf("***WARNING*** htrXxxYyy_va() format string '%s' at address 0x%X > 0x%X may not be a constant.\n",fmt,(unsigned int)fmt,(unsigned int)(&__data_start));
	    }
#endif

	/** Save the current va_list state so we can retry it. **/
	orig_va = va;

	/** Attempt to print the thing to the tmpbuf. **/
	while(1)
	    {
	    rval = vsnprintf(s->Tmpbuf, s->TmpbufSize, fmt, va);

	    /** Sigh.  Some libc's return -1 and some return # bytes that would be written. **/
	    if (rval < 0 || rval > (s->TmpbufSize - 1))
		{
		/** I think I need a bigger box.  Fix it and try again. **/
		new_buf_size = s->TmpbufSize * 2;
		while(new_buf_size < rval) new_buf_size *= 2;
		new_buf = nmSysMalloc(new_buf_size);
		if (!new_buf)
		    {
		    return -1;
		    }
		nmSysFree(s->Tmpbuf);
		s->Tmpbuf = new_buf;
		s->TmpbufSize = new_buf_size;
		va = orig_va;
		}
	    else
		{
		break;
		}
	    }

	/** Ok, now add the tmpbuf normally. **/
	fn(s, s->Tmpbuf);

    return 0;
    }


/*** htrAddBodyItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the body of the document.
 ***/
int
htrAddBodyItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddBodyItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddExpressionItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the body of the document.
 ***/
int
htrAddExpressionItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddExpressionItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddStylesheetItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the stylesheet definition of the document.
 ***/
int
htrAddStylesheetItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddStylesheetItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddHeaderItem_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the header of the document.
 ***/
int
htrAddHeaderItem_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddHeaderItem, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddBodyParam_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to the body tag of the document.
 ***/
int
htrAddBodyParam_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddBodyParam, fmt, va);
	va_end(va);

    return 0;
    }


/*** htrAddScriptWgtr_va() - use a vararg list to add a formatted string
 *** to wgtr function of the document
 ***/
int
htrAddScriptWgtr_va(pHtSession s, char* fmt, ...)
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddScriptWgtr, fmt, va);
	va_end(va);

    return 0;
    }



/*** htrAddScriptInit_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to startup function of the document.
 ***/
int
htrAddScriptInit_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddScriptInit, fmt, va);
	va_end(va);

    return 0;
    }

/*** htrAddScriptCleanup_va() - use a vararg list (like sprintf, etc) to add a
 *** formatted string to cleanup function of the document.
 ***/
int
htrAddScriptCleanup_va(pHtSession s, char* fmt, ... )
    {
    va_list va;

	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddScriptCleanup, fmt, va);
	va_end(va);

    return 0;
    }



/*** htrAddScriptInclude -- adds a script src= type entry between the html
 *** header and html body.
 ***/
int
htrAddScriptInclude(pHtSession s, char* filename, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameIncludes), filename)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = filename;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(filename)+1;
	sv->Value = "";
	sv->Alloc = (flags & HTR_F_NAMEALLOC);

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameIncludes), filename, (char*)sv);
	xaAddItem(&(s->Page.Includes), (char*)sv);

    return 0;
    }


/*** htrAddScriptFunction -- adds a script function to the list of functions
 *** that will be output.  Note that duplicate functions won't be added, so
 *** the widget drivers need not keep track of this.
 ***/
int
htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameFunctions), fn_name)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = fn_name;
	if (flags & HTR_F_NAMEALLOC) sv->NameSize = strlen(fn_name)+1;
	sv->Value = fn_text;
	if (flags & HTR_F_VALUEALLOC) sv->ValueSize = strlen(fn_text)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameFunctions), fn_name, (char*)sv);
	xaAddItem(&(s->Page.Functions), (char*)sv);

    return 0;
    }


/*** htrAddScriptGlobal -- adds a global variable to the list of variables
 *** to be output in the HTML JavaScript section.  Duplicates are suppressed.
 ***/
int
htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags)
    {
    pStrValue sv;

    	/** Alloc the string val. **/
	if (xhLookup(&(s->Page.NameGlobals), var_name)) return 0;
	sv = (pStrValue)nmMalloc(sizeof(StrValue));
	if (!sv) return -1;
	sv->Name = var_name;
	sv->NameSize = strlen(var_name)+1;
	sv->Value = initialization;
	sv->ValueSize = strlen(initialization)+1;
	sv->Alloc = flags;

	/** Add to the hash table and array **/
	xhAdd(&(s->Page.NameGlobals), var_name, (char*)sv);
	xaAddItem(&(s->Page.Globals), (char*)sv);

    return 0;
    }


/*** htrAddScriptInit -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int
htrAddScriptInit(pHtSession s, char* init_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Inits), init_text);
    }


/*** htrAddScriptWgtr - adds some text to the wgtr function
 ***/
int
htrAddScriptWgtr(pHtSession s, char* wgtr_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Wgtr), wgtr_text);
    }


/*** htrAddScriptCleanup -- adds some initialization text that runs outside of a
 *** function context in the HTML JavaScript.
 ***/
int
htrAddScriptCleanup(pHtSession s, char* init_text)
    {
    return htr_internal_AddTextToArray(&(s->Page.Cleanups), init_text);
    }

#if 00
/*** htrAddEventHandler - adds an event handler script code segment for a
 *** given event on a given object (usually the 'document').
 ***/
int
htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code)
    {
    pHtNameArray obj, evt, drv;

    	/** Is this object already listed? **/
	obj = (pHtNameArray)xhLookup(&(s->Page.EventScripts.HashTable), event_src);

	/** If not, create new object for this event source **/
	if (!obj)
	    {
	    obj = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!obj) return -1;
	    memccpy(obj->Name, event_src, 0, 127);
	    obj->Name[127] = '\0';
	    xhInit(&(obj->HashTable),257,0);
	    xaInit(&(obj->Array),16);
	    xhAdd(&(s->Page.EventScripts.HashTable), obj->Name, (void*)(obj));
	    xaAddItem(&(s->Page.EventScripts.Array), (void*)obj);
	    }

	/** Is this event name already listed? **/
	evt = (pHtNameArray)xhLookup(&(obj->HashTable), event);

	/** If not already, create new. **/
	if (!evt)
	    {
	    evt = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!evt) return -1;
	    memccpy(evt->Name, event,0,127);
	    evt->Name[127] = '\0';
	    xhInit(&(evt->HashTable),257,0);
	    xaInit(&(evt->Array),16);
	    xhAdd(&(obj->HashTable), evt->Name, (void*)evt);
	    xaAddItem(&(obj->Array), (void*)evt);
	    }

	/** Is the driver name already listed? **/
	drv = (pHtNameArray)xhLookup(&(evt->HashTable),drvname);

	/** If not already, add new. **/
	if (!drv)
	    {
	    drv = (pHtNameArray)nmMalloc(sizeof(HtNameArray));
	    if (!drv) return -1;
	    memccpy(drv->Name, drvname, 0, 127);
	    drv->Name[127] = '\0';
	    xaInit(&(drv->Array),16);
	    xhAdd(&(evt->HashTable), drv->Name, (void*)drv);
	    xaAddItem(&(evt->Array), (void*)drv);

	    /** Ok, got event and object.  Now, add script text. **/
            htr_internal_AddTextToArray(&(drv->Array), handler_code);
	    }

    return 0;
    }
#endif

/*** htrAddEventHandlerFunction - adds an event handler script code segment for
 *** a given event on a given object (usually the 'document').
 ***
 *** GRB note -- event_src is no longer used, should always be 'document'.
 ***          -- drvname is no longer used, just ignored.
 ***/
int
htrAddEventHandlerFunction(pHtSession s, char* event_src, char* event, char* drvname, char* function)
    {
    pHtDomEvent e = NULL;
    pHtDomEvent e_srch;
    int i,cnt;

	/** Look it up? 
	 ** (GRB note -- is this slow enough to need an XHashTable?)
	 **/
	cnt = xaCount(&s->Page.EventHandlers);
	for(i=0;i<cnt;i++)
	    {
	    e_srch = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    if (!strcmp(event, e_srch->DomEvent))
		{
		e = e_srch;
		break;
		}
	    }
	
	/** Make a new one? **/
	if (!e)
	    {
	    e = (pHtDomEvent)nmMalloc(sizeof(HtDomEvent));
	    if (!e) return -1;
	    strtcpy(e->DomEvent, event, sizeof(e->DomEvent));
	    xaInit(&e->Handlers,64);
	    xaAddItem(&s->Page.EventHandlers, e);
	    }

	/** Add our handler **/
	cnt = xaCount(&e->Handlers);
	for(i=0;i<cnt;i++)
	    {
	    if (!strcmp(function, (char*)xaGetItem(&e->Handlers,i)))
		return 0;
	    }
	xaAddItem(&e->Handlers, function);

    return 0;
	
    /*char buf[HT_SBUF_SIZE];
    snprintf(buf, HT_SBUF_SIZE,
	"    handler_return = %s(e);\n"
	"    if(handler_return & EVENT_PREVENT_DEFAULT_ACTION)\n"
	"        prevent_default = true;\n"
	"    if(handler_return & EVENT_HALT)\n"
	"        return !prevent_default;\n",
	function);
    buf[HT_SBUF_SIZE-1] = '\0';

    return htrAddEventHandler(s, event_src, event, drvname, buf);*/
    }


/*** htrDisableBody - disables the <BODY> </BODY> tags so that, for instance,
 *** a frameset item can be used.
 ***/
int
htrDisableBody(pHtSession s)
    {
    s->DisableBody = 1;
    return 0;
    }


/*** htrAddEvent - adds an event to a driver.
 ***/
int
htrAddEvent(pHtDriver drv, char* event_name)
    {
    pHtEventAction event;

	/** Create the action **/
	event = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	if (!event) return -1;
	memccpy(event->Name, event_name, 0, 31);
	event->Name[31] = '\0';
	xaInit(&(event->Parameters),16);
	xaAddItem(&drv->Events, (void*)event);

    return 0;
    }


/*** htrAddAction - adds an action to a widget.
 ***/
int
htrAddAction(pHtDriver drv, char* action_name)
    {
    pHtEventAction action;

	/** Create the action **/
	action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	if (!action) return -1;
	memccpy(action->Name, action_name, 0, 31);
	action->Name[31] = '\0';
	xaInit(&(action->Parameters),16);
	xaAddItem(&drv->Actions, (void*)action);

    return 0;
    }


/*** htrAddParam - adds a parameter to a widget's action or event
 ***/
int
htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype)
    {
    pHtEventAction ea = NULL;
    int i;
    pHtParam p;

	/** Look for a matching event/action **/
	for(i=0;i<drv->Actions.nItems;i++)
	    {
	    if (!strcmp(((pHtEventAction)(drv->Actions.Items[i]))->Name, eventaction))
	        {
		ea = (pHtEventAction)(drv->Actions.Items[i]);
		break;
		}
	    }
	if (!ea) for(i=0;i<drv->Events.nItems;i++)
	    {
	    if (!strcmp(((pHtEventAction)(drv->Events.Items[i]))->Name, eventaction))
	        {
		ea = (pHtEventAction)(drv->Events.Items[i]);
		break;
		}
	    }
	if (!ea) return -1;

	/** Add the parameter **/
	p = nmSysMalloc(sizeof(HtParam));
	if (!p) return -1;
	memccpy(p->ParamName, param_name, 0, 31);
	p->ParamName[31] = '\0';
	p->DataType = datatype;
	xaAddItem(&(ea->Parameters), (void*)p);

    return 0;
    }


/*** htrAddBodyItemLayer_va - adds an entire "layer" to the document.  A layer
 *** in the traditional netscape-4 sense of a layer, which might be implemented
 *** in a number of different ways in the actual user agent itself.
 ***/
int
htrAddBodyItemLayer_va(pHtSession s, int flags, char* id, int cnt, const char* fmt, ...)
    {
    va_list va;

	/** Add the opening tag **/
	htrAddBodyItemLayerStart(s, flags, id, cnt);

	/** Add the content **/
	va_start(va, fmt);
	htr_internal_QPAddText(s, htrAddBodyItem, (char*)fmt, va);
	va_end(va);

	/** Add the closing tag **/
	htrAddBodyItemLayerEnd(s, flags);

    return 0;
    }


/*** htrAddBodyItemLayerStart - adds just the opening tag sequence
 *** but not some content for a layer.  Does not add the closing tag sequence
 *** for the layer.
 ***
 *** WARNING!!! DO NOT ALLOW THE END-USER TO INFLUENCE THE VALUE OF THE 'id'
 *** PARAMETER WHICH IS A FORMAT STRING FOR THE LAYER'S ID!!!
 ***/
int
htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt)
    {
    char* starttag;
    char id_sbuf[64];

	if(s->Capabilities.HTML40)
	    {
	    if (flags & HTR_LAYER_F_DYNAMIC)
		starttag = "IFRAME frameBorder=\"0\"";
	    else
		starttag = "DIV";
	    }
	else
	    {
	    starttag = "DIV";
	    }

	/** Add it. **/
	qpfPrintf(NULL, id_sbuf,sizeof(id_sbuf),id,cnt);
	htrAddBodyItem_va(s, "<%STR id=\"%STR&HTE\">", starttag, id_sbuf);

    return 0;
    }


/*** htrAddBodyItemLayerEnd - adds the ending tag
 *** for a layer.  Does not emit the starting tag.
 ***/
int
htrAddBodyItemLayerEnd(pHtSession s, int flags)
    {
    char* endtag;

	if(s->Capabilities.HTML40)
	    {
	    if (flags & HTR_LAYER_F_DYNAMIC)
		endtag = "IFRAME";
	    else
		endtag = "DIV";
	    }
	else
	    {
	    endtag = "DIV";
	    }

	/** Add it. **/
	htrAddBodyItem_va(s, "</%STR>", endtag);

    return 0;
    }


/*** htrAddExpression - adds an expression to control a given property of
 *** an object.  When any object reference in the expression changes, the
 *** expression will be re-run to modify the object's property.  The
 *** object in question can then put a watchpoint on the property, causing
 *** actual actions to occur based on changes in the value of the expression
 *** during application operation.
 ***/
int
htrAddExpression(pHtSession s, char* objname, char* property, pExpression exp)
    {
    int i,first;
    XArray objs, props;
    XString xs,exptxt;
    char* obj;
    char* prop;

	xaInit(&objs, 16);
	xaInit(&props, 16);
	xsInit(&xs);
	xsInit(&exptxt);
	expGetPropList(exp, &objs, &props);

	xsCopy(&xs,"new Array(",-1);
	first=1;
	for(i=0;i<objs.nItems;i++)
	    {
	    obj = (char*)(objs.Items[i]);
	    prop = (char*)(props.Items[i]);
	    if (obj && prop)
		{
		xsConcatQPrintf(&xs,"%[,%]new Array('%STR&SYM','%STR&SYM')", !first, obj, prop);
		first = 0;
		}
	    }
	xsConcatenate(&xs,")",1);
	expGenerateText(exp, NULL, xsWrite, &exptxt, '\\', "javascript");
	htrAddExpressionItem_va(s, "    pg_expression('%STR&SYM','%STR&SYM','%STR&ESCQ',%STR,'%STR&SYM');\n", objname, property, exptxt.String, xs.String, s->Namespace->DName);

	for(i=0;i<objs.nItems;i++)
	    {
	    if (objs.Items[i]) nmSysFree(objs.Items[i]);
	    if (props.Items[i]) nmSysFree(props.Items[i]);
	    }
	xaDeInit(&objs);
	xaDeInit(&props);
	xsDeInit(&xs);
	xsDeInit(&exptxt);

    return 0;
    }


/*** htrCheckAddExpression - checks if an expression should be added for a
 *** given widget property, and deploys it to the client if so.
 ***/
int
htrCheckAddExpression(pHtSession s, pWgtrNode tree, char* w_name, char* property)
    {
    pExpression code;

        if (wgtrGetPropertyType(tree,property) == DATA_T_CODE)
            {
            wgtrGetPropertyValue(tree,property,DATA_T_CODE,POD(&code));
            htrAddExpression(s, w_name, property, code);
            }

    return 0;
    }


/*** htrRenderSubwidgets - generates the code for all subwidgets within
 *** the current widget.  This is  a generic function that does not
 *** necessarily apply to all widgets that contain other widgets, but
 *** is useful for your basic ordinary "container" type widget, such
 *** as panes and tab pages.
 ***/
int
htrRenderSubwidgets(pHtSession s, pWgtrNode widget, int zlevel)
    {
//    pObjQuery qy;
//    pObject sub_widget_obj;
    int i, count;

	/** Open the query for subwidgets **/
	/*
	qy = objOpenQuery(widget_obj, "", NULL, NULL, NULL);
	if (qy)
	    {
	    while((sub_widget_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_widget_obj, zlevel, docname, layername);
		objClose(sub_widget_obj);
		}
	    objQueryClose(qy);
	    }
	**/
	count = xaCount(&(widget->Children));
	for (i=0;i<count;i++) 
	    htrRenderWidget(s, xaGetItem(&(widget->Children), i), zlevel);

    return 0;
    }


/*** htr_internal_GenInclude - generate an include statement for an
 *** external javascript file; or if collapse_includes is set, insert
 *** the thing directly in the output.  If file cannot be opened,
 *** then an include statement is generated in any event.
 ***/
int
htr_internal_GenInclude(pFile output, pHtSession s, char* filename)
    {
    pStruct c_param;
    pObject include_file;
    char buf[256];
    int rcnt;

	/** Insert file directly? **/
	c_param = stLookup_ne(s->Params, "ls__collapse_includes");
	if (c_param && !strcasecmp(c_param->StrVal,"yes"))
	    {
	    include_file = objOpen(s->ObjSession, filename, O_RDONLY, 0600, "application/x-javascript");
	    if (include_file)
		{
		fdPrintf(output, "<SCRIPT language=\"javascript\" DEFER>\n// Included from: %s\n\n", filename);
		while((rcnt = objRead(include_file, buf, sizeof(buf), 0, 0)) > 0)
		    {
		    fdWrite(output, buf, rcnt, 0, FD_U_PACKET);
		    }
		objClose(include_file);
		fdPrintf(output, "\n</SCRIPT>\n");
		return 0;
		}
	    }

	/** Otherwise, just generate an include statement **/
	fdPrintf(output, "\n<SCRIPT language=\"javascript\" src=\"%s\" DEFER></SCRIPT>\n", filename);

    return 0;
    }

#if 00
/*** htr_internal_BuildClientWgtr_r - the recursive part of client-side wgtr generation
 ***/
int
htr_internal_BuildClientWgtr_r(pHtSession s, pWgtrNode tree)
    {
    int i;
    char visual[6];

	/** visual or non-visual? **/
	if (tree->Flags & WGTR_F_NONVISUAL) sprintf(visual, "false");
	else sprintf(visual, "true");

	/** create an object for this node if there isn't one **/
	/** NOTE: yes, it is indeed possible to have a visual widget without a layer.
	 ** Static tables, for example (AAARRGGGGG - MJM)
	 **/
	if (tree->RenderFlags & HT_WGTF_NOOBJECT)
	    {
	    htrAddScriptWgtr_va(s, "    %s = new Object();\n", tree->Name);
	    }
	htrAddScriptWgtr_va(s, "    wgtrAddToTree(%s, '%s', '%s', curr_node[0], %s);\n", 
	    tree->Name, tree->Name, tree->Type, visual);

	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    if (!(tree->RenderFlags & HT_WGTF_NOOBJECT)) 
		htrAddScriptWgtr_va(s, "    curr_node.unshift(%s);\n", tree->Name, tree->Name);
	    htr_internal_BuildClientWgtr_r(s, xaGetItem(&(tree->Children), i));		
	    if (!(tree->RenderFlags & HT_WGTF_NOOBJECT)) 
		htrAddScriptWgtr_va(s, "    curr_node.shift();\n", tree->Name);
	    }
    }

/*** htr_internal_BuildClientWgtr - responsible for generating the DHTML to build
 *** a client-side representation of the widget tree.
 ***/
int
htr_internal_BuildClientWgtr(pHtSession s, pWgtrNode tree)
    {
	htrAddScriptWgtr(s, "    var client_node;\n");
	htrAddScriptWgtr(s, "    var curr_node = new Array(0)\n");
	htrAddScriptInclude(s, "/sys/js/ht_utils_wgtr.js", 0);

	return htr_internal_BuildClientWgtr_r(s, tree);
    }
#endif

/*** htr_internal_BuildClientWgtr - generate the DHTML to represent the widget
 *** tree.
 ***/
int
htr_internal_BuildClientWgtr_r(pHtSession s, pWgtrNode tree, int indent)
    {
    int i;
    int childcnt = xaCount(&tree->Children);
    char* objinit;
    char* ctrinit;
    pHtDMPrivateData inf = wgtrGetDMPrivateData(tree);

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"HTR","Could not render application: resource exhaustion occurred");
	    return -1;
	    }

	objinit = inf?(inf->ObjectLinkage):NULL;
	ctrinit = inf?(inf->ContainerLinkage):NULL;
	htrAddScriptWgtr_va(s, 
		"        %STR&*LEN{name:'%STR&SYM', obj:%STR, cobj:%STR, type:'%STR&ESCQ', vis:%STR, sub:", 
		indent*4, "                                        ",
		tree->Name, objinit?objinit:"\"new Object()\"",
		ctrinit?ctrinit:"\"_obj\"",
		tree->Type, (tree->Flags & WGTR_F_NONVISUAL)?"false":"true");

	if (childcnt)
	    {
	    htrAddScriptWgtr_va(s, "\n        %STR&*LEN    [\n", indent*4, "                                        ");
	    for(i=0;i<childcnt;i++)
		{
		if (htr_internal_BuildClientWgtr_r(s, xaGetItem(&(tree->Children), i), indent+1) < 0)
		    return -1;
		if (i == childcnt-1) 
		    htrAddScriptWgtr(s, "\n");
		else
		    htrAddScriptWgtr(s, ",\n");
		}
	    htrAddScriptWgtr_va(s, "        %STR&*LEN    ] }", indent*4, "                                        ");
	    }
	else
	    {
	    htrAddScriptWgtr(s, "[] }");
	    }

    return 0;
    }

int
htrBuildClientWgtr(pHtSession s, pWgtrNode tree)
    {

	htrAddScriptInclude(s, "/sys/js/ht_utils_wgtr.js", 0);
	htrAddScriptWgtr_va(s, "    pre_%STR&SYM =\n", tree->DName);
	htr_internal_BuildClientWgtr_r(s, tree, 0);
	htrAddScriptWgtr(s, ";\n");

    return 0;
    }


/*** htr_internal_InitNamespace() - generate the code to build the namespace
 *** initializations on the client.
 ***/
int
htr_internal_InitNamespace(pHtSession s, pHtNamespace ns)
    {
    pHtNamespace child;

	/** If this namespace is within another, link to that in the tree
	 ** init by setting 'cobj', otherwise leave the parent linkage totally
	 ** empty.
	 **/
	if (ns->ParentCtr[0] && ns->Parent)
	    htrAddScriptWgtr_va(s, "    %STR&SYM = wgtrSetupTree(pre_%STR&SYM, \"%STR&SYM\", {cobj:wgtrGetContainer(wgtrGetNode(%STR&SYM,\"%STR&SYM\"))});\n",
		    ns->DName, ns->DName, ns->DName, ns->Parent->DName, ns->ParentCtr);
	else
	    htrAddScriptWgtr_va(s, "    %STR&SYM = wgtrSetupTree(pre_%STR&SYM, \"%STR&SYM\", null);\n", 
		    ns->DName, ns->DName, ns->DName);
	htrAddScriptWgtr_va(s, "    pg_namespaces[\"%STR&SYM\"] = %STR&SYM;\n",
		ns->DName, ns->DName);

	/** Init child namespaces too **/
	for(child = ns->FirstChild; child; child=child->NextSibling)
	    {
	    htr_internal_InitNamespace(s, child);
	    }

    return 0;
    }


int
htr_internal_FreeNamespace(pHtNamespace ns)
    {
    pHtNamespace child, next;

	/** if 'ns' does not have a parent, then it is builtin to the session
	 ** 'page' structure and need not be freed.
	 **/
	child = ns->FirstChild;
	if (ns->Parent) nmFree(ns, sizeof(HtNamespace));

	/** Free up sub namespaces too **/
	while(child)
	    {
	    next = child->NextSibling;
	    htr_internal_FreeNamespace(child);
	    child = next;
	    }

    return 0;
    }


/*** htrRender - generate an HTML document given the app structure subtree
 *** as an open ObjectSystem object.
 ***/
int
htrRender(pFile output, pObjSession obj_s, pWgtrNode tree, pStruct params, pWgtrClientInfo c_info)
    {
    pHtSession s;
    int i,n,j,k,cnt,cnt2;
    pStrValue tmp;
    char* ptr;
    pStrValue sv;
    char sbuf[HT_SBUF_SIZE];
    char ename[40];
    pHtDomEvent e;
    char* agent = NULL;
    char* classname = NULL;
    int rval;

	/** What UA is on the other end of the connection? **/
	agent = (char*)mssGetParam("User-Agent");
	if (!agent)
	    {
	    mssError(1, "HTR", "User-Agent undefined in the session parameters");
	    return -1;
	    }

    	/** Initialize the session **/
	s = (pHtSession)nmMalloc(sizeof(HtSession));
	if (!s) return -1;
	memset(s,0,sizeof(HtSession));
	s->Params = params;
//	s->ObjSession = appstruct->Session;
	s->ObjSession = obj_s;
	s->ClientInfo = c_info;
	s->Namespace = &(s->Page.RootNamespace);
	strtcpy(s->Namespace->DName, wgtrGetRootDName(tree), sizeof(s->Namespace->DName));

	/** Parent container name specified? **/
	if ((ptr = htrParamValue(s, "cx__graft")))
	    s->GraftPoint = nmSysStrdup(ptr);
	else
	    s->GraftPoint = NULL;

	/** Did user request a class of widgets? **/
	classname = (char*)mssGetParam("Class");
	if (classname)
	    {
	    s->Class = (pHtClass)xhLookup(&(HTR.Classes), classname);
	    if(!s->Class)
		mssError(1,"HTR","Warning: class %s is not defined... acting like it wasn't specified",classname);
	    }
	else
	    s->Class = NULL;

	/** find the right capabilities for the class we're using **/
	if(s->Class)
	    {
	    pHtCapabilities pCap = htr_internal_GetBrowserCapabilities(agent, s->Class);
	    if(pCap)
		{
		s->Capabilities = *pCap;
		}
	    else
		{
		mssError(1,"HTR","no capabilities found for %s in class %s",agent, s->Class->ClassName);
		memset(&(s->Capabilities),0,sizeof(HtCapabilities));
		}
	    }
	else
	    {
	    /** somehow decide on a widget priority, and go down the list
		till you find a workable one **/
	    /** for now, I'm going to get them in the order they are in the hash,
		which is no order at all :) **/
	    /** also, this sets the class when it finds capabilities....
		is that a good thing? -- not sure **/
	    int i;
	    pHtCapabilities pCap = NULL;
	    for(i=0;i<HTR.Classes.nRows && !pCap;i++)
		{
		pXHashEntry ptr = (pXHashEntry)xaGetItem(&(HTR.Classes.Rows),i);
		while(ptr && !pCap)
		    {
		    pCap = htr_internal_GetBrowserCapabilities(agent, (pHtClass)ptr->Data);
		    if(pCap)
			{
			s->Class = (pHtClass)ptr->Data;
			}
		    ptr = ptr->Next;
		    }
		}
	    if(pCap)
		{
		s->Capabilities = *pCap;
		}
	    else
		{
		mssError(1,"HTR","no capabilities found for %s in any class",agent);
		memset(&(s->Capabilities),0,sizeof(HtCapabilities));
		}
	    }

	/** Setup the page structures **/
	s->Tmpbuf = nmSysMalloc(512);
	s->TmpbufSize = 512;
	if (!s->Tmpbuf)
	    {
	    if (s->GraftPoint) nmSysFree(s->GraftPoint);
	    nmFree(s, sizeof(HtSession));
	    return -1;
	    }
	xhInit(&(s->Page.NameFunctions),257,0);
	xaInit(&(s->Page.Functions),32);
	xhInit(&(s->Page.NameIncludes),257,0);
	xaInit(&(s->Page.Includes),32);
	xhInit(&(s->Page.NameGlobals),257,0);
	xaInit(&(s->Page.Globals),64);
	xaInit(&(s->Page.Inits),64);
	xaInit(&(s->Page.Cleanups),64);
	xaInit(&(s->Page.HtmlBody),64);
	xaInit(&(s->Page.HtmlHeader),64);
	xaInit(&(s->Page.HtmlStylesheet),64);
	xaInit(&(s->Page.HtmlBodyParams),16);
	xaInit(&(s->Page.HtmlExpressionInit),16);
	/*xaInit(&(s->Page.EventScripts.Array),16);
	xhInit(&(s->Page.EventScripts.HashTable),257,0);*/
	xaInit(&s->Page.EventHandlers,16);
	xaInit(&(s->Page.Wgtr), 64);
	s->Page.HtmlBodyFile = NULL;
	s->Page.HtmlHeaderFile = NULL;
	s->Page.HtmlStylesheetFile = NULL;
	s->DisableBody = 0;

	/** first thing in the startup() function should be calling build_wgtr **/
	htrAddScriptInit_va(s, "    build_wgtr_%STR&SYM();\n",
		s->Namespace->DName);
	htrAddScriptInit_va(s, "    var nodes = wgtrNodeList(%STR&SYM);\n"
			       "    var rootname = \"%STR&SYM\";\n",
		s->Namespace->DName, s->Namespace->DName);

	/** Set the application key **/
	htrAddScriptInit_va(s, "    akey = '%STR&ESCQ';\n", c_info->AKey);

	/** Render the top-level widget. **/
	rval = htrRenderWidget(s, tree, 10);

	/** Assemble the various objects into a widget tree **/
	htrBuildClientWgtr(s, tree);

	/** Generate the namespace initialization. **/
	htr_internal_InitNamespace(s, s->Namespace);

	htr_internal_FreeNamespace(s->Namespace);

	/** Add wgtr debug window **/
#ifdef WGTR_DBG_WINDOW
	htrAddScriptWgtr_va(s, "    wgtrWalk(%STR&SYM);\n", tree->Name);
	htrAddScriptWgtr(s, "    ifcLoadDef(\"net/centrallix/button.ifc\");\n");
	htrAddStylesheetItem(s, "\t#dbgwnd {position: absolute; top: 400; left: 50;}\n");
	htrAddBodyItem(s,   "<div id=\"dbgwnd\"><form name=\"dbgform\">"
			    "<textarea name=\"dbgtxt\" cols=\"80\" rows=\"10\"></textarea>"
			    "</form></div>\n");
#endif


	if (rval < 0)
	    {
	    fdPrintf(output, "<HTML><HEAD><TITLE>Error</TITLE></HEAD><BODY bgcolor=\"white\"><h1>An Error occured while attempting to render this document</h1><br><pre>");
	    mssPrintError(output);
	    }
	
	/** Output the DOCTYPE for browsers supporting HTML 4.0 -- this will make them use HTML 4.0 Strict **/
	/** FIXME: should probably specify the DTD.... **/
	if(s->Capabilities.HTML40 && !s->Capabilities.Dom0IE)
	    fdWrite(output, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n",91,0,FD_U_PACKET);

	/** Write the HTML out... **/
	snprintf(sbuf, HT_SBUF_SIZE, "<!--\nGenerated by Centrallix v%s (http://www.centrallix.org)\n"
				     "(c) 1998-2006 by LightSys Technology Services, Inc.\n\n", PACKAGE_VERSION);
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "This DHTML document contains Javascript and other DHTML\n"
				     "generated from Centrallix which is licensed under the\n"
				     "GNU GPL (http://www.gnu.org/licenses/gpl.txt).  Any copying\n");
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "modifying, or redistributing of this generated code falls\n"
				     "under the restrictions of the GPL.\n"
				     "-->\n");
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	fdWrite(output, "<HTML>\n<HEAD>\n",14,0,FD_U_PACKET);
	snprintf(sbuf, HT_SBUF_SIZE, "    <META NAME=\"Generator\" CONTENT=\"Centrallix v%s\">\n", cx__version);
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	fdPrintf(output, "    <META NAME=\"Pragma\" CONTENT=\"no-cache\">\n");

	fdWrite(output, "    <STYLE TYPE=\"text/css\">\n", 28, 0, FD_U_PACKET);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlStylesheet.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output, "    </STYLE>\n", 13, 0, FD_U_PACKET);
	/** Write the HTML header items. **/
	for(i=0;i<s->Page.HtmlHeader.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlHeader.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }

	/** Write the script globals **/
	fdWrite(output, "<SCRIPT language=\"javascript\" DEFER>\n\n\n", 39,0,FD_U_PACKET);
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Globals.Items[i]);
	    if (sv->Value[0])
		qpfPrintf(NULL, sbuf,HT_SBUF_SIZE,"if (typeof %STR&SYM == 'undefined') var %STR&SYM = %STR;\n", sv->Name, sv->Name, sv->Value);
	    else
		qpfPrintf(NULL, sbuf,HT_SBUF_SIZE,"var %STR&SYM;\n", sv->Name);
	    fdWrite(output, sbuf, strlen(sbuf),0,FD_U_PACKET);
	    }

	/** Write the includes **/
	fdWrite(output, "\n</SCRIPT>\n\n", 12,0,FD_U_PACKET);

	/** include ht_render.js **/
	htr_internal_GenInclude(output, s, "/sys/js/ht_render.js");

	/** include browser-specific geometry js **/
	if(s->Capabilities.Dom0IE)
	    {
	    htr_internal_GenInclude(output, s, "/sys/js/ht_geom_dom0ie.js");
	    }
	else if (s->Capabilities.Dom0NS)
	    {
	    htr_internal_GenInclude(output, s, "/sys/js/ht_geom_dom0ns.js");
	    }
	else if (s->Capabilities.Dom1HTML)
	    {
	    htr_internal_GenInclude(output, s, "/sys/js/ht_geom_dom1html.js");
	    }
	else
	    {
	    /** cannot render **/
	    }

	for(i=0;i<s->Page.Includes.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Includes.Items[i]);
	    htr_internal_GenInclude(output, s, sv->Name);
	    }
	fdWrite(output, "<SCRIPT language=\"javascript\" DEFER>\n\n", 38,0,FD_U_PACKET);

	/** Write the script functions **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    sv = (pStrValue)(s->Page.Functions.Items[i]);
	    fdWrite(output, sv->Value, strlen(sv->Value),0,FD_U_PACKET);
	    }

	/** Write the event scripts themselves. **/
	/*for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
	        snprintf(sbuf,HT_SBUF_SIZE,"\nfunction e%d_%d(e)\n    {\n",i,j);
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
	        snprintf(sbuf,HT_SBUF_SIZE,"    var e = htr_event(e);\n    var ly = (typeof e.target.layer != \"undefined\" && e.target.layer != null)?e.target.layer:e.target;\n    var handler_return;\n    var prevent_default=false;\n");
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
		for(k=0;k<tmp_a2->Array.nItems;k++)
		    {
		    tmp_a3 = (pHtNameArray)(tmp_a2->Array.Items[k]);
		    for(l=0;l<tmp_a3->Array.nItems;l++)
		        {
		        ptr = (char*)(tmp_a3->Array.Items[l]);
		        fdWrite(output,ptr+8,*(int*)ptr,0,FD_U_PACKET);
			}
		    }
		fdPrintf(output,"    return !prevent_default;\n    }\n");
		}
	    }*/

	/** Link up the events **/

	/** Write the event capture lines **/
	fdPrintf(output,"\nfunction events_%s()\n    {\n",s->Namespace->DName);
	cnt = xaCount(&s->Page.EventHandlers);
	strcpy(sbuf,"    if(window.Event)\n        htr_captureevents(");
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    if (i) strcat(sbuf, " | ");
	    strcat(sbuf,"Event.");
	    strcat(sbuf,e->DomEvent);
	    }
	strcat(sbuf,");\n");
	fdWrite(output, sbuf, strlen(sbuf), 0, FD_U_PACKET);
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    n = strlen(e->DomEvent);
	    if (n >= sizeof(ename)) n = sizeof(ename)-1;
	    for(k=0;k<=n;k++) ename[k] = tolower(e->DomEvent[k]);
	    ename[k] = '\0';
	    cnt2 = xaCount(&e->Handlers);
	    for(j=0;j<cnt2;j++)
		{
		fdPrintf(output, "    htr_addeventhandler(\"%s\",\"%s\");\n",
			ename, xaGetItem(&e->Handlers, j));
		}
	    if (!strcmp(ename,"mousemove"))
		fdPrintf(output, "    htr_addeventlistener('%s', document, htr_mousemovehandler);\n",
			ename);
	    else
		fdPrintf(output, "    htr_addeventlistener('%s', document, htr_eventhandler);\n",
			ename);
	    }
#if 00
	for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    snprintf(sbuf,HT_SBUF_SIZE,"    if(window.Event)\n    %.64s.captureEvents(",tmp_a->Name);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		if (j!=0) strcat(sbuf," | ");
		strcat(sbuf,"Event.");
		strcat(sbuf,tmp_a2->Name);
		}
	    strcat(sbuf,");\n");
	    fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		n = strlen(tmp_a2->Name);
		if (n >= sizeof(ename)) n = sizeof(ename)-1;
		for(k=0;k<=n;k++) ename[k] = tolower(tmp_a2->Name[k]);
		ename[k] = '\0';
		/*snprintf(sbuf,HT_SBUF_SIZE,"    %.64s.on%s=e%d_%d;\n",tmp_a->Name,ename,i,j);
		fdWrite(output,sbuf,strlen(sbuf),0,FD_U_PACKET);*/
		if (!strcmp(ename,"mousemove"))
		    fdPrintf(output, "    %.64s.on%s = htr_mousemovehandler;\n",
			tmp_a->Name, ename);
		else
		    fdPrintf(output, "    %.64s.on%s = htr_eventhandler;\n",
			tmp_a->Name, ename);
		}
	    }
#endif

	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the expression initializations **/
	fdPrintf(output,"\nfunction expinit_%s()\n    {\n",s->Namespace->DName);
	for(i=0;i<s->Page.HtmlExpressionInit.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlExpressionInit.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the wgtr declaration **/
	fdPrintf(output, "\nfunction build_wgtr_%s()\n    {\n",
		s->Namespace->DName);
	for (i=0;i<s->Page.Wgtr.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Wgtr.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n, 0, FD_U_PACKET);
	    }
	fdWrite(output, "    }\n", 6, 0, FD_U_PACKET);

	/** Write the initialization lines **/
	fdPrintf(output,"\nfunction startup_%s()\n    {\n", s->Namespace->DName);
	htr_internal_writeCxCapabilities(s,output);

	for(i=0;i<s->Page.Inits.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Inits.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdPrintf(output,"    events_%s();\n", s->Namespace->DName);
	fdPrintf(output,"    expinit_%s();\n", s->Namespace->DName);
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);

	/** Write the cleanup lines **/
	fdWrite(output,"\nfunction cleanup()\n    {\n",26,0,FD_U_PACKET);
	for(i=0;i<s->Page.Cleanups.nItems;i++)
	    {
	    ptr = (char*)(s->Page.Cleanups.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }
	fdWrite(output,"    }\n",6,0,FD_U_PACKET);


	/** If the body part is disabled, skip over body section generation **/
	if (s->DisableBody == 0)
	    {
	    /** Write the HTML body params **/
	    fdWrite(output, "\n</SCRIPT>\n</HEAD>",18,0,FD_U_PACKET);
	    fdWrite(output, "\n<BODY", 6,0,FD_U_PACKET);
	    for(i=0;i<s->Page.HtmlBodyParams.nItems;i++)
	        {
	        ptr = (char*)(s->Page.HtmlBodyParams.Items[i]);
	        n = *(int*)ptr;
	        fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	        }
	    /** work around the Netscape 4.x bug regarding page resizing **/
	    if(s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
		{
		fdWrite(output, " onResize=\"location.reload()\"",29,0,FD_U_PACKET);
		}
	    /*fdPrintf(output, " onLoad=\"startup();\" onUnload=\"cleanup();\"");*/
	    fdPrintf(output, ">\n");
	    }
	else
	    {
	    fdWrite(output, "\n</SCRIPT>\n",11,0,FD_U_PACKET);
	    }

	/** Write the HTML body. **/
	for(i=0;i<s->Page.HtmlBody.nItems;i++)
	    {
	    ptr = (char*)(s->Page.HtmlBody.Items[i]);
	    n = *(int*)ptr;
	    fdWrite(output, ptr+8, n,0,FD_U_PACKET);
	    }

	if (s->DisableBody == 0)
	    {
	    fdWrite(output, "</BODY>\n</HTML>\n",16,0,FD_U_PACKET);
	    }
	else
	    {
	    fdWrite(output, "\n</HTML>\n",9,0,FD_U_PACKET);
	    }

	/** Deinitialize the session and page structures **/
	for(i=0;i<s->Page.Functions.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Functions.Items[i]);
	    xhRemove(&(s->Page.NameFunctions),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    if (tmp->Alloc & HTR_F_VALUEALLOC) nmFree(tmp->Value,tmp->ValueSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Functions));
	xhDeInit(&(s->Page.NameFunctions));
	for(i=0;i<s->Page.Includes.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Includes.Items[i]);
	    xhRemove(&(s->Page.NameIncludes),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Includes));
	xhDeInit(&(s->Page.NameIncludes));
	for(i=0;i<s->Page.Globals.nItems;i++)
	    {
	    tmp = (pStrValue)(s->Page.Globals.Items[i]);
	    xhRemove(&(s->Page.NameGlobals),tmp->Name);
	    if (tmp->Alloc & HTR_F_NAMEALLOC) nmFree(tmp->Name,tmp->NameSize);
	    if (tmp->Alloc & HTR_F_VALUEALLOC) nmFree(tmp->Value,tmp->ValueSize);
	    nmFree(tmp,sizeof(StrValue));
	    }
	xaDeInit(&(s->Page.Globals));
	xhDeInit(&(s->Page.NameGlobals));
	for(i=0;i<s->Page.Inits.nItems;i++) nmFree(s->Page.Inits.Items[i],2048);
	xaDeInit(&(s->Page.Inits));
	for (i=0;i<s->Page.Wgtr.nItems;i++) nmFree(s->Page.Wgtr.Items[i], 2048);
	xaDeInit(&(s->Page.Wgtr));
	for(i=0;i<s->Page.Cleanups.nItems;i++) nmFree(s->Page.Cleanups.Items[i],2048);
	xaDeInit(&(s->Page.Cleanups));
	for(i=0;i<s->Page.HtmlBody.nItems;i++) nmFree(s->Page.HtmlBody.Items[i],2048);
	xaDeInit(&(s->Page.HtmlBody));
	for(i=0;i<s->Page.HtmlHeader.nItems;i++) nmFree(s->Page.HtmlHeader.Items[i],2048);
	xaDeInit(&(s->Page.HtmlHeader));
	for(i=0;i<s->Page.HtmlStylesheet.nItems;i++) nmFree(s->Page.HtmlStylesheet.Items[i],2048);
	xaDeInit(&(s->Page.HtmlStylesheet));
	for(i=0;i<s->Page.HtmlBodyParams.nItems;i++) nmFree(s->Page.HtmlBodyParams.Items[i],2048);
	xaDeInit(&(s->Page.HtmlBodyParams));
	for(i=0;i<s->Page.HtmlExpressionInit.nItems;i++) nmFree(s->Page.HtmlExpressionInit.Items[i],2048);
	xaDeInit(&(s->Page.HtmlExpressionInit));

	/** Clean up the event script structure, which is multi-level. **/
	/*for(i=0;i<s->Page.EventScripts.Array.nItems;i++)
	    {
	    tmp_a = (pHtNameArray)(s->Page.EventScripts.Array.Items[i]);
	    for(j=0;j<tmp_a->Array.nItems;j++)
	        {
	        tmp_a2 = (pHtNameArray)(tmp_a->Array.Items[j]);
		for(k=0;k<tmp_a2->Array.nItems;k++)
		    {
		    tmp_a3 = (pHtNameArray)(tmp_a2->Array.Items[k]);
		    for(l=0;l<tmp_a3->Array.nItems;l++)
		        {
			nmFree(tmp_a3->Array.Items[l],2048);
			}
	            xaDeInit(&(tmp_a3->Array));
	            xhRemove(&(tmp_a2->HashTable),tmp_a3->Name);
		    nmFree(tmp_a3,sizeof(HtNameArray));
		    }
	        xaDeInit(&(tmp_a2->Array));
		xhDeInit(&(tmp_a2->HashTable));
	        xhRemove(&(tmp_a->HashTable),tmp_a2->Name);
	        nmFree(tmp_a2,sizeof(HtNameArray));
		}
	    xaDeInit(&(tmp_a->Array));
	    xhDeInit(&(tmp_a->HashTable));
	    xhRemove(&(s->Page.EventScripts.HashTable),tmp_a->Name);
	    nmFree(tmp_a,sizeof(HtNameArray));
	    }
	xhDeInit(&(s->Page.EventScripts.HashTable));
	xaDeInit(&(s->Page.EventScripts.Array));*/
	cnt = xaCount(&s->Page.EventHandlers);
	for(i=0;i<cnt;i++)
	    {
	    e = (pHtDomEvent)xaGetItem(&s->Page.EventHandlers,i);
	    /** these must all be string constants; no need to free **/
	    /*cnt2 = xaCount(&e->Handlers);
	    for(j=0;j<cnt2;j++)
		{
		nmSysFree((char*)xaGetItem(&e->Handlers, j));
		}*/
	    xaDeInit(&e->Handlers);
	    nmFree(e, sizeof(HtDomEvent));
	    }
	xaDeInit(&s->Page.EventHandlers);

	nmSysFree(s->Tmpbuf);

	if (s->GraftPoint) nmSysFree(s->GraftPoint);

	htr_internal_FreeDMPrivateData(tree);

	nmFree(s,sizeof(HtSession));

    return 0;
    }


/*** htrAllocDriver - allocates a driver structure that can later be
 *** registered by using the next function below, htrRegisterDriver().
 ***/
pHtDriver
htrAllocDriver()
    {
    pHtDriver drv;

	/** Allocate the driver structure **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return NULL;

	/** Init some of the basic array structures **/
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	xaInit(&(drv->PseudoTypes), 4);
    return drv;
    }


/*** htrAddSupport - adds support for a class to a driver (by telling the class)
 ***   note: _must_ be called after the driver registers
 ***/
int
htrAddSupport(pHtDriver drv, char* className)
    {
    int i;

	pHtClass class = (pHtClass)xhLookup(&(HTR.Classes),className);
	if(!class)
	    {
	    mssError(1,"HTR","unable to find class '%s' for widget driver '%s'",className,drv->WidgetName);
	    return -1;
	    }
	/** Add main type name to hash **/
	if (xhAdd(&(class->WidgetDrivers),drv->WidgetName, (void*)drv) < 0) return -1;

	/** Add pseudo-type names to hash **/
	for (i=0;i<xaCount(&(drv->PseudoTypes));i++)
	    {
	    if (xhAdd(&(class->WidgetDrivers),xaGetItem(&(drv->PseudoTypes), i), (void*)drv) < 0) return -1;
	    }

    return 0;
    }

/*** htrRegisterDriver - register a new driver with the rendering system
 *** and map the widget name to the driver's structure for later access.
 ***/
int
htrRegisterDriver(pHtDriver drv)
    {
    	/** Add to the drivers listing and the widget name map. **/
	xaAddItem(&(HTR.Drivers),(void*)drv);

	/** Add some entries to our hash, for fast driver look-up **/

    return 0;
    }

/*** htrLookupDriver - returns the proper driver for a given type.
 ***/
pHtDriver
htrLookupDriver(pHtSession s, char* type_name)
    {
    pXHashTable widget_drivers = NULL;

	if (!s->Class)
	    {
	    mssError(1, "HTR", "Class not defined in HtSession!");
	    return NULL;
	    }
	widget_drivers = &(s->Class->WidgetDrivers);
	if (!widget_drivers)
	    {
	    mssError(1, "HTR", "No widgets defined for useragent/class combo");
	    return NULL;
	    }
	return (pHtDriver)xhLookup(widget_drivers, type_name+7);
    }

/*** htrInitialize - initialize the system and the global variables and
 *** structures.
 ***/
int
htrInitialize()
    {
    	/** Initialize the global hash tables and arrays **/
	xaInit(&(HTR.Drivers),64);
	xhInit(&(HTR.Classes), 63, 0);

	/** Register the classes, user agents and the regular expressions to match them.  **/
	htrRegisterUserAgents();
	wgtrAddDeploymentMethod("DHTML", htrRender);

    return 0;
    }


/*** htrGetBackground - gets the background image or color from the config
 *** and converts it into a string we can use in the DHTML
 ***
 *** obj - an open Object for the config data
 *** prefix - the prefix to add to 'bgcolor' and 'background' for the attrs
 *** as_style - set to 1 to build the string as a style rather than as HTML
 *** buf - the buffer to print into
 *** buflen - the buffer length we can use
 ***
 *** returns with buf set to "" if any error occurs.
 ***/
int
htrGetBackground(pWgtrNode tree, char* prefix, int as_style, char* buf, int buflen)
    {
    char bgcolor_name[64];
    char background_name[128];
    char* bgcolor = "bgcolor";
    char* background = "background";
    char* ptr;

	/** init buf **/
	if (buflen < 1) return -1;
	buf[0] = '\0';

	/** Prefix supplied? **/
	if (prefix && *prefix)
	    {
	    qpfPrintf(NULL, bgcolor_name,sizeof(bgcolor_name),"%STR&SYM_bgcolor",prefix);
	    qpfPrintf(NULL, background_name,sizeof(background_name),"%STR&SYM_background",prefix);
	    bgcolor = bgcolor_name;
	    background = background_name;
	    }

	/** Image? **/
	if (wgtrGetPropertyValue(tree, background, DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (strpbrk(ptr,"\"'\n\r\t")) return -1;
	    if (as_style)
		qpfPrintf(NULL, buf,buflen,"background-image: URL('%STR&HTE');",ptr);
	    else
		qpfPrintf(NULL, buf,buflen,"background='%STR&HTE'",ptr);
	    }
	else if (wgtrGetPropertyValue(tree, bgcolor, DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    /** Background color **/
	    if (strpbrk(ptr,"\"'\n\r\t;}<>&")) return -1;
	    if (as_style)
		qpfPrintf(NULL, buf,buflen,"background-color: %STR&HTE;",ptr);
	    else
		qpfPrintf(NULL, buf,buflen,"bgColor='%STR&HTE'",ptr);
	    }
	else
	    {
	    return -1;
	    }

    return 0;
    }


/*** htrGetBoolean() - a convenience routine to retrieve a boolean value
 *** from the structure file.  Boolean values can be specified as yes/no,
 *** true/false, on/off, y/n, or 1/0.  This routine checks for those.
 *** Return value = 1 if true, 0 if false, -1 if error, and default_value
 *** if not found.  default_value can be set to -1 to indicate an error
 *** on a nonexistent attribute.
 ***/
int
htrGetBoolean(pWgtrNode wgt, char* attrname, int default_value)
    {
    int t;
    int rval = default_value;
    char* ptr;
    int n;

	/** type of attr (need to check number if 1/0) **/
	t = wgtrGetPropertyType(wgt,attrname);
	if (t < 0) return default_value;

	/** integer? **/
	if (t == DATA_T_INTEGER)
	    {
	    if (wgtrGetPropertyValue(wgt,attrname,t,POD(&n)) == 0)
		{
		rval = (n != 0);
		}
	    }
	else if (t == DATA_T_STRING)
	    {
	    /** string? **/
	    if (wgtrGetPropertyValue(wgt,attrname,t,POD(&ptr)) == 0)
		{
		if (!strcasecmp(ptr,"yes") || !strcasecmp(ptr,"true") || !strcasecmp(ptr,"on") || !strcasecmp(ptr,"y"))
		    {
		    rval = 1;
		    }
		else if (!strcasecmp(ptr,"no") || !strcasecmp(ptr,"false") || !strcasecmp(ptr,"off") || !strcasecmp(ptr,"n"))
		    {
		    rval = 0;
		    }
		}
	    }
	else
	    {
	    mssError(1,"HT","Invalid data type for attribute '%s'", attrname);
	    rval = -1;
	    }

    return rval;
    }


/*** htrParamValue() - for use by widget drivers; get the value of a param
 *** passed in to the application or component.
 ***/
char*
htrParamValue(pHtSession s, char* paramname)
    {
    pStruct attr;

	/** Make sure this isn't a reserved param **/
	if (!strncmp(paramname,"ls__",4)) return NULL;

	/** Look for it. **/
	attr = stLookup_ne(s->Params, paramname);
	if (!attr) return NULL;

    return attr->StrVal;
    }


/*** htr_internal_CheckDMPrivateData() - check to see if widget private info
 *** structure is allocated, and put it in there if it is not.
 ***/
pHtDMPrivateData
htr_internal_CheckDMPrivateData(pWgtrNode widget)
    {
    pHtDMPrivateData inf = wgtrGetDMPrivateData(widget);
    
	if (!inf)
	    {
	    inf = (pHtDMPrivateData)nmMalloc(sizeof(HtDMPrivateData));
	    memset(inf, 0, sizeof(HtDMPrivateData));
	    wgtrSetDMPrivateData(widget, inf);
	    }

    return inf;
    }


/*** htr_internal_FreeDMPrivateData() - walk through the widget tree, and
 *** free up our DMPrivateData structures.
 ***/
int
htr_internal_FreeDMPrivateData(pWgtrNode widget)
    {
    pHtDMPrivateData inf = wgtrGetDMPrivateData(widget);
    int i, cnt;

	cnt = xaCount(&widget->Children);
	for(i=0;i<cnt;i++)
	    htr_internal_FreeDMPrivateData((pWgtrNode)xaGetItem(&widget->Children, i));

	if (inf)
	    {
	    if (inf->ObjectLinkage) nmSysFree(inf->ObjectLinkage);
	    if (inf->ContainerLinkage) nmSysFree(inf->ContainerLinkage);
	    if (inf->Param) nmSysFree(inf->Param);
	    nmFree(inf, sizeof(HtDMPrivateData));
	    wgtrSetDMPrivateData(widget, NULL);
	    }

    return 0;
    }


/*** htrAddWgtrObjLinkage() - specify what function/object to call to find out
 *** what the actual client-side object is that represents an object inside
 *** the widget tree.
 ***/
int
htrAddWgtrObjLinkage(pHtSession s, pWgtrNode widget, char* linkage)
    {
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->ObjectLinkage = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddWgtrObjLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrObjLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];

	va_start(va, fmt);
	qpfPrintf_va(NULL, buf, sizeof(buf), fmt, va);
	va_end(va);

    return htrAddWgtrObjLinkage(s, widget, buf);
    }


/*** htrAddWgtrCtrLinkage() - specify what function/object to call to find out
 *** what the actual client-side object is that represents an object inside
 *** the widget tree.
 ***/
int
htrAddWgtrCtrLinkage(pHtSession s, pWgtrNode widget, char* linkage)
    {
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->ContainerLinkage = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, linkage, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddWgtrCtrLinkage_va() - varargs version of the above.
 ***/
int
htrAddWgtrCtrLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...)
    {
    va_list va;
    char buf[256];

	va_start(va, fmt);
	qpfPrintf_va(NULL, buf, sizeof(buf), fmt, va);
	va_end(va);

    return htrAddWgtrCtrLinkage(s, widget, buf);
    }


/*** htrAddWgtrInit() - sets the initialization function for the widget
 ***/
int
htrAddWgtrInit(pHtSession s, pWgtrNode widget, char* func, char* paramfmt, ...)
    {
    va_list va;
    char buf[256];
    pHtDMPrivateData inf = htr_internal_CheckDMPrivateData(widget);
    
	inf->InitFunc = func;
	va_start(va, paramfmt);
	vsnprintf(buf, sizeof(buf), paramfmt, va);
	va_end(va);
	inf->Param = nmSysStrdup(objDataToStringTmp(DATA_T_STRING, buf, DATA_F_QUOTED));

    return 0;
    }


/*** htrAddNamespace() - adds a namespace context for expression evaluation
 *** et al.
 ***/
int
htrAddNamespace(pHtSession s, pWgtrNode container, char* nspace)
    {
    pHtNamespace new_ns;
    char* ptr;

	/** Allocate a new namespace **/
	new_ns = (pHtNamespace)nmMalloc(sizeof(HtNamespace));
	if (!new_ns) return -1;
	new_ns->Parent = s->Namespace;
	strtcpy(new_ns->DName, nspace, sizeof(new_ns->DName));
	wgtrGetPropertyValue(container, "name", DATA_T_STRING, POD(&ptr));
	strtcpy(new_ns->ParentCtr, ptr, sizeof(new_ns->ParentCtr));

	/** Link it in **/
	new_ns->FirstChild = NULL;
	new_ns->NextSibling = s->Namespace->FirstChild;
	s->Namespace->FirstChild = new_ns;
	s->Namespace = new_ns;

	/** Add script inits **/
	htrAddScriptInit_va(s, "    nodes = wgtrNodeList(%STR&SYM);\n"
			       "    rootname = \"%STR&SYM\";\n",
		nspace, nspace);

    return 0;
    }


/*** htrLeaveNamespace() - revert back to the parent namespace of the one
 *** currently in use.
 ***/
int
htrLeaveNamespace(pHtSession s)
    {

	s->Namespace = s->Namespace->Parent;

	/** Add script inits **/
	htrAddScriptInit_va(s, "    nodes = wgtrNodeList(%STR&SYM);\n"
			       "    rootname = \"%STR&SYM\";\n",
		s->Namespace->DName, s->Namespace->DName);

    return 0;
    }

