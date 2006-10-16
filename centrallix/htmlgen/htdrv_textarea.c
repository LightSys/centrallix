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
/* Module: 	htdrv_textarea.c         				*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	July 9, 2002						*/
/* Description:	HTML Widget driver for a multi-line textarea.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: htdrv_textarea.c,v 1.23 2006/10/16 18:34:34 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textarea.c,v $

    $Log: htdrv_textarea.c,v $
    Revision 1.23  2006/10/16 18:34:34  gbeeley
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

    Revision 1.22  2005/06/23 22:08:01  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.21  2005/02/26 06:33:29  gbeeley
    - just some brainstorming.  Thinking of multiple data formats.

    Revision 1.20  2004/08/29 17:32:32  pfinley
    Textarea widget crossbrowser support... I have tested on Mozilla 1.7rc1,
    Firefox 0.9.3, and Netscape 4.79.  Also fixed a JS syntax error with
    loading page/image.

    Revision 1.19  2004/08/04 20:03:11  mmcgill
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

    Revision 1.18  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.17  2004/08/02 14:09:35  mmcgill
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

    Revision 1.16  2004/07/19 15:30:41  mmcgill
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

    Revision 1.15  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.14  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.13  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.12  2002/08/01 15:14:38  lkehresman
    Tagged textarea images with .kind, .layer, and .mainlayer

    Revision 1.11  2002/07/30 21:38:37  pfinley
    Added events: MouseUp,MouseDown,MouseOver,MouseOut,MouseMove,DataChange,
    GetFocus,LoseFocus to textarea widget.

    Revision 1.10  2002/07/19 14:54:22  pfinley
    - Modified the page mousedown & mouseover handlers so that the cursor ibeam
    "can't" be clicked on (only supports the global cursor).
    - Modified the editbox & textarea files to support the new global cursor.

    Revision 1.9  2002/07/16 19:24:34  pfinley
    uses the new header stylesheet function

    Revision 1.8  2002/07/16 19:16:21  pfinley
    textarea c file with js includes

    Revision 1.7  2002/07/12 20:16:13  pfinley
    Undid previous change :)

    Revision 1.6  2002/07/12 19:46:22  pfinley
    added cvs logging to file.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTX;


/*** httxRender - generate the HTML code for the textarea widget.
 ***/
int
httxRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    int mode = 0; /* 0=text, 1=html, 2=wiki */
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];
    int box_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom2Events)
	    {
	    mssError(1,"HTTX","Netscape, IE, or Dom2Events support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTX.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (wgtrGetPropertyValue(tree,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (wgtrGetPropertyValue(tree,"readonly",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Allow HTML? **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"text")) mode = 0;
	    else if (!strcasecmp(ptr,"html")) mode = 1;
	    else if (!strcasecmp(ptr,"wiki")) mode = 2;
	    else
		{
		mssError(1,"HTTX","Textarea widget 'mode' property must be either 'text','html', or 'wiki'");
		return -1;
		}
	    }

	/** Background color/image? **/
	htrGetBackground(tree, NULL, s->Capabilities.CSS2?1:0, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Style of Textarea - raised/lowered **/
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
	    strtcpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	    }
	else 
	    { 
	    fieldname[0]='\0';
	    } 

	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Write Style header items. **/
	if (s->Capabilities.Dom1HTML)
	    htrAddStylesheetItem_va(s,"\t#tx%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; overflow:hidden; }\n",id,x,y,w-2*box_offset,z);
	else if (s->Capabilities.Dom0NS)
	    htrAddStylesheetItem_va(s,"\t#tx%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tx%dbase\")",id);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "tx_current", "null", 0);
	htrAddScriptGlobal(s, "tx_cur_mainlayer", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_textarea.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);

	htrAddEventHandler(s, "document","MOUSEUP", "tx", 
	    "\n"
	    "    if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "tx",
	    "\n"
	    "    if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "tx", 
	    "\n"
	    "    if (ly.kind == 'tx')\n"
	    "        {\n"
	    "        if (!tx_cur_mainlayer)\n"
	    "            {\n"
	    "            cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "            tx_cur_mainlayer = ly.mainlayer;\n"
	    "            }\n"
	    "        }\n"
	    "\n");

//	htrAddEventHandler(s, "document","MOUSEOUT", "tx", 
//	    "\n"
//	    "\n");

	htrAddEventHandler(s, "document","MOUSEMOVE", "tx", 
	    "\n"
	    "   if (tx_cur_mainlayer && ly.kind != 'tx')\n"			// 
	    "      {\n"								//
	    "      cn_activate(tx_cur_mainlayer, 'MouseOut');\n"		// This is MouseOut Detection!
	    "      tx_cur_mainlayer = null;\n"					// 
	    "      }\n"								//
	    "   if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseMove');\n"
	    "\n");
	    
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    tx_init({layer:nodes[\"%s\"], fieldname:\"%s\", isReadonly:%d, mode:%d, mainBackground:\"%s\"});\n",
	    name, fieldname, is_readonly, mode, main_bg);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"tx%dbase\">\n",id);

	/** Use CSS border or table for drawing? **/
	if (s->Capabilities.CSS2)
	    {
	    if (is_raised)
		htrAddStylesheetItem_va(s, "\t#tx%dbase { border-style:solid; border-width:1px; border-color: white gray gray white; %s }\n", id, main_bg);
	    else
		htrAddStylesheetItem_va(s, "\t#tx%dbase { border-style:solid; border-width:1px; border-color: gray white white gray; %s }\n", id, main_bg);
	    if (h >= 0)
		htrAddStylesheetItem_va(s,"\t#tx%dbase { height:%dpx; }\n", id, h-2*box_offset);
	    }
	else
	    {
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
	    }

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** httxInitialize - register with the ht_render module.
 ***/
int
httxInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Multiline Textarea Driver");
	strcpy(drv->WidgetName,"textarea");
	drv->Render = httxRender;

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);

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

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTX.idcnt = 0;

    return 0;
    }
