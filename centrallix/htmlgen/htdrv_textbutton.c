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

    $Id: htdrv_textbutton.c,v 1.6 2002/03/16 05:12:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textbutton.c,v $

    $Log: htdrv_textbutton.c,v $
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
    char sbuf[320];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w,h;
    int id;
    int is_ts = 1;
    char* nptr;

    	/** Get an id for this. **/
	id = (HTTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) h = -1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Threestate button or twostate? **/
	if (objGetAttrValue(w_obj,"tristate",POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	if (objGetAttrValue(w_obj,"text",POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	memccpy(text,ptr,'\0',63);
	text[63]=0;

	/** Get fgnd colors 1,2, and background color **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(bgcolor,"bgcolor=%.100s",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(bgcolor,"background='%.90s'",ptr);
	else
	    strcpy(bgcolor,"");
	if (objGetAttrValue(w_obj,"fgcolor1",POD(&ptr)) == 0)
	    sprintf(fgcolor1,"%.63s",ptr);
	else
	    strcpy(fgcolor1,"white");
	if (objGetAttrValue(w_obj,"fgcolor2",POD(&ptr)) == 0)
	    sprintf(fgcolor2,"%.63s",ptr);
	else
	    strcpy(fgcolor2,"black");

	/** Ok, write the style header items. **/
	snprintf(sbuf,320,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%dpane2 { POSITION:absolute; VISIBILITY:inherit; LEFT:-1; TOP:-1; WIDTH:%d; Z-INDEX:%d; }\n",id,w-1,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%dtop { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%d; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%dbtm { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%d; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%drgt { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",z+2);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#tb%dlft { POSITION:absolute; VISIBILITY:%s; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%d; }\n",id,is_ts?"hidden":"inherit",z+2);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "tb_init", "\n"
		"function tb_init(l,l2,top,btm,rgt,lft,w,h,p,ts,nm)\n"
		"    {\n"
		"    l.LSParent = p;\n"
	     	"    l.nofocus = true;\n"
		"    l2.nofocus = true;\n"
		"    top.nofocus = true;\n"
		"    rgt.nofocus = true;\n"
		"    btm.nofocus = true;\n"
		"    lft.nofocus = true;\n"
		"    l.document.kind = 'tb';\n"
		"    l2.document.kind = 'tb';\n"
		"    l.document.layer = l;\n"
		"    l2.document.layer = l;\n"
		"    l.buttonName = nm;\n"
	        "    l.l2 = l2;\n"
		"    l.tp = top;\n"
		"    l.btm = btm;\n"
		"    l.lft = lft;\n"
		"    l.rgt = rgt;\n"
		"    l.kind = 'tb';\n"
		"    l.clip.width = w;\n"
		"    if (h != -1) l.clip.height = h;\n"
		"    top.bgColor = '#FFFFFF';\n"
		"    lft.bgColor = '#FFFFFF';\n"
		"    btm.bgColor = '#7A7A7A';\n"
		"    rgt.bgColor = '#7A7A7A';\n"
		"    lft.clip.height = l.clip.height;\n"
		"    rgt.clip.height = l.clip.height;\n"
		"    rgt.pageX = l.pageX + l.clip.width - 2;\n"
		"    btm.pageY = l.pageY + l.clip.height - 2;\n"
		"    l.mode = 0;\n"
		"    l.tristate = ts;\n"
		"    }\n" ,0);

	/** Script initialization call. **/
	snprintf(sbuf,320,"    %s = %s.layers.tb%dpane;\n",nptr, parentname, id);
	htrAddScriptInit(s, sbuf);
	snprintf(sbuf,320,"    tb_init(%s,%s.document.layers.tb%dpane2,%s.document.layers.tb%dtop,%s.document.layers.tb%dbtm,%s.document.layers.tb%drgt,%s.document.layers.tb%dlft,%d,%d,%s,%d,\"%s\");\n",
		nptr, nptr, id, nptr, id, nptr, id, nptr, id, nptr, id, w, h, parentobj,is_ts, nptr);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> elements for the layers. **/
	if (h != -1)
	    {
	    snprintf(sbuf,320,"<DIV ID=\"tb%dpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %s width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%d></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,320,"<DIV ID=\"tb%dpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%d></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
	    htrAddBodyItem(s, sbuf);
	    }
	else
	    {
	    snprintf(sbuf,320,"<DIV ID=\"tb%dpane\"><TABLE border=0 cellspacing=0 cellpadding=3 %s width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text);
	    htrAddBodyItem(s, sbuf);
	    snprintf(sbuf,320,"<DIV ID=\"tb%dpane2\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%d><TR><TD align=center valign=middle><FONT COLOR='%s'><B>%s</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text);
	    htrAddBodyItem(s, sbuf);
	    }
	snprintf(sbuf,320,"<DIV ID=\"tb%dtop\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%d></DIV>\n",id,w);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,320,"<DIV ID=\"tb%dbtm\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%d></DIV>\n",id,w);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,320,"<DIV ID=\"tb%drgt\"><IMG SRC=/sys/images/trans_1.gif height=%d width=1></DIV>\n",id,(h==-1)?1:h);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,320,"<DIV ID=\"tb%dlft\"><IMG SRC=/sys/images/trans_1.gif height=%d width=1></DIV>\n",id,(h==-1)?1:h);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,320,"</DIV>\n");
	htrAddBodyItem(s, sbuf);

	/** Add a function to be used to change the state (mode) of the button **/
	htrAddScriptFunction(s, "tb_setmode", "\n"
		"function tb_setmode(layer,mode)\n"
		"    {\n"
		"    if (mode != layer.mode)\n"
		"        {\n"
		"        layer.mode = mode;\n"
		"        if (layer.tristate == 0 && mode == 0) mode = 1;\n"
		"        switch(mode)\n"
		"            {\n"
		"            case 0: /* no point no click */\n"
		"                layer.rgt.visibility = 'hidden';\n"
		"                layer.lft.visibility = 'hidden';\n"
		"                layer.tp.visibility = 'hidden';\n"
		"                layer.btm.visibility = 'hidden';\n"
		"                break;\n"
		"            case 1: /* point, but no click */\n"
		"                layer.rgt.visibility = 'inherit';\n"
		"                layer.lft.visibility = 'inherit';\n"
		"                layer.tp.visibility = 'inherit';\n"
		"                layer.btm.visibility = 'inherit';\n"
		"                layer.tp.bgColor = '#FFFFFF';\n"
		"                layer.lft.bgColor = '#FFFFFF';\n"
		"                layer.btm.bgColor = '#7A7A7A';\n"
		"                layer.rgt.bgColor = '#7A7A7A';\n"
		"                break;\n"
		"            case 2: /* point and click */\n"
		"                layer.rgt.visibility = 'inherit';\n"
		"                layer.lft.visibility = 'inherit';\n"
		"                layer.tp.visibility = 'inherit';\n"
		"                layer.btm.visibility = 'inherit';\n"
		"                layer.tp.bgColor = '#7A7A7A';\n"
		"                layer.lft.bgColor = '#7A7A7A';\n"
		"                layer.btm.bgColor = '#FFFFFF';\n"
		"                layer.rgt.bgColor = '#FFFFFF';\n"
		"                break;\n"
		"            }\n"
		"        }\n"
		"    }\n", 0);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","tb",
		"    //alert(show_obj(e.target));\n"
		"    if (e.target != null && e.target.kind=='tb')\n"
		"        {\n"
		"        if (e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        ly.moveBy(1,1);\n"
		"        tb_setmode(ly,2);\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","tb",
		"    if (e.target != null && e.target.kind == 'tb')\n"
		"        {\n"
		"        if (e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        ly.moveBy(-1,-1);\n"
		"        if (e.pageX >= ly.pageX &&\n"
		"            e.pageX < ly.pageX + ly.clip.width &&\n"
		"            e.pageY >= ly.pageY &&\n"
		"            e.pageY < ly.pageY + ly.clip.height)\n"
		"            {\n"
		"            tb_setmode(ly,1);\n"
		"            if (ly.EventClick != null)\n"
		"                {\n"
		"                eparam = new Object();\n"
		"                eparam.Caller = ly;\n"
		"                cn_activate(ly, 'Click', eparam);\n"
		"                delete eparam;\n"
		"                }\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            tb_setmode(ly,0);\n"
		"            }\n"
		"        }\n" );

	htrAddEventHandler(s, "document","MOUSEOVER","tb",
		"    if (e.target != null && e.target.kind == 'tb')\n"
		"        {\n"
		"        if (e.target.mode != 2) tb_setmode(e.target,1);\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","tb",
		"    if (e.target != null && e.target.kind == 'tb')\n"
		"        {\n"
		"        if (e.target.mode != 2) tb_setmode(e.target,0);\n"
		"        }\n");

	/** Check for more sub-widgets within the textbutton. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+3, parentname, nptr);
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
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Text Button Widget Driver");
	strcpy(drv->WidgetName,"textbutton");
	drv->Render = httbtnRender;
	drv->Verify = httbtnVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");

	/** Register. **/
	htrRegisterDriver(drv);

	HTTBTN.idcnt = 0;

    return 0;
    }
