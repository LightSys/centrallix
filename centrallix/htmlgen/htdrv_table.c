#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2007 LightSys Technology Services, Inc.		*/
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
/*									*/
/* Module: 	htdrv_table.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 29, 1999 					*/
/* Description:	HTML Widget driver for a data-driven table.  Has three	*/
/*		different modes -- static, dynamicpage, and dynamicrow.	*/
/*									*/
/*		Static means an inline table that can't be updated	*/
/*		without a parent container being completely reloaded.	*/
/*		DynamicPage means a table in a layer that can be	*/
/*		reloaded dynamically as a whole when necessary.  Good	*/
/*		when you need forward/back without reloading the page.	*/
/*		DynamicRow means each row is its own layer.  Good when	*/
/*		you need to insert rows dynamically and delete rows	*/
/*		dynamically at the client side without reloading the	*/
/*		whole table contents.					*/
/*									*/
/*		A static table's query is performed on the server side	*/
/*		and the HTML is generated at the server.  Both dynamic	*/
/*		types are built from a client-side query.  Static 	*/
/*		tables are generally best when the data will be read-	*/
/*		only.  Dynamicrow tables use the most client resources.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_table.c,v 1.51 2007/04/19 21:26:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_table.c,v $

    $Log: htdrv_table.c,v $
    Revision 1.51  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.50  2007/04/05 03:34:54  gbeeley
    - (feature) Port of the widget/table to Mozilla

    Revision 1.49  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.48  2006/10/16 18:34:34  gbeeley
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

    Revision 1.47  2005/10/18 22:51:44  gbeeley
    - (bugfix) correct deletion of orderby items.

    Revision 1.46  2005/06/23 22:08:01  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.45  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

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

    Revision 1.43  2004/08/04 20:03:10  mmcgill
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

    Revision 1.42  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.41  2004/08/02 14:09:35  mmcgill
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

    Revision 1.40  2004/07/20 21:28:52  mmcgill
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

    Revision 1.39  2004/07/19 15:30:40  mmcgill
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

    Revision 1.38  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.37  2003/06/05 20:53:02  gbeeley
    Fix for "t not found" error on scrolling on some tables.

    Revision 1.36  2003/06/03 19:27:09  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.35  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.34  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.33  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.32  2002/08/26 20:49:33  lkehresman
    Added DblClick event to the table rows taht does essentially thte same
    thing that the Click event does.

    Revision 1.31  2002/08/18 18:46:27  jorupp
     * made the entire current record available on the event object (accessable as .data)

    Revision 1.30  2002/08/13 04:36:29  anoncvs_obe
    Changed the 't' table inf structure to be dynamically allocated to
    save the 1.2k that was used on the stack.

    Revision 1.29  2002/08/05 19:43:37  lkehresman
    Fixed the static table to reference the standard ".layer" rather than ".Layer"

    Revision 1.28  2002/07/25 20:05:15  mcancel
    Adding the function htrAddScriptInclude to the static table render
    function so the javascript code will be seen...

    Revision 1.27  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.26  2002/07/25 16:54:18  pfinley
    completely undoing the change made yesterday with aliasing of click events
    to mouseup... they are now two separate events. don't believe the lies i said
    yesterday :)

    Revision 1.25  2002/07/24 18:12:03  pfinley
    Updated Click events to be MouseUp events. Now all Click events must be
    specified as MouseUp within the Widget's event handler, or they will not
    work propery (Click can still be used as a event connector to the widget).

    Revision 1.24  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.23  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.22  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.21  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.20  2002/06/24 17:28:25  jorupp
     * fix bug where records would stay hidden following a query that returned 1 record when the next one returned multiple records

    Revision 1.19  2002/06/19 21:22:45  lkehresman
    Added a losefocushandler to the table.  Not having this broke static tables.

    Revision 1.18  2002/06/10 21:47:45  jorupp
     * bit of code cleanup
     * added movable borders to the dynamic table

    Revision 1.17  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.16  2002/06/03 18:43:45  jorupp
     * fixed a bug with the handling of empty fields in the dynamic table

    Revision 1.15  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.14  2002/05/31 02:40:38  lkehresman
    Added a horrible hack to fix the hang problem.  The previous hack involved
    alert windows.  This one is at least transparent (as it uses a while loop
    to pause).  Still VERY ugly, but I have no other ideas.

    Revision 1.13  2002/05/30 05:01:31  jorupp
     * OSRC has a Sync Action (best used to tie two OSRCs together on a table selection)
     * NOTE: with multiple tables in an app file, netscape seems to like to hang (the JS engine at least)
        while rendering the page.  uncomment line 1109 in htdrv_table.c to fix it (at the expense of extra alerts)
        -- I tried to figure this out, but was unsuccessful....

    Revision 1.12  2002/05/30 03:55:21  lkehresman
    editbox:  * added readonly flag so the editbox is always only readonly
              * made disabled appear visually
    table:    * fixed a typo

    Revision 1.11  2002/05/01 02:25:50  jorupp
     * more changes

    Revision 1.10  2002/04/30 18:08:43  jorupp
     * more additions to the table -- now it can scroll~
     * made the osrc and form play nice with the table
     * minor changes to form sample

    Revision 1.9  2002/04/28 00:30:53  jorupp
     * full sorting support added to table

    Revision 1.8  2002/04/27 22:47:45  jorupp
     * re-wrote form and osrc interaction -- more happens now in the form
     * lots of fun stuff in the table....check form.app for an example (not completely working yet)
     * the osrc is still a little bit buggy.  If you get it screwed up, let me know how to reproduce it.

    Revision 1.7  2002/04/27 18:42:22  jorupp
     * the dynamicrow table widget will now change the current osrc row when you click a row...

    Revision 1.6  2002/04/27 06:37:45  jorupp
     * some bug fixes in the form
     * cleaned up some debugging output in the label
     * added a dynamic table widget

    Revision 1.5  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.4  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.3  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.2  2001/10/16 23:53:01  gbeeley
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
    HTTBL;

typedef struct
    {
    char name[64];
    char sbuf[160];
    char tbl_bgnd[128];
    char hdr_bgnd[128];
    char row_bgnd1[128];
    char row_bgnd2[128];
    char row_bgndhigh[128];
    char textcolor[64];
    char textcolorhighlight[64];
    char titlecolor[64];
    int x,y,w,h;
    int id;
    int mode;
    int outer_border;
    int inner_border;
    int inner_padding;
    pStructInf col_infs[24];
    int ncols;
    int windowsize;
    int rowheight;
    int cellhspacing;
    int cellvspacing;
    int followcurrent;
    int dragcols;
    int colsep;
    int gridinemptyrows;
    } httbl_struct;

int
httblRenderDynamic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    int colid;
    int colw;
    char *coltitle;
    char *ptr;
    int i;
    pWgtrNode sub_tree;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTBL","Netscape 4 DOM or W3C DOM support required");
	    return -1;
	    }

	/** STYLE for the layer **/
	htrAddStylesheetItem_va(s,"\t#tbld%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; } \n",t->id,t->x,t->y,t->w-18,z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%POSscroll { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:18px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",t->id,t->x+t->w-18,t->y+t->rowheight,t->h-t->rowheight,z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%POSbox { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:18px; HEIGHT:18px; Z-INDEX:%POS; }\n",t->id,z+2);

	/** HTML body <DIV> element for the layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSpane\"></DIV>\n",t->id);
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSscroll\">\n",t->id);
	htrAddBodyItem(s,"<TABLE border=0 cellspacing=0 cellpadding=0 width=18>\n");
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico13b.gif NAME=u></TD></TR>\n");
	htrAddBodyItem_va(s,"<TR><TD height=%POS></TD></TR>\n",t->h-2*18-t->rowheight-t->cellvspacing);
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico12b.gif NAME=d></TD></TR>\n");
	htrAddBodyItem(s,"</TABLE>\n");
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSbox\"><IMG SRC=/sys/images/ico14b.gif NAME=b></DIV>\n",t->id);
	htrAddBodyItem(s,"</DIV>\n");

	htrAddScriptGlobal(s,"tbld_current","null",0);
	htrAddScriptGlobal(s,"tbldb_current","null",0);
	htrAddScriptGlobal(s,"tbldb_start","null",0);
	htrAddScriptGlobal(s,"tbldbdbl_current","null",0);

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);

	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tbld%POSpane\")",t->id);

	htrAddScriptInit_va(s,"    tbld_init({tablename:'%STR&SYM', table:nodes[\"%STR&SYM\"], scroll:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"tbld%POSscroll\"), boxname:\"tbld%POSbox\", name:\"%STR&SYM\", height:%INT, width:%INT, innerpadding:%INT, innerborder:%INT, windowsize:%INT, rowheight:%INT, cellhspacing:%INT, cellvspacing:%INT, textcolor:\"%STR&ESCQ\", textcolorhighlight:\"%STR&ESCQ\", titlecolor:\"%STR&ESCQ\", rowbgnd1:\"%STR&ESCQ\", rowbgnd2:\"%STR&ESCQ\", rowbgndhigh:\"%STR&ESCQ\", hdrbgnd:\"%STR&ESCQ\", followcurrent:%INT, dragcols:%INT, colsep:%INT, gridinemptyrows:%INT, cols:new Array(",
		t->name,t->name,t->name,t->id,t->id,t->name,t->h,t->w-18,
		t->inner_padding,t->inner_border,t->windowsize,t->rowheight,
		t->cellvspacing, t->cellhspacing,t->textcolor, 
		t->textcolorhighlight, t->titlecolor,t->row_bgnd1,t->row_bgnd2,
		t->row_bgndhigh,t->hdr_bgnd,t->followcurrent,t->dragcols,
		t->colsep,t->gridinemptyrows);
	
	for(colid=0;colid<t->ncols;colid++)
	    {
	    stAttrValue(stLookup(t->col_infs[colid],"title"),NULL,&coltitle,0);
	    stAttrValue(stLookup(t->col_infs[colid],"width"),&colw,NULL,0);
	    htrAddScriptInit_va(s,"new Array(\"%STR&SYM\",\"%STR&ESCQ\",%POS),",t->col_infs[colid]->Name,coltitle,colw);
	    }

	htrAddScriptInit(s,"null)});\n");

	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (strcmp(ptr,"widget/table-column") != 0) //got columns earlier
		htrRenderWidget(s, sub_tree, z+3);
	    }

	htrAddEventHandlerFunction(s,"document","MOUSEOVER","tbld","tbld_mouseover");
	htrAddEventHandlerFunction(s,"document","MOUSEDOWN","tbld","tbld_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","tbld","tbld_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tbld","tbld_mouseup");

    return 0;
    }


int
httblRenderStatic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    pObject qy_obj;
    pObjQuery qy;
    char* ptr;
    char* sql;
    int rowid,type,rval;
    char* attr;
    char* str;
    ObjData od;
    int colid;
    int n;

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);

	/** flag ourselves as not having an associated layer **/
	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	htrAddBodyItem_va(s,"<TABLE %[width=%POS%] border=%POS cellspacing=0 cellpadding=0 %STR><TR><TD>\n", 
		t->w >= 0, t->w - (t->outer_border + (t->outer_border?1:0))*2, t->outer_border, t->tbl_bgnd);
	htrAddBodyItem_va(s,"<TABLE border=0 background=/sys/images/trans_1.gif cellspacing=%POS cellpadding=%POS %[width=%POS%]>\n",
		t->inner_border, t->inner_padding, t->w >= 0, t->w - (t->outer_border + (t->outer_border?1:0))*2);
	if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0)
	    {
	    mssError(1,"HTTBL","Static datatable must have SQL property");
	    return -1;
	    }
	qy = objMultiQuery(s->ObjSession, sql);
	if (!qy)
	    {
	    mssError(0,"HTTBL","Could not open query for static datatable");
	    return -1;
	    }
	rowid = 0;
	while((qy_obj = objQueryFetch(qy, O_RDONLY)))
	    {
	    if (rowid == 0)
		{
		/** Do table header if header data provided. **/
		htrAddBodyItem_va(s,"    <TR %STR>", t->hdr_bgnd);
		if (t->ncols == 0)
		    {
		    for(colid=0,attr = objGetFirstAttr(qy_obj); attr; colid++,attr = objGetNextAttr(qy_obj))
			{
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%STR&SYM_\" src=/sys/images/trans_1.gif align=top>", t->name);
			    }
			else
			    htrAddBodyItem(s,"<TH align=left>");
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%STR&HTE'>",t->titlecolor);
			    }
			htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		else
		    {
		    for(colid = 0; colid < t->ncols; colid++)
			{
			attr = t->col_infs[colid]->Name;
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%STR&SYM_\" src=/sys/images/trans_1.gif align=top>", t->name);
			    }
			else
			    {
			    htrAddBodyItem(s,"<TH align=left>");
			    }
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%STR&HTE'>",t->titlecolor);
			    }
			if (stAttrValue(stLookup(t->col_infs[colid],"title"), NULL, &ptr, 0) == 0)
			    htrAddBodyItem(s,ptr);
			else
			    htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		htrAddBodyItem(s,"</TR>\n");
		}
	    htrAddBodyItem_va(s,"    <TR %STR>", (rowid&1)?((*(t->row_bgnd2))?t->row_bgnd2:t->row_bgnd1):t->row_bgnd1);

	    /** Build the row contents -- loop through attrs and convert to strings **/
	    colid = 0;
	    if (t->ncols == 0)
		attr = objGetFirstAttr(qy_obj);
	    else
		attr = t->col_infs[colid]->Name;
	    while(attr)
		{
		if (t->ncols && stAttrValue(stLookup(t->col_infs[colid],"width"),&n,NULL,0) == 0 && n >= 0)
		    {
		    htrAddBodyItem_va(s,"<TD width=%POS nowrap>",n*7);
		    }
		else
		    {
		    htrAddBodyItem(s,"<TD nowrap>");
		    }
		type = objGetAttrType(qy_obj,attr);
		rval = objGetAttrValue(qy_obj,attr,type,&od);
		if (rval == 0)
		    {
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    }
		else if (rval == 1)
		    {
		    str = "NULL";
		    }
		else
		    {
		    str = NULL;
		    }
		if (colid==0)
		    {
		    htrAddBodyItem_va(s,"<IMG name=\"xy_%STR&SYM_%STR&HTE\" src=/sys/images/trans_1.gif align=top>", t->name, str?str:"");
		    }
		if (*(t->textcolor))
		    {
		    htrAddBodyItem_va(s,"<FONT COLOR=\"%STR&HTE\">",t->textcolor);
		    }
		if (str) htrAddBodyItem(s,str);
		if (*(t->textcolor))
		    {
		    htrAddBodyItem(s,"</FONT>");
		    }
		htrAddBodyItem(s,"</TD>");

		/** Next attr **/
		if (t->ncols == 0)
		    attr = objGetNextAttr(qy_obj);
		else
		    attr = (colid < t->ncols-1)?(t->col_infs[++colid]->Name):NULL;
		}
	    htrAddBodyItem(s,"</TR>\n");
	    objClose(qy_obj);
	    rowid++;
	    }
	objQueryClose(qy);
	htrAddBodyItem(s,"</TABLE></TD></TR></TABLE>\n");

	/** Call init function **/
 	htrAddScriptInit_va(s,"    tbls_init({parentLayer:wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])), name:\"%STR&SYM\", width:%INT, cp:%INT, cs:%INT});\n",t->name,t->name,t->w,t->inner_padding,t->inner_border);
 
    return 0;
    }


