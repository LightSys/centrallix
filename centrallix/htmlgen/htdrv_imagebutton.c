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

    $Id: htdrv_imagebutton.c,v 1.20 2002/07/31 22:03:44 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_imagebutton.c,v $

    $Log: htdrv_imagebutton.c,v $
    Revision 1.20  2002/07/31 22:03:44  lkehresman
    Fixed mouseup issues when mouseup occurred outside the image for:
      * dropdown scroll images
      * imagebutton images

    Revision 1.19  2002/07/25 18:45:40  lkehresman
    Standardized event connectors for imagebutton and textbutton, and took
    advantage of the checking done in the cn_activate function so it isn't
    necessary outside the function.

    Revision 1.18  2002/07/25 16:54:18  pfinley
    completely undoing the change made yesterday with aliasing of click events
    to mouseup... they are now two separate events. don't believe the lies i said
    yesterday :)

    Revision 1.17  2002/07/24 18:12:03  pfinley
    Updated Click events to be MouseUp events. Now all Click events must be
    specified as MouseUp within the Widget's event handler, or they will not
    work propery (Click can still be used as a event connector to the widget).

    Revision 1.16  2002/07/24 15:14:28  pfinley
    added more actions

    Revision 1.15  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.14  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.13  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.12  2002/06/19 21:22:45  lkehresman
    Added a losefocushandler to the table.  Not having this broke static tables.

    Revision 1.11  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.10  2002/06/02 22:47:23  jorupp
     * fix some problems I introduced before

    Revision 1.9  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.8  2002/05/31 01:26:41  lkehresman
    * modified the window header HTML to make it look nicer
    * fixed a truncation problem with the image button

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
    char n_img[128];
    char p_img[128];
    char c_img[128];
    char d_img[128];
    char enabled[10];
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
	if (objGetAttrValue(w_obj,"disabledimage",POD(&ptr)) == 0)
	    {
	    memccpy(d_img,ptr,'\0',127);
	    d_img[127]=0;
	    }
	else
	    {
	    strcpy(d_img, n_img);
	    }

	if (objGetAttrValue(w_obj,"enabled",POD(&ptr)) == 0)
	    {
	    memccpy(enabled,ptr,'\0',10);
	    enabled[10]=0;
	    }
	else
	    {
	    strcpy(enabled, "true");
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#ib%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
	htrAddScriptGlobal(s, "ib_cur_img", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_imagebutton.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    %s = %s.layers.ib%dpane;\n",nptr, parentname, id);
	htrAddScriptInit_va(s,"    ib_init(%s,'%s','%s','%s','%s',%d,%d,%s,'%s',%s);\n",
	        nptr, n_img, p_img, c_img, d_img, w, h, parentobj,nptr,enabled);

	/** HTML body <DIV> elements for the layers. **/
	if (h == -1)
	    if(strcmp(enabled,"false"))
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0></DIV>\n",id,n_img);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0></DIV>\n",id,d_img);
	else
	    if(strcmp(enabled,"false"))
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0 width=%d height=%d></DIV>\n",id,n_img,w,h);
	    else
		htrAddBodyItem_va(s,"<DIV ID=\"ib%dpane\"><IMG SRC=%s border=0 width=%d height=%d></DIV>\n",id,d_img,w,h);

	/** Add the event handling scripts **/
	htrAddEventHandler(s, "document","MOUSEDOWN","ib",
		"    if (e.target != null && e.target.kind=='ib' && e.target.layer.enabled==true)\n"
		"        {\n"
		"        e.target.src = e.target.layer.cImage.src;\n"
		"        cn_activate(e.target.layer, 'MouseDown');\n"
		"        ib_cur_img = e.target;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEUP","ib",
		"    if (ib_cur_img)\n"
		"        {\n"
		"        if (e.pageX >= ib_cur_img.layer.pageX &&\n"
		"            e.pageX < ib_cur_img.layer.pageX + ib_cur_img.layer.clip.width &&\n"
		"            e.pageY >= ib_cur_img.layer.pageY &&\n"
		"            e.pageY < ib_cur_img.layer.pageY + ib_cur_img.layer.clip.height)\n"
		"            {\n"
		"            cn_activate(e.target.layer, 'Click');\n"
		"            cn_activate(e.target.layer, 'MouseUp');\n"
		"            ib_cur_img.src = ib_cur_img.layer.pImage.src;\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            ib_cur_img.src = ib_cur_img.layer.nImage.src;\n"
		"            }\n"
		"        ib_cur_img = null;\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOVER","ib",
		"    if (e.target != null && e.target.kind == 'ib' && e.target.enabled == true)\n"
		"        {\n"
		"        if (e.target.img && (e.target.img.src != e.target.cImage.src)) e.target.img.src = e.target.pImage.src;\n"
		"        cn_activate(ly, 'MouseOver');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","ib",
		"    if (e.target != null && e.target.kind == 'ib' && e.target.enabled == true)\n"
		"        {\n"
		"        if (e.target.img && (e.target.img.src != e.target.cImage.src)) e.target.img.src = e.target.nImage.src;\n"
		"        cn_activate(ly, 'MouseOut');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEMOVE","ib",
		"    if (e.target != null && e.target.kind == 'ib' && ly.enabled == true)\n"
		"        {\n"
		"        cn_activate(ly, 'MouseMove');\n"
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
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML ImageButton Widget Driver");
	strcpy(drv->WidgetName,"imagebutton");
	drv->Render = htibtnRender;
	drv->Verify = htibtnVerify;
	strcpy(drv->Target, "Netscape47x:default");

	htrAddAction(drv,"Enable");
	htrAddAction(drv,"Disable");
	
	/** Add the 'click' event **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	HTIBTN.idcnt = 0;

    return 0;
    }
