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
/* Module: 	htdrv_treeview.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 1, 1998 					*/
/* Description:	HTML Widget driver for a treeview.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_treeview.c,v 1.25 2004/03/17 20:29:50 jasonyip Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_treeview.c,v $

    $Log: htdrv_treeview.c,v $
    Revision 1.25  2004/03/17 20:29:50  jasonyip

    Fixed the problem of cannot click on the open folder picture on treeview:
    - changed .href == null to .nodeName != 'A'
    - changed .href != null to .nodeName == 'A'

    Revision 1.24  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.23  2004/02/24 20:21:57  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.22  2003/11/18 05:58:34  gbeeley
    - messing with efficiency issues

    Revision 1.21  2003/08/02 22:12:06  jorupp
     * got treeview pretty much working (a bit slow though)
    	* I split up several of the functions so that the Mozilla debugger's profiler could help me out more
     * scrollpane displays, doesn't scroll

    Revision 1.20  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.19  2002/12/04 00:19:12  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.18  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.17  2002/07/31 14:45:58  lkehresman
    * Added standard events to treeview
    * Standardized x.document.layer and x.mainlayer to point to the rigth layers

    Revision 1.16  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.15  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.14  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.13  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.12  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.11  2002/04/25 22:51:29  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.10  2002/03/17 03:51:03  jorupp
    * treeview now returns value on function call (in alert window)
    * implimented basics of 3-button confirm window on the form side
        still need to update several functions to use it

    Revision 1.9  2002/03/16 01:56:14  jorupp
     * code cleanup
     * added right/middle click functionality on text -- allows you to run code on an object

    Revision 1.8  2002/03/14 22:02:58  jorupp
     * bugfixes, dropdown doesn't throw errors when being cleared/reset

    Revision 1.7  2002/03/14 17:58:52  jorupp
     * added: change root object/array or run function by clicking hyperlink

    Revision 1.6  2002/03/14 05:11:49  jorupp
     * bugfixes

    Revision 1.5  2002/03/14 03:29:51  jorupp
     * updated form to prepend a : to the fieldname when using for a query
     * updated osrc to take the query given it by the form, submit it to the server,
        iterate through the results, and store them in the replica
     * bug fixes to treeview (DOMviewer mode) -- added ability to change scaler values

    Revision 1.4  2002/03/13 01:59:43  jorupp
     * Changed treeview to allow it to operate in javascript 'DomViewer' mode
         check the sample file for an example and usage guidelines

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

    Revision 1.1.1.1  2001/08/13 18:00:52  gbeeley
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
    HTTREE;


/*** httreeVerify - not written yet.
 ***/
int
httreeVerify()
    {
    return 0;
    }


/*** httreeRender - generate the HTML code for the page.
 ***/
