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
/* Module: 	htdrv_editbox.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line editbox.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_editbox.c,v 1.49 2008/03/04 01:10:56 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_editbox.c,v $

    $Log: htdrv_editbox.c,v $
    Revision 1.49  2008/03/04 01:10:56  gbeeley
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

    Revision 1.48  2007/06/06 15:22:47  gbeeley
    - (feature) allow tooltip on editbox, but tooltip will be overridden if
      there is overly long content

    Revision 1.47  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.46  2007/04/03 15:50:04  gbeeley
    - (feature) adding capability to pass a widget to a component as a
      parameter (by reference).
    - (bugfix) changed the layout logic slightly in the apos module to better
      handle ratios of flexibility and size when resizing.

    Revision 1.45  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.44  2006/10/16 18:34:33  gbeeley
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

    Revision 1.43  2006/08/04 18:56:51  gbeeley
    - (fix) adding 'px' to CSS values that should have a dimension.

    Revision 1.42  2006/04/07 06:30:42  gbeeley
    - (feature) show small arrowheads at left and right edges of editbox when
      text overflows the left or right side.

    Revision 1.41  2005/06/23 22:07:58  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.40  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.39  2004/08/18 04:54:25  gbeeley
    - proper keyboard input in IE6 and Moz (including handling the keydown vs.
      keypress event issue).
    - editbox appearance/functionality now relatively consistent across IE6,
      Moz, and NS4.

    Revision 1.38  2004/08/04 20:03:08  mmcgill
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

    Revision 1.37  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.36  2004/08/02 14:09:34  mmcgill
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

    Revision 1.35  2004/07/19 15:30:39  mmcgill
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

    Revision 1.34  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.33  2004/03/25 03:08:54  jasonyip

    Added htrGetBackground support for the table tag.

    Revision 1.32  2004/03/17 20:31:10  jasonyip

    A bug in the table tag - bgColor was missed.

    Revision 1.31  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.30  2004/02/24 20:21:56  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.29  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.28  2003/06/03 19:27:09  gbeeley
    Updates to properties mostly relating to true/false vs. yes/no

    Revision 1.27  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.26  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.25  2002/07/25 20:38:40  pfinley
    Change event needed to be DataChange event.

    Revision 1.24  2002/07/25 19:48:17  pfinley
    Standardized event connectors for editbox widget.  It now has:
      Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove,Change,GetFocus,LoseFocus

    Revision 1.23  2002/07/19 14:54:22  pfinley
    - Modified the page mousedown & mouseover handlers so that the cursor ibeam
    "can't" be clicked on (only supports the global cursor).
    - Modified the editbox & textarea files to support the new global cursor.

    Revision 1.22  2002/07/16 19:31:03  lkehresman
    Added an include that I forgot earlier

    Revision 1.21  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.20  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.19  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.18  2002/06/03 05:09:25  jorupp
     * impliment the form view mode correctly
     * fix scrolling back in the OSRC (from the table)
     * throw DataChange event when any data is changed

    Revision 1.17  2002/05/31 04:03:02  lkehresman
    Added check if the background color doesn't exist.

    Revision 1.16  2002/05/30 04:22:18  lkehresman
    Changed where focus notify was getting called.

    Revision 1.15  2002/05/30 04:17:21  lkehresman
    * Modified editbox to take advantage of enablenew and enablemodify
    * Fixed ChangeStatus stuff in form

    Revision 1.14  2002/05/30 03:55:21  lkehresman
    editbox:  * added readonly flag so the editbox is always only readonly
              * made disabled appear visually
    table:    * fixed a typo

    Revision 1.13  2002/05/03 01:40:56  jheth
    Defined fieldname size to be 60 (from 30) in ht_render.h - HT_FIELDNAME_SIZE

    Revision 1.12  2002/04/25 22:51:29  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.11  2002/03/23 01:18:09  lkehresman
    Fixed focus detection and form notification on editbox and anything that
    uses keyboard input.

    Revision 1.10  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.9  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.8  2002/03/05 01:55:09  lkehresman
    Added "clearvalue" method to form widgets

    Revision 1.7  2002/03/05 01:22:26  lkehresman
    Changed where the DataNotify form method is getting called.  Previously it
    would only get called when the edit box lost focus.  This was bad because
    clicking a button doesn't make the edit box lose focus.  Now DataNotify is
    getting called on key events.  Much better, and more of what you would expect.

    Revision 1.6  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.5  2002/02/27 02:37:19  jorupp
    * moved editbox I-beam movement functionality to function
    * cleaned up form, added comments, etc.

    Revision 1.4  2002/02/23 04:28:29  jorupp
    bug fixes in form, I-bar in editbox is reset on a setvalue()

    Revision 1.3  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

 **END-CVSDATA***********************************************************/

/** globals **/
static struct
    {
    int		idcnt;
    }
    HTEB;


/*** htebRender - generate the HTML code for the editbox widget.
 ***/
