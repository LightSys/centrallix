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
/* Module: 	htdrv_scrollpane.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 30, 1998					*/
/* Description:	HTML Widget driver for a scrollpane -- a css layer with	*/
/*		a scrollable layer and a scrollbar for scrolling the	*/
/*		layer.  Can contain most objects, except for framesets.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_scrollpane.c,v 1.24 2004/08/30 03:20:19 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_scrollpane.c,v $

    $Log: htdrv_scrollpane.c,v $
    Revision 1.24  2004/08/30 03:20:19  gbeeley
    - updates for widgets
    - bugfix for htrRender() handling of event handler function return values

    Revision 1.23  2004/08/02 14:09:34  mmcgill
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

    Revision 1.22  2004/07/19 15:30:40  mmcgill
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

    Revision 1.21  2004/03/10 10:51:09  jasonyip

    These are the latest IE-Port files.
    -Modified the browser check to support IE
    -Added some else-if blocks to support IE
    -Added support for geometry library
    -Beware of the document.getElementById to check the parentname does not contain a substring of 'document', otherwise there will be an error on doucument.document

    Revision 1.20  2004/02/24 20:21:57  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.19  2003/11/14 19:45:50  gbeeley
    - make sure only one sp_mv_timeout is active at a time
    - trying direct styles for positioning on W3C browsers

    Revision 1.18  2003/08/04 01:01:45  jorupp
     * scrollpane and treeview now work in Mozilla
     * restructured some of the scrollpane code to keep my debugging sanity

    Revision 1.17  2003/08/02 22:12:06  jorupp
     * got treeview pretty much working (a bit slow though)
    	* I split up several of the functions so that the Mozilla debugger's profiler could help me out more
     * scrollpane displays, doesn't scroll

    Revision 1.16  2003/07/27 03:24:54  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

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

    Revision 1.12  2002/08/13 04:17:35  gbeeley
    Fixed problem with the scrollpane that caused it to shift static content
    down an amount equal to the entire inner dimensions of the scrollpane
    (thus virtually hiding its content).  Static content includes static
    mode widget/html and static mode widget/table.  Example of problem
    was centrallix-os/samples/ReportingSystem.app.

    Revision 1.11  2002/08/01 18:24:44  pfinley
    Added a table to the body of the sparea layer so that it would span the
    entire height and width of the widget (-2 for the border).  Prior it was
    cutting off the pgarea boxes of text areas.

    Revision 1.10  2002/08/01 14:48:46  pfinley
    Added events to the scrollpane widget:
       MouseUp,MouseDown,MouseOver,MouseOut,MouseMove

    Revision 1.9  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.8  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.7  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

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

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
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
    HTSPANE;


/*** htspaneRender - generate the HTML code for the page.
 ***/
