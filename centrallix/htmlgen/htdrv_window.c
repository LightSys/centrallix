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

    $Id: htdrv_window.c,v 1.10 2002/03/13 19:48:45 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_window.c,v $

    $Log: htdrv_window.c,v $
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
htwinRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[HT_SBUF_SIZE];
    char sbuf2[HT_SBUF_SIZE];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w,h;
    int tbw,tbh,bx,by,bw,bh;
    int id;
    int visible = 1;
    char* nptr;
    char bgnd[128] = "";
    char hdr_bgnd[128] = "";
    char txtcolor[64] = "";
    int has_titlebar = 1;
    char title[128];
    int is_dialog_style = 0;

    	/** Get an id for this. **/
	id = (HTWIN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x = 0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y = 0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) 
	    {
	    mssError(1,"HTWIN","HTMLWindow widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,'\0',63);
	name[63]=0;

	/** Check background color **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(bgnd,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(bgnd,"background='%.110s'",ptr);

	/** Check header background color/image **/
	if (objGetAttrValue(w_obj,"hdr_bgcolor",POD(&ptr)) == 0)
	    sprintf(hdr_bgnd,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"hdr_background",POD(&ptr)) == 0)
	    sprintf(hdr_bgnd,"background='%.110s'",ptr);

	/** Check title text color. **/
	if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
	    sprintf(txtcolor,"%.63s",ptr);
	else
	    strcpy(txtcolor,"black");

	/** Check window title. **/
	if (objGetAttrValue(w_obj,"title",POD(&ptr)) == 0)
	    sprintf(title,"%.127s",ptr);
	else
	    strcpy(title,name);

	/** Marked not visible? **/
	if (objGetAttrValue(w_obj,"visible",POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** No titlebar? **/
	if (objGetAttrValue(w_obj,"titlebar",POD(&ptr)) == 0 && !strcmp(ptr,"no"))
	    has_titlebar = 0;

	/** Dialog or window style? **/
	if (objGetAttrValue(w_obj,"style",POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
	    is_dialog_style = 1;

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

	/** Ok, write the style header items. **/
	snprintf(sbuf,HT_SBUF_SIZE,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"\t#wn%dbase { POSITION:absolute; VISIBILITY:%s; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; clip:rect(%d,%d); Z-INDEX:%d; }\n",
		id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"\t#wn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; clip:rect(%d,%d); Z-INDEX:%d; }\n",
		id, bx, by, bw, bh, bw, bh, z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

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

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "wn_init", "\n"
		"function wn_init(l,ml,h)\n"
		"    {\n"
		"    l.document.layer = l;\n"
		"    ml.document.Layer = ml;\n"
		"    l.orig_height = h;\n"
		"    l.mainLayer = ml;\n"
		"    l.kind = 'wn';\n"
		"    for(i=0;i<l.document.images.length;i++)\n"
		"        {\n"
		"        l.document.images[i].layer = l;\n"
		"        l.document.images[i].kind = 'wn';\n"
		"        }\n"
		"    wn_bring_top(l);\n"
		"    l.ActionSetVisibility = wn_setvisibility;\n"
		"    return l;\n"
		"    }\n",0);

	/* Action handlers for WindowShade
	**   * added by Luke Ehresman (lehresma@css.tayloru.edu)
	**   * October 7, 2001
	**/
	htrAddScriptFunction(s, "wn_unset_windowshade", "\n"
		"function wn_unset_windowshade()\n"
		"    {\n"
		"    wn_clicked = 0;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "wn_windowshade", "\n"
		"function wn_windowshade(layer)\n"
		"    {\n"
		"    if (wn_clicked == 1)\n"
		"        {\n"
		"        wn_clicked = 0;\n"
		"        if (layer.clip.height != 23)\n"
		"            layer.clip.height = 23;\n"
		"        else\n"
		"            layer.clip.height = layer.orig_height;\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        wn_clicked = 1;\n"
		"        setTimeout(\"wn_unset_windowshade()\", 1200);\n" // 2 second delay
		"        }\n"
		"    }\n", 0);

	/** Action handler for SetVisibility **/
	htrAddScriptFunction(s, "wn_setvisibility", "\n"
		"function wn_setvisibility(aparam)\n"
		"    {\n"
		"    if (aparam.IsVisible == null || aparam.IsVisible == 1 || aparam.IsVisible == '1')\n"
		"        {\n"
		"        wn_bring_top(this);\n"
		"        this.visibility = 'inherit';\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        this.visibility = 'hidden';\n"
		"        }\n"
		"    }\n", 0);

	/** Function for handling delayed-move event. **/
	htrAddScriptFunction(s, "wn_domove", "\n"
		"function wn_domove()\n"
		"    {\n"
		"    if (wn_current != null) wn_current.moveToAbsolute((wn_newx<0)?0:wn_newx,(wn_newy<0)?0:wn_newy);\n"
		"    wn_clicked = 0;\n"
		"    return true;\n"
		"    }\n",0);

	/** Function to adjust the Z of a window and its contents. **/
	htrAddScriptFunction(s, "wn_adjust_z", "\n"
		"function wn_adjust_z(l,zi)\n"
		"    {\n"
		"    if (zi < 0) l.zIndex += zi;\n"
		"    for(i=0;i<l.document.layers.length;i++)\n"
		"        {\n"
		"        //wn_adjust_z(l.document.layers[i],zi);\n"
		"        }\n"
		"    if (zi > 0) l.zIndex += zi;\n"
		"    if (l.zIndex > wn_top_z) wn_top_z = l.zIndex;\n"
		"    wn_clicked = 0;\n"
		"    return true;\n"
		"    }\n", 0);

	/** Function to bring a window to the 'top' **/
	htrAddScriptFunction(s, "wn_bring_top", "\n"
		"function wn_bring_top(l)\n"
		"    {\n"
		"    if (wn_topwin == l) return true;\n"
		"    wn_adjust_z(l, wn_top_z - l.zIndex + 4);\n"
		"    wn_topwin = l;\n"
		"    wn_clicked = 0;\n"
		"    }\n", 0);

	/** Event handler for mousedown -- initial click **/
	htrAddEventHandler(s, "document","MOUSEDOWN","wn",
		"    if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"    else ly = e.target;\n"
		"    if (ly.kind == 'wn')\n"
		"        {\n"
		"        if (e.target.name == 'close') e.target.src = '/sys/images/02close.gif';\n"
		"        else if (e.pageY < ly.pageY + 20)\n"
		"            {\n"
		"            wn_current = ly;\n"
		"            wn_msx = e.pageX;\n"
		"            wn_msy = e.pageY;\n"
		"            wn_newx = null;\n"
		"            wn_newy = null;\n"
		"            wn_moved = 0;\n"
		"            wn_windowshade(e.target.layer);\n"
		"            }\n"
		"        }\n");

	/** Mouse up event handler -- when user releases the button **/
	htrAddEventHandler(s, "document","MOUSEUP","wn",
		"    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')\n"
		"        {\n"
		"        e.target.src = '/sys/images/01close.gif';\n"
		"        e.target.layer.visibility = 'hidden';\n"
		"        }\n"
		"    else if (ly.document != null && ly.document.images.length > 6 && ly.document.images[6].name == 'close')\n"
		"        {\n"
		"        ly.document.images[6].src = '/sys/images/01close.gif';\n"
		"        }\n"
		"    if (wn_current != null)\n"
		"        {\n"
		"        if (wn_moved == 0) wn_bring_top(wn_current);\n"
		"        }\n"
		"    wn_current = null;\n");

	/** Mouse move event handler -- when user drags the window **/
	htrAddEventHandler(s, "document","MOUSEMOVE","wn",
		"    if (wn_current != null)\n"
		"        {\n"
		"        wn_clicked = 0;\n"
		"        clearTimeout();\n"
		"        if (wn_newx == null)\n"
		"            {\n"
		"            wn_newx = wn_current.pageX + e.pageX-wn_msx;\n"
		"            wn_newy = wn_current.pageY + e.pageY-wn_msy;\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            wn_newx += (e.pageX - wn_msx);\n"
		"            wn_newy += (e.pageY - wn_msy);\n"
		"            }\n"
		"        setTimeout(wn_domove,60);\n"
		"        wn_moved = 1;\n"
		"        wn_msx = e.pageX;\n"
		"        wn_msy = e.pageY;\n"
		"        return false;\n"
		"        }\n");

	/** Script initialization call. **/
	snprintf(sbuf,HT_SBUF_SIZE,"    %s = wn_init(%s.layers.wn%dbase,%s.layers.wn%dbase.document.layers.wn%dmain, %d);\n", 
		name,parentname,id,parentname,id,id,h);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> elements for the layers. **/
	/** This is the top white edge of the window **/
	snprintf(sbuf,HT_SBUF_SIZE,"<DIV ID=\"wn%dbase\"><TABLE %s border=0 cellspacing=0 cellpadding=0>\n",id,bgnd);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",is_dialog_style?(tbw):(tbw-2));
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD></TR>\n");
	htrAddBodyItem(s, sbuf);
	
	/** Titlebar for window, if specified. **/
	if (has_titlebar)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD width=1><IMG SRC=/sys/images/white_1x1.png width=1 height=22></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD width=%d %s colspan=%d><IMG SRC=/sys/images/01close.gif name=close align=left><FONT COLOR='%s'> %s</FONT></TD>\n",
	    	tbw,hdr_bgnd,is_dialog_style?1:3,txtcolor,title);
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD width=1><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=22></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }

	/** This is the beveled-down edge below the top of the window **/
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD colspan=2><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD>\n",w-3);
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	else
	    {
	    if (has_titlebar)
	        {
	        snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	        htrAddBodyItem(s, sbuf);
	        snprintf(sbuf,HT_SBUF_SIZE,"    <TD colspan=2><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD></TR>\n",w-1);
	        htrAddBodyItem(s, sbuf);
	        snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD colspan=2><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",w-1);
	        htrAddBodyItem(s, sbuf);
	        snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	        htrAddBodyItem(s, sbuf);
		}
	    }

	/** This is the left side of the window. **/
	snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=%d></TD>\n",bh);
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=%d></TD>\n",bh);
	    htrAddBodyItem(s, sbuf);
	    }

	/** Here's where the content goes... **/
	snprintf(sbuf,HT_SBUF_SIZE,"    <TD>\n");
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s,"&nbsp;</TD>\n");

	/** Right edge of the window **/
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=%d></TD>\n",bh);
	    htrAddBodyItem(s, sbuf);
	    }
	snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=%d></TD></TR>\n",bh);
	htrAddBodyItem(s, sbuf);

	/** And... bottom edge of the window. **/
	if (!is_dialog_style)
	    {
	    snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD colspan = 2><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",w-3);
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,HT_SBUF_SIZE,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	snprintf(sbuf,HT_SBUF_SIZE,"<TR><TD colspan=5><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD></TR>\n",w);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s,"</TABLE>\n");

	snprintf(sbuf,HT_SBUF_SIZE,"<DIV ID=\"wn%dmain\">\n",id);
	htrAddBodyItem(s, sbuf);


	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	snprintf(sbuf,HT_SBUF_SIZE,"%s.mainLayer.document",name);
	snprintf(sbuf2,HT_SBUF_SIZE,"%s.mainLayer",name);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+2, sbuf, sbuf2);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	htrAddBodyItem(s,"</DIV></DIV>\n");

    return 0;
    }


/*** htwinInitialize - register with the ht_render module.
 ***/
int
htwinInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML 'Window' Widget Driver");
	strcpy(drv->WidgetName,"htmlwindow");
	drv->Render = htwinRender;
	drv->Verify = htwinVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Add the 'set visibility' action **/
	htrAddAction(drv,"SetVisibility");
	htrAddParam(drv,"SetVisibility","IsVisible",DATA_T_INTEGER);

	/** Add the 'window closed' event **/
	htrAddEvent(drv,"Close");

	/** Register. **/
	htrRegisterDriver(drv);

	HTWIN.idcnt = 0;

    return 0;
    }
