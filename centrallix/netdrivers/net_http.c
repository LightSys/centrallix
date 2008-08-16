#include "net_http.h"

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
/* Module: 	net_http.c              				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing an HTTP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_http.c,v 1.86 2008/08/16 00:31:38 thr4wn Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_http.c,v $

    $Log: net_http.c,v $
    Revision 1.86  2008/08/16 00:31:38  thr4wn
    I made some more modification of documentation and begun logic for
    caching generated WgtrNode instances (see centrallix-sysdoc/misc.txt)

    Revision 1.85  2008/06/25 22:48:12  jncraton
    - (change) split net_http into separate files
    - (change) replaced nht_internal_UnConvertChar with qprintf filter
    - (change) replaced nht_internal_escape with qprintf filter
    - (change) replaced nht_internal_decode64 with qprintf filter
    - (change) removed nht_internal_Encode64
    - (change) removed nht_internal_EncodeHTML

    Revision 1.84  2008/06/25 01:01:56  gbeeley
    - (feature) adding support for sysinfo /users directory which contains a
      list of currently logged-in users (as managed by NHT).
    - (change) adding ls__autoclose_sr option to OSML queries, where a query
      will automatically close if all rows in the result set were fetched.
      Good for performance as well as for lock contention.
    - (change) adding cx__noact option to all requests (used by the OSRC now),
      which tells the HTTP module to not count the request as end-user-
      initiated activity.  Useful for allowing sessions to idle out even if a
      regular query refresh is occurring, e.g., for a chat session.

    Revision 1.83  2008/04/06 20:51:30  gbeeley
    - (change) adding config option accept_localhost_only which is enabled in
      the default configuration file at present.  This prevents connections
      except from 127.0.0.1 (connections from elsewhere will get an error
      message).  If unspecified in the configuration, this option is not
      enabled.
    - (security) switching to MTASK mtSetSecContext interface instead of
      mtSetUserID.  See centrallix-lib commit log for discussion on this.
    - (change) Give an authorization error on a NULL password instead of
      treating it as a parse error.

    Revision 1.82  2008/03/28 07:00:36  gbeeley
    - (bugfix) only set values on a new object if the values are not NULL.

    Revision 1.81  2008/03/26 01:08:29  gbeeley
    - (change) switching to fdQPrintf() for file listing output, for better
      robustness

    Revision 1.80  2008/03/08 01:45:50  gbeeley
    - (bugfix) gracefully handle a reopen_sql situation if autoname fails.

    Revision 1.79  2008/03/04 01:10:57  gbeeley
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

    Revision 1.78  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.77  2008/02/17 07:45:15  gbeeley
    - (change) removal of 'encode' argument to WriteAttrs et al.
    - (performance) slight reduction of data xfer size when fetching data.
    - (change) ls__reopen_sql argument to OSML "create" allows for joins and
      computed fields to be taken into account when a new record is inserted.

    Revision 1.76  2008/01/18 23:54:26  gbeeley
    - (change) add entropy to pool from web connection timings.

    Revision 1.75  2007/09/18 18:00:57  gbeeley
    - (change) allow encoding of attribute name so that attribute names can
      contain spaces and special characters.

    Revision 1.74  2007/07/25 16:57:23  gbeeley
    - (change) prepping codebase for addition of layered/reverse-inheritance
      development.

    Revision 1.73  2007/06/06 15:20:09  gbeeley
    - (feature) pass templates on to components, etc.

    Revision 1.72  2007/05/29 15:17:13  gbeeley
    - CLK_TCK deprecation has finally taken form.  This was fixed in centrallix-lib
      but not in this module.

    Revision 1.71  2007/04/19 21:22:54  gbeeley
    - (change) slightly reworked ConnHandler function, added stub for POST
      method.

    Revision 1.70  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.69  2007/03/10 05:13:03  gbeeley
    - (change) log session starts

    Revision 1.68  2007/03/05 20:04:45  gbeeley
    - (feature) allow session cookie name to be specified in centrallix.conf.
    - (workaround) put a <br> after each attr when sending attr-value list to
      client, as NS4 seems to be dropping some data when all the attrs are
      together on one "line" of html.  Symptom was "** ERROR **" on occasion
      in form fields.

    Revision 1.67  2007/03/04 03:56:49  gbeeley
    - (feature) have server feed updated values after an update or create, so
      client can get data that may have been changed by the server during the
      update or insert operation, due to business rules or auto keying.

    Revision 1.66  2007/03/01 21:55:13  gbeeley
    - (change) Use CXID for cookie name instead of LSID.

    Revision 1.65  2007/02/26 16:40:39  gbeeley
    - (bugfix) adding of cx__akey threw off OSML setattrs operation.

    Revision 1.64  2007/02/22 23:25:14  gbeeley
    - (feature) adding initial framework for CXSS, the security subsystem.
    - (feature) CXSS entropy pool and key generation, basic framework.
    - (feature) adding xmlhttprequest capability
    - (change) CXSS requires OpenSSL, adding that check to the build
    - (security) Adding application key to thwart request spoofing attacks.
      Once the AML is active, application keying will be more important and
      will be handled there instead of in net_http.

    Revision 1.63  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.62  2006/10/19 21:53:23  gbeeley
    - (feature) First cut at the component-based client side development
      system.  Only rendering of the components works right now; interaction
      with the components and their containers is not yet functional.  For
      an example, see "debugwin.cmp" and "window_test.app" in the samples
      directory of centrallix-os.

    Revision 1.61  2006/10/16 18:34:34  gbeeley
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

    Revision 1.60  2006/10/04 17:20:50  gbeeley
    - (feature) allow application to adjust to user agent's configured text
      font size.  Especially the Mozilla versions in CentOS have terrible
      line spacing problems.
    - (feature) to allow the above, added minimum widget height management to
      the auto-layout module (apos)
    - (change) allow floating windows to grow in size if more room is needed
      inside the window.
    - (change) for auto-layout, go with the minimum flexibility in any row or
      column rather than the average.  Not sure of all of the impact of
      doing this.

    Revision 1.59  2006/07/19 20:43:41  gbeeley
    - change cx__width/cx__height to just cx__geom
    - allow cx__geom=design to render app as designed without any scaling
    - prep work for reworking the HTTP network driver (sorry for the delay in
      getting this committed!)

    Revision 1.58  2006/04/07 06:42:30  gbeeley
    - (bugfix) memory_leaks -= 2;
    - (bugfix) be graceful if netAcceptTCP() returns NULL.

    Revision 1.57  2005/10/18 22:49:33  gbeeley
    - (bugfix) always attempt to use autoname; let the OSML figure out whether
      autoname can be used for an object or not.

    Revision 1.56  2005/09/26 06:24:04  gbeeley
    - (major feature) Added templating mechanism via the wgtr module.
      To use a widget template on an app, specify widget_template= in the
      app.  See the objcanvas_test sample for an example of this.
    - (bugfix) Fixed a null value issue in the sybase driver
    - (bugfix) Fixed a wgtr issue on handling presentation hints settings.

    Revision 1.55  2005/09/17 02:58:39  gbeeley
    - change order of property detection since innerHeight seems more
      reliable on mozilla.  document.body.clientHeight reflects actual
      <body> tag height, not window height, evidently.  Another approach
      would be to set css height on <body> to 100%.

    Revision 1.54  2005/02/26 06:35:53  gbeeley
    - pass URL params in the OpenCtl parameters.

    Revision 1.53  2004/12/31 04:37:26  gbeeley
    - oops - need a pragma no-cache on the execmethod call
    - pass error and null information on attributes to the client

    Revision 1.52  2004/08/30 03:18:20  gbeeley
    - directory indexing (default document) now supported via net_http.
    - need some way to make objOpen less noisy when we *know* that it is OK
      for the open to fail based on file nonexistence.

    Revision 1.51  2004/08/29 19:32:53  pfinley
    fixing last commit... put the paren in the wrong place

    Revision 1.50  2004/08/29 17:32:32  pfinley
    Textarea widget crossbrowser support... I have tested on Mozilla 1.7rc1,
    Firefox 0.9.3, and Netscape 4.79.  Also fixed a JS syntax error with
    loading page/image.

    Revision 1.49  2004/08/29 04:21:38  jorupp
     * fix Greg's variant of this issue

    Revision 1.48  2004/08/29 04:19:40  jorupp
     * make the URL rewriting with the client dimensions a bit more robust.

    Revision 1.47  2004/08/28 06:48:08  jorupp
     * remove some unneed printfs
     * add a check to make sure we get the right cookie, in case there is one for another application on the same server

    Revision 1.46  2004/08/17 03:46:41  gbeeley
    - ignore "null" connections from MSIE
    - better error reporting when wgtr routines fail
    - use location.replace to make the browser's Back button work

    Revision 1.45  2004/08/15 03:10:48  gbeeley
    - moving client canvas size detection logic from htmlgen to net_http so
      that it can be passed to wgtrVerify(), later to be used in adjusting
      geometry of application to fit browser window.

    Revision 1.44  2004/08/13 18:46:13  mmcgill
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

    Revision 1.43  2004/08/02 14:09:36  mmcgill
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

    Revision 1.42  2004/07/19 15:30:43  mmcgill
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

    Revision 1.41  2004/06/12 04:02:28  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.40  2004/02/25 19:59:57  gbeeley
    - fixing problem in net_http; nht_internal_GET should not open the
      target_obj when operating in OSML-over-HTTP mode.
    - adding OBJ_O_AUTONAME support to sybase driver.  Uses select max()+1
      approach for integer fields which are left unspecified.

    Revision 1.39  2004/02/24 20:11:00  gbeeley
    - fixing some date/time related problems
    - efficiency improvement for net_http allowing browser to actually
      cache .js files and images.

    Revision 1.38  2003/11/30 02:09:40  gbeeley
    - adding autoquery modes to OSRC (never, onload, onfirstreveal, or
      oneachreveal)
    - adding serialized loader queue for preventing communcations with the
      server from interfering with each other (netscape bug)
    - pg_debug() writes to a "debug:" dynamic html widget via AddText()
    - obscure/reveal subsystem initial implementation

    Revision 1.37  2003/11/12 22:18:42  gbeeley
    - Begin addition of generalized server->client messages
    - Addition of delete support for osml-over-http

    Revision 1.36  2003/06/03 23:31:05  gbeeley
    Adding pro forma netscape 4.8 support.

    Revision 1.35  2003/05/30 17:58:27  gbeeley
    - turned off OSML API debugging
    - fixed bug in WriteOneAttr() that was truncating a string

    Revision 1.34  2003/05/30 17:39:51  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.33  2003/04/25 05:06:57  gbeeley
    Added insert support to OSML-over-HTTP, and very remedial Trx support
    with the objCommit API method and Commit osdriver method.  CSV datafile
    driver is the only driver supporting it at present.

    Revision 1.32  2003/03/12 03:19:09  lkehresman
    * Added basic presentation hint support to multiquery.  It only returns
      hints for the first result set, which is the wrong way to do it.  I went
      ahead and committed this so that peter and rupp can start working on the
      other stuff while I work on implementing this correctly.

    * Hints are now presented to the client in the form:
      <a target=XHANDLE HREF='http://ATTRIBUTE/?HINTS#TYPE'>
      where HINTS = hintname=value&hintname=value

    Revision 1.31  2002/12/23 06:22:04  jorupp
     * added ability to take flags (numbers only) to ls__req=read

    Revision 1.30  2002/11/22 20:57:32  gbeeley
    Converted part of net_http to use fdPrintf() as a test of the new
    functionality.

    Revision 1.29  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.28  2002/09/27 22:26:06  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.27  2002/07/31 18:36:20  mattphillips
    Let's make use of the HAVE_LIBZ defined by ./configure...  We asked autoconf
    to test for libz, but we didn't do anything with the results of its test.
    This wraps all the gzip stuff in #ifdef's so we will not use it if the system
    we built on doesn't have it.

    Revision 1.26  2002/07/21 05:05:57  jorupp
     * updated net_http.c to take advantage of gziped output (except for non-html docs for Netscape 4.7)
     * modified config file with new parameter, enable_gzip (0/1)
     * updated build scripts to reflect new dependency

    Revision 1.25  2002/07/12 19:57:00  gbeeley
    Added support for encoding of object attributes, such as those returned
    in a query result set.  Use &ls__encode=1 on the URL line.  Use the
    javascript function unescape() to get the original data back.

    Revision 1.24  2002/07/11 21:03:28  gbeeley
    Fixed problem with doing "setattrs" OSML operation on money and
    datetime data types.

    Revision 1.23  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.22  2002/06/14 15:13:42  jorupp
     * store the entire User-Agent line for comparison, not just the first word

    Revision 1.21  2002/06/10 00:21:00  nehresma
    Much cleaner (and safer) way of copying multiple lexer tokens into a buffer.
    Should have been doing this all along.  :)

    Revision 1.20  2002/06/09 23:55:23  nehresma
    sanity checking..

    Revision 1.19  2002/06/09 23:44:47  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.18  2002/05/06 22:46:55  gbeeley
    Updating net_http a bit to properly return OK on a ping.

    Revision 1.17  2002/05/03 03:52:43  gbeeley
    Added preliminary session watchdog support to the net_http module.
    The watchdog timer parameters are controllable via the config file.

    Revision 1.16  2002/05/01 02:20:31  gbeeley
    Modification in net_http: ls__req=close now allows multiple object
    ids to be strung together in the ls__oid parameter.

    Revision 1.15  2002/04/25 22:50:00  gbeeley
    Added ability to reference default session in ls__mode=osml operations
    instead of having to create a new one.  Note that the session is the
    transaction context, so be careful just using the default (which
    applies to all connections from the user's browser).  Set the ls__sid
    to "XDEFAULT" to use the default.

    Revision 1.14  2002/04/25 19:29:30  gbeeley
    Added handle support to object ids and query ids in the OSML over HTTP
    communication mechanism.

    Revision 1.13  2002/04/25 18:01:15  gbeeley
    Started adding Handle abstraction in net_http.c.  Testing first with
    just handlized ObjSession structures.

    Revision 1.12  2002/03/23 05:34:26  gbeeley
    Added "pragma: no-cache" headers to the "osml" mode responses to help
    avoid browser caching of that dynamic data.

    Revision 1.11  2002/03/23 05:09:16  gbeeley
    Fixed a logic error in net_http's ls__startat osml feature.  Improved
    OSML error text.

    Revision 1.10  2002/03/23 03:52:54  gbeeley
    Fixed a potential security blooper when the cookie was copied to a tmp
    buffer.

    Revision 1.9  2002/03/23 03:41:02  gbeeley
    Fixed the ages-old problem in net_http.c where cookies weren't anchored
    at the / directory, so zillions of sessions might be created...

    Revision 1.8  2002/03/23 01:30:44  gbeeley
    Added ls__startat option to the osml "queryfetch" mechanism, in the
    net_http.c driver.  Set ls__startat to the number of the first object
    you want returned, where 1 is the first object (in other words,
    ls__startat-1 objects will be skipped over).  Started to add a LIMIT
    clause to the multiquery module, but thought this would be easier and
    just as effective for now.

    Revision 1.7  2002/03/16 06:50:20  gbeeley
    Changed some sprintfs to snprintfs, just for safety's sake.

    Revision 1.6  2002/03/16 04:26:25  gbeeley
    Added functionality in net_http's object access routines so that it,
    when appropriate, sends the metadata attributes also, including the
    following:  "name", "inner_type", "outer_type", and "annotation".

    Revision 1.5  2002/02/14 00:55:20  gbeeley
    Added configuration file centrallix.conf capability.  You now MUST have
    this file installed, default is /usr/local/etc/centrallix.conf, in order
    to use Centrallix.  A sample centrallix.conf is found in the centrallix-os
    package in the "doc/install" directory.  Conf file allows specification of
    file locations, TCP port, server string, auth realm, auth method, and log
    method.  rootnode.type is now an attribute in the conf file instead of
    being a separate file, and thus is no longer used.

    Revision 1.4  2001/11/12 20:43:44  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.3  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:57  gbeeley
    Centrallix Core initial import

    Revision 1.3  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.2  2001/08/07 02:49:25  gbeeley
    Changed cookie from =LS-xxxx to LSID=LS-xxxx

    Revision 1.1.1.1  2001/08/07 02:31:22  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** Functions for enumerating users for the cx.sysinfo directory ***/