/*** httblRender - generate the HTML code for the page.
 ***/
int
httblRender(pHtSession s, pWgtrNode tree, int z)
    {
    pWgtrNode sub_tree;
    char* ptr;
    char* str;
    pStructInf attr_inf;
    int n, i;
    httbl_struct* t;
    int rval;

	t = (httbl_struct*)nmMalloc(sizeof(httbl_struct));
	if (!t) return -1;

	t->tbl_bgnd[0]='\0';
	t->hdr_bgnd[0]='\0';
	t->row_bgnd1[0]='\0';
	t->row_bgnd2[0]='\0';
	t->row_bgndhigh[0]='\0';
	t->textcolor[0]='\0';
	t->textcolorhighlight[0]='\0';
	t->titlecolor[0]='\0';
	t->x=-1;
	t->y=-1;
	t->mode=0;
	t->outer_border=0;
	t->inner_border=0;
	t->inner_padding=0;
	t->followcurrent=1;
    
    	/** Get an id for thit. **/
	t->id = (HTTBL.idcnt++);

	/** Backwards compat for the time being **/
	wgtrRenameProperty(tree, "row_bgcolor1", "row1_bgcolor");
	wgtrRenameProperty(tree, "row_background1", "row1_background");
	wgtrRenameProperty(tree, "row_bgcolor2", "row2_bgcolor");
	wgtrRenameProperty(tree, "row_background2", "row2_background");
	wgtrRenameProperty(tree, "row_bgcolorhighlight", "rowhighlight_bgcolor");
	wgtrRenameProperty(tree, "row_backgroundhighlight", "rowhighlight_background");

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&(t->x))) != 0) t->x = -1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&(t->y))) != 0) t->y = -1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&(t->w))) != 0) t->w = -1;
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&(t->h))) != 0) t->h = -1;
	if (wgtrGetPropertyValue(tree,"windowsize",DATA_T_INTEGER,POD(&(t->windowsize))) != 0) t->windowsize = -1;
	if (wgtrGetPropertyValue(tree,"rowheight",DATA_T_INTEGER,POD(&(t->rowheight))) != 0) t->rowheight = 15;
	if (wgtrGetPropertyValue(tree,"cellhspacing",DATA_T_INTEGER,POD(&(t->cellhspacing))) != 0) t->cellhspacing = 1;
	if (wgtrGetPropertyValue(tree,"cellvspacing",DATA_T_INTEGER,POD(&(t->cellvspacing))) != 0) t->cellvspacing = 1;

	if (wgtrGetPropertyValue(tree,"dragcols",DATA_T_INTEGER,POD(&(t->dragcols))) != 0) t->dragcols = 1;
	if (wgtrGetPropertyValue(tree,"colsep",DATA_T_INTEGER,POD(&(t->colsep))) != 0) t->colsep = 1;
	if (wgtrGetPropertyValue(tree,"gridinemptyrows",DATA_T_INTEGER,POD(&(t->gridinemptyrows))) != 0) t->gridinemptyrows = 1;

	/** Should we follow the current record around? **/
	if (wgtrGetPropertyValue(tree,"followcurrent",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) t->followcurrent = 0;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    nmFree(t, sizeof(httbl_struct));
	    return -1;
	    }
	strtcpy(t->name,ptr,sizeof(t->name));

	/** Mode of table operation.  Defaults to 0 (static) **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"static")) t->mode = 0;
	    else if (!strcmp(ptr,"dynamicpage")) t->mode = 1;
	    else if (!strcmp(ptr,"dynamicrow")) t->mode = 2;
	    else
	        {
		mssError(1,"HTTBL","Widget '%s' mode '%s' is invalid.",t->name,ptr);
		nmFree(t, sizeof(httbl_struct));
		return -1;
		}
	    }

	/** Get background color/image for table header **/
	htrGetBackground(tree, NULL, !s->Capabilities.Dom0NS, t->tbl_bgnd, sizeof(t->tbl_bgnd));

	/** Get background color/image for header row **/
	htrGetBackground(tree, "hdr", !s->Capabilities.Dom0NS, t->hdr_bgnd, sizeof(t->hdr_bgnd));

	/** Get background color/image for rows **/
	htrGetBackground(tree, "row1", !s->Capabilities.Dom0NS, t->row_bgnd1, sizeof(t->row_bgnd1));
	htrGetBackground(tree, "row2", !s->Capabilities.Dom0NS, t->row_bgnd2, sizeof(t->row_bgnd2));
	htrGetBackground(tree, "rowhighlight", !s->Capabilities.Dom0NS, t->row_bgndhigh, sizeof(t->row_bgndhigh));

	/** Get borders and padding information **/
	wgtrGetPropertyValue(tree,"outer_border",DATA_T_INTEGER,POD(&(t->outer_border)));
	wgtrGetPropertyValue(tree,"inner_border",DATA_T_INTEGER,POD(&(t->inner_border)));
	wgtrGetPropertyValue(tree,"inner_padding",DATA_T_INTEGER,POD(&(t->inner_padding)));

	/** Text color information **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->textcolor,ptr,sizeof(t->textcolor));

	/** Text color information **/
	if (wgtrGetPropertyValue(tree,"textcolorhighlight",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->textcolorhighlight,ptr,sizeof(t->textcolorhighlight));

	/** Title text color information **/
	if (wgtrGetPropertyValue(tree,"titlecolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->titlecolor,ptr,sizeof(t->titlecolor));
	if (!*t->titlecolor) strcpy(t->titlecolor,t->textcolor);

	/** Get column data **/
	t->ncols = 0;
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/table-column") != 0)
		{
		/** no layer associated with this guy **/
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING,POD(&ptr));
		if (wgtrGetPropertyValue(sub_tree, "fieldname", DATA_T_STRING, POD(&ptr)) < 0)
		    {
		    mssError(1, "HTTBL", "Couldn't get 'fieldname' for '%s'", sub_tree->Name);
		    nmFree(t, sizeof(httbl_struct));
		    return -1;
		    }
		t->col_infs[t->ncols] = stCreateStruct(ptr, "widget/table-column");
		attr_inf = stAddAttr(t->col_infs[t->ncols], "width");
		if (wgtrGetPropertyValue(sub_tree, "width", DATA_T_INTEGER,POD(&n)) == 0)
		    stAddValue(attr_inf, NULL, n);
		else
		    stAddValue(attr_inf, NULL, -1);
		attr_inf = stAddAttr(t->col_infs[t->ncols], "title");
		if (wgtrGetPropertyValue(sub_tree, "title", DATA_T_STRING,POD(&ptr)) == 0)
		    {
		    str = nmSysStrdup(ptr);
		    stAddValue(attr_inf, str, 0);
		    }
		else
		    stAddValue(attr_inf, t->col_infs[t->ncols]->Name, 0);
		t->ncols++;
		}
	    }
	if(t->mode==0)
	    {
	    rval = httblRenderStatic(s, tree, z, t);
	    nmFree(t, sizeof(httbl_struct));
	    }
	else
	    {
	    rval = httblRenderDynamic(s, tree, z, t);
	    nmFree(t, sizeof(httbl_struct));
	    }

    return rval;
    }


/*** httblInitialize - register with the ht_render module.
 ***/
int
httblInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML DataTable Driver");
	strcpy(drv->WidgetName,"table");
	drv->Render = httblRender;
	xaAddItem(&(drv->PseudoTypes), "table-column");

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"DblClick");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBL.idcnt = 0;

    return 0;
    }
