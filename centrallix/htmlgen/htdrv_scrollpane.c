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

    $Id: htdrv_scrollpane.c,v 1.4 2002/06/09 23:44:46 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_scrollpane.c,v $

    $Log: htdrv_scrollpane.c,v $
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


/*** htspaneVerify - not written yet.
 ***/
int
htspaneVerify()
    {
    return 0;
    }


/*** htspaneRender - generate the HTML code for the page.
 ***/
int
htspaneRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w,h;
    int id;
    int visible = 1;
    char* nptr;
    char bcolor[64] = "";
    char bimage[64] = "";

    	/** Get an id for this. **/
	id = (HTSPANE.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) 
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTSPANE","ScrollPane widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,'\0',63);
	name[63]=0;

	/** Check background color **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    {
	    memccpy(bcolor,ptr,'\0',63);
	    bcolor[63]=0;
	    }
	if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    {
	    memccpy(bimage,ptr,'\0',63);
	    bimage[63]=0;
	    }

	/** Marked not visible? **/
	if (objGetAttrValue(w_obj,"visible",POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"false")) visible = 0;
	    }

	/** Ok, write the style header items. **/
	snprintf(sbuf,160,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"\t#sp%dpane { POSITION:absolute; VISIBILITY:%s; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; clip:rect(%d,%d); Z-INDEX:%d; }\n",id,visible?"inherit":"hidden",x,y,w,h,w,h, z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"\t#sp%darea { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:0; WIDTH:%d; Z-INDEX:%d; }\n",id,w-18,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"\t#sp%dthum { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:18; WIDTH:18; Z-INDEX:%d; }\n",id,w-18,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write globals for internal use **/
	htrAddScriptGlobal(s, "sp_target_img", "null", 0);
	htrAddScriptGlobal(s, "sp_click_x","0",0);
	htrAddScriptGlobal(s, "sp_click_y","0",0);
	htrAddScriptGlobal(s, "sp_thum_y","0",0);
	htrAddScriptGlobal(s, "sp_mv_timeout","null",0);
	htrAddScriptGlobal(s, "sp_mv_incr","0",0);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Utility function... **/
	htrAddScriptFunction(s, "subst_last", "\n"
		"function subst_last(str,subst)\n"
		"    {\n"
		"    return str.substring(0,str.length-subst.length)+subst;\n"
		"    }\n", 0);

	/** Add another utility function for helping handle the mousemove event **/
	htrAddScriptFunction(s, "do_mv", "\n"
		"function do_mv()\n"
		"    {\n"
		"    var ti=sp_target_img;\n"
		"    if (ti.kind=='sp' && sp_mv_incr > 0)\n"
		"        {\n"
		"        h=ti.area.clip.height;\n"
		"        d=h-ti.pane.clip.height;\n"
		"        incr=sp_mv_incr;\n"
		"        if(d<0) incr=0; else if (d+ti.area.y<incr) incr=d+ti.area.y;\n"
		"        for(i=0;i<ti.pane.document.layers.length;i++) if (ti.pane.document.layers[i] != ti.thum)\n"
		"            ti.pane.document.layers[i].y-=incr;\n"
		"        v=ti.pane.clip.height-(3*18);\n"
		"        if (d<=0) ti.thum.y=18;\n"
		"        else ti.thum.y=18+v*(-ti.area.y/d);\n"
		"        }\n"
		"    else if (ti.kind=='sp' && sp_mv_incr < 0)\n"
		"        {\n"
		"        h=ti.area.clip.height;\n"
		"        d=h-ti.pane.clip.height;\n"
		"        incr = -sp_mv_incr;\n"
		"        if(d<0)incr=0; else if (ti.area.y>-incr) incr=-ti.area.y;\n"
		"        for(i=0;i<ti.pane.document.layers.length;i++) if (ti.pane.document.layers[i] != ti.thum)\n"
		"            ti.pane.document.layers[i].y+=incr;\n"
		"        v=ti.pane.clip.height-(3*18);\n"
		"        if(d<=0) ti.thum.y=18;\n"
		"        else ti.thum.y=18+v*(-ti.area.y/d);\n"
		"        }\n"
		"    return true;\n"
		"    }\n",0);
	htrAddScriptFunction(s, "tm_mv", "\n"
		"function tm_mv()\n"
		"    {\n"
		"    do_mv();\n"
		"    sp_mv_timeout=setTimeout(tm_mv,50);\n"
		"    return false;\n"
		"    }\n",0);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "sp_init", "\n"
		"function sp_init(l,aname,tname,p)\n"
		"    {\n"
		"    var alayer=null;\n"
		"    var tlayer=null;\n"
		"    for(i=0;i<l.layers.length;i++)\n"
		"        {\n"
		"        ml=l.layers[i];\n"
		"        if(ml.name==aname) alayer=ml;\n"
		"        if(ml.name==tname) tlayer=ml;\n"
		"        }\n"
		"    for(i=0;i<l.document.images.length;i++)\n"
		"        {\n"
		"        img=l.document.images[i];\n"
		"        if(img.name=='d' || img.name=='u' || img.name=='b')\n"
		"            {\n"
		"            img.pane=l;\n"
		"            img.area=alayer;\n"
		"            img.thum=tlayer;\n"
		"            img.kind='sp';\n"
		"            }\n"
		"        }\n"
		"    tlayer.document.images[0].kind='sp';\n"
		"    tlayer.document.images[0].thum=tlayer;\n"
		"    tlayer.document.images[0].area=alayer;\n"
		"    tlayer.document.images[0].pane=l;\n"
		"    alayer.clip.width=l.clip.width-18;\n"
		"    alayer.maxwidth=alayer.clip.width;\n"
		"    alayer.minwidth=alayer.clip.width;\n"
		"    tlayer.nofocus = true;\n"
		"    alayer.nofocus = true;\n"
		"    alayer.document.Layer = alayer;\n"
		"    tlayer.document.Layer = tlayer;\n"
		"    l.document.Layer = l;\n"
		"    l.LSParent = p;\n"
		"    }\n",0);

	/** Script initialization call. **/
	snprintf(sbuf,160,"    sp_init(%s.layers.sp%dpane,\"sp%darea\",\"sp%dthum\",%s);\n", parentname,id,id,id,parentobj);
	htrAddScriptInit(s, sbuf);
	snprintf(sbuf,160,"    %s=%s.layers.sp%dpane;\n",name,parentname,id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> elements for the layers. **/
	snprintf(sbuf,160,"<DIV ID=\"sp%dpane\"><TABLE %s%s %s%s border=0 cellspacing=0 cellpadding=0 width=%d>",id,(*bcolor)?"bgcolor=":"",bcolor, (*bimage)?"background=":"",bimage, w);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "<TR><TD align=right><IMG SRC=/sys/images/ico13b.gif NAME=u></TD></TR><TR><TD align=right>");
	snprintf(sbuf,160,"<IMG SRC=/sys/images/trans_1.gif height=%d width=18 name='b'>",h-36);
	htrAddBodyItem(s,sbuf);
	htrAddBodyItem(s,"</TD></TR><TR><TD align=right><IMG SRC=/sys/images/ico12b.gif NAME=d></TD></TR></TABLE>\n");
	snprintf(sbuf,160,"<DIV ID=\"sp%dthum\"><IMG SRC=/sys/images/ico14b.gif NAME=t></DIV>\n<DIV ID=\"sp%darea\">",id,id);
	htrAddBodyItem(s,sbuf);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","sp",
		"    sp_target_img=e.target;\n"
		"    if (sp_target_img != null && sp_target_img.kind=='sp' && (sp_target_img.name=='u' || sp_target_img.name=='d'))\n"
		"        {\n"
		"        if (sp_target_img.name=='u') sp_mv_incr=-10; else sp_mv_incr=+10;\n"
		"        sp_target_img.src = subst_last(sp_target_img.src,\"c.gif\");\n"
		"        do_mv();\n"
		"        sp_mv_timeout = setTimeout(tm_mv,300);\n"
		"        }\n"
		"    else if (sp_target_img != null && sp_target_img.kind=='sp' && sp_target_img.name=='t')\n"
		"        {\n"
		"        sp_click_x = e.pageX;\n"
		"        sp_click_y = e.pageY;\n"
		"        sp_thum_y = sp_target_img.thum.pageY;\n"
		"        }\n"
		"    else if (sp_target_img != null && sp_target_img.kind=='sp' && sp_target_img.name=='b')\n"
		"        {\n"
		"        sp_mv_incr=sp_target_img.height+36;\n"
		"        if (e.pageY < sp_target_img.thum.pageY+9) sp_mv_incr = -sp_mv_incr;\n"
		"        do_mv();\n"
		"        sp_mv_timeout = setTimeout(tm_mv,300);\n"
		"        }\n"
		"    else sp_target_img = null;\n");
	htrAddEventHandler(s, "document","MOUSEMOVE","sp",
		"    ti=sp_target_img;\n"
		"    if (ti != null && ti.kind=='sp' && ti.name=='t')\n"
		"        {\n"
		"        v=ti.pane.clip.height-(3*18);\n"
		"        new_y=sp_thum_y + (e.pageY-sp_click_y);\n"
		"        if (new_y > ti.pane.pageY+18+v) new_y=ti.pane.pageY+18+v;\n"
		"        if (new_y < ti.pane.pageY+18) new_y=ti.pane.pageY+18;\n"
		"        ti.thum.pageY=new_y;\n"
		"        h=ti.area.clip.height;\n"
		"        d=h-ti.pane.clip.height;\n"
		"        if (d<0) d=0;\n"
		"        yincr = (((ti.thum.y-18)/v)*-d) - ti.area.y;\n"
		"        for(i=0;i<ti.pane.document.layers.length;i++) if (ti.pane.document.layers[i] != ti.thum)\n"
		"            ti.pane.document.layers[i].y+=yincr;\n"
		"        return false;\n"
		"        }\n");
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
		"            sp_target_img.src = subst_last(sp_target_img.src,\"b.gif\");\n"
		"        sp_target_img = null;\n"
		"        }\n");

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	snprintf(sbuf,160,"%s.document.layers.sp%darea.document",name,id);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+2, sbuf, name);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** Finish off the last <DIV> **/
	htrAddBodyItem(s,"</DIV></DIV>\n");

    return 0;
    }


/*** htspaneInitialize - register with the ht_render module.
 ***/
int
htspaneInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ScrollPane Widget Driver");
	strcpy(drv->WidgetName,"scrollpane");
	drv->Render = htspaneRender;
	drv->Verify = htspaneVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTSPANE.idcnt = 0;

    return 0;
    }
