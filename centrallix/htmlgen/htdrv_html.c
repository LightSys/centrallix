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
/* Module: 	htdrv_html.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 3, 1998 					*/
/* Description:	HTML Widget driver for HTML text, either given as an	*/
/*		immediate string or pulled from an ObjectSystem source	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_html.c,v 1.21 2004/08/04 20:03:09 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_html.c,v $

    $Log: htdrv_html.c,v $
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
hthtmlRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[320];
    char src[128] = "";
    int x=-1,y=-1,w,h;
    int id,cnt, i;
    int mode = 0;
    char* nptr = NULL;
    pObject content_obj;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTHTML","Netscape DOM support required");
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
	    memccpy(src,ptr,0,127);
	    src[127] = 0;
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    /** Only give x and y if supplied. **/
	    if (x < 0 || y < 0)
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%dpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%dpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%dfader { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z+1);
	        }
	    else
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%dfader { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z+1);
	        }

            /** Write named global **/
            nptr = (char*)nmMalloc(strlen(name)+1);
            strcpy(nptr,name);
            htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
            htrAddScriptGlobal(s, "ht_fadeobj", "null", 0);
    
	    htrAddScriptInclude(s, "/sys/js/htdrv_html.js", 0);
	    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	    /** Event handler for click-on-link. **/
	    htrAddEventHandler(s, "document","CLICK","ht",
	    	    "    if (e.target != null && e.target.kind == 'ht')\n"
		    "        {\n"
		    "        return ht_click(e);\n"
		    "        }\n");

	    htrAddEventHandler(s,"document","MOUSEOVER","ht", "    if (ly.kind == 'ht') cn_activate(ly.mainlayer,'MouseOver');\n");
	    htrAddEventHandler(s,"document","MOUSEOUT", "ht", "    if (ly.kind == 'ht') cn_activate(ly.mainlayer,'MouseOut');\n");
	    htrAddEventHandler(s,"document","MOUSEMOVE","ht", "    if (ly.kind == 'ht') cn_activate(ly.mainlayer,'MouseMove');\n");
	    htrAddEventHandler(s,"document","MOUSEDOWN","ht", "    if (ly.kind == 'ht') cn_activate(ly.mainlayer,'MouseDown');\n");
	    htrAddEventHandler(s,"document","MOUSEUP",  "ht", "    if (ly.kind == 'ht') cn_activate(ly.mainlayer,'MouseUp');\n");

            /** Script initialization call. **/
            htrAddScriptInit_va(s,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,%s.layers.ht%dfader,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit_va(s,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
    
            /** HTML body <DIV> element for the layer. **/
            htrAddBodyItem_va(s,"<DIV background=\"/sys/images/fade_lrwipe_01.gif\" ID=\"ht%dfader\"></DIV><DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id,id);
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

        /** Check for more sub-widgets within the html entity. **/
	if (mode == 1)
	    {
            /*sprintf(sbuf,"%s.document",nptr,id);*/
            snprintf(sbuf,320,"%s.document",nptr);
	    }
	else
	    {
	    memccpy(sbuf,parentname,0,319);
	    sbuf[319] = '\0';
	    nptr = parentobj;
	    }




	/** render subwidgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3, sbuf, nptr);


        /** End the containing layer. **/
        if (mode == 1) htrAddBodyItem(s, "</DIV>\n");

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