pXArray
nht_internal_UsersAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "session_cnt");

    return xa;
    }
pXArray
nht_internal_UsersObjList(void* ctx)
    {
    pXArray xa;
    int i;
    pNhtUser usr;

	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 64);
	for(i=0;i<xaCount(&NHT.UsersList);i++) 
	    {
	    usr = (pNhtUser)xaGetItem(&NHT.UsersList, i);
	    if (usr->SessionCnt > 0)
		xaAddItem(xa, usr->Username);
	    }

    return xa;
    }
int
nht_internal_UsersAttrType(void *ctx, char* objname, char* attrname)
    {

	if (!objname || !attrname) return -1;
	if (!strcmp(attrname, "session_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;

    return -1;
    }
int
nht_internal_UsersAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    pNhtUser usr;

	if (!objname || !attrname) return -1;
	usr = (pNhtUser)xhLookup(&(NHT.UsersByName), objname);
	if (!usr || usr->SessionCnt == 0) return -1;
	if (!strcmp(attrname, "session_cnt"))
	    val->Integer = usr->SessionCnt;
	else if (!strcmp(attrname, "name"))
	    val->String = usr->Username;
	else
	    return -1;

    return 0;
    }


/*** nht_internal_RegisterUsers() - register a handler for listing
 *** users logged into the server.
 ***/
int
nht_internal_RegisterUsers()
    {
    pSysInfoData si;

	/** thread list **/
	si = sysAllocData("/users", nht_internal_UsersAttrList, nht_internal_UsersObjList, NULL, nht_internal_UsersAttrType, nht_internal_UsersAttrValue, NULL, 0);
	sysRegister(si, NULL);

    return 0;
    }


/*** nht_internal_WriteResponse() - write the HTTP response header,
 *** not including content.
 ***/
int
nht_internal_WriteResponse(pNhtConn conn, int code, char* text, int contentlen, char* contenttype, char* pragma, char* resptxt)
    {
    int wcnt, rval;
    struct tm* thetime;
    time_t tval;
    char tbuf[40];

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);

	wcnt = fdQPrintf(conn->ConnFD, 
		"HTTP/1.0 %INT %STR\r\n"
		"Server: %STR\r\n"
		"Date: %STR\r\n"
		"%[Content-Length: %INT\r\n%]"
		"%[Content-Type: %STR\r\n%]"
		"%[Pragma: %STR\r\n%]",
		code,
		text,
		NHT.ServerString,
		tbuf,
		contentlen > 0, contentlen,
		contenttype != NULL, contenttype,
		pragma != NULL, pragma);
	if (wcnt < 0) return wcnt;

	if (resptxt)
	    {
	    rval = nht_internal_WriteConn(conn, resptxt, strlen(resptxt), 0);
	    if (rval < 0) return rval;
	    wcnt += rval;
	    }

    return wcnt;
    }


