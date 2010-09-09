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
/* Module: 	htdrv_textbutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 5, 1998  					*/
/* Description:	HTML Widget driver for a 'text button', which frames a	*/
/*		text string in a 3d-like box that simulates the 3d	*/
/*		clicking action when the user points and clicks.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_textbutton.c,v 1.42 2010/09/09 01:16:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textbutton.c,v $

    $Log: htdrv_textbutton.c,v $
    Revision 1.42  2010/09/09 01:16:09  gbeeley
    - (feature) the 'text' property now can be assigned a dynamic expression

    Revision 1.41  2008/07/16 00:34:57  thr4wn
    Added a bunch of documentation in different README files. Also added documentation in certain parts of the code itself.

    Revision 1.40  2008/03/04 01:10:57  gbeeley
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

    Revision 1.39  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.38  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.37  2007/03/10 03:49:30  gbeeley
    - (change) pass button text to client as a param

    Revision 1.36  2006/10/16 18:34:34  gbeeley
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

    Revision 1.35  2006/10/04 17:12:54  gbeeley
    - (bugfix) Newer versions of Gecko handle clipping regions differently than
      anything else out there.  Created a capability flag to handle that.
    - (bugfix) Useragent.cfg processing was sometimes ignoring sub-definitions.

    Revision 1.34  2006/06/22 00:22:07  gbeeley
    - better tracking of what button got pushed, to match up the mousedown/up
      events.

    Revision 1.33  2005/09/17 02:55:55  gbeeley
    - fix regressions in moz/ie support with regard to new init function
      call protocol.

    Revision 1.32  2005/06/23 22:08:01  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.31  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.30  2004/08/17 03:47:18  gbeeley
    - got textbutton appearance and functionality consistent across MSIE, Moz,
      and NS4.

    Revision 1.29  2004/08/04 20:03:11  mmcgill
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

    Revision 1.28  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.27  2004/08/02 14:09:35  mmcgill
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

    Revision 1.26  2004/07/19 15:30:41  mmcgill
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

    Revision 1.25  2004/03/11 23:17:44  jasonyip

    Fixed the moveBy to use the geometry libary.

    Revision 1.24  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.23  2003/11/18 05:58:00  gbeeley
    - attempting to fix text centering

    Revision 1.22  2003/07/27 03:24:54  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

    Revision 1.21  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.20  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.19  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.18  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.17  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.16  2002/07/25 18:45:40  lkehresman
    Standardized event connectors for imagebutton and textbutton, and took
    advantage of the checking done in the cn_activate function so it isn't
    necessary outside the function.

    Revision 1.15  2002/07/25 16:54:18  pfinley
    completely undoing the change made yesterday with aliasing of click events
    to mouseup... they are now two separate events. don't believe the lies i said
    yesterday :)

    Revision 1.14  2002/07/24 18:12:03  pfinley
    Updated Click events to be MouseUp events. Now all Click events must be
    specified as MouseUp within the Widget's event handler, or they will not
    work propery (Click can still be used as a event connector to the widget).

    Revision 1.13  2002/07/20 20:00:25  lkehresman
    Added a mousemove event connector to textbutton

    Revision 1.12  2002/07/20 16:30:21  lkehresman
    Added four new standard event connectors to the textbutton (MouseUp,
    MouseDown, MouseOver, MouseOut)

    Revision 1.11  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.10  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.9  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.8  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.7  2002/05/30 00:16:18  jorupp
     * switching to the _va functions...

    Revision 1.6  2002/03/16 05:12:02  gbeeley
    Added the buttonName javascript property for imagebuttons and text-
    buttons.  Allows them to be identified more easily via javascript.

    Revision 1.5  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.4  2002/01/09 15:27:37  gbeeley
    Fixed a bug where the borders of a textbutton would sometimes show up
    when the button's container was not visible.

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
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
    HTTBTN;


/*** httbtnRender - generate the HTML code for the page.
 ***/
