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

    $Id: htdrv_window.c,v 1.1 2001/08/13 18:00:52 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_window.c,v $

    $Log: htdrv_window.c,v $
    Revision 1.1  2001/08/13 18:00:52  gbeeley
    Initial revision

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
    char sbuf[256];
    char sbuf2[256];
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
	sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#wn%dbase { POSITION:absolute; VISIBILITY:%s; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; clip:rect(%d,%d); Z-INDEX:%d; }\n",
		id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#wn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; clip:rect(%d,%d); Z-INDEX:%d; }\n",
		id, bx, by, bw, bh, bw, bh, z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"    </STYLE>\n");
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

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "wn_init", "\n"
		"function wn_init(l,ml)\n"
		"    {\n"
		"    l.document.layer = l;\n"
		"    ml.document.Layer = ml;\n"
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
		"    if (wn_current != null) wn_current.moveTo((wn_newx<0)?0:wn_newx,(wn_newy<0)?0:wn_newy);\n"
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
		"    return true;\n"
		"    }\n", 0);

	/** Function to bring a window to the 'top' **/
	htrAddScriptFunction(s, "wn_bring_top", "\n"
		"function wn_bring_top(l)\n"
		"    {\n"
		"    if (wn_topwin == l) return true;\n"
		"    wn_adjust_z(l, wn_top_z - l.zIndex + 4);\n"
		"    wn_topwin = l;\n"
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
		"            }\n"
		"        }\n");

	/** Mouse up event handler -- when user releases the button **/
	htrAddEventHandler(s, "document","MOUSEUP","wn",
		"    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')\n"
		"        {\n"
		"        e.target.src = '/sys/images/01close.gif';\n"
		"        e.target.layer.visibility = 'hidden';\n"
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
	sprintf(sbuf,"    %s = wn_init(%s.layers.wn%dbase,%s.layers.wn%dbase.document.layers.wn%dmain);\n", 
		name,parentname,id,parentname,id,id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> elements for the layers. **/
	/** This is the top white edge of the window **/
	sprintf(sbuf,"<DIV ID=\"wn%dbase\"><TABLE %s border=0 cellspacing=0 cellpadding=0>\n",id,bgnd);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",is_dialog_style?(tbw):(tbw-2));
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD></TR>\n");
	htrAddBodyItem(s, sbuf);
	
	/** Titlebar for window, if specified. **/
	if (has_titlebar)
	    {
	    sprintf(sbuf,"<TR><TD width=1><IMG SRC=/sys/images/white_1x1.png width=1 height=22></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD width=%d %s colspan=%d><IMG SRC=/sys/images/01close.gif name=close align=left><FONT COLOR='%s'> %s</FONT></TD>\n",
	    	tbw,hdr_bgnd,is_dialog_style?1:3,txtcolor,title);
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD width=1><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=22></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }

	/** This is the beveled-down edge below the top of the window **/
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD colspan=2><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD>\n",w-3);
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	else
	    {
	    if (has_titlebar)
	        {
	        sprintf(sbuf,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	        htrAddBodyItem(s, sbuf);
	        sprintf(sbuf,"    <TD colspan=2><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD></TR>\n",w-1);
	        htrAddBodyItem(s, sbuf);
	        sprintf(sbuf,"<TR><TD colspan=2><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",w-1);
	        htrAddBodyItem(s, sbuf);
	        sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	        htrAddBodyItem(s, sbuf);
		}
	    }

	/** This is the left side of the window. **/
	sprintf(sbuf,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=%d></TD>\n",bh);
	htrAddBodyItem(s, sbuf);
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=%d></TD>\n",bh);
	    htrAddBodyItem(s, sbuf);
	    }

	/** Here's where the content goes... **/
	sprintf(sbuf,"    <TD>\n");
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s,"&nbsp;</TD>\n");

	/** Right edge of the window **/
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/white_1x1.png width=1 height=%d></TD>\n",bh);
	    htrAddBodyItem(s, sbuf);
	    }
	sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=%d></TD></TR>\n",bh);
	htrAddBodyItem(s, sbuf);

	/** And... bottom edge of the window. **/
	if (!is_dialog_style)
	    {
	    sprintf(sbuf,"<TR><TD><IMG SRC=/sys/images/white_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD>\n");
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD colspan = 2><IMG SRC=/sys/images/white_1x1.png width=%d height=1></TD>\n",w-3);
	    htrAddBodyItem(s, sbuf);
	    sprintf(sbuf,"    <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=1></TD></TR>\n");
	    htrAddBodyItem(s, sbuf);
	    }
	sprintf(sbuf,"<TR><TD colspan=5><IMG SRC=/sys/images/dkgrey_1x1.png width=%d height=1></TD></TR>\n",w);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s,"</TABLE>\n");

	sprintf(sbuf,"<DIV ID=\"wn%dmain\">\n",id);
	htrAddBodyItem(s, sbuf);

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	sprintf(sbuf,"%s.mainLayer.document",name);
	sprintf(sbuf2,"%s.mainLayer",name);
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
    pHtEventAction event,action;
    pHtParam param;

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
	action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	strcpy(action->Name,"SetVisibility");
	xaInit(&action->Parameters,16);
	param = (pHtParam)nmSysMalloc(sizeof(HtParam));
	strcpy(param->ParamName,"IsVisible");
	param->DataType = DATA_T_INTEGER;
	xaAddItem(&action->Parameters,(void*)param);
	xaAddItem(&drv->Actions,(void*)action);

	/** Add the 'window closed' event **/
	event = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	strcpy(event->Name,"Close");
	xaAddItem(&drv->Events,(void*)event);

	/** Register. **/
	htrRegisterDriver(drv);

	HTWIN.idcnt = 0;

    return 0;
    }
