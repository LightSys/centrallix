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
#include "cxlib/strtcpy.h"

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



/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTX;


/*** httxRender - generate the HTML code for the textarea widget.
 ***/
int
httxRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    int mode = 0; /* 0=text, 1=html, 2=wiki */
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int box_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom2Events)
	    {
	    mssError(1,"HTTX","Netscape, IE, or Dom2Events support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTX.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","Textarea widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (wgtrGetPropertyValue(tree,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (wgtrGetPropertyValue(tree,"readonly",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Allow HTML? **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"text")) mode = 0;
	    else if (!strcasecmp(ptr,"html")) mode = 1;
	    else if (!strcasecmp(ptr,"wiki")) mode = 2;
	    else
		{
		mssError(1,"HTTX","Textarea widget 'mode' property must be either 'text','html', or 'wiki'");
		return -1;
		}
	    }

	/** Background color/image? **/
	htrGetBackground(tree, NULL, s->Capabilities.CSS2?1:0, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Style of Textarea - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"raised")) is_raised = 1;
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

	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    strtcpy(fieldname,ptr,HT_FIELDNAME_SIZE);
	    }
	else 
	    { 
	    fieldname[0]='\0';
	    } 

	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Write Style header items. **/
	if (s->Capabilities.Dom1HTML)
	    htrAddStylesheetItem_va(s,"\t#tx%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%INT; overflow:hidden; }\n",id,x,y,w-2*box_offset,z);
	else if (s->Capabilities.Dom0NS)
	    htrAddStylesheetItem_va(s,"\t#tx%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tx%POSbase\")",id);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "tx_current", "null", 0);
	htrAddScriptGlobal(s, "tx_cur_mainlayer", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_textarea.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);

	htrAddEventHandlerFunction(s, "document","MOUSEUP", "tx", "tx_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "tx","tx_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "tx", "tx_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "tx", "tx_mousemove");
	    
	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    tx_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), fieldname:\"%STR&JSSTR\", form:\"%STR&JSSTR\", isReadonly:%INT, mode:%INT, mainBackground:\"%STR&JSSTR\"});\n",
	    name, fieldname, form, is_readonly, mode, main_bg);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"tx%POSbase\">\n",id);

	/** Use CSS border or table for drawing? **/
	if (s->Capabilities.CSS2)
	    {
	    if (is_raised)
		htrAddStylesheetItem_va(s, "\t#tx%POSbase { border-style:solid; border-width:1px; border-color: white gray gray white; %STR }\n", id, main_bg);
	    else
		htrAddStylesheetItem_va(s, "\t#tx%POSbase { border-style:solid; border-width:1px; border-color: gray white white gray; %STR }\n", id, main_bg);
	    if (h >= 0)
		htrAddStylesheetItem_va(s,"\t#tx%POSbase { height:%POSpx; }\n", id, h-2*box_offset);
	    }
	else
	    {
	    htrAddBodyItem_va(s, "    <TABLE width=%POS cellspacing=0 cellpadding=0 border=0 %STR>\n",w,main_bg);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c1);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c1,w-2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n",c1);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD>\n",c1,h-2);
	    htrAddBodyItem(s,    "            <TD>&nbsp;</TD>\n");
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD></TR>\n",c2,h-2);
	    htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c2,w-2);
	    htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n    </TABLE>\n\n",c2);
	    }

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

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

	htrAddSupport(drv, "dhtml");

	HTTX.idcnt = 0;

    return 0;
    }
