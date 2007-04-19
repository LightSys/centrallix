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
/* Module: 	htdrv_html.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 3, 1998 					*/
/* Description:	HTML Widget driver for HTML text, either given as an	*/
/*		immediate string or pulled from an ObjectSystem source	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_html.c,v 1.28 2007/04/19 21:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_html.c,v $

    $Log: htdrv_html.c,v $
    Revision 1.28  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.27  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.26  2006/10/16 18:34:33  gbeeley
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

    Revision 1.25  2005/06/23 22:07:59  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.24  2005/02/26 06:32:21  gbeeley
    - Get HTML widget working in Mozilla.

    Revision 1.23  2004/09/02 03:27:20  gbeeley
    - first (imperfect) cut at conversion of html widget

    Revision 1.22  2004/08/13 18:46:13  mmcgill
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

    Revision 1.21  2004/08/04 20:03:09  mmcgill
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

    Revision 1.20  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.19  2004/08/02 14:09:34  mmcgill
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

    Revision 1.18  2004/07/19 15:30:40  mmcgill
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

    Revision 1.17  2003/11/30 02:09:40  gbeeley
    - adding autoquery modes to OSRC (never, onload, onfirstreveal, or
      oneachreveal)
    - adding serialized loader queue for preventing communcations with the
      server from interfering with each other (netscape bug)
    - pg_debug() writes to a "debug:" dynamic html widget via AddText()
    - obscure/reveal subsystem initial implementation

    Revision 1.16  2003/11/12 22:17:24  gbeeley
    Declaring dependency on strings htutils module

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

    Revision 1.13  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.12  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.11  2002/07/30 13:59:17  lkehresman
    * Added standard events to the html widget
    * Standardized the layer references (x.document.layer is itself, x.mainlayer
         is the top layer)

    Revision 1.10  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.9  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.8  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.7  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/19 16:31:04  lkehresman
    * Changed snprintf to *_va functions in several places
    * Allow fading to both static and dynamic pages

    Revision 1.4  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

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
    HTHTML;


/*** hthtmlRender - generate the HTML code for the page.
 ***/
int
hthtmlRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char sbuf[320];
    char src[128] = "";
    int x=-1,y=-1,w,h;
    int id,cnt, i;
    int mode = 0;
    pObject content_obj;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTHTML","NS4 or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTHTML.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=-1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=-1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTHTML","HTML widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get source html objectsystem entry. **/
	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(src,ptr,sizeof(src));
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    /** Only give x and y if supplied. **/
	    if (x < 0 || y < 0)
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { POSITION:relative; VISIBILITY:hidden; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z+1);
	        }
	    else
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z+1);
	        }

	    if (s->Capabilities.CSS1)
		{
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { overflow:hidden; }\n",id);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { overflow:hidden; }\n",id);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { overflow:hidden; }\n",id);
		htrAddStylesheetItem_va(s,"\t#ht%POSloader { overflow:hidden; visibility:hidden; position:absolute; top:0px; left:0px; width:0px; height:0px; }\n", id);
		}

            /** Write named global **/
	    htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"ht%POSpane\")",id);
	    htrAddWgtrCtrLinkage(s, tree, "_obj");

            htrAddScriptGlobal(s, "ht_fadeobj", "null", 0);
    
	    htrAddScriptInclude(s, "/sys/js/htdrv_html.js", 0);
	    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	    /** Event handler for click-on-link. **/
	    htrAddEventHandlerFunction(s, "document","CLICK","ht","ht_click");
	    htrAddEventHandlerFunction(s,"document","MOUSEOVER","ht","ht_mouseover");
	    htrAddEventHandlerFunction(s,"document","MOUSEOUT", "ht", "ht_mouseout");
	    htrAddEventHandlerFunction(s,"document","MOUSEMOVE","ht", "ht_mousemove");
	    htrAddEventHandlerFunction(s,"document","MOUSEDOWN","ht", "ht_mousedown");
	    htrAddEventHandlerFunction(s,"document","MOUSEUP",  "ht", "ht_mouseup");

            /** Script initialization call. **/
	    if (s->Capabilities.Dom0NS)
		htrAddScriptInit_va(s,"    ht_init({layer:nodes[\"%STR&SYM\"], layer2:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"ht%POSpane2\"), faderLayer:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"ht%POSfader\"), source:\"%STR&ESCQ\", width:%INT, height:%INT, loader:null});\n",
                    name, name, id, name, id, 
		    src, w,h);
	    else
		htrAddScriptInit_va(s,"    ht_init({layer:nodes[\"%STR&SYM\"], layer2:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"ht%POSpane2\"), faderLayer:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"ht%POSfader\"), source:\"%STR&ESCQ\", width:%INT, height:%INT, loader:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])), \"ht%POSloader\")});\n",
                    name, name, id, name, id, 
		    src, w,h, name, id);
    
            /** HTML body <DIV> element for the layer. **/
            htrAddBodyItem_va(s,"<DIV background=\"/sys/images/fade_lrwipe_01.gif\" ID=\"ht%POSfader\"></DIV>",id);
	    htrAddBodyItemLayer_va(s, 0, "ht%POSpane2", id, "");
	    if (!s->Capabilities.Dom0NS)
		htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "ht%POSloader", id, "");
	    htrAddBodyItemLayerStart(s, 0, "ht%POSpane", id);
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NOOBJECT;
	    }

        /** If prefix text given, put it. **/
        if (wgtrGetPropertyValue(tree, "prologue", DATA_T_STRING,POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If full text given, put it. **/
        if (wgtrGetPropertyValue(tree, "content", DATA_T_STRING,POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If source is an objectsystem entry... **/
        if (src[0] && strncmp(src,"http:",5) && strncmp(src,"debug:",6))
            {
            content_obj = objOpen(s->ObjSession,src,O_RDONLY,0600,"text/html");
            if (content_obj)
                {
                while((cnt = objRead(content_obj, sbuf, 159,0,0)) > 0)
                    {
                    sbuf[cnt]=0;
                    htrAddBodyItem(s, sbuf);
                    }
                objClose(content_obj);
                }
            }

        /** If post text given, put it. **/
        if (wgtrGetPropertyValue(tree, "epilogue", DATA_T_STRING, POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

	/** render subwidgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

        /** End the containing layer. **/
        if (mode == 1) htrAddBodyItemLayerEnd(s, 0);

    return 0;
    }


/*** hthtmlInitialize - register with the ht_render module.
 ***/
int
hthtmlInitialize()
    {
    pHtDriver drv;

        /** Allocate the driver **/
        drv = htrAllocDriver();
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"HTML Textual Source Driver");
        strcpy(drv->WidgetName,"html");
        drv->Render = hthtmlRender;

	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

        /** Add the 'load page' action **/
	htrAddAction(drv,"LoadPage");
	htrAddParam(drv,"LoadPage","Mode",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Source",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Transition",DATA_T_STRING);

        /** Register. **/
        htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

        HTHTML.idcnt = 0;

    return 0;
    }
