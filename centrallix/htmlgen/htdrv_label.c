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
/* Module: 	htdrv_label.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line label.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_label.c,v $
    Revision 1.11  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.10  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.9  2002/07/07 00:23:12  jorupp
     * fixed a bug with the table tag not being closed (why did this work before?
     * added px qualifiers on CSS definitions for HTML 4.0 Strict

    Revision 1.8  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.7  2002/05/31 05:03:32  jorupp
     * OSRC now can do a DoubleSync -- check kardia for an example

    Revision 1.6  2002/04/27 06:37:45  jorupp
     * some bug fixes in the form
     * cleaned up some debugging output in the label
     * added a dynamic table widget

    Revision 1.5  2002/04/26 15:00:53  jorupp
     * fixing yet another mistake I made yesterday merging my changes with Greg's
     * the align attribute of a label now works

    Revision 1.4  2002/04/25 23:05:09  jorupp
     * fixed bug I introduced when I didn't fix a collision with Greg's code right...

    Revision 1.3  2002/04/25 23:02:52  jorupp
     * added alternate alignment for labels (right or center should work)
     * fixed osrc/form bug

    Revision 1.2  2002/04/25 22:51:29  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.1  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTLBL;


/*** htlblVerify - not written yet.
 ***/
int
htlblVerify()
    {
    return 0;
    }


/*** htlblRender - generate the HTML code for the label widget.
 ***/
int
htlblRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char align[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    char* nptr;
    char *text;

    	/** Get an id for this. **/
	id = (HTLBL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTLBL","Label widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTLBL","Label widget must have a 'height' property");
	    return -1;
	    }

	if(objGetAttrValue(w_obj,"text",POD(&ptr)) == 0)
	    {
	    text=nmMalloc(strlen(ptr)+1);
	    strcpy(text,ptr);
	    }
	else
	    {
	    text=nmMalloc(1);
	    text[0]='\0';
	    }

	align[0]='\0';
	if(objGetAttrValue(w_obj,"align",POD(&ptr)) == 0)
	    {
	    strcpy(align,ptr);
	    }
	else
	    {
	    strcpy(align,"left");
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

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#lbl%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);


	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"lbl%d\">\n",id);
	htrAddBodyItem_va(s, "<table border=0 width=\"%i\"><tr><td align=\"%s\">%s</td></tr></table>\n",w,align,text);
	htrAddBodyItem(s, "</DIV>\n");

	nmFree(text,strlen(text)+1);

    return 0;
    }


/*** htlblInitialize - register with the ht_render module.
 ***/
int
htlblInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Label Driver");
	strcpy(drv->WidgetName,"label");
	drv->Render = htlblRender;
	drv->Verify = htlblVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Register. **/
	htrRegisterDriver(drv);

	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Label Driver");
	strcpy(drv->WidgetName,"label");
	drv->Render = htlblRender;
	drv->Verify = htlblVerify;
	strcpy(drv->Target, "Mozilla:default");

	/** Register. **/
	htrRegisterDriver(drv);

	HTLBL.idcnt = 0;

    return 0;
    }
