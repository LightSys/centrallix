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
/* Module: 	htdrv_clock.c         					*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	August 7, 2002						*/
/* Description:	HTML Widget driver for a local time clock.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: htdrv_clock.c,v 1.1 2002/08/08 21:57:46 pfinley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_clock.c,v $


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCL;


/*** htclVerify - not written yet.
 ***/
int
htclVerify()
    {
    return 0;
    }


/*** htclRender - generate the HTML code for the clock widget.
 ***/
int
htclRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char fgcolor1[16];
    char fgcolor2[16];
    int shadowed = 0;
    int shadowx = 0;
    int shadowy = 0;
    int size = 2;
    int moveable = 0;
    int bold = 0;
    int x=-1,y=-1,w,h;
    int id;
    char* nptr;
    char fieldname[HT_FIELDNAME_SIZE];
    pObject sub_w_obj;
    pObjQuery qy;

    	/** Get an id for this. **/
	id = (HTCL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTCL","Clock widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTCL","Clock widget must have a 'height' property");
	    return -1;
	    }
	
	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor=%.40s",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get text color **/
	if (objGetAttrValue(w_obj,"fgcolor1",POD(&ptr)) == 0)
	   sprintf(fgcolor1,"%.40s",ptr);
	else
	   strcpy(fgcolor1,"black");

	/* Shadowed text? */
	if (objGetAttrValue(w_obj,"shadowed",POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    {
	    shadowed = 1;
	    if (objGetAttrValue(w_obj,"fgcolor2",POD(&ptr)) == 0)
		strcpy(fgcolor2,ptr);
	    else
	        strcpy(fgcolor2,"#777777");
	    if (objGetAttrValue(w_obj,"shadowx",POD(&ptr)) == 0)
	        shadowx = (int)ptr;
	    else
	        shadowx = 1;
	    if (objGetAttrValue(w_obj,"shadowy",POD(&ptr)) == 0)
	        shadowy = (int)ptr;
	    else
	        shadowy = 1;
	    }

	/** Bold text? **/
	if (objGetAttrValue(w_obj,"bold",POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    bold = 1;

	/** Get text size **/
	if (objGetAttrValue(w_obj,"size",POD(&ptr)) == 0)
	    size = (int)ptr;

	if (objGetAttrValue(w_obj,"moveable",POD(&ptr)) == 0 && !strcmp(ptr,"true"))
	    moveable = 1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Get fieldname **/
	if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
	    strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	else 
	    fieldname[0]='\0';

	/** Write Style header items. **/
	htrAddStylesheetItem_va(s,"\t#cl%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddStylesheetItem_va(s,"\t#cl%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,0,0,w,z+2);
	htrAddStylesheetItem_va(s,"\t#cl%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,0,0,w,z+2);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Other global variables **/
	htrAddScriptGlobal(s, "cl_move", "false", 0);
	htrAddScriptGlobal(s, "cl_xOffset", "null", 0);
	htrAddScriptGlobal(s, "cl_yOffset", "null", 0);

	/** Javascript include files **/
	htrAddScriptInclude(s, "/sys/js/htdrv_clock.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** Event Handlers **/
	htrAddEventHandler(s, "document","MOUSEUP", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseUp');\n"
	    "        if (ly.mainlayer.moveable) cl_move = false;\n"
	    "        }\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "cl",
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseDown');\n"
	    "        if (ly.mainlayer.moveable)\n"
	    "            {\n"
	    "            cl_move = true;\n"
	    "            cl_xOffset = e.pageX - ly.mainlayer.pageX;\n"
	    "            cl_yOffset = e.pageY - ly.mainlayer.pageY;\n"
	    "            }\n"
	    "        }\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl') cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOUT", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl') cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEMOVE", "cl", 
	    "\n"
	    "    if (ly.kind == 'cl')\n"
	    "        {\n"
	    "        cn_activate(ly.mainlayer, 'MouseMove');\n"
	    "        if (ly.mainlayer.moveable && cl_move) ly.mainlayer.moveToAbsolute(e.pageX-cl_xOffset, e.pageY-cl_yOffset);\n"
	    "        }\n"
	    "\n");
	    
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = cl_init(%s.layers.cl%dbase, %s.layers.cl%dbase.document.layers.cl%dcon1, %s.layers.cl%dbase.document.layers.cl%dcon2,\"%s\",\"%s\",%d,\"%s\",\"%s\",%d,%d,%d,%d,%d);\n",
		nptr, parentname, id,
		parentname, id, id,
		parentname, id, id,
		fieldname, main_bg, shadowed,
		fgcolor1, fgcolor2,
		size, moveable, bold,
		shadowx, shadowy);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"cl%dbase\">\n",id);
	htrAddBodyItem_va(s, "    <BODY %s><TABLE width=%d height=%d border=0 cellpadding=0 cellspacing=0><TR><TD></TD></TR></TABLE></BODY>\n",main_bg,w,h);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "    <DIV ID=\"cl%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s,    "</DIV>\n");

	/** Check for more sub-widgets **/
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


/*** htclInitialize - register with the ht_render module.
 ***/
int
htclInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Local Time Clock Driver");
	strcpy(drv->WidgetName,"clock");
	drv->Render = htclRender;
	drv->Verify = htclVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Events **/ 
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	HTCL.idcnt = 0;

    return 0;
    }