/*** nht_internal_WriteErrResponse() - write an HTTP error response
 *** header without any real content.  Use the normal WriteResponse
 *** routine if you want more flexible content.
 ***/
int
nht_internal_WriteErrResponse(pNhtConn conn, int code, char* text)
    {
    int wcnt, rval;

	wcnt = nht_internal_WriteResponse(conn, code, text, strlen(text), "text/html", NULL, NULL);
	if (wcnt < 0) return wcnt;
	rval = nht_internal_WriteConn(conn, text, strlen(text), 0);
	if (rval < 0) return rval;
	wcnt += rval;

    return wcnt;
    }


/*** nht_internal_FreeControlMsg() - release memory used by a control
 *** message, its parameters, etc.
 ***/
int
nht_internal_FreeControlMsg(pNhtControlMsg cm)
    {
    int i;
    pNhtControlMsgParam cmp;

	/** Destroy semaphore? **/
	if (cm->ResponseSem) syDestroySem(cm->ResponseSem, SEM_U_HARDCLOSE);

	/** Release params? **/
	for(i=0;i<cm->Params.nItems;i++)
	    {
	    cmp = (pNhtControlMsgParam)cm->Params.Items[i];
	    if (cmp->P1) nmSysFree(cmp->P1);
	    if (cmp->P2) nmSysFree(cmp->P2);
	    if (cmp->P3) nmSysFree(cmp->P3);
	    if (cmp->P3a) nmSysFree(cmp->P3a);
	    if (cmp->P3b) nmSysFree(cmp->P3b);
	    if (cmp->P3c) nmSysFree(cmp->P3c);
	    if (cmp->P3d) nmSysFree(cmp->P3d);
	    nmFree(cmp, sizeof(NhtControlMsgParam));
	    }

	/** Free the control msg structure itself **/
	nmFree(cm, sizeof(NhtControlMsg));

    return 0;
    }

#if 0
int
nht_internal_CacheHandler(pNhtConn conn)
    {
#if 0
    pWgtrNode tree = xmGet(conn->NhtSession->CachedApps2, "0");

#if 1
    /** cache the app **/
    char buf[1<<10];
    sprintf(buf, "%i", NHT.numbCachedApps);
    NHT.numbCachedApps++;
    xmAdd(&nsess->CachedApps, buf, tree);
#else
    xaAddItem(&nsess->CachedApps2, NHT.numbCachedApps)
    NHT.numbCachedApps++;
#endif

    if(! (wgtrVerify(tree, client_info) >= 0))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}

    rval = wgtrRender(output, s, tree, app_params, client_info, method);

    if(tree) wgtrFree(tree);
    return rval;
    //fdQPrintf(conn->ConnFD, "Hello World!");    
#endif
    }
#endif



/*** nht_internal_ControlMsgHandler() - the main handler for all connections
 *** which access /INTERNAL/control, thus requesting to receive control
 *** messages from the system.
 ***/