int
httbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    char bgstyle[128];
    char disable_color[64];
    int x,y,w,h;
    int id, i;
    int is_ts = 1;
    char* dptr;
    int is_enabled = 1;
    pExpression code;
    int box_offset;
    int clip_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTBTN","Netscape DOM or (W3C DOM1 HTML and W3C DOM2 CSS) support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** box adjustment... arrgh **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;
	clip_offset = s->Capabilities.CSSClip?1:0;

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Threestate button or twostate? **/
	if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	ptr = "-";
	if (!htrCheckAddExpression(s, tree, name, "text") && wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	strtcpy(text,ptr,sizeof(text));

	/** Get fgnd colors 1,2, and background color **/
	htrGetBackground(tree, NULL, 0, bgcolor, sizeof(bgcolor));
	htrGetBackground(tree, NULL, 1, bgstyle, sizeof(bgstyle));

	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
	else
	    strcpy(fgcolor1,"white");
	if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
	else
	    strcpy(fgcolor2,"black");
	if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(disable_color,ptr,sizeof(disable_color));
	else
	    strcpy(disable_color,"#808080");

	htrAddScriptGlobal(s, "tb_current", "null", 0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tb%POSpane\")",id);
	htrAddWgtrCtrLinkage(s, tree, "_obj");

	/** Include the javascript code for the textbutton **/
	htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0);

	dptr = wgtrGetDName(tree);
	htrAddScriptInit_va(s, "    %STR&SYM = nodes['%STR&SYM'];\n", dptr, name);

	if(s->Capabilities.Dom0NS)
	    {
	    /** Ok, write the style header items. **/
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane2 { POSITION:absolute; VISIBILITY:%STR; LEFT:-1; TOP:-1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"inherit":"hidden",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane3 { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"hidden":"inherit",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%POStop { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSbtm { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSrgt { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSlft { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);

	    /** Script initialization call. **/
	    htrAddScriptInit_va(s, "    tb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"tb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"tb%POSpane3\"), top:htr_subel(%STR&SYM, \"tb%POStop\"), bottom:htr_subel(%STR&SYM, \"tb%POSbtm\"), right:htr_subel(%STR&SYM, \"tb%POSrgt\"), left:htr_subel(%STR&SYM, \"tb%POSlft\"), width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		    dptr, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, w, h, is_ts, name, text);

	    /** HTML body <DIV> elements for the layers. **/
	    if (h >= 0)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,h);
		}
	    else
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=3 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text);
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POStop\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSbtm\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSrgt\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSlft\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem(s,   "</DIV>\n");
	    }
	else if(s->Capabilities.CSS2)
	    {
	    if(h >=0 )
		{
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx %INTpx %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,h-1-2*box_offset+2*clip_offset,0);
		htrAddStylesheetItem_va(s,"\t#tb%POSpane2, #tb%POSpane3 { height: %POSpx;}\n",id,id,h-3);
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { height: %POSpx;}\n",id,h-1-2*box_offset);
		}
	    else
		{
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx auto %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,0);
		}
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane, #tb%POSpane2, #tb%POSpane3 { cursor:default; text-align: center; }\n",id,id,id);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { %STR border-width: 1px; border-style: solid; border-color: white gray gray white; }\n",id,bgstyle);
	    /*htrAddStylesheetItem_va(s,"\t#tb%dpane { color: %s; }\n",id,fgcolor2);*/
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane2 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:-1px; top: -1px; width:%POSpx; }\n",id,is_enabled?"inherit":"hidden",z+1,w-3);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane3 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:0px; top: 0px; width:%POSpx; }\n",id,is_enabled?"hidden":"inherit",z+1,w-3);

	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center>\n",id,h-3,fgcolor2,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,fgcolor1,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,disable_color,text);
	    htrAddBodyItem(s,   "</DIV>");

	    /** Script initialization call. **/
	    htrAddScriptInit_va(s, "    tb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"tb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"tb%POSpane3\"), top:null, bottom:null, right:null, left:null, width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		    dptr, dptr, id, dptr, id, w, h, is_ts, name, text);
	    }
	else
	    {
	    mssError(0,"HTTBTN","Unable to render for this browser");
	    return -1;
	    }

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tb", "tb_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "tb", "tb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tb", "tb_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "tb", "tb_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tb", "tb_mousemove");

	/** IE handles dblclick strangely **/
	if (s->Capabilities.Dom0IE)
	    htrAddEventHandlerFunction(s, "document", "DBLCLICK", "tb", "tb_dblclick");

	/** Check for more sub-widgets within the textbutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

    return 0;
    }


/*** httbtnInitialize. - register with the ht_render module.
 ***/
int
httbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Text Button Widget Driver");
	strcpy(drv->WidgetName,"textbutton");
	drv->Render = httbtnRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBTN.idcnt = 0;

    return 0;
    }
