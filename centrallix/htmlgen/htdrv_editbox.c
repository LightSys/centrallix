#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
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

    $Id: htdrv_editbox.c,v 1.38 2004/08/04 20:03:08 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_editbox.c,v $

    $Log: htdrv_editbox.c,v $
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
htebRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    /*char sbuf[HT_SBUF_SIZE];*/
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    char* nptr;
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE)
	    {
	    mssError(1,"HTEB","Netscape or Internet Explorer DOM support required");
	    return -1;
	    }

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

	/** Background color/image? **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!s->Capabilities.Dom0IE)
	        {
	        sprintf(main_bg,"bgColor='%.40s'",ptr);
		}
	    else
	        {
	        sprintf(main_bg,"%.40s",ptr);
	        }
	    }
	else if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

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
	    {
	    strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	    }
	else
	    {
	    fieldname[0]='\0';
	    }

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom1HTML)
	    htrAddStylesheetItem_va(s,"\t#eb%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; overflow:hidden }\n",id,x,y,w,z);
	else if (s->Capabilities.Dom0NS)
	    htrAddStylesheetItem_va(s,"\t#eb%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

	htrAddStylesheetItem_va(s,"\t#eb%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddStylesheetItem_va(s,"\t#eb%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "eb_current", "null", 0);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_editbox.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	htrAddEventHandler(s, "document","MOUSEUP", "eb",
	    "\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'Click');\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "eb",
	    "\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "eb",
	    "\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'MouseOver');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOUT", "eb",
	    "\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'MouseOut');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEMOVE", "eb",
	    "\n"
	    "    if (ly.kind == 'eb') cn_activate(ly, 'MouseMove');\n"
	    "\n");

	/** Script initialization call. **/
	if (s->Capabilities.Dom1HTML)
	    {
	    htrAddScriptInit_va(s, "    %s = eb_init(%s.getElementById(\"eb%dbase\"), %s.getElementById(\"eb%dbase\").document.getElementById(\"eb%dcon1\"),%s.getElementById(\"eb%dbase\").document.getElementById(\"eb%dcon2\"),\"%s\", %d, \"%s\");\n",
		nptr, parentname, id,
		parentname, id, id,
		parentname, id, id,
		fieldname, is_readonly, main_bg);
	    }
	else if (s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit_va(s, "    %s = eb_init(%s.layers.eb%dbase, %s.layers.eb%dbase.document.layers.eb%dcon1,%s.layers.eb%dbase.document.layers.eb%dcon2,\"%s\", %d, \"%s\");\n",
		nptr, parentname, id,
		parentname, id, id,
		parentname, id, id,
		fieldname, is_readonly, main_bg);
	    }

	/** Set object parent **/
	htrAddScriptInit_va(s, "    htr_set_parent(%s, \"%s\", %s);\n",
		nptr, nptr, parentobj);

	htrGetBackground(tree, NULL, s->Capabilities.Dom2CSS, main_bg, sizeof(main_bg));

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"eb%dbase\">\n",id);
	htrAddBodyItem_va(s, "    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem_va(s, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem_va(s, "<DIV ID=\"eb%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "<DIV ID=\"eb%dcon2\"></DIV>\n",id);

	/** Check for objects within the editbox. **/
	/** The editbox can have no subwidgets **/
	/*sprintf(sbuf,"%s.mainlayer.document",nptr);*/
	/*sprintf(sbuf2,"%s.mainlayer",nptr);*/
	/*htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);*/




	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1, parentname, nptr);


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
