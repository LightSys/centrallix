#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h> //for regex functions
#include <regex.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_LIBZ 1
#endif

#include "centrallix.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "obj.h"
#include "stparse_ne.h"
#include "stparse.h"
#include "htmlparse.h"
#include "cxlib/xhandle.h"
#include "cxlib/magic.h"
#include "wgtr.h"
#include "iface.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxss/cxss.h"

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

    $Id: net_http.c,v 1.72 2007/05/29 15:17:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_http.c,v $

    $Log: net_http.c,v $
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

#define DEBUG_OSML	0

/*** This structure is used for wait-for-conn-to-finish ***/
typedef struct
    {
    int		TriggerID;
    pSemaphore	TriggerSem;
    int		LinkCnt;
    }
    NhtConnTrigger, *pNhtConnTrigger;

/*** Parameter/information for a control message, as embedded within a
 *** link on the page.
 ***/
typedef struct
    {
    char*	P1;		/* param #1: link TARGET */
    char*	P2;		/* param #2: link TXT */
    char*	P3;		/* param #3: link HREF, if entire HREF. */
    char*	P3a;		/* param #3a: link SERVER if partitioned HREF. */
    char*	P3b;		/* param #3b: link PATH if partitioned HREF. */
    char*	P3c;		/* param #3c: link QUERY if partitioned HREF. */
    char*	P3d;		/* param #3d: link JUMP-TARGET if partitioned HREF. */
    }
    NhtControlMsgParam, *pNhtControlMsgParam;

/*** This structure is used for server-to-client OOB control messages. ***/
typedef struct _NCM
    {
    int		MsgType;	/* NHT_CONTROL_T_xxx */
    XArray	Params;		/* xarray of pNhtControlMsgParam */
    pSemaphore	ResponseSem;	/* if set, control msg posts here when user responds */
    int		(*ResponseFn)(); /* if set, control msg calls this fn when user responds */
    int		Status;		/* status - NHT_CONTROL_S_xxx */
    char*	Response;	/* response string received from client */
    void*	Context;	/* caller-defined */
    }
    NhtControlMsg, *pNhtControlMsg;

#define NHT_CONTROL_T_ERROR	1	/* error message */
#define NHT_CONTROL_T_QUERY	2	/* query user for information */
#define NHT_CONTROL_T_GOODBYE	4	/* server shutting down */
#define NHT_CONTROL_T_REPMSG	8	/* replication message */
#define NHT_CONTROL_T_EVENT	16	/* remote control channel event */

#define NHT_CONTROL_S_QUEUED	0	/* message queued waiting for client */
#define NHT_CONTROL_S_SENT	1	/* message sent to client */
#define NHT_CONTROL_S_ERROR	2	/* could not get client's response */
#define NHT_CONTROL_S_RESPONSE	3	/* client has responded to message */

/*** User structure
 ***/
typedef struct
    {
    char		Username[32];
    int			SessionCnt;
    }
    NhtUser, *pNhtUser;


/*** This is used to keep track of user/password/cookie information ***/
typedef struct
    {
    char	Username[32];
    char	Password[32];
    char	Cookie[64];
    char	AKey[64];
    char	HTTPVer[16];
    int		ver_10:1;	/* is HTTP/1.0 compatible */
    int		ver_11:1;	/* is HTTP/1.1 compatible */
    void*	Session;
    int		IsNewCookie;
    pObjSession	ObjSess;
    pSemaphore	Errors;
    XArray	ErrorList;	/* xarray of xstring */
    XArray	Triggers;	/* xarray of pNhtConnTrigger */
    HandleContext Hctx;
    handle_t	WatchdogTimer;
    handle_t	InactivityTimer;
    int		LinkCnt;
    pSemaphore	ControlMsgs;
    XArray	ControlMsgsList;
    pNhtUser	User;
    int		LastAccess;
    }
    NhtSessionData, *pNhtSessionData;


/*** Timer structure.  The rule on the deallocation of these is that if
 *** you successfully remove it from the Timers list while holding the
 *** TimerDataMutex, then the thing is yours to deallocate and no one
 *** else should be messing with it.
 ***/
typedef struct
    {
    unsigned long	Time;		/* # msec this goes for */
    int			(*ExpireFn)();
    void*		ExpireArg;
    unsigned long	ExpireTick;	/* internal expiration time mark */
    int			SeqID;
    handle_t		Handle;
    }
    NhtTimer, *pNhtTimer;


/*** Connection data ***/
typedef struct
    {
    pFile	ConnFD;
    pStruct	ReqURL;
    pNhtSessionData NhtSession;
    int		InBody;
    int		BytesWritten;
    int		ResultCode;
    int		Port;
    char*	URL;
    char	AcceptEncoding[160];
    char*	Referrer;
    char	UserAgent[160];
    char	RequestContentType[64];
    char	RequestBoundary[128];
    char	Method[16];
    char	HTTPVer[16];
    char	Cookie[160];
    char	Auth[160];
    char	Destination[256];
    char	IfModifiedSince[64];
    char	Username[32];
    char	Password[32];
    char	IPAddr[20];
    int		Size;
    }
    NhtConn, *pNhtConn;


/*** GLOBALS ***/
struct 
    {
    XHashTable	CookieSessions;
    XArray	Sessions;
    char	ServerString[80];
    char	Realm[80];
    char	SessionCookie[32];
    pSemaphore	TimerUpdateSem;
    pSemaphore	TimerDataMutex;
    XArray	Timers;
    HandleContext TimerHctx;
    int		WatchdogTime;
    int		InactivityTime;
    regex_t*	reNet47;
    int		EnableGzip;
    int		CondenseJS;
    char*	DirIndex[16];
    int		UserSessionLimit;
    XHashTable	Users;
    long long	AccCnt;
    pFile	AccessLogFD;
    char	AccessLogFile[256];
    int		ClkTck;
    }
    NHT;

int nht_internal_UnConvertChar(int ch, char** bufptr, int maxlen);
//extern int htrRender(pFile, pObject, pStruct);
int nht_internal_RemoveWatchdog(handle_t th);


/*** nht_internal_AllocConn() - allocates a connection structure and
 *** initializes it given a network connection.
 ***/
pNhtConn
nht_internal_AllocConn(pFile net_conn)
    {
    pNhtConn conn;
    char* remoteip;

	/** Allocate and zero-out the structure **/
	conn = (pNhtConn)nmMalloc(sizeof(NhtConn));
	if (!conn) return NULL;
	memset(conn, 0, sizeof(NhtConn));
	conn->ConnFD = net_conn;
	conn->Size = -1;

	/** Get the remote IP and port **/
	remoteip = netGetRemoteIP(net_conn, NET_U_NOBLOCK);
	if (remoteip) strtcpy(conn->IPAddr, remoteip, sizeof(conn->IPAddr));
	conn->Port = netGetRemotePort(net_conn);

    return conn;
    }


/*** nht_internal_FreeConn() - releases a connection structure and 
 *** closes the associated network connection.
 ***/
int
nht_internal_FreeConn(pNhtConn conn)
    {

	/** Close the connection **/
	netCloseTCP(conn->ConnFD, 1000, 0);

	/** Deallocate the structure **/
	if (conn->Referrer) nmSysFree(conn->Referrer);
	if (conn->URL) nmSysFree(conn->URL);

	/** Unlink from the session. **/
	if (conn->NhtSession) nht_internal_UnlinkSess(conn->NhtSession);

	/** Release the connection structure **/
	nmFree(conn, sizeof(NhtConn));

    return 0;
    }


/*** nht_internal_WriteConn() - write data to a network connection
 ***/
int
nht_internal_WriteConn(pNhtConn conn, char* buf, int len, int is_hdr)
    {
    int wcnt;

	if (len == -1) len = strlen(buf);

	if (!is_hdr && !conn->InBody)
	    {
	    conn->InBody = 1;
	    fdWrite(conn->ConnFD, "\r\n", 2, 0, FD_U_PACKET);
	    }

	wcnt = fdWrite(conn->ConnFD, buf, len, 0, FD_U_PACKET);
	if (wcnt > 0 && !is_hdr)
	    conn->BytesWritten += wcnt;

    return wcnt;
    }


/*** nht_internal_PrintfConn() - write data to the network connection,
 *** formatted.
 ***/
