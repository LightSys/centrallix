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
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_window.c      					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 16, 1999					*/
/* Description:	HTML Widget driver for a window -- a DHTML layer that	*/
/*		can be dragged around the screen and appears to have	*/
/*		a 'titlebar' with a close (X) button on it.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_window.c,v 1.36 2004/07/19 15:30:42 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_window.c,v $

    $Log: htdrv_window.c,v $
    Revision 1.36  2004/07/19 15:30:42  mmcgill
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

    Revision 1.35  2003/11/22 16:37:18  jorupp
     * add support for moving event handler scripts to the .js code
     	note: the underlying implimentation in ht_render.c_will_ change, this was
    	just to get opinions on the API and output
     * moved event handlers for htdrv_window from the .c to the .js

    Revision 1.34  2003/08/02 22:12:06  jorupp
     * got treeview pretty much working (a bit slow though)
    	* I split up several of the functions so that the Mozilla debugger's profiler could help me out more
     * scrollpane displays, doesn't scroll

    Revision 1.33  2003/07/20 03:41:17  jorupp
     * got window mostly working in Mozilla

    Revision 1.32  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.31  2002/12/04 00:19:12  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.30  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.29  2002/08/23 17:31:05  lkehresman
    moved window_current global to the page widget so it is always defined
    even if it is null.  This prevents javascript errors from the objectsource
    widget when no windows exist in the app.

    Revision 1.28  2002/08/15 14:08:47  pfinley
    fixed a bit masking issue with the page closetype flag.

    Revision 1.27  2002/08/15 13:58:16  pfinley
    Made graphical window closing and shading properties of the window widget,
    rather than globally of the page.

    Revision 1.26  2002/08/14 20:16:38  pfinley
    Added some visual effets for the window:
     - graphical window shading (enable by setting gshade="true")
     - added 3 new closing types (enable by setting closetype="shrink1","shrink2", or "shrink3")

    These are gloabl changes, and can only be set on the page widget... these
    will become part of theming once it is implemented (i think).

    Revision 1.25  2002/08/12 17:51:16  pfinley
    - added an attract option to the page widget. if this is set, centrallix
      windows will attract to the edges of the browser window. set to how many
      pixels from border to attract.
    - also fixed a mainlayer issue with the window widget which allowed for
      the contents of a window to be draged and shaded (very interesting :)

    Revision 1.24  2002/08/01 19:22:25  lkehresman
    Renamed what was previously known as mainlayer (aka ml) to be ContentLayer
    so that it is more descriptive and doesn't conflict with other mainlayer
    properties.

    Revision 1.23  2002/07/30 12:45:44  lkehresman
    * Added standard events to window
    * Reworked the window layer properties a bit so they are standard
        (x.document.layer points to itself, x.mainlayer points to top layer)
    * Added a bugfix to the connector (it was missing brackets around a
        conditional statement
    * Changed the subwidget parsing to pass different parameters if the
        subwidget is a connector.

    Revision 1.22  2002/07/20 20:16:52  lkehresman
    Added ToggleVisibility event connector to the window widget

    Revision 1.21  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.20  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.19  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.18  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.17  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.16  2002/06/18 15:41:52  lkehresman
    Fixed a bug that made window header backgrounds transparent if the color
    wasn't specifically set.  Now it grabs the normal background color/image
    by default but the default can be overwritten by "hdr_bgcolor" and
    "hdr_background".

    Revision 1.15  2002/06/17 21:35:56  jorupp
     * allowed for window inside of window (same basic method used with form and osrc)

    Revision 1.14  2002/06/09 23:44:47  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.13  2002/06/06 17:12:23  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.12  2002/06/01 19:49:30  lkehresman
    A couple changes that provide visual enhancements to the window widget

    Revision 1.11  2002/05/31 01:26:41  lkehresman
    * modified the window header HTML to make it look nicer
    * fixed a truncation problem with the image button

    Revision 1.10  2002/03/13 19:48:45  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.9  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.8  2002/02/13 19:35:55  lkehresman
    Fixed another bug I introduced.  ly.document isn't even always there.

    Revision 1.7  2002/02/13 19:30:48  lkehresman
    Fixed a bug I introduced with my last commit.  ly.document.images[6] doesn't always exist.

    Revision 1.6  2002/02/13 19:20:40  lkehresman
    Fixed a minor bug that wouldn't reset the "X" image if the close button
    was clicked, but the mouse moved out of the image border.

    Revision 1.5  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.4  2001/10/09 01:14:52  lkehresman
    Made a few modifications to the behavior of windowshading.  It now forgets
    clicks that do other things such as raising windows--it won't count that as
    the first click of a double-click.

    Revision 1.3  2001/10/08 04:17:14  lkehresman
     * Cleaned up the generated code for windowshading (Beely-standard Complient)
     * Testing out emailing CVS commits

    Revision 1.2  2001/10/08 03:59:54  lkehresman
    Added window shading support

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
    HTWIN;


/*** htwinVerify - not written yet.
 ***/
