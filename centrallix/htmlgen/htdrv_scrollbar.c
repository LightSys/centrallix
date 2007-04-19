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
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_scrollbar.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	July 14, 2003    					*/
/* Description:	HTML Widget driver for a scrollbar - either horizontal	*/
/*		or vertical.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_scrollbar.c,v 1.12 2007/04/19 21:26:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_scrollbar.c,v $

    $Log: htdrv_scrollbar.c,v $
    Revision 1.12  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.11  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.10  2006/10/16 18:34:34  gbeeley
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

    Revision 1.9  2005/06/23 22:08:00  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.8  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.7  2004/08/04 20:03:09  mmcgill
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

    Revision 1.6  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.5  2004/08/02 14:09:34  mmcgill
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

    Revision 1.4  2004/07/19 15:30:40  mmcgill
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

    Revision 1.3  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.2  2003/07/15 01:59:50  gbeeley
    Fixing bug with the low-value limiter on dragging the thumb around.

    Revision 1.1  2003/07/15 01:57:51  gbeeley
    Adding an independent DHTML scrollbar widget that will be used to
    control scrolling/etc on other widgets.

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTSB;


/*** htsbRender - generate the HTML code for the page.
 ***/
int
htsbRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int x,y,w,h,r;
    int id, i;
    int visible = 1;
    char bcolor[64] = "";
    char bimage[64] = "";
    int is_horizontal = 0;
    pExpression code;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTSB","Netscape 4.x, IE, or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTSB.idcnt++);

	/** Which direction? **/
	if (wgtrGetPropertyValue(tree,"direction",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"vertical")) is_horizontal = 0;
	    else if (!strcasecmp(ptr,"horizontal")) is_horizontal = 1;
	    }

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTSB","Scrollbar widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTSB","Scrollbar widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    if (is_horizontal)
		{
		mssError(1,"HTSB","Horizontal scrollbar widgets must have a 'width' property");
		return -1;
		}
	    else
		{
		w = 18;
		}
	    }
	if (is_horizontal && w <= 18*3)
	    {
	    mssError(1,"HTSB","Horizontal scrollbar width must be greater than %d", 18*3);
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    if (!is_horizontal)
		{
		mssError(1,"HTSB","Vertical scrollbar widgets must have a 'height' property");
		return -1;
		}
	    else
		{
		h = 18;
		}
	    }
	if (!is_horizontal && h <= 18*3)
	    {
	    mssError(1,"HTSB","Vertical scrollbar height must be greater than %d", 18*3);
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Range of scrollbar (static or dynamic property) **/
	if (wgtrGetPropertyType(tree,"range") == DATA_T_INTEGER && wgtrGetPropertyValue(tree,"range",DATA_T_INTEGER,POD(&r)) != 0)
	    {
	    if (is_horizontal) r = w;
	    else r = h;
	    }
	if (wgtrGetPropertyType(tree,"range") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"range", DATA_T_CODE, POD(&code));
	    htrAddExpression(s, name, "range", code);
	    }

	/** Check background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bcolor,ptr,sizeof(bcolor));
	    }
	if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(bimage,ptr,sizeof(bimage));
	    }

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#sb%POSpane { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; clip:rect(0px,%POSpx,%POSpx,0px); Z-INDEX:%POS; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	if (is_horizontal)
	    htrAddStylesheetItem_va(s,"\t#sb%POSthum { POSITION:absolute; VISIBILITY:inherit; LEFT:18px; TOP:0px; WIDTH:18px; Z-INDEX:%POS; }\n",id,z+1);
	else
	    htrAddStylesheetItem_va(s,"\t#sb%POSthum { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:18px; Z-INDEX:%POS; }\n",id,z+1);

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "sb_target_img", "null", 0);
	htrAddScriptGlobal(s, "sb_click_x","0",0);
	htrAddScriptGlobal(s, "sb_click_y","0",0);
	htrAddScriptGlobal(s, "sb_thum_x","0",0);
	htrAddScriptGlobal(s, "sb_thum_y","0",0);
	htrAddScriptGlobal(s, "sb_mv_timeout","null",0);
	htrAddScriptGlobal(s, "sb_mv_incr","0",0);
	htrAddScriptGlobal(s, "sb_cur_mainlayer","null",0);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"sb%POSpane\")",id);

	htrAddScriptInclude(s, "/sys/js/htdrv_scrollbar.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    sb_init({layer:nodes[\"%STR&SYM\"], tname:\"sb%POSthum\", isHorizontal:%INT, range:%INT});\n", name, id, is_horizontal, r);

	/** HTML body <DIV> elements for the layers. **/
	htrAddBodyItem_va(s,"<DIV ID=\"sb%POSpane\"><TABLE %[bgcolor=%]%STR&DQUOT %[background=%]%STR&DQUOT border=0 cellspacing=0 cellpadding=0 width=%POS>", id, *bcolor, bcolor, *bimage, bimage, w);
	if (is_horizontal)
	    {
	    htrAddBodyItem(s,   "<TR><TD align=right><IMG SRC=/sys/images/ico19b.gif width=18 height=18 NAME=u></TD><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC=/sys/images/trans_1.gif height=18 width=%POS name='b'>",w-36);
	    htrAddBodyItem(s,   "</TD><TD align=right><IMG SRC=/sys/images/ico18b.gif width=18 height=18 NAME=d></TD></TR></TABLE>\n");
	    }
	else
	    {
	    htrAddBodyItem(s,   "<TR><TD align=right><IMG SRC=/sys/images/ico13b.gif width=18 height=18 NAME=u></TD></TR><TR><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC=/sys/images/trans_1.gif height=%POS width=18 name='b'>",h-36);
	    htrAddBodyItem(s,   "</TD></TR><TR><TD align=right><IMG SRC=/sys/images/ico12b.gif width=18 height=18 NAME=d></TD></TR></TABLE>\n");
	    }
	htrAddBodyItem_va(s,"<DIV ID=\"sb%POSthum\"><IMG SRC=/sys/images/ico14b.gif NAME=t></DIV>",id);

	/** Add the event handling scripts **/

	htrAddEventHandlerFunction(s, "document","MOUSEDOWN","sb","sb_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","sb","sb_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","sb","sb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "sb","sb_mouseover");

	/** Check for more sub-widgets within the scrollbar (visual ones not allowed). **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2);

	/** Finish off the last <DIV> **/
	htrAddBodyItem(s,"</DIV>\n");

    return 0;
    }


/*** htsbInitialize - register with the ht_render module.
 ***/
int
htsbInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Scrollbar Widget Driver");
	strcpy(drv->WidgetName,"scrollbar");
	drv->Render = htsbRender;

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	htrAddAction(drv,"MoveTo");
	htrAddParam(drv,"MoveTo","Value",DATA_T_INTEGER);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");
	HTSB.idcnt = 0;

    return 0;
    }
