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
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    char* nptr;
    char *text;
    char *c1;
    char *c2;

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
	printf("text -> %s\n",text);

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
	htrAddHeaderItem(s,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem_va(s,"\t#lbl%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,"    </STYLE>\n");

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** Label text encoding function **/
	htrAddScriptFunction(s, "lbl_encode", "\n"
		"function lbl_encode(s)\n"
		"    {\n"
		"    rs = '';\n"
		"    for(i=0;i<s.length;i++)\n"
		"        {\n"
		"        if (s[i] == '<') rs += '&lt;';\n"
		"        else if (s[i] == '>') rs += '&gt;';\n"
		"        else if (s[i] == '&') rs += '&amp;';\n"
		"        else if (s[i] == ' ') rs += '&nbsp;';\n"
		"        else rs += s[i];\n"
		"        }\n"
		"    return rs;\n"
		"    }\n", 0);
	/** Label initializer **/
	htrAddScriptFunction(s, "lbl_init", "\n"
		"function lbl_init(l,text,align,width)\n"
		"    {\n"
		"    l.kind = 'label'\n"
		"    l.document.write(\"<table border=0 width=\"+width+\"><tr><td align='\"+align+\"'>\");\n"
		"    l.document.write(lbl_encode(new String(text)));\n"
		"    l.document.write(\"</td></tr></table>\");\n"
		"    l.document.close();\n"
		"    return l;\n"
		"    }\n", 0);
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = lbl_init(%s.layers.lbl%d,\"%s\",\"%s\",%i);\n", nptr, parentname, id,text,align,w);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"lbl%d\">\n",id);
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

	/** Register. **/
	htrRegisterDriver(drv);

	HTLBL.idcnt = 0;

    return 0;
    }