int
nht_internal_ControlMsgHandler(pNhtConn conn, pStruct url_inf)
    {
    pNhtControlMsg cm, usr_cm;
    pNhtControlMsgParam cmp;
    pNhtSessionData sess = conn->NhtSession;
    int i;
    char* response = NULL;
    char* cm_ptr = "0";
    char* err_ptr = NULL;
    int wait_for_sem = 1;
    char* ptr;

	/** No delay? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_nowait"), &ptr) == 0 && !strcmp(ptr,"1"))
	    wait_for_sem = 0;

	/** Control message response? **/
	stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_response"), &response);
	if (response)
	    {
	    /** Get control message id **/
	    stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_id"), &cm_ptr);
	    usr_cm = (pNhtControlMsg)strtoul(cm_ptr, NULL, 16);
	    cm = NULL;
	    for(i=0;i<sess->ControlMsgsList.nItems;i++)
		{
		if ((pNhtControlMsg)(sess->ControlMsgsList.Items[i]) == usr_cm)
		    {
		    cm = usr_cm;
		    break;
		    }
		}

	    /** No such id? **/
	    if (!cm)
		{
		nht_internal_WriteResponse(conn, 200, "OK", -1, "text/html", "no-cache", "<A HREF=0.0 TARGET=0>NO SUCH MESSAGE</A>\r\n");
		return 0;
		}

	    /** Is this an error? **/
	    stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_error"), &err_ptr);
	    if (err_ptr)
		{
		cm->Status = NHT_CONTROL_S_ERROR;
		cm->Response = err_ptr;
		}
	    else
		{
		cm->Status = NHT_CONTROL_S_RESPONSE;
		cm->Response = response;
		}

	    /** Remove from queue and do response action (sem or fn call) **/
	    xaRemoveItem(&(sess->ControlMsgsList), xaFindItem(&(sess->ControlMsgsList), cm));
	    if (cm->ResponseSem)
		syPostSem(cm->ResponseSem, 1, 0);
	    else if (cm->ResponseFn)
		cm->ResponseFn(cm);
	    else
		nht_internal_FreeControlMsg(cm);

	    /** Tell syGetSem to return immediately, below **/
	    wait_for_sem = 0;
	    }

	/** Send header **/
	nht_internal_WriteResponse(conn, 200, "OK", -1, "text/html", "no-cache", NULL);

    	/** Wait on the control msgs semaphore **/
	while (1)
	    {
	    if (syGetSem(sess->ControlMsgs, 1, wait_for_sem?0:SEM_U_NOBLOCK) < 0)
		{
		nht_internal_WriteConn(conn, "<A HREF=0.0 TARGET=0>END OF CONTROL MESSAGES</A>\r\n", -1, 0);
		return 0;
		}

	    /** Grab one control message **/
	    for(i=0;i<sess->ControlMsgsList.nItems;i++)
		{
		cm = (pNhtControlMsg)(sess->ControlMsgsList.Items[i]);
		if (cm->Status == NHT_CONTROL_S_QUEUED)
		    {
		    cm->Status = NHT_CONTROL_S_SENT;
		    break;
		    }
		}

	    /** Send ctl message header **/
	    nht_internal_QPrintfConn(conn, 0, "<A HREF=%INT.%INT TARGET=%INT>CONTROL MESSAGE</A>\r\n",
			    cm->MsgType,
			    cm->Params.nItems,
			    (unsigned int)cm);

	    /** Send parameters **/
	    for(i=0;i<cm->Params.nItems;i++)
		{
		cmp = (pNhtControlMsgParam)(cm->Params.Items[i]);
		if (cmp->P3a)
		    {
		    /** split-up HREF **/
		    fdPrintf(conn->ConnFD, "<A HREF=\"http://%s/%s?%s#%s\" TARGET=\"%s\">%s</A>\r\n",
			    cmp->P3a,
			    cmp->P3b?cmp->P3b:"",
			    cmp->P3c?cmp->P3c:"",
			    cmp->P3d?cmp->P3d:"",
			    cmp->P1?cmp->P1:"",
			    cmp->P2?cmp->P2:"");
		    }
		else
		    {
		    /** unified HREF **/
		    fdPrintf(conn->ConnFD, "<A HREF=\"%s\" TARGET=\"%s\">%s</A>\r\n",
			    cmp->P3?cmp->P3:"",
			    cmp->P1?cmp->P1:"",
			    cmp->P2?cmp->P2:"");
		    }
		}
	    printf("NHT: sending message\n");

	    /** Dequeue message if no response needed? **/
	    if (!cm->ResponseSem && !cm->ResponseFn)
		{
		xaRemoveItem(&(sess->ControlMsgsList), xaFindItem(&(sess->ControlMsgsList), cm));
		nht_internal_FreeControlMsg(cm);
		}

	    wait_for_sem = 0;
	    }

    return 0;
    }



/*** nht_internal_ErrorHandler - handle the printing of notice and error
 *** messages to the error stream for the client, if the client has such
 *** an error stream (which is how this is called).
 ***/