int
nht_internal_QPrintfConn(pNhtConn conn, int is_hdr, char* fmt, ...)
    {
    va_list va;
    int wcnt;

	if (!is_hdr && !conn->InBody)
	    {
	    conn->InBody = 1;
	    fdWrite(conn->ConnFD, "\r\n", 2, 0, FD_U_PACKET);
	    }

	va_start(va, fmt);
	wcnt = fdQPrintf_va(conn->ConnFD, fmt, va);
	va_end(va);
	if (wcnt > 0 && !is_hdr)
	    conn->BytesWritten += wcnt;

    return wcnt;
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


/*** nht_internal_LinkSess() - link to a session, thus increasing its link
 *** count.
 ***/
int
nht_internal_LinkSess(pNhtSessionData sess)
    {
    sess->LinkCnt++;
    sess->LastAccess = NHT.AccCnt++;
    return 0;
    }


/*** nht_internal_UnlinkSess_r() - does the work of freeing an individual 
 *** handle within the session.
 ***/
int
nht_internal_UnlinkSess_r(void* v)
    {
    if (ISMAGIC(v, MGK_OBJSESSION)) 
	{
	objCloseSession((pObjSession)v);
	}
    return 0;
    }


/*** nht_internal_UnlinkSess() - free a session when its link count reaches 0.
 ***/
int
nht_internal_UnlinkSess(pNhtSessionData sess)
    {
    pXString errmsg;
    pNhtConnTrigger trg;
    pNhtControlMsg cm;

	/** Bump the link cnt down **/
	sess->LinkCnt--;

	/** Time to close the session down? **/
	if (sess->LinkCnt <= 0)
	    {
	    printf("NHT: releasing session for username [%s], cookie [%s]\n", sess->Username, sess->Cookie);

	    /** Kill the inactivity timers first so we don't get re-entered **/
	    nht_internal_RemoveWatchdog(sess->WatchdogTimer);
	    nht_internal_RemoveWatchdog(sess->InactivityTimer);

	    /** Decrement user session count **/
	    sess->User->SessionCnt--;

	    /** Remove the session from the global session list. **/
	    xhRemove(&(NHT.CookieSessions), sess->Cookie);
	    xaRemoveItem(&(NHT.Sessions), xaFindItem(&(NHT.Sessions), (void*)sess));

	    /** First, close all open handles. **/
	    xhnClearHandles(&(sess->Hctx), nht_internal_UnlinkSess_r);

	    /** Close the master session. **/
	    objCloseSession(sess->ObjSess);

	    /** Destroy the errors semaphore **/
	    syDestroySem(sess->Errors, SEM_U_HARDCLOSE);
	    syDestroySem(sess->ControlMsgs, SEM_U_HARDCLOSE);

	    /** Clear the errors list **/
	    while(sess->ErrorList.nItems)
		{
		errmsg = (pXString)(sess->ErrorList.Items[0]);
		xaRemoveItem(&sess->ErrorList, 0);
		xsDeInit(errmsg);
		nmFree(errmsg,sizeof(XString));
		}

	    /** Clear the triggers list **/
	    while(sess->Triggers.nItems)
		{
		trg = (pNhtConnTrigger)(sess->Triggers.Items[0]);
		trg->LinkCnt--;
		if (trg->LinkCnt <= 0)
		    {
		    syDestroySem(trg->TriggerSem,SEM_U_HARDCLOSE);
		    xaRemoveItem(&(sess->Triggers), 0);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		}

	    /** Clear control msg list **/
	    while(sess->ErrorList.nItems)
		{
		cm = (pNhtControlMsg)(sess->ControlMsgsList.Items[0]);
		xaRemoveItem(&sess->ControlMsgsList, 0);
		nht_internal_FreeControlMsg(cm);
		}

	    /** Dealloc the xarrays and such **/
	    xaDeInit(&(sess->Triggers));
	    xaDeInit(&(sess->ErrorList));
	    xaDeInit(&(sess->ControlMsgsList));
	    xhnDeInitContext(&(sess->Hctx));
	    nmFree(sess, sizeof(NhtSessionData));
	    }

    return 0;
    }


/*** nht_internal_Watchdog() - manages the session watchdog timers.  These
 *** timers cause sessions to automatically time out if a certain operation
 *** isn't performed every so often.
 ***
 *** fyi - if the tick comparison arithmetic looks funny, it's because we
 *** need to use modulus arithmetic to prevent failure of this routine if
 *** (gasp!) Centrallix has been running for several hundred days (at
 *** CLK_TCK==100, the 32bit tick counter wraps at just under 500 days).
 ***/
void
nht_internal_Watchdog(void* v)
    {
    unsigned long cur_tick, next_tick;
    int i;
    pNhtTimer t;
    EventReq timer_event, semaphore_event;
    pEventReq ev[2] = {&timer_event, &semaphore_event};
    pNhtTimer expired_timer;

	/** Set the thread's name **/
	thSetName(NULL,"NHT Watchdog Manager Task");

	/** Enter our loop of compiling timer data and then waiting on
	 ** either an expiration or on the update semaphore (which tells us
	 ** that the timer data changed)
	 **/
	while(1)
	    {
	    /** Acquire the timer data mutex **/
	    syGetSem(NHT.TimerDataMutex, 1, 0);

	    /** Scan the timer list for next expiration. **/
	    cur_tick = mtLastTick();
	    next_tick = cur_tick + 0x7FFFF;
	    for(i=0;i<NHT.Timers.nItems;i++)
		{
		t = (pNhtTimer)(NHT.Timers.Items[i]);
		if (t->ExpireTick - next_tick > (unsigned long)0x80000000)
		    {
		    next_tick = t->ExpireTick;
		    }
		}
	    syPostSem(NHT.TimerDataMutex, 1, 0);

	    /** Has the next tick already expired?  If not, wait for it **/
	    if (cur_tick - next_tick > (unsigned long)0x80000000)
		{
		/** Setup our event list for the multiwait operation **/
		ev[0]->Object = NULL;
		ev[0]->ObjType = OBJ_T_MTASK;
		ev[0]->EventType = EV_T_MT_TIMER;
		ev[0]->ReqLen = (next_tick - cur_tick)*(1000/NHT.ClkTck);
		ev[1]->Object = NHT.TimerUpdateSem;
		ev[1]->ObjType = OBJ_T_SEM;
		ev[1]->EventType = EV_T_SEM_GET;
		ev[1]->ReqLen = 1;
		thMultiWait(2, ev);

		/** If the semaphore completed, reset the process... **/
		if (semaphore_event.Status == EV_S_COMPLETE) 
		    {
		    /** Clean out any posted updates... **/
		    while(syGetSem(NHT.TimerUpdateSem, 1, SEM_U_NOBLOCK) >= 0) ;

		    /** Restart the loop and wait for more timers **/
		    continue;
		    }
		}

	    /** Ok, one or more events need to be fired off.
	     ** We search this over again each time we look for another
	     ** timer, since the expire routine may have messed with the
	     ** timer table.
	     **/
	    cur_tick = mtLastTick();
	    do  {
		syGetSem(NHT.TimerDataMutex, 1, 0);
		expired_timer = NULL;

		/** Look for a single expired timer **/
		for(i=0;i<NHT.Timers.nItems;i++)
		    {
		    t = (pNhtTimer)(NHT.Timers.Items[i]);
		    if (t->ExpireTick - cur_tick > (unsigned long)0x80000000)
			{
			xaRemoveItem(&(NHT.Timers), i);
			expired_timer = t;
			break;
			}
		    }
		syPostSem(NHT.TimerDataMutex, 1, 0);

		/** Here is how the watchdog barks... **/
		if (expired_timer)
		    {
		    expired_timer->ExpireFn(expired_timer->ExpireArg, expired_timer->Handle);
		    xhnFreeHandle(&(NHT.TimerHctx), expired_timer->Handle);
		    nmFree(expired_timer, sizeof(NhtTimer));
		    }
		}
		while(expired_timer); /* end do loop */
	    }


    thExit();
    }


/*** nht_internal_AddWatchdog() - adds a timer to the watchdog timer list.
 ***/
handle_t
nht_internal_AddWatchdog(int timer_msec, int (*expire_fn)(), void* expire_arg)
    {
    pNhtTimer t;

	/** First, allocate ourselves a timer. **/
	t = (pNhtTimer)nmMalloc(sizeof(NhtTimer));
	if (!t) return XHN_INVALID_HANDLE;
	t->ExpireFn = expire_fn;
	t->ExpireArg = expire_arg;
	t->Time = timer_msec;
	t->ExpireTick = mtLastTick() + (timer_msec*NHT.ClkTck/1000);
        t->Handle = xhnAllocHandle(&(NHT.TimerHctx), t);

	/** Add it to the list... **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	xaAddItem(&(NHT.Timers), (void*)t);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and post to the semaphore so the watchdog thread sees it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return t->Handle;
    }


/*** nht_internal_RemoveWatchdog() - removes a timer from the timer list.
 *** Returns 0 if it successfully removed it, or -1 if the timer did not
 *** exist.  In either case, the caller MUST consider the pointer 't'
 *** as invalid once the call returns.
 ***/
int
nht_internal_RemoveWatchdog(handle_t th)
    {
    int idx;
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

	/** Remove the thing from the list, if it exists. **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	idx = xaFindItem(&(NHT.Timers), (void*)t);
	if (idx >= 0) xaRemoveItem(&(NHT.Timers), idx);
	syPostSem(NHT.TimerDataMutex, 1, 0);
	if (idx < 0) return -1;

	/** Free the timer. **/
	xhnFreeHandle(&(NHT.TimerHctx), th);
	nmFree(t, sizeof(NhtTimer));

	/** ... and let the watchdog thread know about it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return 0;
    }


/*** nht_internal_ResetWatchdog() - resets the countdown timer for a 
 *** given watchdog to its original value.  Returns 0 if successful,
 *** or -1 if the thing is no longer valid.
 ***/
int
nht_internal_ResetWatchdog(handle_t th)
    {
    int idx;
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

	/** Find the timer on the list, if it exists. **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	idx = xaFindItem(&(NHT.Timers), (void*)t);
	if (idx >= 0) t->ExpireTick = mtLastTick() + (t->Time*NHT.ClkTck/1000);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and let the watchdog thread know about it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return (idx >= 0)?0:-1;
    }


/*** nht_internal_WTimeout() - this routine is called when the session 
 *** watchdog timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_internal_WTimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Unlink from the session. **/
	nht_internal_UnlinkSess(sess);

    return 0;
    }


/*** nht_internal_ITimeout() - this routine is called when the session 
 *** inactivity timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_internal_ITimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Right now, does the same as WTimeout, so call that. **/
	nht_internal_WTimeout(sess);

    return 0;
    }


/*** nht_internal_DiscardASession - search for sessions by a given user,
 *** and discard a suitably old one.
 ***/
int
nht_internal_DiscardASession(pNhtUser usr)
    {
    int i;
    pNhtSessionData search_s, old_s = NULL;

	for(i=0;i<xaCount(&NHT.Sessions);i++)
	    {
	    search_s = xaGetItem(&NHT.Sessions, i);
	    if (!old_s || old_s->LastAccess > search_s->LastAccess)
		old_s = search_s;
	    }
	if (old_s)
	    nht_internal_UnlinkSess(old_s);

    return 0;
    }


/*** nht_internal_ConstructPathname - constructs the proper OSML pathname
 *** for the open-object operation, given the apparent pathname and url
 *** parameters.  This primarily involves recovering the 'ls__type' setting
 *** and putting it back in the path.
 ***/
int
nht_internal_ConstructPathname(pStruct url_inf)
    {
    pStruct param_inf;
    char* oldpath;
    char* newpath;
    char* old_param;
    char* new_param;
    char* ptr;
    int len;
    int i;
    int first_param = 1;

    	/** Does it have ls__type? **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    param_inf = url_inf->SubInf[i];
	    if (param_inf->Type != ST_T_ATTRIB) continue;
	    if ((!strncmp(param_inf->Name, "ls__", 4) || !strncmp(param_inf->Name, "cx__",4)) && strcmp(param_inf->Name, "ls__type"))
		continue;
	    oldpath = url_inf->StrVal;
	    stAttrValue_ne(param_inf,&old_param);

	    /** Get an encoded param **/
	    len = strlen(old_param)*3+1;
	    new_param = (char*)nmSysMalloc(len);
	    ptr = new_param;
	    while(*old_param) nht_internal_UnConvertChar(*(old_param++), &ptr, len - (ptr - new_param));
	    *ptr = '\0';

	    /** Build the new pathname **/
	    newpath = (char*)nmSysMalloc(strlen(oldpath) + strlen(param_inf->Name) + 
		    strlen(new_param) + 4);
	    sprintf(newpath, "%s%c%s=%s", oldpath, first_param?'?':'&', 
		    param_inf->Name, new_param);

	    /** set the new path and remove the old one **/
	    if (url_inf->StrAlloc) nmSysFree(url_inf->StrVal);
	    url_inf->StrVal = newpath;
	    url_inf->StrAlloc = 1;
	    first_param=0;
	    }

    return 0;
    }


/*** nht_internal_StartTrigger - starts a page that has trigger information
 *** on it
 ***/
int
nht_internal_StartTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;

    	/** Make a new conn completion trigger struct **/
	trg = (pNhtConnTrigger)nmMalloc(sizeof(NhtConnTrigger));
	trg->LinkCnt = 1;
	trg->TriggerSem = syCreateSem(0, 0);
	trg->TriggerID = t_id;
	xaAddItem(&(sess->Triggers), (void*)trg);

    return 0;
    }


/*** nht_internal_EndTrigger - releases a wait on a page completion.
 ***/
int
nht_internal_EndTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;
    int i;

    	/** Search for the trigger **/
	for(i=0;i<sess->Triggers.nItems;i++)
	    {
	    trg = (pNhtConnTrigger)(sess->Triggers.Items[i]);
	    if (trg->TriggerID == t_id)
	        {
		syPostSem(trg->TriggerSem,1,0);
		trg->LinkCnt--;
		if (trg->LinkCnt == 0)
		    {
		    syDestroySem(trg->TriggerSem,0);
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		break;
		}
	    }

    return 0;
    }


/*** nht_internal_WaitTrigger - waits on a trigger on a page.
 ***/
int
nht_internal_WaitTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;
    int i;

    	/** Search for the trigger **/
	for(i=0;i<sess->Triggers.nItems;i++)
	    {
	    trg = (pNhtConnTrigger)(sess->Triggers.Items[i]);
	    if (trg->TriggerID == t_id)
	        {
		trg->LinkCnt++;
		if (syGetSem(trg->TriggerSem, 1, 0) < 0)
		    {
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    break;
		    }
		trg->LinkCnt--;
		if (trg->LinkCnt == 0)
		    {
		    syDestroySem(trg->TriggerSem,0);
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		break;
		}
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


/*** nht_internal_Encode64 - encode a string to MIME base64 encoding.
 ***/
int
nht_internal_Encode64(unsigned char* dst, unsigned char* src, int maxdst)
    {
    static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        /** Step through src 3 bytes at a time, generating 4 dst bytes for each 3 src **/
        while(*src)
            {
            /** First 6 bits of source[0] --> first byte dst. **/
            if (maxdst < 5) return -1;
            dst[0] = b64[src[0]>>2];

            /** Second dst byte from last 2 bits of src[0] and first 4 of src[1] **/
            if (src[1] == '\0')
                {
                dst[1] = b64[(src[0]&0x03)<<4];
                dst[2] = '=';
                dst[3] = '=';
                dst += 4;
                break;
                }
            dst[1] = b64[((src[0]&0x03)<<4) | (src[1]>>4)];

            /** Third dst byte from second 4 bits of src[1] and first 2 of src[2] **/
            if (src[2] == '\0')
                {
                dst[2] = b64[(src[1]&0x0F)<<2];
                dst[3] = '=';
                dst += 4;
                break;
                }
            dst[2] = b64[((src[1]&0x0F)<<2) | (src[2]>>6)];

            /** Last dst byte from last 6 bits of src[2] **/
            dst[3] = b64[(src[2]&0x3F)];

            /** Increment ctrs **/
            maxdst -= 4;
            dst += 4;
            src += 3;
            }

        /** Null-terminate the thing **/
        *dst = '\0';

    return 0;
    }


/*** nht_internal_Decode64 - decodes a MIME base64 encoded string.
 ***/
int
nht_internal_Decode64(char* dst, char* src, int maxdst)
    {
    static char b64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* ptr;
    int ix;

    	/** Step through src 4 bytes at a time. **/
	while(*src)
	    {
	    /** First 6 bits. **/
	    if (maxdst < 4) 
	        {
		mssError(1,"NHT","Could not decode HTTP data field - internal resources exceeded");
		return -1;
		}
	    ptr = strchr(b64,src[0]);
	    if (!ptr) 
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[0] = ix<<2;

	    /** Second six bits are split between dst[0] and dst[1] **/
	    ptr = strchr(b64,src[1]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[0] |= ix>>4;
	    dst[1] = (ix<<4)&0xF0;

	    /** Third six bits are nonmandatory and split between dst[1] and [2] **/
	    if (src[2] == '=' && src[3] == '=')
	        {
		maxdst -= 1;
		dst += 1;
		src += 4;
		break;
		}
	    ptr = strchr(b64,src[2]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[1] |= ix>>2;
	    dst[2] = (ix<<6)&0xC0;

	    /** Fourth six bits are nonmandatory and a part of dst[2]. **/
	    if (src[3] == '=')
	        {
		maxdst -= 2;
		dst += 2;
		src += 4;
		break;
		}
	    ptr = strchr(b64,src[3]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[2] |= ix;
	    maxdst -= 3;
	    src += 4;
	    dst += 3;
	    }

	/** Null terminate the destination **/
	dst[0] = 0;

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


/*** nht_internal_Escape - convert a string to a format that is suitable for
 *** sending to the client and decoding then with javascript's unescape()
 *** function.  Basically, we convert to %xx anything except [A-Za-z0-9].
 ***/
int
nht_internal_Escape(pXString dst, char* src)
    {
    char* ok_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char* hex_code = "0123456789abcdef";
    int len;
    char buf[4] = "%20";
	
	/** go through and encode the chars. **/
	while(*src)
	    {
	    /** Find a "span" of ok chars **/
	    len = strspn(src, ok_chars);
	    if (len < 0) len = 0;
	    if (len)
		{
		/** Concat the span onto the xstring **/
		xsConcatenate(dst, src, len);
		src += len;
		}
	    else
		{
		/** Convert one char **/
		buf[1] = hex_code[(((unsigned char)(*src))>>4)&0x0F];
		buf[2] = hex_code[((unsigned char)(*src))&0x0F];
		xsConcatenate(dst, buf, 3);
		src++;
		}
	    }

    return 0;
    }



/*** nht_internal_UnConvertChar - convert a character back to its escaped
 *** notation such as %xx or +.
 ***/
int
nht_internal_UnConvertChar(int ch, char** bufptr, int maxlen)
    {
    static char hex[] = "0123456789abcdef";

    	/** Room for at least one char? **/
	if (maxlen == 0) return -1;

	/** Else if special char, convert to %xx **/
	if (ch == '/' || ch == '.' || ch == '?' || ch == '%' || ch == '&' || ch == '=' || ch == '+' || ch == ' ')
	    {
	    if (maxlen < 3) return -1;
	    *((*bufptr)++) = '%';
	    *((*bufptr)++) = hex[(ch & 0xF0)>>4];
	    *((*bufptr)++) = hex[(ch & 0x0F)];
	    }

	/** Else convert directly **/
	else *((*bufptr)++) = (ch&0xFF);

    return 0;
    }


/*** nht_internal_EncodeHTML - encode a string such that it can cleanly
 *** be shown in HTML format (change ' ', '<', '>', '&' to &xxx; representation).
 ***/
int
nht_internal_EncodeHTML(int ch, char** bufptr, int maxlen)
    {

    	/** Room for at least one char? **/
	if (maxlen == 0) return -1;

	/** If special char, convert it. **/
	if (ch == ' ')
	    {
	    if (maxlen < 6) return -1;
	    memcpy(*bufptr, "&nbsp;", 6);
	    (*bufptr) += 6;
	    }
	else if (ch == '<')
	    {
	    if (maxlen < 4) return -1;
	    memcpy(*bufptr, "&lt;", 4);
	    (*bufptr) += 4;
	    }
	else if (ch == '>')
	    {
	    if (maxlen < 4) return -1;
	    memcpy(*bufptr, "&gt;", 4);
	    (*bufptr) += 4;
	    }
	else if (ch == '&')
	    {
	    if (maxlen < 5) return -1;
	    memcpy(*bufptr, "&amp;", 5);
	    (*bufptr) += 5;
	    }

	/** Else convert char directly **/
	else *((*bufptr)++) = (ch & 0xFF);

    return 0;
    }


/*** nht_internal_WriteOneAttr - put one attribute's information into the
 *** outbound data connection stream.
 ***/
int
nht_internal_WriteOneAttr(pObject obj, pNhtConn conn, handle_t tgt, char* attrname, int encode)
    {
    ObjData od;
    char* dptr;
    int type,rval;
    XString xs, hints;
    pObjPresentationHints ph;
    static char* coltypenames[] = {"unknown","integer","string","double","datetime","intvec","stringvec","money",""};

	/** Get type **/
	type = objGetAttrType(obj,attrname);
	if (type < 0) return -1;

	/** Presentation Hints **/
	xsInit(&hints);
	ph = objPresentationHints(obj, attrname);
	hntEncodeHints(ph, &hints);
	objFreeHints(ph);

	/** Get value **/
	rval = objGetAttrValue(obj,attrname,type,&od);
	if (rval != 0) 
	    dptr = "";
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) 
	    dptr = objDataToStringTmp(type, (void*)&od, 0);
	else
	    dptr = objDataToStringTmp(type, (void*)(od.String), 0);
	if (!dptr) dptr = "";

	/** Write the HTML output. **/
	xsInit(&xs);
	xsPrintf(&xs, "<A TARGET=X" XHN_HANDLE_PRT " HREF='http://%.40s/?%s#%s'>%s:", 
		tgt, attrname, hints.String, coltypenames[type], (rval==0)?"V":((rval==1)?"N":"E"));
	if (encode)
	    nht_internal_Escape(&xs, dptr);
	else
	    xsConcatenate(&xs,dptr,-1);
	xsConcatenate(&xs,"</A><br>\n",9);
	fdWrite(conn->ConnFD,xs.String,strlen(xs.String),0,0);
	/*printf("%s",xs.String);*/
	xsDeInit(&xs);
	xsDeInit(&hints);

    return 0;
    }


/*** nht_internal_WriteAttrs - write an HTML-encoded attribute list for the
 *** object to the connection, given an object and a connection.
 ***/
int
nht_internal_WriteAttrs(pObject obj, pNhtConn conn, handle_t tgt, int put_meta, int encode)
    {
    char* attr;

	/** Loop throught the attributes. **/
	if (put_meta)
	    {
	    nht_internal_WriteOneAttr(obj, conn, tgt, "name", encode);
	    nht_internal_WriteOneAttr(obj, conn, tgt, "inner_type", encode);
	    nht_internal_WriteOneAttr(obj, conn, tgt, "outer_type", encode);
	    nht_internal_WriteOneAttr(obj, conn, tgt, "annotation", encode);
	    }
	for(attr = objGetFirstAttr(obj); attr; attr = objGetNextAttr(obj))
	    {
	    nht_internal_WriteOneAttr(obj, conn, tgt, attr, encode);
	    }

    return 0;
    }



/*** nht_internal_UpdateNotify() - this routine is called if the UI requested
 *** notifications on an object modification, and such a modification has
 *** indeed occurred.
 ***/
int
nht_internal_UpdateNotify(void* v)
    {
    pObjNotification n = (pObjNotification)v;
    pNhtSessionData sess = (pNhtSessionData)(n->Context);
    handle_t obj_handle = xhnHandle(&(sess->Hctx), n->Obj);
    pNhtControlMsg cm;
    pNhtControlMsgParam cmp;
    XString xs;
    char* dptr;
    int type;

	/** Allocate a control message and set it up **/
	cm = (pNhtControlMsg)nmMalloc(sizeof(NhtControlMsg));
	if (!cm) return -ENOMEM;
	cm->MsgType = NHT_CONTROL_T_REPMSG;
	xaInit(&(cm->Params), 1);
	cm->ResponseSem = NULL;
	cm->ResponseFn = NULL;
	cm->Status = NHT_CONTROL_S_QUEUED;
	cm->Response = NULL;
	cm->Context = NULL;

	/** Parameter indicates what changed and how **/
	cmp = (pNhtControlMsgParam)nmMalloc(sizeof(NhtControlMsgParam));
	if (!cmp)
	    {
	    nmFree(cm, sizeof(NhtControlMsg));
	    return -ENOMEM;
	    }
	memset(cmp, 0, sizeof(NhtControlMsgParam));
	xsInit(&xs);
	xsPrintf(&xs, "X" XHN_HANDLE_PRT, obj_handle);
	cmp->P1 = nmSysStrdup(xs.String);
	xsPrintf(&xs, "http://%s/", n->Name);
	cmp->P3 = nmSysStrdup(xs.String);
	type = n->NewAttrValue.DataType;
	if (n->NewAttrValue.Flags & DATA_TF_NULL) 
	    dptr = "";
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) 
	    dptr = objDataToStringTmp(type, (void*)&n->NewAttrValue.Data, 0);
	else
	    dptr = objDataToStringTmp(type, (void*)(n->NewAttrValue.Data.String), 0);
	if (!dptr) dptr = "";
	xsCopy(&xs, "", 0);
	nht_internal_Escape(&xs, dptr);
	cmp->P2 = nmSysStrdup(xs.String);
	xaAddItem(&cm->Params, (void*)cmp);

	/** Enqueue the thing. **/
	xaAddItem(&sess->ControlMsgsList, (void*)cm);
	syPostSem(sess->ControlMsgs, 1, 0);

    return 0;
    }


/*** nht_internal_Log - generate an access logfile entry
 ***/
int
nht_internal_Log(pNhtConn conn)
    {
    /*pStruct req_inf = conn->ReqURL;*/

	/** logging not available? **/
	if (!NHT.AccessLogFD)
	    return 0;

	/** Print the log message **/
	fdQPrintf(NHT.AccessLogFD, 
		"%STR %STR %STR [%STR] \"%STR&ESCQ\" %INT %INT \"%STR&ESCQ\" \"%STR&ESCQ\"\n",
		conn->IPAddr,
		"-",
		conn->Username,
		"",
		conn->ReqURL?(conn->ReqURL->StrVal):"",
		conn->ResultCode,
		conn->BytesWritten,
		conn->Referrer?conn->Referrer:"",
		conn->UserAgent?conn->UserAgent:"");


    return 0;
    }



/*** nht_internal_OSML - direct OSML access from the client.  This will take
 *** the form of a number of different OSML operations available seemingly
 *** seamlessly (hopefully) from within the JavaScript functionality in an
 *** DHTML document.
 ***/
int
nht_internal_OSML(pNhtConn conn, pObject target_obj, char* request, pStruct req_inf)
    {
    pNhtSessionData sess = conn->NhtSession;
    char* ptr;
    char* newptr;
    pObjSession objsess;
    pObject obj = NULL;
    pObjQuery qy = NULL;
    char* sid = NULL;
    char sbuf[256];
    char sbuf2[256];
    char hexbuf[3];
    int mode,mask;
    char* usrtype;
    int i,t,n,o,cnt,start,flags,len,rval;
    pStruct subinf;
    MoneyType m;
    DateTime dt;
    pDateTime pdt;
    pMoneyType pm;
    double dbl;
    char* where;
    char* orderby;
    int retval;		/** FIXME FIXME FIXME FIXME FIXME FIXME **/
    
    handle_t session_handle;
    handle_t query_handle;
    handle_t obj_handle;
    int encode_attrs = 0;

	if (DEBUG_OSML) stPrint_ne(req_inf);

    	/** Choose the request to perform **/
	if (!strcmp(request,"opensession"))
	    {
	    objsess = objOpenSession(req_inf->StrVal);
	    if (!objsess) 
		session_handle = XHN_INVALID_HANDLE;
	    else
		session_handle = xhnAllocHandle(&(sess->Hctx), objsess);
	    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    session_handle);
	    if (DEBUG_OSML) printf("ls__mode=opensession X" XHN_HANDLE_PRT "\n", session_handle);
	    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
	    }
	else 
	    {
	    /** Need to encode result set? **/
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__encode"),&ptr) >= 0 && !strcmp(ptr,"1"))
		encode_attrs = 1;

	    /** Get the session data **/
	    stAttrValue_ne(stLookup_ne(req_inf,"ls__sid"),&sid);
	    if (!sid) 
		{
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Session ID required for OSML request '%s'",request);
		return -1;
		}
	    if (!strcmp(sid,"XDEFAULT"))
		{
		session_handle = XHN_INVALID_HANDLE;
		objsess = sess->ObjSess;
		}
	    else
		{
		session_handle = xhnStringToHandle(sid+1,NULL,16);
		objsess = (pObjSession)xhnHandlePtr(&(sess->Hctx), session_handle);
		}
	    if (!objsess || !ISMAGIC(objsess, MGK_OBJSESSION))
		{
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Invalid Session ID in OSML request");
		return -1;
		}

	    /** Get object handle, as needed.  If the client specified an
	     ** oid, it had better be a valid one.
	     **/
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__oid"),&ptr) < 0)
		{
		obj_handle = XHN_INVALID_HANDLE;
		}
	    else
		{
		obj_handle = xhnStringToHandle(ptr+1, NULL, 16);
		obj = (pObject)xhnHandlePtr(&(sess->Hctx), obj_handle);
		if (!obj || !ISMAGIC(obj, MGK_OBJECT))
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Invalid Object ID in OSML request");
		    return -1;
		    }
		}

	    /** Get the query handle, as needed.  If the client specified a
	     ** query handle, as with the object handle, it had better be a
	     ** valid one.
	     **/
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__qid"),&ptr) < 0)
		{
		query_handle = XHN_INVALID_HANDLE;
		}
	    else
		{
		query_handle = xhnStringToHandle(ptr+1, NULL, 16);
		qy = (pObjQuery)xhnHandlePtr(&(sess->Hctx), query_handle);
		if (!qy || !ISMAGIC(qy, MGK_OBJQUERY))
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Invalid Query ID in OSML request");
		    return -1;
		    }
		}

	    /** Does this request require an object handle? **/
	    if (obj_handle == XHN_INVALID_HANDLE && (!strcmp(request,"close") || !strcmp(request,"objquery") ||
		!strcmp(request,"read") || !strcmp(request,"write") || !strcmp(request,"attrs") || 
		!strcmp(request, "setattrs") || !strcmp(request,"delete")))
		{
		snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Object ID required for OSML '%s' request", request);
		return -1;
		}

	    /** Does this request require a query handle? **/
	    if (query_handle == XHN_INVALID_HANDLE && (!strcmp(request,"queryfetch") || !strcmp(request,"queryclose")))
		{
		snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Query ID required for OSML '%s' request", request);
		return -1;
		}

	    /** Again check the request... **/
	    if (!strcmp(request,"closesession"))
	        {
		if (session_handle == XHN_INVALID_HANDLE)
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Illegal attempt to close the default OSML session.");
		    return -1;
		    }
		xhnFreeHandle(&(sess->Hctx), session_handle);
	        objCloseSession(objsess);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
	        }
	    else if (!strcmp(request,"open"))
	        {
		/** Get the info and open the object **/
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__usrtype"),&usrtype) < 0) return -1;
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		mode = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmask"),&ptr) < 0) return -1;
		mask = strtol(ptr,NULL,0);
		obj = objOpen(objsess, req_inf->StrVal, mode, mask, usrtype);
		if (!obj)
		    obj_handle = XHN_INVALID_HANDLE;
		else
		    obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    obj_handle);
		if (DEBUG_OSML) printf("ls__mode=open X" XHN_HANDLE_PRT "\n", obj_handle);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);

		if (obj && stAttrValue_ne(stLookup_ne(req_inf,"ls__notify"),&ptr) >= 0 && !strcmp(ptr,"1"))
		    objRequestNotify(obj, nht_internal_UpdateNotify, sess, OBJ_RN_F_ATTRIB);

		/** Include an attribute listing **/
		nht_internal_WriteAttrs(obj,conn,obj_handle,1,encode_attrs);
	        }
	    else if (!strcmp(request,"close"))
	        {
		/** For this, we loop through a comma-separated list of object
		 ** ids to close.
		 **/
		ptr = NULL;
		stAttrValue_ne(stLookup_ne(req_inf,"ls__oid"),&ptr);
		while(ptr && *ptr)
		    {
		    obj_handle = xhnStringToHandle(ptr+1, &newptr, 16);
		    if (newptr <= ptr+1) break;
		    ptr = newptr;
		    obj = (pObject)xhnHandlePtr(&(sess->Hctx), obj_handle);
		    if (!obj || !ISMAGIC(obj, MGK_OBJECT)) 
			{
			mssError(1,"NHT","Invalid object id(s) in OSML 'close' request");
			continue;
			}
		    xhnFreeHandle(&(sess->Hctx), obj_handle);
		    objClose(obj);
		    }
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
	        }
	    else if (!strcmp(request,"multiquery"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__sql"),&ptr) < 0) return -1;
		qy = objMultiQuery(objsess, ptr);
		if (!qy)
		    query_handle = XHN_INVALID_HANDLE;
		else
		    query_handle = xhnAllocHandle(&(sess->Hctx), qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    query_handle);
		if (DEBUG_OSML) printf("ls__mode=multiquery X" XHN_HANDLE_PRT "\n", query_handle);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"objquery"))
	        {
		where=NULL;
		orderby=NULL;
		stAttrValue_ne(stLookup_ne(req_inf,"ls__where"),&where);
		stAttrValue_ne(stLookup_ne(req_inf,"ls__orderby"),&orderby);
		qy = objOpenQuery(obj,where,orderby,NULL,NULL);
		if (!qy)
		    query_handle = XHN_INVALID_HANDLE;
		else
		    query_handle = xhnAllocHandle(&(sess->Hctx), qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    query_handle);
		if (DEBUG_OSML) printf("ls__mode=objquery X" XHN_HANDLE_PRT "\n", query_handle);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"queryfetch"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		mode = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__rowcount"),&ptr) < 0)
		    n = 0x7FFFFFFF;
		else
		    n = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__startat"),&ptr) < 0)
		    start = 0;
		else
		    start = strtol(ptr,NULL,0) - 1;
		if (start < 0) start = 0;
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		         0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		while(start > 0 && (obj = objQueryFetch(qy,mode)))
		    {
		    objClose(obj);
		    start--;
		    }
		while(n > 0 && (obj = objQueryFetch(qy,mode)))
		    {
		    obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
		    if (DEBUG_OSML) printf("ls__mode=queryfetch X" XHN_HANDLE_PRT "\n", obj_handle);
		    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__notify"),&ptr) >= 0 && !strcmp(ptr,"1"))
			objRequestNotify(obj, nht_internal_UpdateNotify, sess, OBJ_RN_F_ATTRIB);
		    nht_internal_WriteAttrs(obj,conn,obj_handle,1,encode_attrs);
		    n--;
		    }
		}
	    else if (!strcmp(request,"queryclose"))
	        {
		xhnFreeHandle(&(sess->Hctx), query_handle);
		objQueryClose(qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"read"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__bytecount"),&ptr) < 0)
		    n = 0x7FFFFFFF;
		else
		    n = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__offset"),&ptr) < 0)
		    o = -1;
		else
		    o = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__flags"),&ptr) < 0)
		    flags = 0;
		else
		    flags = strtol(ptr,NULL,0);
		start = 1;
		while(n > 0 && (cnt=objRead(obj,sbuf,(256>n)?n:256,(o != -1)?o:0,(o != -1)?flags|OBJ_U_SEEK:flags)) > 0)
		    {
		    if(start)
			{
			snprintf(sbuf2,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=X%8.8X>",
			    0);
			fdWrite(conn->ConnFD, sbuf2, strlen(sbuf2), 0,0);
			start = 0;
			}
		    for(i=0;i<cnt;i++)
		        {
		        sprintf(hexbuf,"%2.2X",((unsigned char*)sbuf)[i]);
			fdWrite(conn->ConnFD,hexbuf,2,0,0);
			}
		    n -= cnt;
		    o = -1;
		    }
		if(start)
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=X%8.8X>",
			cnt);
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    start = 0;
		    }
		fdWrite(conn->ConnFD, "</A>\r\n", 6,0,0);
		}
	    else if (!strcmp(request,"write"))
	        {
		}
	    else if (!strcmp(request,"attrs"))
	        {
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		         0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		nht_internal_WriteAttrs(obj,conn,obj_handle,1,encode_attrs);
		}
	    else if (!strcmp(request,"setattrs") || !strcmp(request,"create"))
	        {
		/** First, if creating, open the new object. **/
		if (!strcmp(request,"create"))
		    {
		    len = strlen(req_inf->StrVal);
		    /* let osml determine whether or not it is autoname - it is in a much better position to do so. */
		    /*if (len < 1 || req_inf->StrVal[len-1] != '*' || (len >= 2 && req_inf->StrVal[len-2] != '/'))
			obj = objOpen(objsess, req_inf->StrVal, O_CREAT | O_RDWR, 0600, "system/object");
		    else*/
			obj = objOpen(objsess, req_inf->StrVal, OBJ_O_AUTONAME | O_CREAT | O_RDWR, 0600, "system/object");
		    if (!obj)
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			mssError(0,"NHT","Could not create object");
			return -1;
			}
		    else
			{
			obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
			}
		    }
		else
		    obj_handle = 0;

		/** Find all GET params that are NOT like ls__thingy **/
		for(i=0;i<req_inf->nSubInf;i++)
		    {
		    subinf = req_inf->SubInf[i];
		    if (strncmp(subinf->Name,"ls__",4) != 0 && strncmp(subinf->Name,"cx__",4) != 0)
		        {
			t = objGetAttrType(obj, subinf->Name);
			if (t < 0) return -1;
			switch(t)
			    {
			    case DATA_T_INTEGER:
			        n = objDataToInteger(DATA_T_STRING, subinf->StrVal, NULL);
				retval=objSetAttrValue(obj,subinf->Name,DATA_T_INTEGER,POD(&n));
				break;

			    case DATA_T_DOUBLE:
			        dbl = objDataToDouble(DATA_T_STRING, subinf->StrVal);
				retval=objSetAttrValue(obj,subinf->Name,DATA_T_DOUBLE,POD(&dbl));
				break;

			    case DATA_T_STRING:
			        retval=objSetAttrValue(obj,subinf->Name,DATA_T_STRING,POD(&(subinf->StrVal)));
				break;

			    case DATA_T_DATETIME:
			        objDataToDateTime(DATA_T_STRING, subinf->StrVal, &dt, NULL);
				pdt = &dt;
				retval=objSetAttrValue(obj,subinf->Name,DATA_T_DATETIME,POD(&pdt));
				break;

			    case DATA_T_MONEY:
				pm = &m;
			        objDataToMoney(DATA_T_STRING, subinf->StrVal, &m);
				retval=objSetAttrValue(obj,subinf->Name,DATA_T_MONEY,POD(&pm));
				break;

			    case DATA_T_STRINGVEC:
			    case DATA_T_INTVEC:
			    case DATA_T_UNAVAILABLE: 
			        return -1;
			    }
			//printf("%i\n",retval);
			}
		    }
		if (!strcmp(request,"create"))
		    {
		    rval = objCommit(objsess);
		    if (rval < 0)
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			objClose(obj);
			}
		    else
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
				 obj_handle);
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			if (DEBUG_OSML) printf("ls__mode=create X" XHN_HANDLE_PRT "\n", obj_handle);
			nht_internal_WriteAttrs(obj,conn,obj_handle,1,encode_attrs);
			}
		    }
		else
		    {
		    rval = objCommit(objsess);
		    if (rval < 0)
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			}
		    else
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
				 0);
			}
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    nht_internal_WriteAttrs(obj,conn,obj_handle,1,encode_attrs);
		    }
		}
	    else if (!strcmp(request,"delete"))
		{
		/** Delete an object that is currently open **/
		ptr = NULL;
		stAttrValue_ne(stLookup_ne(req_inf,"ls__oid"),&ptr);
		while(ptr && *ptr)
		    {
		    obj_handle = xhnStringToHandle(ptr+1, &newptr, 16);
		    if (newptr <= ptr+1) break;
		    ptr = newptr;
		    obj = (pObject)xhnHandlePtr(&(sess->Hctx), obj_handle);
		    if (!obj || !ISMAGIC(obj, MGK_OBJECT)) 
			{
			mssError(1,"NHT","Invalid object id(s) in OSML 'delete' request");
			continue;
			}
		    xhnFreeHandle(&(sess->Hctx), obj_handle);

		    /** Delete it **/
		    rval = objDeleteObj(obj);
		    if (rval < 0) break;
		    }
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=%s>&nbsp;</A>\r\n",
		    (rval==0)?"X00000000":"ERR");
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    }
	
    return 0;
    }



