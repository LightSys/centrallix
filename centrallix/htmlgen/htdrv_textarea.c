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
/* Module: 	htdrv_textarea.c         				*/
/* Author:	Peter Finley (PMF)					*/
/* Creation:	July 9, 2002						*/
/* Description:	HTML Widget driver for a multi-line textarea.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: htdrv_textarea.c,v 1.13 2002/09/27 22:26:05 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_textarea.c,v $

    $Log: htdrv_textarea.c,v $
    Revision 1.13  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.12  2002/08/01 15:14:38  lkehresman
    Tagged textarea images with .kind, .layer, and .mainlayer

    Revision 1.11  2002/07/30 21:38:37  pfinley
    Added events: MouseUp,MouseDown,MouseOver,MouseOut,MouseMove,DataChange,
    GetFocus,LoseFocus to textarea widget.

    Revision 1.10  2002/07/19 14:54:22  pfinley
    - Modified the page mousedown & mouseover handlers so that the cursor ibeam
    "can't" be clicked on (only supports the global cursor).
    - Modified the editbox & textarea files to support the new global cursor.

    Revision 1.9  2002/07/16 19:24:34  pfinley
    uses the new header stylesheet function

    Revision 1.8  2002/07/16 19:16:21  pfinley
    textarea c file with js includes

    Revision 1.7  2002/07/12 20:16:13  pfinley
    Undid previous change :)

    Revision 1.6  2002/07/12 19:46:22  pfinley
    added cvs logging to file.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTX;


/*** httxVerify - not written yet.
 ***/
int
httxVerify()
    {
    return 0;
    }


/*** httxRender - generate the HTML code for the editbox widget.
 ***/
int
httxRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    /*char sbuf[HT_SBUF_SIZE];*/
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_readonly = 0;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];
    pObject sub_w_obj;
    pObjQuery qy;

    	/** Get an id for this. **/
	id = (HTTX.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (objGetAttrValue(w_obj,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (objGetAttrValue(w_obj,"readonly",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"bgColor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Style of Textarea - raised/lowered **/
	if (objGetAttrValue(w_obj,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
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

	if (objGetAttrValue(w_obj,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    strncpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	    }
	else 
	    { 
	    fieldname[0]='\0';
	    } 

	/** Write Style header items. **/
	htrAddStylesheetItem_va(s,"\t#tx%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "tx_current", "null", 0);
	htrAddScriptGlobal(s, "tx_cur_mainlayer", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_textarea.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);

	htrAddEventHandler(s, "document","MOUSEUP", "tx", 
	    "\n"
	    "    if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseUp');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEDOWN", "tx",
	    "\n"
	    "    if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseDown');\n"
	    "\n");

	htrAddEventHandler(s, "document","MOUSEOVER", "tx", 
	    "\n"
	    "    if (ly.kind == 'tx')\n"
	    "        {\n"
	    "        if (!tx_cur_mainlayer)\n"
	    "            {\n"
	    "            cn_activate(ly.mainlayer, 'MouseOver');\n"
	    "            tx_cur_mainlayer = ly.mainlayer;\n"
	    "            }\n"
	    "        }\n"
	    "\n");

//	htrAddEventHandler(s, "document","MOUSEOUT", "tx", 
//	    "\n"
//	    "\n");

	htrAddEventHandler(s, "document","MOUSEMOVE", "tx", 
	    "\n"
	    "   if (tx_cur_mainlayer && ly.kind != 'tx')\n"			// 
	    "      {\n"								//
	    "      cn_activate(tx_cur_mainlayer, 'MouseOut');\n"		// This is MouseOut Detection!
	    "      tx_cur_mainlayer = null;\n"					// 
	    "      }\n"								//
	    "   if (ly.kind == 'tx') cn_activate(ly.mainlayer, 'MouseMove');\n"
	    "\n");
	    
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    %s = tx_init(%s.layers.tx%dbase, \"%s\", %d, \"%s\");\n",
		nptr, parentname, id, 
		fieldname, is_readonly, main_bg);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"tx%dbase\"><BODY %s>\n",id, main_bg);
	htrAddBodyItem_va(s, "    <TABLE width=%d cellspacing=0 cellpadding=0 border=0>\n",w);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem_va(s, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);

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

	/** End the containing layer. **/
	htrAddBodyItem(s, "</BODY></DIV>\n");

    return 0;
    }


/*** httxInitialize - register with the ht_render module.
 ***/
int
httxInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Multiline Textarea Driver");
	strcpy(drv->WidgetName,"textarea");
	drv->Render = httxRender;
	drv->Verify = httxVerify;
	strcpy(drv->Target, "Netscape47x:default");

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);

	/** Events **/ 
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"GetFocus");
	htrAddEvent(drv,"LoseFocus");

	/** Register. **/
	htrRegisterDriver(drv);

	HTTX.idcnt = 0;

    return 0;
    }