int
nht_internal_ErrorHandler(pNhtConn net_conn)
    {
    pNhtSessionData nsess = net_conn->NhtSession;
    pXString errmsg;

    	/** Wait on the errors semaphore **/
	if (syGetSem(nsess->Errors, 1, 0) < 0)
	    {
	    fdPrintf(net_conn->ConnFD,"HTTP/1.0 200 OK\r\n"
			 "Server: %s\r\n"
			 "Pragma: no-cache\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
	    return -1;
	    }

	/** Grab one error **/
	errmsg = (pXString)(nsess->ErrorList.Items[0]);
	xaRemoveItem(&nsess->ErrorList, 0);

	/** Format the error and print it as HTML. **/
	fdPrintf(net_conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Pragma: no-cache\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "<HTML><BODY><PRE><A NAME=\"Message\">",NHT.ServerString);
	fdWrite(net_conn->ConnFD,errmsg->String,strlen(errmsg->String),0,0);
	fdPrintf(net_conn->ConnFD,"</A></PRE></BODY></HTML>\r\n");

	/** Discard the string **/
	xsDeInit(errmsg);
	nmFree(errmsg,sizeof(XString));

    return 0;
    }


/*** nht_internal_GenerateError - grab up the current error text listing
 *** and queue it on the outbound error queue for this session, so that
 *** the error stream reader in the DHTML client can pick it up and show
 *** it to the user.
 ***/
int
nht_internal_GenerateError(pNhtSessionData nsess)
    {
    pXString errmsg;

    	/** Pick up the error msg text **/
	errmsg = (pXString)nmMalloc(sizeof(XString));
	xsInit(errmsg);
	mssStringError(errmsg);

	/** Queue it and post to the semaphore **/
	xaAddItem(&nsess->ErrorList, (void*)errmsg);
	syPostSem(nsess->Errors, 1, 0);

    return 0;
    }


/*** nht_internal_CreateCookie - generate a random string value that can
 *** be used as an HTTP cookie.
 ***/
int
nht_internal_CreateCookie(char* ck)
    {
    int key[4];

	cxssGenerateKey((unsigned char*)key, sizeof(key));
	sprintf(ck,"%s=%8.8x%8.8x%8.8x%8.8x",
		NHT.SessionCookie,
		key[0], key[1], key[2], key[3]);

    return 0;
    }


/*** nht_internal_Hex16ToInt - convert a 16-bit hex value, in four chars, to
 *** an unsigned integer.
 ***/
unsigned int
nht_internal_Hex16ToInt(char* hex)
    {
    char hex2[5];
    memcpy(hex2, hex, 4);
    hex2[4] = '\0';
    return strtoul(hex2, NULL, 16);
    }


/*** nht_internal_POST - handle the HTTP POST method.
 ***/
int
nht_internal_POST(pNhtConn conn, pStruct url_inf, int size)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pStruct find_inf;

	/** app key must be specified for all POST operations. **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || strcmp(find_inf->StrVal, nsess->AKey) != 0)
	    return -1;

    return 0;
    }


/*** nht_internal_GET - handle the HTTP GET method, reading a document or
 *** attribute list, etc.
 ***/
int
nht_internal_GET(pNhtConn conn, pStruct url_inf, char* if_modified_since)
    {
    pNhtSessionData nsess = conn->NhtSession;
    int cnt;
    pStruct find_inf,find_inf2;
    pObjQuery query;
    char* dptr;
    char* ptr;
    char* aptr;
    char* acceptencoding;
    pObject target_obj, sub_obj, tmp_obj;
    char* bufptr;
    char path[256];
    int rowid;
    int tid = -1;
    int convert_text = 0;
    pDateTime dt = NULL;
    DateTime dtval;
    struct tm systime;
    struct tm* thetime;
    time_t tval;
    char tbuf[32];
    int send_info = 0;
    pObjectInfo objinfo;
    char* gptr;
    int client_h, client_w;
    int gzip;
    int i;
    WgtrClientInfo wgtr_params;
    int akey_match = 0;
    char* tptr;
    char* tptr2;
    char* lptr;
    char* lptr2;
    char* pptr;

	acceptencoding=(char*)mssGetParam("Accept-Encoding");

//START INTERNAL handler -------------------------------------------------------------------
//TODO (from Seth): should this be moved out of nht_internal_GET and back into nht_internal_ConnHandler?

    	/*printf("GET called, stack ptr = %8.8X\n",&cnt);*/
        /** If we're opening the "errorstream", pass of processing to err handler **/
	if (!strncmp(url_inf->StrVal,"/INTERNAL/errorstream",21))
	    {
		return nht_internal_ErrorHandler(conn);
	    }
	else if (!strncmp(url_inf->StrVal, "/INTERNAL/control", 17))
	    {
		return nht_internal_ControlMsgHandler(conn, url_inf);
	    }
#if 0 //TODO: finish the caching ability. (this section could very well belong somewhere else)
	else if (!strncmp(url_inf->StrVal, "/INTERNAL/cache", 15))
	    {
		return htrRenderObject(conn->ConnFD, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML", nsess);
	    }
#endif

//END INTERNAL handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	/** app key specified? **/ //the app key feature is to prevent CSRFing attacks
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (find_inf && !strcmp(find_inf->StrVal, nsess->AKey))
	    akey_match = 1;

	/** Check GET mode. **/
	find_inf = stLookup_ne(url_inf,"ls__mode");

	/** Ok, open the object here, if not using OSML mode. **/
	if (!find_inf || strcmp(find_inf->StrVal,"osml") != 0)
	    {
	    target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	    if (!target_obj)
		{
		nht_internal_GenerateError(nsess);
		fdPrintf(conn->ConnFD,"HTTP/1.0 404 Not Found\r\n"
			     "Server: %s\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
		mssPrintError(conn->ConnFD);
		netCloseTCP(conn->ConnFD,1000,0);
		nht_internal_UnlinkSess(nsess);
		thExit();
		}

	    /** Directory indexing? **/
	    objGetAttrValue(target_obj, "inner_type", DATA_T_STRING, POD(&ptr));
	    if (!strcmp(ptr,"system/void") && NHT.DirIndex[0] && (!find_inf || !strcmp(find_inf->StrVal,"content")))
		{
		/** Check the object type. **/
		objGetAttrValue(target_obj, "outer_type", DATA_T_STRING,POD(&ptr));

		/** no dirindex on .app files! **/
		if (strcmp(ptr,"widget/page") && strcmp(ptr,"widget/frameset") &&
			strcmp(ptr,"widget/component-decl"))
		    {
		    tmp_obj = NULL;
		    for(i=0;i<sizeof(NHT.DirIndex)/sizeof(char*);i++)
			{
			if (NHT.DirIndex[i])
			    {
			    snprintf(path,sizeof(path),"%s/%s",url_inf->StrVal,NHT.DirIndex[i]);
			    tmp_obj = objOpen(nsess->ObjSess, path, O_RDONLY, 0600, "text/html");
			    if (tmp_obj) break;
			    }
			}
		    if (tmp_obj)
			{
			objClose(target_obj);
			target_obj = tmp_obj;
			tmp_obj = NULL;
			}
		    }
		}

	    /** Do we need to set params as a part of the open? **/
	    if (akey_match)
		nht_internal_CkParams(url_inf, target_obj);
	    }
	else
	    {
	    target_obj = NULL;
	    }

	/** WAIT TRIGGER mode. **/
	if (find_inf && !strcmp(find_inf->StrVal,"triggerwait"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__waitid");
	    if (find_inf)
	        {
		tid = strtol(find_inf->StrVal,NULL,0);
		nht_internal_WaitTrigger(nsess,tid);
		}
	    }

	/** Check object's modification time **/
	if (target_obj && objGetAttrValue(target_obj, "last_modification", DATA_T_DATETIME, POD(&dt)) == 0)
	    {
	    memcpy(&dtval, dt, sizeof(DateTime));
	    dt = &dtval;
	    }
	else
	    {
	    dt = NULL;
	    }

	/** Should we bother comparing if-modified-since? **/
	/** FIXME - GRB this is not working yet **/
#if 0
	if (dt && *if_modified_since)
	    {
	    /** ims is in GMT; convert it **/
	    strptime(if_modified_since, "%a, %d %b %Y %T", &systime);

	    if (objDataToDateTime(DATA_T_STRING, if_modified_since, &ims_dtval, NULL) == 0)
		{
		printf("comparing %lld to %lld\n", dtval.Value, ims_dtval.Value);
		}
	    }
#endif

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);

	/** Ok, issue the HTTP header for this one. **/
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	if (nsess->IsNewCookie)
	    {
	    fdPrintf(conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Date: %s GMT\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n", 
		     tbuf, NHT.ServerString, nsess->Cookie);
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    fdPrintf(conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Date: %s GMT\r\n"
		     "Server: %s\r\n",
		     tbuf, NHT.ServerString);
	    }

	/** Exit now if wait trigger. **/
	if (tid != -1)
	    {
	    fdWrite(conn->ConnFD,"OK\r\n",4,0,0);
	    objClose(target_obj);
	    return 0;
	    }

	/** Add last modified information if we can. **/
	if (dt)
	    {
	    systime.tm_sec = dt->Part.Second;
	    systime.tm_min = dt->Part.Minute;
	    systime.tm_hour = dt->Part.Hour;
	    systime.tm_mday = dt->Part.Day + 1;
	    systime.tm_mon = dt->Part.Month;
	    systime.tm_year = dt->Part.Year;
	    systime.tm_isdst = -1;
	    tval = mktime(&systime);
	    thetime = gmtime(&tval);
	    strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);
	    fdPrintf(conn->ConnFD, "Last-Modified: %s GMT\r\n", tbuf);
	    }

	/** GET CONTENT mode. **/
	if (!find_inf || !strcmp(find_inf->StrVal, "content"))
	    {
	    /** Check the object type. **/
	    objGetAttrValue(target_obj, "outer_type", DATA_T_STRING,POD(&ptr));

	    /** request for application content of some kind **/
	    if (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset") ||
		    !strcmp(ptr,"widget/component-decl"))
	        {
		/** Width and Height of user agent specified? **/
		memset(&wgtr_params, 0, sizeof(wgtr_params));
		gptr = NULL;
		stAttrValue_ne(stLookup_ne(url_inf,"cx__geom"),&gptr);
		if (!gptr || (strlen(gptr) != 20 && strcmp(gptr,"design") != 0))
		    {
		    /** Deploy snippet to get geom from browser **/
		    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
		    nht_internal_GetGeom(target_obj, conn->ConnFD);
		    objClose(target_obj);
		    return 0;
		    }
		if (!strcmp(gptr,"design"))
		    {
		    client_h=0;
		    client_w=0;
		    objGetAttrValue(target_obj, "width", DATA_T_INTEGER, POD(&client_w));
		    objGetAttrValue(target_obj, "height", DATA_T_INTEGER, POD(&client_h));
		    }
		else
		    {
		    client_h = client_w = 0;
		    /*client_w = strtol(gptr,&gptr,10);
		    if (client_w < 0) client_w = 0;
		    if (client_w > 10000) client_w = 10000;
		    if (*gptr == 'x')
			{
			client_h = strtol(gptr+1,NULL,10);
			if (client_h < 0) client_h = 0;
			if (client_h > 10000) client_h = 10000;
			}*/
		    client_w = nht_internal_Hex16ToInt(gptr);
		    client_h = nht_internal_Hex16ToInt(gptr+4);
		    wgtr_params.CharWidth = nht_internal_Hex16ToInt(gptr+8);
		    wgtr_params.CharHeight = nht_internal_Hex16ToInt(gptr+12);
		    wgtr_params.ParagraphHeight = nht_internal_Hex16ToInt(gptr+16);
		    }
		wgtr_params.MaxHeight = client_h;
		wgtr_params.MinHeight = client_h;
		wgtr_params.MaxWidth = client_w;
		wgtr_params.MinWidth = client_w;
		strtcpy(wgtr_params.AKey, nsess->AKey, sizeof(wgtr_params.AKey));

		/** Check for gzip encoding **/
		gzip=0;
#ifdef HAVE_LIBZ
		if(NHT.EnableGzip && acceptencoding && strstr(acceptencoding,"gzip"))
		    gzip=1; /* enable gzip for this request */
#endif
		if(gzip==1)
		    {
		    fdPrintf(conn->ConnFD,"Content-Encoding: gzip\r\n");
		    }
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\nPragma: no-cache\r\n\r\n");
		if(gzip==1)
		    fdSetOptions(conn->ConnFD, FD_UF_GZIP);

		/** Check for template specs **/
		tptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__templates"),&tptr) == 0 && tptr)
		    {
		    cnt = 0;
		    tptr = nmSysStrdup(tptr);
		    tptr2 = strtok(tptr,"|");
		    while(tptr2)
			{
			if (cnt < WGTR_MAX_TEMPLATE)
			    {
			    wgtr_params.Templates[cnt] = tptr2;
			    cnt++;
			    }
			tptr2 = strtok(NULL, "|");
			}
		    }

		/** Check for application layering **/
		lptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__overlays"),&lptr) == 0 && lptr)
		    {
		    cnt = 0;
		    lptr = nmSysStrdup(lptr);
		    lptr2 = strtok(lptr,"|");
		    while(lptr2)
			{
			if (cnt < WGTR_MAX_TEMPLATE)
			    {
			    wgtr_params.Overlays[cnt] = lptr2;
			    cnt++;
			    }
			lptr2 = strtok(NULL, "|");
			}
		    }
		pptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__app_path"),&pptr) == 0 && pptr)
		    {
		    wgtr_params.AppPath = pptr;
		    }

		/** Read the app spec, verify it, and generate it to DHTML **/
		//pWgtrNode tree;
		//if ((tree = wgtrParseOpenObject(target_obj, url_inf, wgtr_params.Templates)) < 0
		    //|| (objpath = objGetPathname(obj)) && wgtrMergeOverlays(tree, objpath, client_info->AppPath, client_info->Overlays, client_info->Templates) < 0 )
		    //|| nht_internal_Parse_and_Cache_an_App(target_obj, url_inf, &wgtr_params, nsess, &tree) < 0
		    //|| nht_internal_Verify_and_Position_and_Render_an_App(conn->ConnFD, nsess, &wgtr_params, "DHTML", tree) < 0)
		if (htrRenderObject(conn->ConnFD, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML", nsess) < 0)
		    {
		    mssError(0, "HTTP", "Invalid application %s of type %s", url_inf->StrVal, ptr);
		    fdPrintf(conn->ConnFD,"<h1>An error occurred while constructing the application:</h1><pre>");
		    mssPrintError(conn->ConnFD);
		    objClose(target_obj);
		    if (tptr) nmSysFree(tptr);
		    if (lptr) nmSysFree(lptr);
		    return -1;
		    }
#if 0
		// store into cache;
		if (wgtrVerify(tree, client_info) < 0)
		    {
		    if(tree) wgtrFree(tree);
		    mssError(0, "HTTP", "Invalid application %s of type %s", url_inf->StrVal, ptr);
		    fdPrintf(conn->ConnFD,"<h1>An error occurred while constructing the application:</h1><pre>");
		    mssPrintError(conn->ConnFD);
		    objClose(target_obj);
		    if (tptr) nmSysFree(tptr);
		    if (lptr) nmSysFree(lptr);
		    return -1;
		    }
#endif
		if (tptr) nmSysFree(tptr);
		if (lptr) nmSysFree(lptr);
	        }
	    /** a client app requested an interface definition **/ 
	    else if (!strcmp(ptr, "iface/definition"))
		{
		/** get the type **/
		objGetAttrValue(target_obj, "type", DATA_T_STRING, POD(&ptr));

		/** end the headers **/
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");

		/** call the html-related interface translating function **/
		if (ifcToHtml(conn->ConnFD, nsess->ObjSess, url_inf->StrVal) < 0)
		    {
		    mssError(0, "NHT", "Error sending Interface info for '%s' to client", url_inf->StrVal);
		    fdQPrintf(conn->ConnFD, "<A TARGET=\"ERR\" HREF=\"%STR&HTE\"></A>", url_inf->StrVal);
		    }
		else
		    {
		    fdQPrintf(conn->ConnFD, "<A NAME=\"%s\" TARGET=\"OK\" HREF=\"%STR&HTE\"></A>", ptr, url_inf->StrVal);
		    }
		}
	    /** some other sort of request **/
	    else
	        {
		int gzip=0;
		char *browser;

		browser=(char*)mssGetParam("User-Agent");

		objGetAttrValue(target_obj,"inner_type", DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"text/plain")) 
		    {
		    ptr = "text/html";
		    convert_text = 1;
		    }

#ifdef HAVE_LIBZ
		if(	NHT.EnableGzip && /* global enable flag */
			obj_internal_IsA(ptr,"text/plain")>0 /* a subtype of text/plain */
			&& acceptencoding && strstr(acceptencoding,"gzip") /* browser wants it gzipped */
			&& (!strcmp(ptr,"text/html") || (browser && regexec(NHT.reNet47,browser,(size_t)0,NULL,0) != 0 ) )
			/* only gzip text/html for Netscape 4.7, which doesn't like it if we gzip .js files */
		  )
		    {
		    gzip=1; /* enable gzip for this request */
		    }
#endif
		if(gzip==1)
		    {
		    fdPrintf(conn->ConnFD,"Content-Encoding: gzip\r\n");
		    }
		fdPrintf(conn->ConnFD,"Content-Type: %s\r\n\r\n", ptr);
		if(gzip==1)
		    {
		    fdSetOptions(conn->ConnFD, FD_UF_GZIP);
		    }
		if (convert_text) fdWrite(conn->ConnFD,"<HTML><PRE>",11,0,FD_U_PACKET);
		bufptr = (char*)nmMalloc(4096);
	        while((cnt=objRead(target_obj,bufptr,4096,0,0)) > 0)
	            {
		    fdWrite(conn->ConnFD,bufptr,cnt,0,FD_U_PACKET);
		    }
		if (convert_text) fdWrite(conn->ConnFD,"</HTML></PRE>",13,0,FD_U_PACKET);
		if (cnt < 0) 
		    {
		    mssError(0,"NHT","Incomplete read of object's content");
		    nht_internal_GenerateError(nsess);
		    }
		nmFree(bufptr, 4096);
	        }
	    }

	/** GET DIRECTORY LISTING mode. **/
	else if (!strcmp(find_inf->StrVal,"list"))
	    {
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__info"),&ptr) >= 0 && !strcmp(ptr,"1"))
		send_info = 1;
	    query = objOpenQuery(target_obj,"",NULL,NULL,NULL);
	    if (query)
	        {
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
		fdQPrintf(conn->ConnFD,"<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=\"%STR&HTE/..\">..</A><BR>\n",url_inf->StrVal);
		dptr = url_inf->StrVal;
		while(*dptr && *dptr == '/' && dptr[1] == '/') dptr++;
		while((sub_obj = objQueryFetch(query,O_RDONLY)))
		    {
		    if (send_info)
			{
			objinfo = objInfo(sub_obj);
			}
		    objGetAttrValue(sub_obj, "name", DATA_T_STRING,POD(&ptr));
		    objGetAttrValue(sub_obj, "annotation", DATA_T_STRING,POD(&aptr));
		    if (send_info && objinfo)
			{
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%INT:%INT:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,objinfo->Flags,objinfo->nSubobjects,aptr);
			}
		    else if (send_info && !objinfo)
			{
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>0:0:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
		    else
			{
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
		    objClose(sub_obj);
		    }
		objQueryClose(query);
		}
	    else
	        {
		nht_internal_GenerateError(nsess);
		}
	    }

	/** SQL QUERY mode **/
	else if (!strcmp(find_inf->StrVal,"query") && akey_match)
	    {
	    /** Change directory to appropriate query root **/
	    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
	    strtcpy(path, objGetWD(nsess->ObjSess), sizeof(path));
	    objSetWD(nsess->ObjSess, target_obj);

	    /** Get the SQL **/
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__sql"),&ptr) >= 0)
	        {
		query = objMultiQuery(nsess->ObjSess, ptr, NULL);
		if (query)
		    {
		    rowid = 0;
		    while((sub_obj = objQueryFetch(query,O_RDONLY)))
		        {
			nht_internal_WriteAttrs(sub_obj,conn,(handle_t)rowid,1);
			objClose(sub_obj);
			rowid++;
			}
		    objQueryClose(query);
		    }
		}

	    /** Switch the current directory back to what it used to be. **/
	    tmp_obj = objOpen(nsess->ObjSess, path, O_RDONLY, 0600, "text/html");
	    objSetWD(nsess->ObjSess, tmp_obj);
	    objClose(tmp_obj);
	    }

	/** GET METHOD LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"methods"))
	    {
	    }

	/** GET ATTRIBUTE-VALUE LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"attr"))
	    {
	    }

	/** Direct OSML Access mode... **/
	else if (!strcmp(find_inf->StrVal,"osml") && akey_match)
	    {
	    find_inf = stLookup_ne(url_inf,"ls__req");
	    nht_internal_OSML(conn,target_obj, find_inf->StrVal, url_inf);
	    }

	/** Exec method mode **/
	else if (!strcmp(find_inf->StrVal,"execmethod") && akey_match)
	    {
	    find_inf = stLookup_ne(url_inf,"ls__methodname");
	    find_inf2 = stLookup_ne(url_inf,"ls__methodparam");
	    fdPrintf(conn->ConnFD, "Content-Type: text/html\r\nPragma: no-cache\r\n\r\n");
	    if (!find_inf || !find_inf2)
	        {
		mssError(1,"NHT","Invalid call to execmethod - requires name and param");
		nht_internal_GenerateError(nsess);
		}
	    else
	        {
	    	ptr = find_inf2->StrVal;
	    	objExecuteMethod(target_obj, find_inf->StrVal, POD(&ptr));
		fdWrite(conn->ConnFD,"OK",2,0,0);
		}
	    }

	/** Close the objectsystem entry. **/
	if (target_obj) objClose(target_obj);

    return 0;
    }


/*** nht_internal_PUT - implements the PUT HTTP method.  Set content_buf to
 *** data to write, otherwise it will be read from the connection if content_buf
 *** is NULL.
 ***/
int
nht_internal_PUT(pNhtConn conn, pStruct url_inf, int size, char* content_buf)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pObject target_obj;
    char sbuf[160];
    int rcnt;
    int type,i,v;
    pStruct sub_inf;
    int already_exist=0;

    	/** See if the object already exists. **/
	target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (target_obj)
	    {
	    objClose(target_obj);
	    already_exist = 1;
	    }

	/** Ok, open the object here. **/
	target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_WRONLY | O_CREAT | O_TRUNC, 0600, "text/html");
	if (!target_obj)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn->ConnFD);
	    netCloseTCP(conn->ConnFD,1000,0);
	    nht_internal_UnlinkSess(nsess);
	    thExit();
	    }

	/** OK, we're ready.  Send the 100 Continue message. **/
	/*sprintf(sbuf,"HTTP/1.1 100 Continue\r\n"
		     "Server: %s\r\n"
		     "\r\n",NHT.ServerString);
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);*/

	/** If size specified, set the size. **/
	if (size >= 0) objSetAttrValue(target_obj, "size", DATA_T_INTEGER,POD(&size));

	/** Set any attributes specified in the url inf **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    sub_inf = url_inf->SubInf[i];
	    type = objGetAttrType(target_obj, sub_inf->Name);
	    if (type == DATA_T_INTEGER)
	        {
		v = strtol(sub_inf->StrVal,NULL,10);
		objSetAttrValue(target_obj, sub_inf->Name, DATA_T_INTEGER,POD(&v));
		}
	    else if (type == DATA_T_STRING)
	        {
		objSetAttrValue(target_obj, sub_inf->Name, DATA_T_STRING, POD(&(sub_inf->StrVal)));
		}
	    }

	/** If content_buf, write that else write from the connection. **/
	if (content_buf)
	    {
	    while(size != 0)
	        {
		rcnt = (size>1024)?1024:size;
		objWrite(target_obj, content_buf, rcnt, 0,0);
		size -= rcnt;
		content_buf += rcnt;
		}
	    }
	else
	    {
	    /** Ok, read from the connection, either until size bytes or until EOF. **/
	    while(size != 0 && (rcnt=fdRead(conn->ConnFD,sbuf,160,0,0)) > 0)
	        {
	        if (size > 0)
	            {
		    size -= rcnt;
		    if (size < 0) 
		        {
		        rcnt += size;
		        size = 0;
			}
		    }
	        if (objWrite(target_obj, sbuf, rcnt, 0,0) < 0) break;
		}
	    }

	/** Close the object. **/
	objClose(target_obj);

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, url_inf->StrVal);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    }
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);

    return 0;
    }