int
htebRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    char* c1;
    char* c2;
    char* tooltip;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int box_offset;
    int dbl_buffer = 0;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom2Events)
	    {
	    mssError(1,"HTEB","Netscape, IE, or Dom2Events support required");
	    return -1;
	    }

	/** Use double buffering? **/
	if (s->Capabilities.Dom0NS)
	    dbl_buffer = 1;

    	/** Get an id for this. **/
	id = (HTEB.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'height' property");
	    return -1;
	    }

	/** Maximum characters to accept from the user **/
	if (wgtrGetPropertyValue(tree,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (wgtrGetPropertyValue(tree,"readonly",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Background color/image **/
	strcpy(main_bg,"");
	htrGetBackground(tree, NULL, s->Capabilities.CSS2?1:0, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** client-side expr for content **/
	htrCheckAddExpression(s,tree,name,"content");

	/** Style of editbox - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"raised")) is_raised = 1;
	if (is_raised)
	    {
	    c1 = "white_1x1.png";
	    c2 = "dkgrey_1x1.png";
	    }
	else
	    {
	    c1 = "dkgrey_1x1.png";
	    c2 = "white_1x1.png";
	    }

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';

	if(wgtrGetPropertyValue(tree,"tooltip",DATA_T_STRING,POD(&ptr)) == 0)
	    tooltip=nmSysStrdup(ptr);
	else
	    tooltip=nmSysStrdup("");

	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom1HTML)
	    htrAddStylesheetItem_va(s,"\t#eb%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; overflow:hidden; }\n",id,x,y,w-2*box_offset,z);
	else if (s->Capabilities.Dom0NS)
	    htrAddStylesheetItem_va(s,"\t#eb%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);

	htrAddStylesheetItem_va(s,"\t#eb%POScon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,5,1,w-10,z+1);
	if (dbl_buffer)
	    htrAddStylesheetItem_va(s,"\t#eb%POScon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,5,1,w-10,z+1);

	/** Write named global **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"eb%POSbase\")",id);
	htrAddWgtrCtrLinkage(s, tree, "_obj");

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "eb_current", "null", 0);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_editbox.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	htrAddEventHandlerFunction(s, "document","MOUSEUP", "eb", "eb_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "eb", "eb_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "eb", "eb_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "eb", "eb_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "eb", "eb_mousemove");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    eb_init({layer:nodes['%STR&SYM'], c1:htr_subel(nodes['%STR&SYM'],\"eb%POScon1\"), %[c2:htr_subel(nodes['%STR&SYM'],\"eb%POScon2\"),%] form:\"%STR&JSSTR\", fieldname:\"%STR&JSSTR\", isReadOnly:%INT, mainBackground:\"%STR&JSSTR\", tooltip:\"%STR&JSSTR\"});\n",
	    name,  name,id,  dbl_buffer,name,id, 
	    form, fieldname, is_readonly, main_bg, tooltip);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"eb%POSbase\">\n",id);

	/** Use CSS border or table for drawing? **/
	if (s->Capabilities.CSS2)
	    {
	    if (is_raised)
		htrAddStylesheetItem_va(s,"\t#eb%POSbase { border-style:solid; border-width:1px; border-color: white gray gray white; %STR }\n",id, main_bg);
	    else
		htrAddStylesheetItem_va(s,"\t#eb%POSbase { border-style:solid; border-width:1px; border-color: gray white white gray; %STR }\n",id, main_bg);
	    if (h >= 0)
		htrAddStylesheetItem_va(s,"\t#eb%POSbase { height:%POSpx; }\n", id, h-2*box_offset);
	    }
	else
	    {
	    htrAddBodyItem_va(s, "    <TABLE width=%POS cellspacing=0 cellpadding=0 border=0 %STR>\n",w,main_bg);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c1);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c1,w-2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n",c1);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD>\n",c1,h-2);
	    htrAddBodyItem(s,    "            <TD>");
	    }
	htrAddBodyItem_va(s, "<table border='0' cellspacing='0' cellpadding='0' width='%POS'><tr><td align='left' valign='middle' height='%POS'><img name='l' src='/sys/images/eb_edg.gif'></td><td>&nbsp;</td><td align='right' valign='middle'><img name='r' src='/sys/images/eb_edg.gif'></td></tr></table>\n", w-2, h-2);
	if (!s->Capabilities.CSS2)
	    {
	    htrAddBodyItem(s,    "</TD>\n");
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD></TR>\n",c2,h-2);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c2,w-2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n    </TABLE>\n\n",c2);
	    }

	htrAddBodyItem_va(s, "<DIV ID=\"eb%POScon1\">&nbsp;</DIV>\n",id);
	if (dbl_buffer)
	    htrAddBodyItem_va(s, "<DIV ID=\"eb%POScon2\">&nbsp;</DIV>\n",id);

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htebInitialize - register with the ht_render module.
 ***/
int
htebInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Editbox Driver");
	strcpy(drv->WidgetName,"editbox");
	drv->Render = htebRender;

	/** Events **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"GetFocus");
	htrAddEvent(drv,"LoseFocus");

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTEB.idcnt = 0;

    return 0;
    }