/*** nht_internal_CkParams - check to see if we need to set parameters as
 *** a part of the object open process.  This is for opening objects for
 *** read access.
 ***/
int
nht_internal_CkParams(pStruct url_inf, pObject obj)
    {
    pStruct find_inf, search_inf;
    int i,t,n;
    char* ptr = NULL;
    DateTime dt;
    pDateTime dtptr;
    MoneyType m;
    pMoneyType mptr;
    double dbl;

	/** Check for the ls__params=yes tag **/
	stAttrValue_ne(find_inf = stLookup_ne(url_inf,"ls__params"), &ptr);
	if (!ptr || strcmp(ptr,"yes")) return 0;

	/** Ok, look for any params not beginning with ls__ **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    search_inf = url_inf->SubInf[i];
	    if (strncmp(search_inf->Name,"ls__",4) && strncmp(search_inf->Name,"cx__",4))
	        {
		/** Get the value and call objSetAttrValue with it **/
		t = objGetAttrType(obj, search_inf->Name);
		switch(t)
		    {
		    case DATA_T_INTEGER:
		        /*if (search_inf->StrVal == NULL)
		            n = search_inf->IntVal[0];
		        else*/
		            n = strtol(search_inf->StrVal, NULL, 10);
		        objSetAttrValue(obj, search_inf->Name, DATA_T_INTEGER,POD(&n));
			break;

		    case DATA_T_STRING:
		        if (search_inf->StrVal != NULL)
		    	    {
		    	    ptr = search_inf->StrVal;
		    	    objSetAttrValue(obj, search_inf->Name, DATA_T_STRING,POD(&ptr));
			    }
			break;
		    
		    case DATA_T_DOUBLE:
		        /*if (search_inf->StrVal == NULL)
		            dbl = search_inf->IntVal[0];
		        else*/
		            dbl = strtod(search_inf->StrVal, NULL);
			objSetAttrValue(obj, search_inf->Name, DATA_T_DOUBLE,POD(&dbl));
			break;

		    case DATA_T_MONEY:
		        /*if (search_inf->StrVal == NULL)
			    {
			    m.WholePart = search_inf->IntVal[0];
			    m.FractionPart = 0;
			    }
			else
			    {*/
			    objDataToMoney(DATA_T_STRING, search_inf->StrVal, &m);
			    /*}*/
			mptr = &m;
			objSetAttrValue(obj, search_inf->Name, DATA_T_MONEY,POD(&mptr));
			break;
		
		    case DATA_T_DATETIME:
		        if (search_inf->StrVal != NULL)
			    {
			    objDataToDateTime(DATA_T_STRING, search_inf->StrVal, &dt,NULL);
			    dtptr = &dt;
			    objSetAttrValue(obj, search_inf->Name, DATA_T_DATETIME,POD(&dtptr));
			    }
			break;
		    }
		}
	    }

    return 0;
    }