/*** nht_internal_COPY - implements the COPY centrallix-http method.
 ***/
int
nht_internal_COPY(pNhtConn conn, pStruct url_inf, char* dest)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pObject source_obj,target_obj;
    int size;
    int already_exist = 0;
    char sbuf[256];
    int rcnt,wcnt;

	/** Ok, open the source object here. **/
	source_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (!source_obj)
	    {
	    snprintf(sbuf,256,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>404 Source Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn->ConnFD);
	    netCloseTCP(conn->ConnFD,1000,0);
	    nht_internal_UnlinkSess(nsess);
	    thExit();
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, source_obj);

	/** Get the size of the original object, if possible **/
	if (objGetAttrValue(source_obj,"size",DATA_T_INTEGER,POD(&size)) != 0) size = -1;

	/** Try to open the new object read-only to see if it exists... **/
	target_obj = objOpen(nsess->ObjSess, dest, O_RDONLY, 0600, "text/html");
	if (target_obj)
	    {
	    objClose(target_obj);
	    already_exist = 1;
	    }

	/** Ok, open the target object for keeps now. **/
	target_obj = objOpen(nsess->ObjSess, dest, O_WRONLY | O_TRUNC | O_CREAT, 0600, "text/html");
	if (!target_obj)
	    {
	    snprintf(sbuf,256,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>404 Target Not Found</H1>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn->ConnFD,1000,0);
	    nht_internal_UnlinkSess(nsess);
	    thExit();
	    }

	/** Set the size of the new document... **/
	if (size >= 0) objSetAttrValue(target_obj, "size", DATA_T_INTEGER,POD(&size));

	/** Do the copy operation. **/
	while((rcnt = objRead(source_obj, sbuf, 256, 0,0)) > 0)
	    {
	    while(rcnt > 0)
	        {
		wcnt = objWrite(target_obj, sbuf, rcnt, 0,0);
		if (wcnt <= 0) break;
		rcnt -= wcnt;
		}
	    }

	/** Close the objects **/
	objClose(source_obj);
	objClose(target_obj);

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, dest);
		}
	    else
	        {
	        snprintf(sbuf,256,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, dest);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,dest);
		}
	    else
	        {
	        snprintf(sbuf,256,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,dest);
		}
	    }
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);

    return 0;
    }