int
htwinVerify()
    {
    return 0;
    }


/*** htwinRender - generate the HTML code for the page.
 ***/
int
htwinRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[HT_SBUF_SIZE];
    char sbuf2[HT_SBUF_SIZE];
    char sbuf3[HT_SBUF_SIZE];
    char sbuf4[HT_SBUF_SIZE];
    pWgtrNode sub_tree;
    int x,y,w,h;
    int tbw,tbh,bx,by,bw,bh;
    int id, i;
    int visible = 1;
    char* nptr;
    char bgnd[128] = "";
    char hdr_bgnd[128] = "";
    char bgnd_style[128] = "";
    char hdr_bgnd_style[128] = "";
    char txtcolor[64] = "";
    int has_titlebar = 1;
    char title[128];
    int is_dialog_style = 0;
    int gshade = 0;
    int closetype = 0;

	if(!(s->Capabilities.Dom0NS || s->Capabilities.Dom1HTML))
	    {
	    mssError(1,"HTWIN","Netscape DOM support or W3C DOM Level 1 support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTWIN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x = 0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y = 0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,'\0',63);
	name[63]=0;

	/** Check background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	    sprintf(bgnd,"bgcolor='%.40s'",ptr);
	    sprintf(hdr_bgnd,"bgcolor='%.40s'",ptr);
	    sprintf(bgnd_style,"background-color: %.40s;",ptr);
	    sprintf(hdr_bgnd_style,"background-color: %.40s;",ptr);
	}
	else if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0) {
	    sprintf(bgnd,"background='%.100s'",ptr);
	    sprintf(hdr_bgnd,"background='%.100s'",ptr);
	    sprintf(bgnd_style,"background-image: %.100s;",ptr);
	    sprintf(hdr_bgnd_style,"background-image: %.100s;",ptr);
	}

	/** Check header background color/image **/
	if (wgtrGetPropertyValue(tree,"hdr_bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(hdr_bgnd,"bgcolor='%.40s'",ptr);
	    sprintf(hdr_bgnd_style,"background-color: %.40s;",ptr);
	    }
	else if (wgtrGetPropertyValue(tree,"hdr_background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(hdr_bgnd,"background='%.100s'",ptr);
	    sprintf(hdr_bgnd_style,"background-image: %.100s;",ptr);
	    }

	/** Check title text color. **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(txtcolor,"%.63s",ptr);
	else
	    strcpy(txtcolor,"black");

	/** Check window title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(title,"%.127s",ptr);
	else
	    strcpy(title,name);

	/** Marked not visible? **/
	if (wgtrGetPropertyValue(tree,"visible",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** No titlebar? **/
	if (wgtrGetPropertyValue(tree,"titlebar",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no"))
	    has_titlebar = 0;

	/** Dialog or window style? **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
	    is_dialog_style = 1;

	/** Graphical window shading? **/
	if (wgtrGetPropertyValue(tree,"gshade",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    gshade = 1;

	/** Graphical window close? **/
	if (wgtrGetPropertyValue(tree,"closetype",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"shrink1")) closetype = 1;
	    else if (!strcmp(ptr,"shrink2")) closetype = 2;
	    else if (!strcmp(ptr,"shrink3")) closetype = 1 | 2;
	    }

	/** Compute titlebar width & height - includes edge below titlebar. **/
	if (has_titlebar)
	    {
	    tbw = w-2;
	    if (is_dialog_style)
	        tbh = 24;
	    else
	        tbh = 23;
	    }
	else
	    {
	    tbw = w-2;
	    tbh = 0;
	    }

	/** Compute window body geometry **/
	if (is_dialog_style)
	    {
	    bx = 1;
	    by = 1+tbh;
	    bw = w-2;
	    bh = h-tbh-2;
	    }
	else
	    {
	    bx = 2;
	    by = 1+tbh;
	    bw = w-4;
	    bh = h-tbh-3;
	    }

	if(s->Capabilities.HTML40 && s->Capabilities.CSS2)
	    {
	    /** Ok, write the style header items. **/
	    htrAddStylesheetItem_va(s,"\t#wn%dbase { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; overflow: hidden; clip:rect(0px, %dpx, %dpx, 0px); Z-INDEX:%d; %s}\n",
		    id,visible?"inherit":"hidden",x,y,w,h, w+2, h+2, z, bgnd_style);
	    htrAddStylesheetItem_va(s,"\t#wn%dbase { border-style: solid; border-width: 1px; border-color: white gray gray white; }\n", id);
	    htrAddStylesheetItem_va(s,"\t#wn%dtitlebar { POSITION: absolute; VISIBILITY: inherit; LEFT: 0px; TOP: 0px; HEIGHT: %dpx; WIDTH: 100%%; overflow: hidden; Z-INDEX: %d; text-color: %s; %s}\n", id, tbh, z+1, txtcolor, hdr_bgnd_style);
	    htrAddStylesheetItem_va(s,"\t#wn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:%dpx; WIDTH: %dpx; HEIGHT:%dpx; overflow: hidden; clip:rect(0px, %dpx, %dpx, 0px); Z-INDEX:%d; }\n",
		    id, tbh, w-2, h-tbh-2, w, h-tbh, z+1);
	    htrAddStylesheetItem_va(s,"\t#wn%dmain { border-style: solid; border-width: 1px; border-color: gray white white gray; }\n", id);
	    htrAddStylesheetItem_va(s,"\t#wn%dclose { vertical-align: middle; }\n",id);
	    }
	else
	    {
	    /** Ok, write the style header items. **/
	    htrAddStylesheetItem_va(s,"\t#wn%dbase { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(%dpx, %dpx); Z-INDEX:%d; }\n",
		    id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	    htrAddStylesheetItem_va(s,"\t#wn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; clip:rect(%dpx,%dpx); Z-INDEX:%d; }\n",
		    id, bx, by, bw, bh, bw, bh, z+1);
	    }

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "wn_top_z","10000",0);
	htrAddScriptGlobal(s, "wn_list","null",0);
	htrAddScriptGlobal(s, "wn_current","null",0);
	htrAddScriptGlobal(s, "wn_newx","null",0);
	htrAddScriptGlobal(s, "wn_newy","null",0);
	htrAddScriptGlobal(s, "wn_topwin","null",0);
	htrAddScriptGlobal(s, "wn_msx","null",0);
	htrAddScriptGlobal(s, "wn_msy","null",0);
	htrAddScriptGlobal(s, "wn_moved","0",0);
	htrAddScriptGlobal(s, "wn_clicked","0",0);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_window.js", 0);
	

	/** Event handler for mousedown -- initial click **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "wn", "wn_mousedown");

	/** Mouse up event handler -- when user releases the button **/
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "wn", "wn_mouseup");

	/** Mouse move event handler -- when user drags the window **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "wn", "wn_mousemove");

	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "wn", "wn_mouseover");

	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "wn", "wn_mouseout");

	if(s->Capabilities.Dom1HTML)
	    {
	    /** Script initialization call. **/
	    htrAddScriptInit_va(s,"    %s = wn_init(document.getElementById('wn%dbase'),document.getElementById('wn%dmain'),%d,%d, document.getElementById('wn%dtitlebar'));\n", 
		    name,id,id,gshade,closetype, id);
	    }
	else if(s->Capabilities.Dom0NS)
	    {
	    /** Script initialization call. **/
	    htrAddScriptInit_va(s,"    %s = wn_init(%s.layers.wn%dbase,%s.layers.wn%dbase.document.layers.wn%dmain,%d,%d);\n", 
		    name,parentname,id,parentname,id,id,gshade,closetype);
	    }

	/** HTML body <DIV> elements for the layers. **/
	/** This is the top white edge of the window **/
	if(s->Capabilities.HTML40 && s->Capabilities.CSS2) 
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%dbase\">\n",id);
	    if (has_titlebar)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"wn%dtitlebar\">\n",id);
		htrAddBodyItem_va(s,"<img id=\"wn%dclose\" name=\"close\" src=\"/sys/images/01close.gif\"/> <span style=\"position: absolute; bottom: 0px; font-weight: 600;\">%s</span>\n", id, title);
		htrAddBodyItem_va(s,"</DIV>\n");
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%dmain\">\n",id);
	    }
	else
	    {
	    htrAddBodyItem_va(s,"<DIV ID=\"wn%dbase\"><TABLE border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n",id);
	    htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" \"width=\"1\" height=\"1\"></TD>\n");
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		}
	    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"%d\" height=\"1\"></TD>\n",is_dialog_style?(tbw):(tbw-2));
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		}
	    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
	    
	    /** Titlebar for window, if specified. **/
	    if (has_titlebar)
		{
		htrAddBodyItem_va(s,"<TR><TD width=\"1\"><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"22\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD width=\"%d\" %s colspan=\"%d\"><TABLE border=\"0\"><TR><TD><IMG src=\"/sys/images/01close.gif\" name=\"close\" align=\"left\"></TD><TD valign=\"middle\"><FONT COLOR='%s'> <b>%s</b></FONT></TD></TR></TABLE></TD>\n",
		    tbw,hdr_bgnd,is_dialog_style?1:3,txtcolor,title);
		htrAddBodyItem_va(s,"    <TD width=\"1\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"22\"></TD></TR>\n");
		}

	    /** This is the beveled-down edge below the top of the window **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%d\" height=\"1\"></TD>\n",w-3);
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		}
	    else
		{
		if (has_titlebar)
		    {
		    htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		    htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%d\" height=\"1\"></TD></TR>\n",w-1);
		    htrAddBodyItem_va(s,"<TR><TD colspan=\"2\"><IMG src=\"/sys/images/white_1x1.png\" width=\"%d\" height=\"1\"></TD>\n",w-1);
		    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		    }
		}

	    /** This is the left side of the window. **/
	    htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"%d\"></TD>\n", bh);
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"%d\"></TD>\n",bh);
		}

	    /** Here's where the content goes... **/
	    htrAddBodyItem(s,"    <TD></TD>\n");

	    /** Right edge of the window **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"%d\"></TD>\n",bh);
		}
	    htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"%d\"></TD></TR>\n",bh);

	    /** And... bottom edge of the window. **/
	    if (!is_dialog_style)
		{
		htrAddBodyItem_va(s,"<TR><TD><IMG src=\"/sys/images/white_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD>\n");
		htrAddBodyItem_va(s,"    <TD colspan=\"2\"><IMG src=\"/sys/images/white_1x1.png\" width=\"%d\" height=\"1\"></TD>\n",w-3);
		htrAddBodyItem_va(s,"    <TD><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"1\" height=\"1\"></TD></TR>\n");
		}
	    htrAddBodyItem_va(s,"<TR><TD colspan=\"5\"><IMG src=\"/sys/images/dkgrey_1x1.png\" width=\"%d\" height=\"1\"></TD></TR>\n",w);
	    htrAddBodyItem(s,"</TABLE>\n");

	    htrAddBodyItem_va(s,"<DIV ID=\"wn%dmain\"><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"%d\" height=\"%d\"><tr %s><td>&nbsp;</td></tr></table>\n",id,bw,bh,bgnd);
	    }


	/** Check for more sub-widgets within the page. **/
	if(s->Capabilities.Dom1HTML)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"%s.ContentLayer",name);
	    }
	else if(s->Capabilities.Dom0NS)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"%s.ContentLayer.document",name);
	    }
	snprintf(sbuf2,HT_SBUF_SIZE,"%s.ContentLayer",name);

	if(s->Capabilities.Dom1HTML)
	    {
	    snprintf(sbuf3,HT_SBUF_SIZE,"%s.mainlayer",name);
	    }
	else
	    {
	    snprintf(sbuf3,HT_SBUF_SIZE,"%s.mainlayer.document",name);
	    }
	snprintf(sbuf4,HT_SBUF_SIZE,"%s.mainlayer",name);
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/connector"))
		{
		htrRenderWidget(s, sub_tree, z+2, sbuf3, sbuf4);
		}
	    else
		{
		htrRenderWidget(s, sub_tree, z+2, sbuf, sbuf2);
		}
	    }

	htrAddBodyItem(s,"</DIV></DIV>\n");

	if(visible==1)
	    htrAddScriptInit(s,
		    "    for (var t in window_current.osrc)\n"
		    "        setTimeout(window_current.osrc[t].osrcname+'.InitQuery();',1);\n"
		    );
	htrAddScriptInit(s,"    window_current = window_current.oldwin;\n");

    return 0;
    }


/*** htwinInitialize - register with the ht_render module.
 ***/
int
htwinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML 'Window' Widget Driver");
	strcpy(drv->WidgetName,"htmlwindow");
	drv->Render = htwinRender;
	drv->Verify = htwinVerify;

	/** Add the 'click' event **/
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Add the 'set visibility' action **/
	htrAddAction(drv,"ToggleVisibility");
	htrAddAction(drv,"SetVisibility");
	htrAddParam(drv,"SetVisibility","IsVisible",DATA_T_INTEGER);
	htrAddParam(drv,"SetVisibility","NoInit",DATA_T_INTEGER);

	/** Add the 'window closed' event **/
	htrAddEvent(drv,"Close");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTWIN.idcnt = 0;

    return 0;
    }
