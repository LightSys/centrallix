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
/* Module: 	htdrv_imagebutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 4, 1998  					*/
/* Description:	HTML Widget driver for an 'image button', or a button	*/
/*		comprised of a set of three images - one the default,	*/
/*		second the image when pointed to, and third the image	*/
/*		when clicked.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_imagebutton.c,v 1.29 2004/08/02 14:09:34 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_imagebutton.c,v $

    $Log: htdrv_imagebutton.c,v $
    Revision 1.29  2004/08/02 14:09:34  mmcgill
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

    Revision 1.28  2004/07/19 15:30:40  mmcgill
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

    Revision 1.27  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.26  2003/07/27 03:24:53  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

    Revision 1.25  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.24  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.23  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.22  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.21  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.20  2002/07/31 22:03:44  lkehresman
    Fixed mouseup issues when mouseup occurred outside the image for:
      * dropdown scroll images
      * imagebutton images

    Revision 1.19  2002/07/25 18:45:40  lkehresman
    Standardized event connectors for imagebutton and textbutton, and took
    advantage of the checking done in the cn_activate function so it isn't
    necessary outside the function.

    Revision 1.18  2002/07/25 16:54:18  pfinley
    completely undoing the change made yesterday with aliasing of click events
    to mouseup... they are now two separate events. don't believe the lies i said
    yesterday :)

    Revision 1.17  2002/07/24 18:12:03  pfinley
    Updated Click events to be MouseUp events. Now all Click events must be
    specified as MouseUp within the Widget's event handler, or they will not
    work propery (Click can still be used as a event connector to the widget).

    Revision 1.16  2002/07/24 15:14:28  pfinley
    added more actions

    Revision 1.15  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.14  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.13  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.12  2002/06/19 21:22:45  lkehresman
    Added a losefocushandler to the table.  Not having this broke static tables.

    Revision 1.11  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.10  2002/06/02 22:47:23  jorupp
     * fix some problems I introduced before

    Revision 1.9  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.8  2002/05/31 01:26:41  lkehresman
    * modified the window header HTML to make it look nicer
    * fixed a truncation problem with the image button

    Revision 1.7  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.6  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.5  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.4  2002/03/16 05:12:02  gbeeley
    Added the buttonName javascript property for imagebuttons and text-
    buttons.  Allows them to be identified more easily via javascript.

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTIBTN;


/*** htibtnRender - generate the HTML code for the page.
 ***/
int
htibtnRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char n_img[128];
    char p_img[128];
    char c_img[128];
    char d_img[128];
    int is_enabled = 1;
    int x,y,w,h;
    int id, i;
    char* nptr;
    pExpression code;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTIBTN","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTIBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get normal, point, and click images **/
	if (wgtrGetPropertyValue(tree,"image",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'image' property");
	    return -1;
	    }
	memccpy(n_img,ptr,'\0',127);
	n_img[127]=0;
	if (wgtrGetPropertyValue(tree,"pointimage",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(p_img,ptr,'\0',127);
	    p_img[127]=0;
	    }
	else
	    {
	    strcpy(p_img, n_img);
	    }
	if (wgtrGetPropertyValue(tree,"clickimage",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(c_img,ptr,'\0',127);
	    c_img[127]=0;
	    }
	else
	    {
	    strcpy(c_img, p_img);
	    }
	if (wgtrGetPropertyValue(tree,"disabledimage",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(d_img,ptr,'\0',127);
	    d_img[127]=0;
	    }
	else
	    {
	    strcpy(d_img, n_img);
	    }

	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ib%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
	htrAddScriptGlobal(s, "ib_cur_img", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_imagebutton.js", 0);

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Script initialization call. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit_va(s,"    %s = %s.layers.ib%dpane;\n",nptr, parentname, id);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    htrAddScriptInit_va(s,"    %s = document.getElementById('ib%dpane');\n",nptr, id);
	    }
	else
	    {
	    mssError(1,"HTIBTN","Cannot render for this browser");
	    }

	htrAddScriptInit_va(s,"    ib_init(%s,'%s','%s','%s','%s',%d,%d,%s,'%s',%d);\n",
	        nptr, n_img, p_img, c_img, d_img, w, h, parentobj,nptr,is_enabled);

	/** HTML body <DIV> elements for the layers. **/
	if (h < 0)
	    if(is_enabled)
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=\"%s\" border=\"0\"></DIV>\n",id,n_img);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=\"%s\" border=\"0\"></DIV>\n",id,d_img);
	else
	    if(is_enabled)
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=\"%s\" border=\"0\" width=\"%d\" height=\"%d\"></DIV>\n",id,n_img,w,h);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=\"%s\" border=\"0\" width=\"%d\" height=\"%d\"></DIV>\n",id,d_img,w,h);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","ib",
		"    if (ly.kind=='ib' && ly.enabled==true)\n"
		"        {\n"
		"        pg_set(ly.img,'src',ly.layer.cImage.src);\n"
		"        cn_activate(ly, 'MouseDown');\n"
		"        ib_cur_img = ly.img;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","ib",
		"    if (ib_cur_img)\n"
		"        {\n"
		"        if (e.pageX >= getPageX(ib_cur_img.layer) &&\n"
		"            e.pageX < getPageX(ib_cur_img.layer) + getClipWidth(ib_cur_img.layer) &&\n"
		"            e.pageY >= getPageY(ib_cur_img.layer) &&\n"
		"            e.pageY < getPageY(ib_cur_img.layer) + getClipHeight(ib_cur_img.layer))\n"
		"            {\n"
		"            cn_activate(ly, 'Click');\n"
		"            cn_activate(ly, 'MouseUp');\n"
		"            pg_set(ib_cur_img,'src',ib_cur_img.layer.pImage.src);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            pg_set(ib_cur_img,'src',ib_cur_img.layer.nImage.src);\n"
		"            }\n"
		"        ib_cur_img = null;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOVER","ib",
		"    if (ly.kind == 'ib' && ly.enabled == true)\n"
		"        {\n"
		"        if (ly.img && (ly.img.src != ly.cImage.src)) pg_set(ly.img,'src',ly.pImage.src);\n"
		"        cn_activate(ly, 'MouseOver');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","ib",
		"    if (ly.kind == 'ib' && ly.enabled == true)\n"
		"        {\n"
		"        if (ly.img && (ly.img.src != ly.cImage.src)) pg_set(ly.img,'src',ly.nImage.src);\n"
		"        cn_activate(ly, 'MouseOut');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEMOVE","ib",
		"    if (ly.kind == 'ib' && ly.enabled == true)\n"
		"        {\n"
		"        if (ly.img && ly.img.src != ly.cImage.src) pg_set(ly.img,'src',ly.pImage.src);\n"
		"        cn_activate(ly, 'MouseMove');\n"
		"        }\n");

	/** Check for more sub-widgets within the imagebutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1, parentname, nptr);

    return 0;
    }


/*** htibtnInitialize - register with the ht_render module.
 ***/
int
htibtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ImageButton Widget Driver");
	strcpy(drv->WidgetName,"imagebutton");
	drv->Render = htibtnRender;

	htrAddAction(drv,"Enable");
	htrAddAction(drv,"Disable");
	
	/** Add the 'click' event **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTIBTN.idcnt = 0;

    return 0;
    }