/*** nht_internal_ParseHeaders - read from the connection and parse the
 *** headers into the NhtConn structure
 ***/
int
nht_internal_ParseHeaders(pNhtConn conn)
    {
    char* msg;
    pLxSession s = NULL;
    int toktype;
    char hdr[64];
    int did_alloc = 1;
    char* ptr;

    	/** Initialize a lexical analyzer session... **/
	s = mlxOpenSession(conn->ConnFD, MLX_F_NODISCARD | MLX_F_DASHKW | MLX_F_ICASE |
		MLX_F_EOL | MLX_F_EOF);

	/** Read in the main request header.  Note - error handler is at function
	 ** tail, as in standard goto-based error handling.
	 **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_EOF)
	    {
	    /** MSIE likes to open connections and then close them without
	     ** sending a request; don't print errors on this condition.
	     **/
	    mlxCloseSession(s);
	    nht_internal_FreeConn(conn);
	    thExit();
	    }

	/** Expecting request method **/
	if (toktype != MLX_TOK_KEYWORD) { msg="Invalid method syntax"; goto error; }
	mlxCopyToken(s,conn->Method,sizeof(conn->Method));
	mlxSetOptions(s,MLX_F_IFSONLY);

	/** Expecting request URL and version **/
	if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Invalid url syntax"; goto error; }
	did_alloc = 1;
	conn->URL = mlxStringVal(s, &did_alloc);
	if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected HTTP version after url"; goto error; }
	mlxCopyToken(s,conn->HTTPVer,sizeof(conn->HTTPVer));
	if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after version"; goto error; }
	mlxUnsetOptions(s,MLX_F_IFSONLY);

	/** Read in the various header parameters. **/
	while((toktype = mlxNextToken(s)) != MLX_TOK_EOL)
	    {
	    if (toktype == MLX_TOK_EOF) break;
	    if (toktype != MLX_TOK_KEYWORD) { msg="Expected HTTP header item"; goto error; }
	    /*ptr = mlxStringVal(s,NULL);*/
	    mlxCopyToken(s,hdr,sizeof(hdr));
	    if (mlxNextToken(s) != MLX_TOK_COLON) { msg="Expected : after HTTP header"; goto error; }

	    /** Got a header item.  Pick an appropriate type. **/
	    if (!strcmp(hdr,"destination"))
	        {
		/** Copy next IFS-only token to destination value **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected filename after dest."; goto error; }
		mlxCopyToken(s,conn->Destination,sizeof(conn->Destination));
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOF) { msg="Expected EOL after filename"; goto error; }
		}
	    else if (!strcmp(hdr,"authorization"))
	        {
		/** Get 'Basic' then the auth string in base64 **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected auth type"; goto error; }
		ptr = mlxStringVal(s,NULL);
		if (strcasecmp(ptr,"basic")) { msg="Can only handle BASIC auth"; goto error; }
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected auth after Basic"; goto error; }
		qpfPrintf(NULL,conn->Auth,sizeof(conn->Auth),"%STR&DB64",mlxStringVal(s,NULL));
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after auth"; goto error; }
		}
	    else if (!strcmp(hdr,"cookie"))
	        {
		/** Copy whole thing. **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Cookie:"; goto error; }
		mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    /** if the token is a string, and the current cookie doesn't look like ours, try the next one **/
		    if (toktype == MLX_TOK_STRING && (strncmp(conn->Cookie,NHT.SessionCookie,strlen(NHT.SessionCookie)) || conn->Cookie[strlen(NHT.SessionCookie)] != '='))
			{
			mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
			}
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		}
	    else if (!strcmp(hdr,"content-length"))
	        {
		/** Get the integer. **/
		if (mlxNextToken(s) != MLX_TOK_INTEGER) { msg="Expected content-length"; goto error; }
		conn->Size = mlxIntVal(s);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after length"; goto error; }
		}
	    else if (!strcmp(hdr,"user-agent"))
	        {
		/** Copy whole User-Agent. **/
		mlxSetOptions(s, MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after User-Agent:"; goto error; }
		/** NOTE: This needs to be freed up at the end of the session.  Is that taken
		          care of by mssEndSession?  I don't think it is, since xhClear is passed
			  a NULL function for free_fn.  This will be a 160 byte memory leak for
			  each session otherwise. 
		    January 6, 2002   NRE
		 **/
		mlxCopyToken(s,conn->UserAgent,sizeof(conn->UserAgent));
		/*while((toktype=mlxNextToken(s)))
		    {
		    if(toktype == MLX_TOK_STRING && strlen(conn->UserAgent)<158)
			{
			strcat(useragent," ");
			mlxCopyToken(s,useragent+strlen(useragent),160-strlen(useragent));
			}
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);*/
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after User-Agent: header"; goto error; }
		}
	    else if (!strcmp(hdr,"content-type"))
		{
		mlxSetOptions(s, MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected type after Content-Type:"; goto error; }
		mlxCopyToken(s, conn->RequestContentType, sizeof(conn->RequestContentType));
		if ((ptr = strchr(conn->RequestContentType, ';')) != NULL) *ptr = '\0';
		while(1)
		    {
		    toktype = mlxNextToken(s);
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_EOF || toktype == MLX_TOK_ERROR) break;
		    if (toktype == MLX_TOK_STRING && !conn->RequestBoundary[0])
			{
			mlxCopyToken(s, conn->RequestBoundary, sizeof(conn->RequestBoundary));
			if (!strncmp(conn->RequestBoundary, "boundary=", 9))
			    {
			    ptr = conn->RequestBoundary + 9;
			    if (*ptr == '"') ptr++;
			    memmove(conn->RequestBoundary, ptr, strlen(ptr)+1);
			    if ((ptr = strrchr(conn->RequestBoundary, '"')) != NULL) *ptr = '\0';
			    }
			}
		    }
		mlxUnsetOptions(s, MLX_F_IFSONLY);
		}
	    else if (!strcmp(hdr,"accept-encoding"))
	        {
		mlxSetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Accept-encoding:"; goto error; }
		mlxCopyToken(s, conn->AcceptEncoding, sizeof(conn->AcceptEncoding));
		/*conn->AcceptEncoding = mlxStringVal(s, &did_alloc);
		acceptencoding = (char*)nmMalloc(160);
		mlxCopyToken(s,acceptencoding,160);
		while((toktype=mlxNextToken(s)))
		    {
		    if(toktype == MLX_TOK_STRING && strlen(acceptencoding)<158)
			{
			strcat(acceptencoding+strlen(acceptencoding)," ");
			mlxCopyToken(s,acceptencoding+strlen(acceptencoding),160-strlen(acceptencoding));
			}
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);*/
		/* printf("accept-encoding: %s\n",acceptencoding); */
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after Accept-Encoding: header"; goto error; }
		}
	    else if (!strcmp(hdr,"if-modified-since"))
		{
		mlxSetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected date after If-Modified-Since:"; goto error; }
		mlxCopyToken(s, conn->IfModifiedSince, sizeof(conn->IfModifiedSince));
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after If-Modified-Since: header"; goto error; }
		}
	    else
	        {
		/** Don't know what it is.  Just skip to end-of-line. **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		}
	    }

	/** Ok, done parsing.  Close the lexer session. **/
	mlxCloseSession(s);
	s = NULL;

    return 0;

    error:
	if (s) mlxCloseSession(s);
    return -1;
    }


/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nhtInitialize()
    {
    /*int i;

	printf("nhtInit called, stack ptr = %8.8X\n",&i);*/

    	/** Initialize the random number generator. **/
	srand48(time(NULL));

	/** Initialize globals **/
	NHT.numbCachedApps = 0;
	memset(&NHT, 0, sizeof(NHT));
	xhInit(&(NHT.CookieSessions),255,0);
	xaInit(&(NHT.Sessions),256);
	NHT.TimerUpdateSem = syCreateSem(0, 0);
	NHT.TimerDataMutex = syCreateSem(1, 0);
	xhnInitContext(&(NHT.TimerHctx));
	xaInit(&(NHT.Timers),512);
	NHT.WatchdogTime = 180;
	NHT.InactivityTime = 1800;
	NHT.CondenseJS = 1; /* not yet implemented */
	NHT.UserSessionLimit = 100;
	xhInit(&(NHT.UsersByName), 255, 0);
	xaInit(&NHT.UsersList, 64);
	NHT.AccCnt = 0;
	NHT.RestrictToLocalhost = 0;

#ifdef _SC_CLK_TCK
        NHT.ClkTck = sysconf(_SC_CLK_TCK);
#else
        NHT.ClkTck = CLK_TCK;
#endif

	nht_internal_RegisterUsers();

	/* intialize the regex for netscape 4.7 -- it has a broken gzip implimentation */
	NHT.reNet47=(regex_t *)nmMalloc(sizeof(regex_t));
	if(!NHT.reNet47 || regcomp(NHT.reNet47, "Mozilla\\/4\\.(7[5-9]|8)",REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    {
	    printf("unable to build Netscape 4.7 regex\n"); /* shouldn't this be mssError? -- but there's no session yet.. */
	    return -1;
	    }

	/** Start the watchdog timer thread **/
	thCreate(nht_internal_Watchdog, 0, NULL);

	/** Start the network listener. **/
	thCreate(nht_internal_Handler, 0, NULL);

	/*printf("nhtInit return from thCreate, stack ptr = %8.8X\n",&i);*/

    return 0;
    }


int CachedAppDeconstructor(pCachedApp this)
    {
    nmSysFree(&this->Key);
    wgtrFree(this->Node);
    return 0;
    }