/*** nht_internal_GetGeom() - deploy a snippet of javascript to the browser
 *** to fetch the window geometry and reload the application.
 ***/
int
nht_internal_GetGeom(pObject target_obj, pFile output)
    {
    char bgnd[128];
    char* ptr;

	/** Do we have a bgcolor / background? **/
	if (objGetAttrValue(target_obj, "bgcolor", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    snprintf(bgnd, sizeof(bgnd), "bgcolor='%.100s'", ptr);
	    }
	else if (objGetAttrValue(target_obj, "background", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    snprintf(bgnd, sizeof(bgnd), "background='%.100s'", ptr);
	    }
	else
	    {
	    strcpy(bgnd, "bgcolor='white'");
	    }

	/** Generate the snippet **/
	fdPrintf(output, "<html>\n"
			 "<head>\n"
			 "    <meta http-equiv=\"Pragma\" CONTENT=\"no-cache\">\n"
			 "    <style type=\"text/css\">\n"
			 "        #l1 { POSITION:absolute; VISIBILITY: hidden; left:0px; top:0px; }\n"
			 "        #l2 { POSITION:absolute; VISIBILITY: hidden; left:0px; top:0px; }\n"
			 "    </style>\n"
			 "</head>\n"
			 "<script language=\"javascript\" src=\"/sys/js/startup.js\" DEFER></script>\n"
			 "<body %s onload='startup();'>\n"
			 "    <img src='/sys/images/loading.gif'>\n"
			 "    <div id=\"l1\">x<br>x</div>\n"
			 "    <div id=\"l2\">xx</div>\n"
			 "</body>\n"
			 "</html>\n", bgnd);

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
    int encode_attrs = 0;
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

	acceptencoding=(char*)mssGetParam("Accept-Encoding");

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

	/** app key specified? **/
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

		/** Read the app spec, verify it, and generate it to DHTML **/
		if (wgtrRenderObject(conn->ConnFD, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML") < 0)
		    {
		    mssError(0, "HTTP", "Invalid application %s of type %s", url_inf->StrVal, ptr);
		    fdPrintf(conn->ConnFD,"<h1>An error occurred while constructing the application:</h1><pre>");
		    mssPrintError(conn->ConnFD);
		    objClose(target_obj);
		    return -1;
		    }
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
		    fdPrintf(conn->ConnFD, "<A TARGET=\"ERR\" HREF=\"%s\"></A>", url_inf->StrVal);
		    }
		else
		    {
		    fdPrintf(conn->ConnFD, "<A NAME=\"%s\" TARGET=\"OK\" HREF=\"%s\"></A>", ptr, url_inf->StrVal);
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
		fdPrintf(conn->ConnFD,"<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=%s/..>..</A><BR>\n",url_inf->StrVal);
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
			fdPrintf(conn->ConnFD,"<A HREF=%s%s%s TARGET='%s'>%d:%d:%s</A><BR>\n",dptr,
			    (dptr[0]=='/' && dptr[1]=='\0')?"":"/",ptr,ptr,objinfo->Flags,objinfo->nSubobjects,aptr);
			}
		    else if (send_info && !objinfo)
			{
			fdPrintf(conn->ConnFD,"<A HREF=%s%s%s TARGET='%s'>0:0:%s</A><BR>\n",dptr,
			    (dptr[0]=='/' && dptr[1]=='\0')?"":"/",ptr,ptr,aptr);
			}
		    else
			{
			fdPrintf(conn->ConnFD,"<A HREF=%s%s%s TARGET='%s'>%s</A><BR>\n",dptr,
			    (dptr[0]=='/' && dptr[1]=='\0')?"":"/",ptr,ptr,aptr);
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

	    /** Need to encode result set? **/
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__encode"),&ptr) >= 0 && !strcmp(ptr,"1"))
		encode_attrs = 1;

	    /** Get the SQL **/
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__sql"),&ptr) >= 0)
	        {
		query = objMultiQuery(nsess->ObjSess, ptr);
		if (query)
		    {
		    rowid = 0;
		    while((sub_obj = objQueryFetch(query,O_RDONLY)))
		        {
			nht_internal_WriteAttrs(sub_obj,conn,(handle_t)rowid,1,encode_attrs);
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
		nht_internal_Decode64(conn->Auth,mlxStringVal(s,NULL),sizeof(conn->Auth));
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
		//printf("accept-encoding: %s\n",acceptencoding);
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


/*** nht_internal_ConnHandler - manages a single incoming HTTP connection
 *** and processes the connection's request.
 ***/
void
nht_internal_ConnHandler(void* connfd_v)
    {
    pFile connfd = (pFile)connfd_v;
    char sbuf[160];
    char* msg = "";
    char* ptr;
    char* usrname;
    char* passwd = NULL;
    pStruct url_inf,find_inf;
    int tid = -1;
    handle_t w_timer = XHN_INVALID_HANDLE, i_timer = XHN_INVALID_HANDLE;
    pNhtUser usr;
    pNhtConn conn = NULL;
    pNhtSessionData nsess;
    int akey[2];

    	/*printf("ConnHandler called, stack ptr = %8.8X\n",&s);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Connection Handler");

	/** Create the connection structure **/
	conn = nht_internal_AllocConn(connfd);
	if (!conn)
	    {
	    netCloseTCP(connfd, 1000, 0);
	    thExit();
	    }

	/** Parse the HTTP Headers... **/
	if (nht_internal_ParseHeaders(conn) < 0)
	    {
	    msg = "Error parsing headers";
	    goto error;
	    }

	/** Did client send authentication? **/
	if (!*(conn->Auth))
	    {
	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
	    		 "Server: %s\r\n"
			 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	    //printf("%s",sbuf);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    nht_internal_FreeConn(conn);
	    thExit();
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(conn->Auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (!usrname || !passwd) 
	    {
	    snprintf(sbuf,160,"HTTP/1.0 400 Bad Request\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Bad Request</H1>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    nht_internal_FreeConn(conn);
	    thExit();
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*(conn->Cookie))
	    {
	    if (conn->Cookie[strlen(conn->Cookie)-1] == ';') conn->Cookie[strlen(conn->Cookie)-1] = '\0';
	    conn->NhtSession = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), conn->Cookie);
	    if (conn->NhtSession)
	        {
		nht_internal_LinkSess(conn->NhtSession);
		if (strcmp(conn->NhtSession->Username,usrname) || strcmp(passwd,conn->NhtSession->Password))
		    {
	    	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
		    		 "Server: %s\r\n"
				 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    nht_internal_FreeConn(conn);
	            thExit();
		    }
		thSetParam(NULL,"mss",conn->NhtSession->Session);
		thSetUserID(NULL,((pMtSession)(conn->NhtSession->Session))->UserID);
		w_timer = conn->NhtSession->WatchdogTimer;
		i_timer = conn->NhtSession->InactivityTimer;
		}
	    }
	else
	    {
	    conn->NhtSession = NULL;
	    }

	/** Watchdog ping? **/
	if (!strcmp(conn->URL,"/INTERNAL/ping"))
	    {
	    if (conn->NhtSession)
		{
		/** Reset only the watchdog timer on a ping. **/
		if (nht_internal_ResetWatchdog(w_timer))
		    {
		    snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
				 "Server: %s\r\n"
				 "Pragma: no-cache\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    nht_internal_FreeConn(conn);
		    thExit();
		    }
		else
		    {
		    snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
				 "Server: %s\r\n"
				 "Pragma: no-cache\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=OK></A>\r\n",NHT.ServerString);
		    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    nht_internal_FreeConn(conn);
		    thExit();
		    }
		}
	    else
		{
		/** No session and this is a watchdog ping?  If so, we don't
		 ** want to automatically re-login the user since that defeats the purpose
		 ** of session timeouts.
		 **/
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
		thExit();
		}
	    }
	else if (conn->NhtSession)
	    {
	    /** Reset the idle and watchdog timers on a normal request **/
	    if (nht_internal_ResetWatchdog(i_timer) < 0 || nht_internal_ResetWatchdog(w_timer) < 0)
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
		thExit();
		}
	    }

	/** No cookie or no session for the given cookie? **/
	if (!conn->NhtSession)
	    {
	    if (mssAuthenticate(usrname, passwd) < 0)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
			     "Server: %s\r\n"
			     "Content-Type: text/html\r\n"
			     "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			     "\r\n"
			     "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	        fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
	        thExit();
		}
	    usr = (pNhtUser)xhLookup(&(NHT.Users), usrname);
	    if (!usr)
		{
		usr = (pNhtUser)nmMalloc(sizeof(NhtUser));
		usr->SessionCnt = 0;
		strtcpy(usr->Username, usrname, sizeof(usr->Username));
		xhAdd(&(NHT.Users), usr->Username, (void*)(usr));
		}
	    if (usr->SessionCnt >= NHT.UserSessionLimit)
		{
		nht_internal_DiscardASession(usr);
		}
	    nsess = (pNhtSessionData)nmMalloc(sizeof(NhtSessionData));
	    strtcpy(nsess->Username, mssUserName(), sizeof(nsess->Username));
	    strtcpy(nsess->Password, mssPassword(), sizeof(nsess->Password));
	    nsess->User = usr;
	    nsess->Session = thGetParam(NULL,"mss");
	    nsess->IsNewCookie = 1;
	    nsess->ObjSess = objOpenSession("/");
	    nsess->Errors = syCreateSem(0,0);
	    nsess->ControlMsgs = syCreateSem(0,0);
	    nsess->WatchdogTimer = nht_internal_AddWatchdog(NHT.WatchdogTime*1000, nht_internal_WTimeout, (void*)nsess);
	    nsess->InactivityTimer = nht_internal_AddWatchdog(NHT.InactivityTime*1000, nht_internal_ITimeout, (void*)nsess);
	    nsess->LinkCnt = 1;
	    xaInit(&nsess->Triggers,16);
	    xaInit(&nsess->ErrorList,16);
	    xaInit(&nsess->ControlMsgsList,16);
	    nht_internal_CreateCookie(nsess->Cookie);
	    cxssGenerateKey((unsigned char*)akey, sizeof(akey));
	    sprintf(nsess->AKey, "%8.8x%8.8x", akey[0], akey[1]);
	    xhnInitContext(&(nsess->Hctx));
	    xhAdd(&(NHT.CookieSessions), nsess->Cookie, (void*)nsess);
	    usr->SessionCnt++;
	    xaAddItem(&(NHT.Sessions), (void*)nsess);
	    conn->NhtSession = nsess;
	    printf("NHT: new session for username [%s], cookie [%s]\n", nsess->Username, nsess->Cookie);
	    nht_internal_LinkSess(conn->NhtSession);
	    }

	/** Set nht session http ver **/
	strtcpy(conn->NhtSession->HTTPVer, conn->HTTPVer, sizeof(conn->NhtSession->HTTPVer));

	/** Version compatibility **/
	if (!strcmp(conn->NhtSession->HTTPVer, "HTTP/1.0"))
	    {
	    conn->NhtSession->ver_10 = 1;
	    conn->NhtSession->ver_11 = 0;
	    }
	else if (!strcmp(conn->NhtSession->HTTPVer, "HTTP/1.1"))
	    {
	    conn->NhtSession->ver_10 = 1;
	    conn->NhtSession->ver_11 = 1;
	    }
	else
	    {
	    conn->NhtSession->ver_10 = 0;
	    conn->NhtSession->ver_11 = 0;
	    }

	/** Set the session's UserAgent if one was found in the headers. **/
	if (conn->UserAgent[0])
	    {
	    mssSetParam("User-Agent", conn->UserAgent);
	    }

	/** Set the session's AcceptEncoding if one was found in the headers. **/
	/*if (acceptencoding)
	    {
	    if (*acceptencoding) mssSetParam("Accept-Encoding", acceptencoding);
	    nmFree(acceptencoding, 160);
	    }*/

	/** Parse out the requested url **/
	/*printf("debug: %s\n",urlptr);*/
	url_inf = htsParseURL(conn->URL);
	if (!url_inf)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 500 Internal Server Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>500 Internal Server Error</H1>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    nht_internal_FreeConn(conn);
	    conn = NULL;
	    }
	nht_internal_ConstructPathname(url_inf);

	/** Need to start an available connection completion trigger on this? **/
	if ((find_inf=stLookup_ne(url_inf,"ls__triggerid")))
	    {
	    tid = strtol(find_inf->StrVal,NULL,0);
	    nht_internal_StartTrigger(conn->NhtSession, tid);
	    }

	/** If the method was GET and an ls__method was specified, use that method **/
	if (!strcmp(conn->Method,"get") && (find_inf=stLookup_ne(url_inf,"ls__method")))
	    {
	    find_inf = stLookup_ne(url_inf,"cx__akey");
	    if (!find_inf || strcmp(find_inf->StrVal, nsess->AKey))
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
		thExit();
		}
	    if (!strcasecmp(find_inf->StrVal,"get"))
	        {
	        nht_internal_GET(conn,url_inf,conn->IfModifiedSince);
		}
	    else if (!strcasecmp(find_inf->StrVal,"copy"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__destination");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__destination for copy</H1>\r\n",NHT.ServerString);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    nht_internal_COPY(conn,url_inf, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"put"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__content for put</H1>\r\n",NHT.ServerString);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_internal_PUT(conn,url_inf,conn->Size,ptr);
		    }
		}
	    }
	else
	    {
	    /** Which method was used? **/
	    if (!strcmp(conn->Method,"get"))
	        {
	        nht_internal_GET(conn,url_inf,conn->IfModifiedSince);
	        }
	    else if (!strcmp(conn->Method,"put"))
	        {
	        nht_internal_PUT(conn,url_inf,conn->Size,NULL);
	        }
	    else if (!strcmp(conn->Method,"post"))
		{
		nht_internal_POST(conn, url_inf, conn->Size);
		}
	    else if (!strcmp(conn->Method,"copy"))
	        {
	        nht_internal_COPY(conn,url_inf, conn->Destination);
	        }
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 501 Not Implemented\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>501 Method Not Implemented</H1>\r\n",NHT.ServerString);
	        fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	        /*netCloseTCP(conn,1000,0);*/
		}
	    }

	/** End a trigger? **/
	if (tid != -1) nht_internal_EndTrigger(conn->NhtSession,tid);

	/*nht_internal_UnlinkSess(conn->NhtSession);*/

	/** Close and exit. **/
	if (url_inf) stFreeInf_ne(url_inf);
	nht_internal_FreeConn(conn);
	conn = NULL;

    thExit();

    error:
	mssError(1,"NHT","Failed to parse HTTP request, exiting thread.");
	snprintf(sbuf,160,"HTTP/1.0 400 Request Error\r\n\r\n%s\r\n",msg);
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	if (conn) nht_internal_FreeConn(conn);
    thExit();
    }


/*** nht_internal_Handler - manages incoming HTTP connections and sends
 *** the appropriate documents/etc to the requesting client.
 ***/
void
nht_internal_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    int i;

    	/*printf("Handler called, stack ptr = %8.8X\n",&listen_socket);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Network Listener");

	/** Get our configuration **/
	strcpy(listen_port,"800");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_http");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    {
		    strtcpy(listen_port, strval, sizeof(listen_port));
		    }
		else
		    {
		    snprintf(listen_port,32,"%d",intval);
		    }
		}

	    /** Find out what server string we should use **/
	    if (stAttrValue(stLookup(my_config, "server_string"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.ServerString, strval, sizeof(NHT.ServerString));
		}
	    else
		{
		snprintf(NHT.ServerString, 80, "Centrallix/%.16s", cx__version);
		}

	    /** Get the realm name **/
	    if (stAttrValue(stLookup(my_config, "auth_realm"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.Realm, strval, sizeof(NHT.Realm));
		}
	    else
		{
		snprintf(NHT.Realm, 80, "Centrallix");
		}

	    /** Directory indexing? **/
	    for(i=0;i<sizeof(NHT.DirIndex)/sizeof(char*);i++)
		{
		stAttrValue(stLookup(my_config,"dir_index"), NULL, &(NHT.DirIndex[i]), i);
		}

	    /** Should we enable gzip? **/
#ifdef HAVE_LIBZ
	    stAttrValue(stLookup(my_config, "enable_gzip"), &(NHT.EnableGzip), NULL, 0);
#endif
	    stAttrValue(stLookup(my_config, "condense_js"), &(NHT.CondenseJS), NULL, 0);

	    /** Get the timer settings **/
	    stAttrValue(stLookup(my_config, "session_watchdog_timer"), &(NHT.WatchdogTime), NULL, 0);
	    stAttrValue(stLookup(my_config, "session_inactivity_timer"), &(NHT.InactivityTime), NULL, 0);

	    /** Session limits **/
	    stAttrValue(stLookup(my_config, "user_session_limit"), &(NHT.UserSessionLimit), NULL, 0);

	    /** Access log file **/
	    if (stAttrValue(stLookup(my_config, "access_log"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.AccessLogFile, strval, sizeof(NHT.Realm));
		NHT.AccessLogFD = fdOpen(NHT.AccessLogFile, O_WRONLY | O_APPEND, 0600);
		if (!NHT.AccessLogFD)
		    {
		    mssErrorErrno(1,"NHT","Could not open access_log file '%s'", NHT.AccessLogFile);
		    }
		}

	    if (stAttrValue(stLookup(my_config, "session_cookie"), NULL, &strval, 0) < 0)
		{
		strval = "CXID";
		}
	    strtcpy(NHT.SessionCookie, strval, sizeof(NHT.SessionCookie));
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NHT","Could not open network listener");
	    thExit();
	    }
	
    	/*printf("Handler return from netListenTCP, stack ptr = %8.8X\n",&listen_socket);*/

	/** Loop, accepting requests **/
	while((connection_socket = netAcceptTCP(listen_socket,0)))
	    {
	    if (!connection_socket)
		{
		thSleep(10);
		continue;
		}
	    if (!thCreate(nht_internal_ConnHandler, 0, connection_socket))
	        {
		mssError(1,"NHT","Could not create thread to handle connection!");
		netCloseTCP(connection_socket,0,0);
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
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
	xhInit(&(NHT.Users), 255, 0);
	NHT.AccCnt = 0;

#ifdef _SC_CLK_TCK
        NHT.ClkTck = sysconf(_SC_CLK_TCK);
#else
        NHT.ClkTck = CLK_TCK;
#endif

	/* intialize the regex for netscape 4.7 -- it has a broken gzip implimentation */
	NHT.reNet47=(regex_t *)nmMalloc(sizeof(regex_t));
	if(!NHT.reNet47 || regcomp(NHT.reNet47, "Mozilla\\/4\\.(7[5-9]|8)",REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    {
	    printf("unable to build Netscape 4.7 regex\n"); // shouldn't this be mssError? -- but there's no session yet..
	    return -1;
	    }

	/** Start the watchdog timer thread **/
	thCreate(nht_internal_Watchdog, 0, NULL);

	/** Start the network listener. **/
	thCreate(nht_internal_Handler, 0, NULL);

	/*printf("nhtInit return from thCreate, stack ptr = %8.8X\n",&i);*/

    return 0;
    }

