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
/* Module: 	htdrv_textbutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 5, 1998  					*/
/* Description:	HTML Widget driver for a 'text button', which frames a	*/
/*		text string in a 3d-like box that simulates the 3d	*/
/*		clicking action when the user points and clicks.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_textbutton.c,v 1.29 2004/08/04 20:03:11 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textbutton.c,v $

    $Log: htdrv_textbutton.c,v $
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
httbtnRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
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
    char* nptr;
    int is_enabled = 1;
    pExpression code;

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
	memccpy(name,ptr,0,63);
	name[63] = 0;

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
	if (wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	memccpy(text,ptr,'\0',63);
	text[63]=0;

	/** Get fgnd colors 1,2, and background color **/
	if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(bgcolor,"bgcolor=%.100s",ptr);
	    sprintf(bgstyle,"background-color: %.90s;",ptr);
	    }
	else if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(bgcolor,"background='%.90s'",ptr);
	    sprintf(bgstyle,"background-image: url('%.90s');",ptr);
	    }
	else
	    {
	    strcpy(bgcolor,"");
	    strcpy(bgstyle,"");
	    }

	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(fgcolor1,"%.63s",ptr);
	else
	    strcpy(fgcolor1,"white");
	if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(fgcolor2,"%.63s",ptr);
	else
	    strcpy(fgcolor2,"black");
	if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(disable_color,"%.63s",ptr);
	else
	    strcpy(disable_color,"#808080");

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0);

	if(s->Capabilities.Dom0NS || s->Capabilities.Dom0IE)
	    {
	    /** Ok, write the style header items. **/
	    htrAddStylesheetItem_va(s,"\t#tb%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane2 { POSITION:absolute; VISIBILITY:%s; LEFT:-1; TOP:-1; WIDTH:%d; Z-INDEX:%d; }\n",id,is_enabled?"inherit":"hidden",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane3 { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; WIDTH:%d; Z-INDEX:%d; }\n",id,is_enabled?"hidden":"inherit",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%dtop { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%d; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%dbtm { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%d; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%drgt { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%dlft { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",z+2);

	    /** Script initialization call. **/
	    if(s->Capabilities.Dom0NS)
	        {
	    	htrAddScriptInit_va(s, "    %s = %s.layers.tb%dpane;\n",nptr, parentname, id);
	    	htrAddScriptInit_va(s, "    tb_init(%s,%s.document.layers.tb%dpane2,%s.document.layers.tb%dpane3,%s.document.layers.tb%dtop,%s.document.layers.tb%dbtm,%s.document.layers.tb%drgt,%s.document.layers.tb%dlft,%d,%d,%s,%d,\"%s\");\n",
		    nptr, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, w, h, parentobj,is_ts, nptr);
		}
	    else if(s->Capabilities.Dom0IE)
	        {
		if(strstr(parentname,"document")!=NULL)
		    {
		    htrAddScriptInit_va(s, "    %s = %s.getElementById(\"tb%dpane\");\n",nptr, parentname, id);
		      htrAddScriptInit_va(s, "    tb_init(%s,%s.document.getElementById(\"tb%dpane2\"),%s.document.getElementById(\"tb%dpane3\"),%s.document.getElementById(\"tb%dtop\"),%s.document.getElementById(\"tb%dbtm\"),%s.document.getElementById(\"tb%drgt\"),%s.document.getElementById(\"tb%dlft\"),%d,%d,%s,%d,\"%s\");\n",
					  nptr, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, w, h, parentobj,is_ts, nptr);
		    }
		else
		    {
		      htrAddScriptInit_va(s, "    %s = %s.document.getElementById(\"tb%dpane\");\n",nptr, parentname, id);
		      htrAddScriptInit_va(s, "    tb_init(%s,%s.document.getElementById(\"tb%dpane2\"),%s.document.getElementById(\"tb%dpane3\"),%s.document.getElementById(\"tb%dtop\"),%s.document.getElementById(\"tb%dbtm\"),%s.document.getElementById(\"tb%drgt\"),%s.document.getElementById(\"tb%dlft\"),%d,%d,%s,%d,\"%s\");\n",
					  nptr, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, w, h, parentobj,is_ts, nptr);
		    }
	        }

	    /** HTML body <DIV> elements for the layers. **/
	    if (h >= 0)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %s width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%d></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%dpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%d></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%dpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%d></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,h);
		}
	    else
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane\"><TABLE border=0 cellspacing=0 cellpadding=3 %s width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane2\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane3\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text);
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dtop\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%d></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dbtm\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%d></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%drgt\"><IMG SRC=/sys/images/trans_1.gif height=%d width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dlft\"><IMG SRC=/sys/images/trans_1.gif height=%d width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem_va(s,"</DIV>\n");
	    }
	else if(s->Capabilities.Dom2CSS)
	    {
	    htrAddStylesheetItem_va(s,"\t#tb%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n",id,x,y,w,z);
	    if(h >=0 )
		{
		htrAddStylesheetItem_va(s,"\t#tb%dpane, #tb%dpane2, #tb%dpane3 { height: %dpx;}\n",id,id,id,h);
		}
	    htrAddStylesheetItem_va(s,"\t#tb%dpane, #tb%dpane2, #tb%dpane3 { font-weight: 600; text-align: center; }\n",id,id,id);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane { %s border-width: 1px; border-style: solid; border-color: white gray gray white; }\n",id,bgstyle);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane { color: %s; }\n",id,fgcolor2);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane2 { color: %s; VISIBILITY: %s; Z-INDEX: %d; position: absolute; left:-1px; top: -1px; width:%dpx; }\n",id,fgcolor1,is_enabled?"inherit":"hidden",z+1,w-1);
	    htrAddStylesheetItem_va(s,"\t#tb%dpane3 { color: %s; VISIBILITY: %s; Z-INDEX: %d; position: absolute; left:0px; top: 0px; width:%dpx; }\n",id,disable_color,is_enabled?"hidden":"inherit",z+1,w-1);

	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane\"><center><table><tr><td height=%d valign=middle align=center>%s</td></tr></table></center>\n",id,h,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane2\"><center><table><tr><td height=%d valign=middle align=center>%s</td></tr></table></center></DIV>\n",id,h,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%dpane3\"><center><table><tr><td height=%d valign=middle align=center>%s</td></tr></table></center></DIV>\n",id,h,text);
	    htrAddBodyItem_va(s,"</DIV>");

	    /** Script initialization call. **/
	    htrAddScriptInit_va(s, "    %s = document.getElementById('tb%dpane');\n",nptr, id);
	    htrAddScriptInit_va(s, "    tb_init(%s,document.getElementById('tb%dpane2'),document.getElementById('tb%dpane3'),null,null,null,null,%d,%d,%s,%d,\"%s\");\n",
		    nptr, id, id, w, h, parentobj,is_ts, nptr);

	    }
	else
	    {
	    mssError(0,"HTTBTN","Unable to render for this browser");
	    return -1;
	    }

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        moveBy(ly,1,1);\n"
		"        tb_setmode(ly,2);\n"
		"        cn_activate(ly, 'MouseDown');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        moveBy(ly,-1,-1);\n"
		"        if (e.pageX >= getPageX(ly) &&\n"
		"            e.pageX < getPageX(ly) + getClipWidth(ly) &&\n"
		"            e.pageY >= getPageY(ly) &&\n"
		"            e.pageY < getPageY(ly) + getClipHeight(ly))\n"
		"            {\n"
		"            tb_setmode(ly,1);\n"
		"            cn_activate(ly, 'Click');\n"
		"            cn_activate(ly, 'MouseUp');\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            tb_setmode(ly,0);\n"
		"            }\n"
		"        }\n" );

	htrAddEventHandler(s, "document","MOUSEOVER","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        if(cx__capabilities.Dom2CSS)\n"
		"            {\n"
		"            if (ly.mode != 2) tb_setmode(ly,1);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            if (e.target.mode != 2) tb_setmode(e.target,1);\n"
		"            }\n"
		"        cn_activate(ly, 'MouseOver');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        if(cx__capabilities.Dom2CSS)\n"
		"            {\n"
		"            if (ly.mode != 2) tb_setmode(ly,0);\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            if (e.target.mode != 2) tb_setmode(e.target,0);\n"
		"            }\n"
		"        cn_activate(ly, 'MouseOut');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEMOVE","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        cn_activate(ly, 'MouseMove');\n"
		"        }\n");




	/** Check for more sub-widgets within the textbutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3, parentname, nptr);


    return 0;
    }


/*** httbtnInitialize - register with the ht_render module.
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
