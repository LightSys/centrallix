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
/* Module: 	htdrv_tab.c             				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 28, 1998 					*/
/* Description:	HTML Widget driver for a tab control.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_pane.c,v 1.15 2002/08/13 04:09:09 anoncvs_obe Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_pane.c,v $

    $Log: htdrv_pane.c,v $
    Revision 1.15  2002/08/13 04:09:09  anoncvs_obe
    Pane widget was adding several pixels' worth of offset as a container,
    causing static content (such as a static widget/html) to be shifted
    right and down a noticeable amount.  Problem was a table without the
    needed border/spacing information set.

    Revision 1.14  2002/08/01 18:27:16  pfinley
    The border images of the pane widget are now tagged with .kind, .layer, &
    .mainlayer properties using the utils tag function.

    Revision 1.13  2002/08/01 14:25:15  lkehresman
    Removed closing body tag.  There was no opening body tag, so Netscape was
    thinking that this was the close of the HTML body.  Thus, it wasn't properly
    rendering widgets after any pane widget.  This was an interesting one to
    try and track down.  ;)

    Revision 1.12  2002/07/29 21:34:40  lkehresman
    fixed typo by peter

    Revision 1.11  2002/07/26 20:23:06  pfinley
    Added standard event handlers: Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove

    Revision 1.10  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.9  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.8  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.7  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.6  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.5  2002/03/13 19:48:46  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/22 17:19:42  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTPN;


/*** htpnVerify - not written yet.
 ***/
int
htpnVerify()
    {
    return 0;
    }


/*** htpnRender - generate the HTML code for the page.
 ***/
int
htpnRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char sbuf2[160];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;

    	/** Get an id for this. **/
	id = (HTPN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTPN","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTPN","Pane widget must have a 'height' property");
	    return -1;
	    }

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Style of pane - raised/lowered **/
	if (objGetAttrValue(w_obj,"style",POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
	if (is_raised)
	    {
	    c1 = "white_1x1.png";
	    c2 = "dkgrey_1x1.png";
	    }
	else
	    {
	    c1 = "dkgrey_1x1.png";
	    c2 = "white_1x1.png";
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#pn%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);
	htrAddStylesheetItem_va(s,"\t#pn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,1,1,w-2,h-2,z+1);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Script include call **/
	htrAddScriptInclude(s, "/sys/js/htdrv_pane.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event Handlers **/
	htrAddEventHandler(s, "document","MOUSEUP", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'Click');\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'MouseOver');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEOUT", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'MouseOut');\n"
	    "\n");
   
	htrAddEventHandler(s, "document","MOUSEMOVE", "pn", 
	    "\n"
	    "    if (ly.kind == 'pn') cn_activate(ly, 'MouseMove');\n"
	    "\n");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = pn_init(%s.layers.pn%dbase, %s.layers.pn%dbase.document.layers.pn%dmain);\n",
		nptr, parentname, id, parentname, id, id);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"pn%dbase\">\n",id);
	htrAddBodyItem_va(s,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem_va(s,"            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem_va(s,"<DIV ID=\"pn%dmain\"><table width=%d height=%d cellspacing=0 cellpadding=0 border=0><tr><td>\n",id, w-2, h-2);

	/** Check for objects within the pane. **/
	snprintf(sbuf,160,"%s.mainlayer.document",nptr);
	snprintf(sbuf2,160,"%s.mainlayer",nptr);
	htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</td></tr></table></DIV></DIV>\n");

    return 0;
    }


/*** htpnInitialize - register with the ht_render module.
 ***/
int
htpnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"pane");
	drv->Render = htpnRender;
	drv->Verify = htpnVerify;
	strcpy(drv->Target, "Netscape47x:default");

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	HTPN.idcnt = 0;

    return 0;
    }
