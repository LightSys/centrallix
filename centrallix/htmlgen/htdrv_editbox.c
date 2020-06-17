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
#include "wgtr.h"

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
/* Module: 	htdrv_editbox.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line editbox.		*/
/************************************************************************/


/** globals **/
static struct
    {
    int		idcnt;
    }
    HTEB;


/*** htebRender - generate the HTML code for the editbox widget.
 ***/
int
htebRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    char descfg[64];
    char descr[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    char* tooltip;
    int maxchars;
    char fieldname[HT_FIELDNAME_SIZE];
    char form[64];
    int box_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !s->Capabilities.Dom2Events)
	    {
	    mssError(1,"HTEB","Netscape, IE, or Dom2Events support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTEB.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'height' property");
	    return -1;
	    }

	/** Maximum characters to accept from the user **/
	if (wgtrGetPropertyValue(tree,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Readonly flag **/
	if (wgtrGetPropertyValue(tree,"readonly",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"yes")) is_readonly = 1;

	/** Background color/image **/
	strcpy(main_bg,"");
	htrGetBackground(tree, NULL, s->Capabilities.CSS2?1:0, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Get description color **/
	if (wgtrGetPropertyValue(tree,"description_fgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(descfg,ptr,sizeof(descfg));
	else
	    strcpy(descfg, "gray");

	/** Get empty field description **/
	if (wgtrGetPropertyValue(tree,"empty_description",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(descr,ptr,sizeof(descr));
	else
	    strcpy(descr, "");

	/** client-side expr for content **/
	htrCheckAddExpression(s,tree,name,"content");

	/** Style of editbox - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"raised")) is_raised = 1;

	/** enable/disable expression for editbox **/
	/*htrCheckAddExpression(s, tree, name, "enabled");*/

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';

	if(wgtrGetPropertyValue(tree,"tooltip",DATA_T_STRING,POD(&ptr)) == 0)
	    tooltip=nmSysStrdup(ptr);
	else
	    tooltip=nmSysStrdup("");

	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#eb%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; overflow:hidden; }\n",id,x,y,w-2*box_offset,z);
	htrAddStylesheetItem_va(s,"\t#eb%POScon1 { VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; border:none; }\n",id,5,0,w-10,z+1);

	/** Write named global **/
	htrAddWgtrObjLinkage_va(s, tree, "eb%POSbase",id);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "eb_current", "null", 0);

	/** Script include to get functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_editbox.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	htrAddEventHandlerFunction(s, "document","MOUSEUP", "eb", "eb_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "eb", "eb_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "eb", "eb_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEOUT", "eb", "eb_mouseout");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "eb", "eb_mousemove");
	htrAddEventHandlerFunction(s, "document","PASTE", "eb", "eb_paste");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    eb_init({layer:wgtrGetNodeRef(ns,'%STR&SYM'), c1:document.getElementById(\"eb%POScon1\"), form:\"%STR&JSSTR\", fieldname:\"%STR&JSSTR\", isReadOnly:%INT, mainBackground:\"%STR&JSSTR\", tooltip:\"%STR&JSSTR\", desc_fgcolor:\"%STR&JSSTR\", empty_desc:\"%STR&JSSTR\"});\n",
	    name,  id,
	    form, fieldname, is_readonly, main_bg,
	    tooltip, descfg, descr);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"eb%POSbase\">\n",id);

	/** Use CSS border for drawing **/
	if (is_raised)
	    htrAddStylesheetItem_va(s,"\t#eb%POSbase { border-style:solid; border-width:1px; border-color: white gray gray white; %STR }\n",id, main_bg);
	else
	    htrAddStylesheetItem_va(s,"\t#eb%POSbase { border-style:solid; border-width:1px; border-color: gray white white gray; %STR }\n",id, main_bg);
	if (h >= 0)
	    htrAddStylesheetItem_va(s,"\t#eb%POSbase { height:%POSpx; }\n\t#eb%POScon1 { height:%POSpx; }\n", id, h-2*box_offset, id, h-2*box_offset-2);

	//htrAddBodyItem_va(s, "<table border='0' cellspacing='0' cellpadding='0' width='%POS'><tr><td align='left' valign='middle' height='%POS'><img name='l' src='/sys/images/eb_edg.gif'></td><td>&nbsp;</td><td align='right' valign='middle'><img name='r' src='/sys/images/eb_edg.gif'></td></tr></table>\n", w-2, h-2);
	//htrAddBodyItem_va(s, "<DIV ID=\"eb%POScon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "<img name=\"l\" src=\"/sys/images/eb_edg.gif\" style=\"vertical-align:10%%\" /><input id=\"eb%POScon1\" /><img name=\"r\" src=\"/sys/images/eb_edg.gif\" style=\"vertical-align:10%%\" />\n",id);

	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

	nmSysFree(tooltip);

    return 0;
    }


/*** htebInitialize - register with the ht_render module.
 ***/
int
htebInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Editbox Driver");
	strcpy(drv->WidgetName,"editbox");
	drv->Render = htebRender;

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

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTEB.idcnt = 0;

    return 0;
    }