int
htspaneRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    int x,y,w,h;
    int id, i;
    int visible = 1;
    char* nptr;
    char bcolor[64] = "";
    char bimage[64] = "";

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE &&!(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTSPANE","Netscape DOM or W3C DOM1 HTML and DOM2 CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTSPANE.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,'\0',63);
	name[63]=0;

	/** Check background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(bcolor,ptr,'\0',63);
	    bcolor[63]=0;
	    }
	if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    memccpy(bimage,ptr,'\0',63);
	    bimage[63]=0;
	    }

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** Ok, write the style header items. **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddStylesheetItem_va(s,"\t#sp%dpane { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(0px,%dpx,%dpx,0px); Z-INDEX:%d; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	    htrAddStylesheetItem_va(s,"\t#sp%darea { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:0px; WIDTH:%dpx; Z-INDEX:%dpx; }\n",id,w-18,z+1);
	    htrAddStylesheetItem_va(s,"\t#sp%dthum { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:18px; WIDTH:18px; Z-INDEX:%dpx; }\n",id,w-18,z+1);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "sp_target_img", "null", 0);
	htrAddScriptGlobal(s, "sp_click_x","0",0);
	htrAddScriptGlobal(s, "sp_click_y","0",0);
	htrAddScriptGlobal(s, "sp_thum_y","0",0);
	htrAddScriptGlobal(s, "sp_mv_timeout","null",0);
	htrAddScriptGlobal(s, "sp_mv_incr","0",0);
	htrAddScriptGlobal(s, "sp_cur_mainlayer","null",0);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_scrollpane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Script initialization call. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddScriptInit_va(s,"    %s=%s.layers.sp%dpane;\n",name,parentname,id);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    htrAddScriptInit_va(s,"    %s=document.getElementById('sp%dpane');\n",name,id);
	    }
	else
	    {
	    mssError(1,"HTSPANE","Cannot render for this browser");
	    }
	htrAddScriptInit_va(s,"    sp_init(%s,\"sp%darea\",\"sp%dthum\",%s);\n", name,id,id,parentobj);

	/** HTML body <DIV> elements for the layers. **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%dpane\"><TABLE %s%s %s%s border='0' cellspacing='0' cellpadding='0' width='%d'>",id,(*bcolor)?"bgcolor=":"",bcolor, (*bimage)?"background=":"",bimage, w);
	    htrAddBodyItem_va(s,"<TR><TD align=right><IMG SRC='/sys/images/ico13b.gif' NAME='u'></TD></TR><TR><TD align=right>");
	    htrAddBodyItem_va(s,"<IMG SRC='/sys/images/trans_1.gif' height='%dpx' width='18px' name='b'>",h-36);
	    htrAddBodyItem_va(s,"</TD></TR><TR><TD align=right><IMG SRC='/sys/images/ico12b.gif' NAME='d'></TD></TR></TABLE>\n");
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%dthum\"><IMG SRC='/sys/images/ico14b.gif' NAME='t'></DIV>\n<DIV ID=\"sp%darea\"><table border='0' cellpadding='0' cellspacing='0' width='%dpx' height='%dpx'><tr><td>",id,id,w-2,h-2);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    //htrAddStylesheetItem_va(s,"\t#sp%dpane { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(0px,%dpx,%dpx,0px); Z-INDEX:%d; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	    //htrAddStylesheetItem_va(s,"\t#sp%darea { HEIGHT: %dpx; WIDTH:%dpx; }\n",id, h, w-18);
	    //htrAddStylesheetItem_va(s,"\t#sp%dthum { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:18px; WIDTH:18px; Z-INDEX:%dpx; }\n",id,w-18,z+1);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%dpane\" style=\"POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(0px,%dpx,%dpx,0px); Z-INDEX:%d;\">\n",id,visible?"inherit":"hidden",x,y,w,h,w,h,z);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%dup\" SRC='/sys/images/ico13b.gif' NAME='u'/>", id);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%dbar\" SRC='/sys/images/trans_1.gif' NAME='b'/>", id);
	    htrAddBodyItem_va(s,"<IMG ID=\"sp%ddown\" SRC='/sys/images/ico12b.gif' NAME='d'/>", id);
	    htrAddStylesheetItem_va(s,"\t#sp%dup { POSITION: absolute; LEFT: %dpx; TOP: 0px; }\n",id, w-18);
	    htrAddStylesheetItem_va(s,"\t#sp%dbar { POSITION: absolute; LEFT: %dpx; TOP: 18px; WIDTH: 18px; HEIGHT: %dpx;}\n",id, w-18, h-36);
	    htrAddStylesheetItem_va(s,"\t#sp%ddown { POSITION: absolute; LEFT: %dpx; TOP: %dpx; }\n",id, w-18, h-18);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%dthum\" style=\"POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:18px; WIDTH:18px; Z-INDEX:%dpx;\"><IMG SRC='/sys/images/ico14b.gif' NAME='t'></DIV>\n", id,w-18,z+1);
	    htrAddBodyItem_va(s,"<DIV ID=\"sp%darea\" style=\"HEIGHT: %dpx; POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:0px; WIDTH:%dpx; Z-INDEX:%dpx;\">",id,h,w-18,z+1);
	    }
	else
	    {
	    mssError(1,"HTSPNE","Browser not supported");
	    }

	/** Add the event handling scripts **/

	htrAddEventHandler(s, "document","MOUSEDOWN","sp",
		"    sp_target_img=e.target;\n"
		"    if (ly.kind == 'sp') cn_activate(ly.mainlayer, 'MouseDown');\n"
		"    if (sp_target_img != null && sp_target_img.kind=='sp' && (sp_target_img.name=='u' || sp_target_img.name=='d'))\n"
		"        {\n"
		"        if (sp_target_img.name=='u') sp_mv_incr=-10; else sp_mv_incr=+10;\n"
		"        pg_set(sp_target_img,'src',htutil_subst_last(sp_target_img.src,\"c.gif\"));\n"
		"        do_mv();\n"
		"        if (!sp_mv_timeout) sp_mv_timeout = setTimeout(tm_mv,300);\n"
		"        return false;\n"
		"        }\n"
		"    else if (sp_target_img != null && sp_target_img.kind=='sp' && sp_target_img.name=='t')\n"
		"        {\n"
		"        sp_click_x = e.pageX;\n"
		"        sp_click_y = e.pageY;\n"
		"        sp_thum_y = getPageY(sp_target_img.thum);\n"
		"        return false;\n"
		"        }\n"
		"    else if (sp_target_img != null && sp_target_img.kind=='sp' && sp_target_img.name=='b')\n"
		"        {\n"
		"        sp_mv_incr=sp_target_img.height+36;\n"
		"        if (e.pageY < getPageY(sp_target_img.thum)+9) sp_mv_incr = -sp_mv_incr;\n"
		"        do_mv();\n"
		"        if (!sp_mv_timeout) sp_mv_timeout = setTimeout(tm_mv,300);\n"
		"        return false;\n"
		"        }\n"
		"    else sp_target_img = null;\n"
		"");
	htrAddEventHandler(s, "document","MOUSEMOVE","sp",
		"    if (ly.kind == 'sp') cn_activate(ly.mainlayer, 'MouseMove');\n"
		"    if (sp_cur_mainlayer && ly.kind != 'sp')\n"
		"        {\n"
		"        cn_activate(sp_cur_mainlayer, 'MouseOut');\n"
		"        sp_cur_mainlayer = null;\n"
		"        }\n"
		"    var ti=sp_target_img;\n"
		"    if (ti != null && ti.kind=='sp' && ti.name=='t')\n"
		"        {\n"
		"        var v=getClipHeight(ti.pane)-(3*18);\n"
		"        var new_y=sp_thum_y + (e.pageY-sp_click_y);\n"
		"        if (new_y > getPageY(ti.pane)+18+v) new_y=getPageY(ti.pane)+18+v;\n"
		"        if (new_y < getPageY(ti.pane)+18) new_y=getPageY(ti.pane)+18;\n"
		"        setPageY(ti.thum,new_y);\n"
		"        var h=getClipHeight(ti.area)+getClipTop(ti.area);\n"
		"        var d=h-getClipHeight(ti.pane);\n"
		"        if (d<0) d=0;\n"
		"        var yincr = (((ti.thum.y-18)/v)*-d) - ti.area.y;\n"
		"        ti.area.y+=yincr;\n"
		"        return false;\n"
		"        }\n"
		"");
	htrAddEventHandler(s, "document","MOUSEUP","sp",
		"    if (sp_mv_timeout != null)\n"
		"        {\n"
		"        clearTimeout(sp_mv_timeout);\n"
		"        sp_mv_timeout = null;\n"
		"        sp_mv_incr = 0;\n"
		"        }\n"
		"    if (sp_target_img != null)\n"
		"        {\n"
		"        if (sp_target_img.name != 'b')\n"
		"            pg_set(sp_target_img,'src',htutil_subst_last(sp_target_img.src,\"b.gif\"));\n"
		"        sp_target_img = null;\n"
		"        }\n"
		"    if (ly.kind == 'sp') cn_activate(ly.mainlayer, 'MouseUp');\n");
	htrAddEventHandler(s, "document", "MOUSEOVER", "sp",
		"    if (ly.kind == 'sp')\n"
		"        {\n"
		"        if (!sp_cur_mainlayer)\n"
		"            {\n"
		"            cn_activate(ly.mainlayer, 'MouseOver');\n"
		"            sp_cur_mainlayer = ly.mainlayer;\n"
		"            }\n"
		"        }\n");
//	htrAddEventHandler(s, "document", "MOUSEOUT", "sp",
//		"");

	/** Check for more sub-widgets within the page. **/
	if(s->Capabilities.Dom0NS)
	    {
	    snprintf(sbuf,160,"%s.document.layers.sp%darea.document",name,id);
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    snprintf(sbuf,160,"document.getElementById('sp%darea')",id);
	    }
	else
	    {
	    mssError(1,"HTSPANE","Cannot render for this browser");
	    }
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2, sbuf, name);

	/** Finish off the last <DIV> **/
	if(s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem(s,"</td></tr></table></DIV></DIV>\n");
	    }
	else if(s->Capabilities.Dom1HTML)
	    {
	    htrAddBodyItem(s,"</DIV></DIV>\n");
	    }
	else
	    {
	    mssError(1,"HTSPNE","browser not supported");
	    }

    return 0;
    }


/*** htspaneInitialize - register with the ht_render module.
 ***/
int
htspaneInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ScrollPane Widget Driver");
	strcpy(drv->WidgetName,"scrollpane");
	drv->Render = htspaneRender;

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");
	HTSPANE.idcnt = 0;

    return 0;
    }
