#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_objcanvas.c       				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 3, 2004					*/
/* Description:	HTML Widget driver for the 'object canvas'		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_objcanvas.c,v 1.2 2005/02/26 06:42:37 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_objcanvas.c,v $

    $Log: htdrv_objcanvas.c,v $
    Revision 1.2  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1  2004/12/31 04:35:17  gbeeley
    - Adding the Object Canvas widget, an objectsource client which allows
      data to be displayed visually on a canvas (useful with maps and diagrams
      and such).  Link a form to the osrc as well and have some fun :)

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTOC;


/*** htocRender - generate the HTML code for the page.
 ***/
int
htocRender(pHtSession s, pWgtrNode oc_node, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char sbuf2[160];
    char main_bg[128];
    char osrc[64];
    int x=-1,y=-1,w,h;
    int id;
    char* nptr;
    int allow_select, show_select;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTOC","Netscape DOM or W3C DOM1 HTML and CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTOC.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(oc_node,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(oc_node,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(oc_node,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTOC","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(oc_node,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTOC","Pane widget must have a 'height' property");
	    return -1;
	    }

	/** Background color/image? **/
	htrGetBackground(oc_node, NULL, s->Capabilities.CSS2, main_bg, sizeof(main_bg));

	/** objectsource specified? **/
	if (wgtrGetPropertyValue(oc_node, "source", DATA_T_STRING, POD(&ptr)) != 0)
	    strcpy(osrc, "null");
	else
	    memccpy(osrc, ptr, 0, sizeof(osrc)-1);
	osrc[sizeof(osrc)-1] = '\0';

	/** allow selection of objects? **/
	allow_select = htrGetBoolean(oc_node, "allow_selection", 0);

	/** show current selection? **/
	show_select = htrGetBoolean(oc_node, "show_selection", 0);

	/** Get name **/
	if (wgtrGetPropertyValue(oc_node,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Add css item for the layer **/
	if (s->Capabilities.CSS2)
	    htrAddStylesheetItem_va(s,"\t#oc%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; overflow: hidden; %s}\n",id,x,y,w,h,z,main_bg);
	else
	    htrAddStylesheetItem_va(s,"\t#oc%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; HEIGHT:%d; Z-INDEX:%d; }\n",id,x,y,w,h,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Include our necessary supporting js files **/
	htrAddScriptInclude(s, "/sys/js/htdrv_objcanvas.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

	/** Event Handlers **/
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "oc", "oc_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "oc", "oc_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "oc", "oc_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "oc", "oc_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "oc", "oc_mouseout");
   
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = oc_init({layer:%s.cxSubElement(\"oc%dbase\"), osrc:%s, allow_select:%d, show_select:%d, name:\"%s\"});\n",
		nptr, parentname, id, osrc, allow_select, show_select, nptr);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"oc%dbase\">\n",id);
	if (!s->Capabilities.CSS2) htrAddBodyItem_va(s,"<BODY %s><table width=%d><tr><td>&nbsp;</td></tr></table>\n",main_bg,w);

	/** Check for objects within the pane. **/
	if (s->Capabilities.Dom0NS)
	    snprintf(sbuf,160,"%s.mainlayer.document",nptr);
	else
	    snprintf(sbuf,160,"%s.mainlayer",nptr);
	snprintf(sbuf2,160,"%s.mainlayer",nptr);
	htrRenderSubwidgets(s, oc_node, sbuf, sbuf2, z+2);

	/** End the containing layer. **/
	if (!s->Capabilities.CSS2) htrAddBodyItem_va(s, "</BODY>");
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htocInitialize - register with the ht_render module.
 ***/
int
htocInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"objcanvas");
	drv->Render = htocRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTOC.idcnt = 0;

    return 0;
    }
