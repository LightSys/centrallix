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

    $Id: htdrv_textbutton.c,v 1.23 2003/11/18 05:58:00 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textbutton.c,v $

    $Log: htdrv_textbutton.c,v $
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


/*** httbtnVerify - not written yet.
 ***/
int
httbtnVerify()
    {
    return 0;
    }


/*** httbtnRender - generate the HTML code for the page.
 ***/
int
httbtnRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    char bgstyle[128];
    char disable_color[64];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w,h;
    int id;
    int is_ts = 1;
    char* nptr;
    int is_enabled = 1;
    pExpression code;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTBTN","Netscape DOM or (W3C DOM1 HTML and W3C DOM2 CSS) support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (objGetAttrType(w_obj,"enabled") == DATA_T_STRING && objGetAttrValue(w_obj,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** User requesting expression for enabled? **/
	if (objGetAttrType(w_obj,"enabled") == DATA_T_CODE)
	    {
	    objGetAttrValue(w_obj,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Threestate button or twostate? **/
	if (objGetAttrValue(w_obj,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	if (objGetAttrValue(w_obj,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	memccpy(text,ptr,'\0',63);
	text[63]=0;

	/** Get fgnd colors 1,2, and background color **/
	if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(bgcolor,"bgcolor=%.100s",ptr);
	    sprintf(bgstyle,"background-color: %.90s;",ptr);
	    }
	else if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    sprintf(bgcolor,"background='%.90s'",ptr);
	    sprintf(bgstyle,"background-image: url('%.90s');",ptr);
	    }
	else
	    {
	    strcpy(bgcolor,"");
	    strcpy(bgstyle,"");
	    }

	if (objGetAttrValue(w_obj,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(fgcolor1,"%.63s",ptr);
	else
	    strcpy(fgcolor1,"white");
	if (objGetAttrValue(w_obj,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(fgcolor2,"%.63s",ptr);
	else
	    strcpy(fgcolor2,"black");
	if (objGetAttrValue(w_obj,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(disable_color,"%.63s",ptr);
	else
	    strcpy(disable_color,"#808080");

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0);

	if(s->Capabilities.Dom0NS)
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
	    htrAddScriptInit_va(s, "    %s = %s.layers.tb%dpane;\n",nptr, parentname, id);
	    htrAddScriptInit_va(s, "    tb_init(%s,%s.document.layers.tb%dpane2,%s.document.layers.tb%dpane3,%s.document.layers.tb%dtop,%s.document.layers.tb%dbtm,%s.document.layers.tb%drgt,%s.document.layers.tb%dlft,%d,%d,%s,%d,\"%s\");\n",
		    nptr, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, w, h, parentobj,is_ts, nptr);

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
		"        ly.moveBy(1,1);\n"
		"        tb_setmode(ly,2);\n"
		"        cn_activate(ly, 'MouseDown');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","tb",
		"    if (ly.kind == 'tb' && ly.enabled)\n"
		"        {\n"
		"        ly.moveBy(-1,-1);\n"
		"        //alert(e.pageX + ' -- ' + ly.pageX + ' -- ' + ly.clip.width);\n"
		"        if (e.pageX >= ly.pageX &&\n"
		"            e.pageX < ly.pageX + ly.clip.width &&\n"
		"            e.pageY >= ly.pageY &&\n"
		"            e.pageY < ly.pageY + ly.clip.height)\n"
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
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		if(htrRenderWidget(s, sub_w_obj, z+3, parentname, nptr)<0)
		    {
		    mssError(0,"HTTBTN","Unable to render child");
		    }

		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

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
	drv->Verify = httbtnVerify;

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
