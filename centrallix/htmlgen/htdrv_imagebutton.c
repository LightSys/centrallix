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
/* Module: 	htdrv_imagebutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 4, 1998  					*/
/* Description:	HTML Widget driver for an 'image button', or a button	*/
/*		comprised of a set of three images - one the default,	*/
/*		second the image when pointed to, and third the image	*/
/*		when clicked.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_imagebutton.c,v 1.7 2002/05/02 01:12:43 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_imagebutton.c,v $

    $Log: htdrv_imagebutton.c,v $
    Revision 1.7  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.6  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.5  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.4  2002/03/16 05:12:02  gbeeley
    Added the buttonName javascript property for imagebuttons and text-
    buttons.  Allows them to be identified more easily via javascript.

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
    HTIBTN;


/*** htibtnVerify - not written yet.
 ***/
int
htibtnVerify()
    {
    return 0;
    }


/*** htibtnRender - generate the HTML code for the page.
 ***/
int
htibtnRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char n_img[128];
    char p_img[128];
    char c_img[128];
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w,h;
    int id;
    char* nptr;

    	/** Get an id for this. **/
	id = (HTIBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'x' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'y' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0)
	    {
	    mssError(1,"HTIBTN","ImageButton must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) h = -1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get normal, point, and click images **/
	if (objGetAttrValue(w_obj,"image",POD(&ptr)) != 0) 
	    {
	    mssError(1,"HTIBTN","ImageButton must have an 'image' property");
	    return -1;
	    }
	memccpy(n_img,ptr,'\0',127);
	n_img[127]=0;
	if (objGetAttrValue(w_obj,"pointimage",POD(&ptr)) == 0)
	    {
	    memccpy(p_img,ptr,'\0',127);
	    p_img[127]=0;
	    }
	else
	    {
	    strcpy(p_img, n_img);
	    }
	if (objGetAttrValue(w_obj,"clickimage",POD(&ptr)) == 0)
	    {
	    memccpy(c_img,ptr,'\0',127);
	    c_img[127]=0;
	    }
	else
	    {
	    strcpy(c_img, p_img);
	    }

	/** Ok, write the style header items. **/
	snprintf(sbuf,160,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"\t#ib%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,160,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "ib_init", "\n"
		"function ib_init(l,n,p,c,w,h,po,nm)\n"
		"    {\n"
		"    l.LSParent = po;\n"
	     	"    l.nofocus = true;\n" 
		"    l.img = l.document.images[0];\n"
		"    l.img.layer = l;\n"
		"    l.img.kind = 'ib';\n"
		"    l.kind = 'ib';\n"
		"    l.clip.width = w;\n"
		"    l.buttonName = nm;\n"
		"    if (h == -1) l.nImage = new Image();\n"
		"    else l.nImage = new Image(w,h);\n"
		"    l.nImage.src = n;\n"
		"    if (h == -1) l.pImage = new Image();\n"
		"    else l.pImage = new Image(w,h);\n"
		"    l.pImage.src = p;\n"
		"    if (h == -1) l.cImage = new Image();\n"
		"    else l.cImage = new Image(w,h);\n"
		"    l.cImage.src = c;\n"
		"    }\n" ,0);

	/** Script initialization call. **/
	snprintf(sbuf,160,"    %s = %s.layers.ib%dpane;\n",nptr, parentname, id);
	htrAddScriptInit(s, sbuf);
	snprintf(sbuf,160,"    ib_init(%s,'%s','%s','%s',%d,%d,%s,'%s');\n",
		nptr, n_img, p_img, c_img, w, h, parentobj,nptr);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> elements for the layers. **/
	if (h == -1)
	    snprintf(sbuf,160,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0></DIV>\n",id,n_img);
	else
	    snprintf(sbuf,160,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0 width=%d height=%d></DIV>\n",id,n_img,w,h);
	htrAddBodyItem(s, sbuf);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","ib",
		"    if (e.target != null && e.target.kind=='ib')\n"
		"        {\n"
		"        e.target.src = e.target.layer.cImage.src;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","ib",
		"    if (e.target != null && e.target.kind == 'ib')\n"
		"        {\n"
		"        if (e.pageX >= e.target.layer.pageX &&\n"
		"            e.pageX < e.target.layer.pageX + e.target.layer.clip.width &&\n"
		"            e.pageY >= e.target.layer.pageY &&\n"
		"            e.pageY < e.target.layer.pageY + e.target.layer.clip.height)\n"
		"            {\n"
		"            e.target.src = e.target.layer.pImage.src;\n"
		"            if (e.target.layer.EventClick != null)\n"
		"                {\n"
		"                eparam = new Object();\n"
		"                eparam.Caller = e.target.layer;\n"
		"                cn_activate(e.target.layer, 'Click', eparam);\n"
		"                delete eparam;\n"
		"                }\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            e.target.src = e.target.layer.nImage.src;\n"
		"            }\n"
		"        }\n" );

	htrAddEventHandler(s, "document","MOUSEOVER","ib",
		"    if (e.target != null && e.target.kind == 'ib')\n"
		"        {\n"
		"        if (e.target.img && (e.target.img.src != e.target.cImage.src)) e.target.img.src = e.target.pImage.src;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","ib",
		"    if (e.target != null && e.target.kind == 'ib')\n"
		"        {\n"
		"        if (e.target.img && (e.target.img.src != e.target.cImage.src)) e.target.img.src = e.target.nImage.src;\n"
		"        }\n");

	/** Check for more sub-widgets within the imagebutton. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+1, parentname, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** htibtnInitialize - register with the ht_render module.
 ***/
int
htibtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ImageButton Widget Driver");
	strcpy(drv->WidgetName,"imagebutton");
	drv->Render = htibtnRender;
	drv->Verify = htibtnVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Add the 'click' event **/
	htrAddEvent(drv,"Click");

	/** Register. **/
	htrRegisterDriver(drv);

	HTIBTN.idcnt = 0;

    return 0;
    }
