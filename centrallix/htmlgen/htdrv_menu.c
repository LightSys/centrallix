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

    $Id: htdrv_menu.c,v 1.6 2002/07/16 18:23:20 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_menu.c,v $

    $Log: htdrv_menu.c,v $
    Revision 1.6  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.5  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.4  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.3  2002/05/02 01:12:43  gbeeley
    Fixed some buggy initialization code where an XArray was not being
    setup prior to being used.  Was causing potential bad pointers to
    realloc() and other various problems, especially once the dynamic
    loader was messing with things.

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

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
    HTMENU;


/*** htmenuVerify - not written yet.
 ***/
int
htmenuVerify()
    {
    return 0;
    }


/*** htmenuRender - generate the HTML code for the page.
 ***/
int
htmenuRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[HT_SBUF_SIZE];
    char bgimg[128] = "";
    char bgcolor[128] = "";
    pObject sub_w_obj;
    pObjQuery qy;
    int x,y,w;
    int id;
    char* nptr;
    int is_permanent = 0;
    int is_horizontal = 0;

    	/** Get an id for this. **/
	id = (HTMENU.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x = -1;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y = -1;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) w = 100;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Get background image / color **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    {
	    memccpy(bgcolor, ptr, '\0', 127);
	    bgcolor[127] = 0;
	    }
	if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    {
	    memccpy(bgimg, ptr, '\0', 127);
	    bgcolor[127] = 0;
	    }

	/** Is this a 'fixed' menu or 'popup'? **/
	if (objGetAttrValue(w_obj,"type", POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr, "fixed")) 
	        {
		is_permanent = 1;
		is_horizontal = 1;
		}
	    }

	/** Is this a horizontal or vertical menu? **/
	if (objGetAttrValue(w_obj,"orientation", POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr, "horizontal")) is_horizontal = 1;
	    else if (!strcmp(ptr, "vertical")) is_horizontal = 0;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#mn%dpane { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; %s%s%s }\n",id,(x==-1)?0:x,(y==-1)?0:y,w,z, (*bgcolor)?"LAYER-BACKGROUND-COLOR:":"",bgcolor,(*bgcolor)?";":"");

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Globals for tracking where last click was **/
	htrAddScriptGlobal(s, "mn_last_x", "0", 0);
	htrAddScriptGlobal(s, "mn_last_y", "0", 0);

	/** Globals for tracking current menu **/
	htrAddScriptGlobal(s, "mn_current", "null", 0);
	htrAddScriptGlobal(s, "mn_top_z", "1500", 0);
	htrAddScriptGlobal(s, "mn_clear", "0", 0);

	/** Function to handle nonfixed menu activation **/
	htrAddScriptFunction(s, "mn_activate", "\n"
		"function mn_activate(aparam)\n"
		"    {\n"
		"    x = aparam.X;\n"
		"    y = aparam.Y;\n"
		"    if (x == null) x = mn_last_x;\n"
		"    if (y == null) y = mn_last_y;\n"
		"    this.moveToAbsolute(x,y);\n"
		"    this.visibility = 'visible';\n"
		"    this.zIndex = (mn_top_z++);\n"
		"    mn_current = this;\n"
		"    }\n", 0);

	/** Our initialization processor function. **/
	htrAddScriptFunction(s, "mn_init", "\n"
		"function mn_init(l,is_p,is_h,po)\n"
		"    {\n"
	     	/*"    l.nofocus = true;\n" */
		"    l.LSParent = po;\n"
		"    l.kind = 'mn';\n"
		"    l.ActionActivate = mn_activate;\n"
		"    if (is_h == 0)\n"
		"        {\n"
		"        w=50;\n"
		"        y = 0;\n"
		"        for(i=0;i<l.document.layers.length;i++)\n"
		"            {\n"
		"            cl=l.document.layers[i];\n"
		"            if (cl.clip.width > w) w = cl.clip.width;\n"
		"            cl.top = y;\n"
		"            cl.left = 0;\n"
		"            y=y+cl.clip.height;\n"
		"            }\n"
		"        l.clip.height = y+1;\n"
		"        l.clip.width = w+1;\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        x = 0;\n"
		"        h=20;\n"
		"        for(i=0;i<l.document.layers.length;i++)\n"
		"            {\n"
		"            cl=l.document.layers[i];\n"
		"            if (cl.clip.height > h) h = cl.clip.height;\n"
		"            cl.left = x;\n"
		"            cl.top = 0;\n"
		"            x=x+cl.clip.width;\n"
		"            }\n"
		"        if (l.clip.width < x+1) l.clip.width = x+1;\n"
		"        l.clip.height = h+1;\n"
		"        }\n"
		"    if (is_p == 1) l.visibility = 'visible';\n"
		"    }\n" ,0);


	/** Script initialization call.   Do this part before the child objs init. **/
	htrAddScriptInit_va(s,"    %s = %s.layers.mn%dpane;\n",nptr, parentname, id);

	/** HTML body <DIV> elements for the menu layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"mn%dpane\">\n",id);

	/** Add the event handling scripts **/
	/** For mousedown, record the coordinates, and check to see if click in a menu. **/
	htrAddEventHandler(s, "document","MOUSEDOWN","mn",
		"    mn_last_x = e.pageX;\n"
		"    mn_last_y = e.pageY;\n"
		"    if (mn_current != null)\n"
		"        {\n"
		"        in_mn = 0;\n"
		"        mn = mn_current;\n"
		"        while(mn.kind == 'mn')\n"
		"            {\n"
		"            if (e.pageX > mn.pageX && e.pageX < mn.pageX + mn.clip.width &&\n"
		"                e.pageY > mn.pageY && e.pageY < mn.pageY + mn.clip.height)\n"
		"                {\n"
		"                in_mn = 1;\n"
		"                mn_clear = 1;\n"
		"                break;\n"
		"                }\n"
		"            mn.visibility = 'hidden';\n"
		"            mn_current = mn_current.LSParent;\n"
		"            mn = mn.LSParent;\n"
		"            }\n"
		"        if (in_mn == 0) mn_current = null;\n"
		"        }\n" );

	htrAddEventHandler(s, "document","MOUSEUP","mn",
		"    if (mn_current != null && mn_clear == 1)\n"
		"        {\n"
		"        mn_clear = 0;\n"
		"        mn_current.visibility = 'hidden';\n"
		"        mn_current = null;\n"
		"        }\n" );

	htrAddEventHandler(s, "document","MOUSEOVER","mn",
		"    if (e.target != null && e.target.kind == 'mn')\n"
		"        {\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOUT","mn",
		"    if (e.target != null && e.target.kind == 'mn')\n"
		"        {\n"
		"        }\n");

	/** Check for more sub-widgets within the imagebutton. **/
	snprintf(sbuf,HT_SBUF_SIZE,"%s.document",nptr);
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+1, sbuf, nptr);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** End the menu object 'div' **/
	htrAddBodyItem(s,"</DIV>\n");

	/** Script initialization call.   Do the main mn_init _AFTER_ the child objs init. **/
	htrAddScriptInit_va(s,"    mn_init(%s,%d,%d,%s);\n", nptr, is_permanent, is_horizontal, parentobj);

    return 0;
    }


/*** htmenuInitialize - register with the ht_render module.
 ***/
int
htmenuInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Menu Widget Driver");
	strcpy(drv->WidgetName,"menu");
	drv->Render = htmenuRender;
	drv->Verify = htmenuVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

	/** Add the 'click' event **/
	htrAddAction(drv,"Activate");
	htrAddParam(drv,"Activate","X",DATA_T_INTEGER);
	htrAddParam(drv,"Activate","Y",DATA_T_INTEGER);

	/** Register. **/
	htrRegisterDriver(drv);

	HTMENU.idcnt = 0;

    return 0;
    }