int
httreeRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char src[128];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w;
    int id;
    char* nptr;
    int show_root = 1;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTREE","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTREE.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTREE","TreeView widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'width' property");
	    return -1;
	    }

	/** Are we showing root of tree or the trunk? **/
	show_root = htrGetBoolean(w_obj, "show_root", 1);
	if (show_root < 0) return -1;

	/** Compensate hidden root position if not shown **/
	if (!show_root)
	    {
	    x -= 20;
	    y -= 20;
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get source directory tree **/
	if (objGetAttrValue(w_obj,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTREE","TreeView widget must have a 'source' property");
	    return -1;
	    }
	memccpy(src,ptr,0,127);
	src[127]=0;


	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,"\t#tv%droot { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n",id,show_root?"inherit":"hidden",x,y,w,z);
	    htrAddStylesheetItem_va(s,"\t#tv%dload { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; clip:rect(0px,0px,0px,0px); Z-INDEX:0; }\n",id);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "tv_tgt_layer", "null", 0);
	htrAddScriptGlobal(s, "tv_target_img","null",0);
	htrAddScriptGlobal(s, "tv_layer_cache","null",0);
	htrAddScriptGlobal(s, "tv_alloc_cnt","0",0);
	htrAddScriptGlobal(s, "tv_cache_cnt","0",0);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Script initialization call. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit_va(s,"    %s = %s.layers.tv%droot;\n",nptr, parentname, id);
	    htrAddScriptInit_va(s,"    tv_init(%s,\"%s\",%s.layers.tv%dload,%s,%d,%s,null);\n",
		    nptr, src, parentname, id, parentname, w, parentobj);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    htrAddScriptInit_va(s,"    %s = document.getElementById('tv%droot');\n",nptr, id);
	    htrAddScriptInit_va(s,"    tv_init(%s,\"%s\",document.getElementById('tv%dload'),%s,%d,%s,null);\n",
		    nptr, src, id, parentname, w, parentobj);
	    }
	else
	    {
	    mssError(1,"HTTREE","cannot render for this browser");
	    }

	/** Script includes **/
	htrAddScriptInclude(s, "/sys/js/htdrv_treeview.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** HTML body <DIV> elements for the layers. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem_va(s, "<DIV ID=\"tv%droot\"><IMG SRC=/sys/images/ico02b.gif align=left>&nbsp;%s</DIV>\n",id,src);
	    htrAddBodyItem_va(s, "<DIV ID=\"tv%dload\"></DIV>\n",id);
	    }
	else
	    {
	    htrAddBodyItem_va(s, "<DIV ID=\"tv%droot\" style=\"POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d;\"><IMG SRC=/sys/images/ico02b.gif align=left>&nbsp;%s</DIV>\n",id,show_root?"inherit":"hidden",x,y,w,z,src);
	    htrAddBodyItem_va(s, "<DIV ID=\"tv%dload\" style=\"POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; clip:rect(0px,0px,0px,0px); Z-INDEX:0;\"></DIV>\n",id);
	    }

	/** Event handler for click-on-url **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddEventHandler(s, "document","CLICK","tv",
		"    if (e.target != null && e.target.kind == 'tv' && e.target.href != null)\n"
		"        {\n"
		"        cn_activate(ly, 'Click');\n"
		"        return tv_click(e);\n"
		"        }\n");
	    }
	else
	    {
	    htrAddEventHandler(s, "document","CLICK","tv",
		"    if (e.target != null && e.target.kind == 'tv' && e.target.nodeName == 'A')\n"
		"        {\n"
		"        cn_activate(ly, 'Click');\n"
		"        return tv_click(e);\n"
		"        }\n");
	    }

	/** Add the event handling scripts **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddEventHandler(s, "document","MOUSEDOWN","tv",
		"    if (ly.kind == 'tv') cn_activate(ly, 'MouseDown');\n"
		"    if (e.target != null && e.target.kind=='tv' && e.target.href == null)\n"
		"        {\n"
		"        if (e.which == 3)\n"
		"            {\n"
		"            return false; //return tv_rclick(e);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            tv_target_img = e.target;\n"
		"            pg_set(tv_target_img,'src',htutil_subst_last(tv_target_img.src,'c.gif'));\n"
		"            }\n"
		"        }\n");
	    }
	else
	    {
	    htrAddEventHandler(s, "document","MOUSEDOWN","tv",
		"    if (ly.kind == 'tv') cn_activate(ly, 'MouseDown');\n"
		"    if (e.target != null && e.target.kind=='tv' && e.target.nodeName != 'A')\n"
		"        {\n"
		"        if (e.which == 3)\n"
		"            {\n"
		"            return false; //return tv_rclick(e);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            tv_target_img = e.target;\n"
		"            pg_set(tv_target_img,'src',htutil_subst_last(tv_target_img.src,'c.gif'));\n"
		"            }\n"
		"        }\n");
	    }

	htrAddEventHandler(s, "document","MOUSEUP","tv",
		"    if (ly.kind == 'tv') cn_activate(ly, 'MouseUp');\n"
		"    if (e.target != null && e.target.kind == 'tv' && e.which == 3) return false;\n"
		"    if (tv_target_img != null && tv_target_img.kind == 'tv')\n"
		"        {\n"
		"        var l = tv_target_img.layer;\n"
		"        tv_target_img = null;\n"
		"        if (l.expanded == 0)\n"
		"            {\n"
		"            return l.expand();\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            return l.collapse();\n"
		"            }\n"
		"        }\n");

	htrAddEventHandler(s,"document","MOUSEOVER","tv","   if (ly.kind == 'tv') cn_activate(ly, 'MouseOver');\n");
	htrAddEventHandler(s,"document","MOUSEMOVE","tv","   if (ly.kind == 'tv') cn_activate(ly, 'MouseMove');\n");
	htrAddEventHandler(s,"document","MOUSEOUT","tv", "   if (ly.kind == 'tv') cn_activate(ly, 'MouseOut');\n");

	/** Check for more sub-widgets within the treeview. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+2, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** httreeInitialize - register with the ht_render module.
 ***/
int
httreeInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Treeview Widget Driver");
	strcpy(drv->WidgetName,"treeview");
	drv->Render = httreeRender;
	drv->Verify = httreeVerify;

	/** Add the 'click item' event **/
	htrAddEvent(drv,"ClickItem");
	htrAddParam(drv,"ClickItem","Pathname",DATA_T_STRING);
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Add the 'rightclick item' event **/
	htrAddEvent(drv,"RightClickItem");
	htrAddParam(drv,"RightClickItem","Pathname",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTREE.idcnt = 0;

    return 0;
    }
