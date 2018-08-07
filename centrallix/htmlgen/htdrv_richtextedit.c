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
/* Module: 	htdrv_richtext.c         				*/
/* Author:	Judson Hayes/ Jordan Thompson					*/
/* Creation:	July 20, 2018						*/
/* Description:	HTML Widget driver for a rich text editior.		*/
/************************************************************************/



/** globals **/
static struct
    {
    int		idcnt;
    }
    HTTX;


/*** httxRender - generate the HTML code for the richtextedit widget.
 ***/
int
htrteRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char elementid[16];
    //char main_bg[128];
    int x=-1,y=-1,w,h;
    int id, i;
    int is_readonly = 0;
    int is_raised = 0;
    int mode = 0; /* 0=text, 1=html, 2=wiki */
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
	    mssError(1,"HTTX","richtextedit widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTTX","richtextedit widget must have a 'height' property");
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
		mssError(1,"HTTX","richtextedit widget 'mode' property must be either 'text','html', or 'wiki'");
		return -1;
		}
	    }

	/** Background color/image? **/
	//htrGetBackground(tree, NULL, 1, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Style of richtextedit - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"raised")) is_raised = 1;

	/** Form linkage **/
	if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(form,ptr,sizeof(form));
	else
	    form[0]='\0';

	if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0)
                    strtcpy(fieldname,ptr,sizeof(fieldname));
	else
	    fieldname[0]='\0';

        if((strcmp(form,"") != 0) && (strcmp(fieldname,"")==0)) {
                mssError(1,"HTTX","if there is a form specified within the widget, there must be a fieldname specified");
                return -1;
        }
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;

	/** Write Style header items. **/
	snprintf(elementid, sizeof(elementid), "#rte%dbase", id);
	htrFormatElement(s, tree, elementid, 0,
		x, y, w-2*box_offset, h-2*box_offset, z, "",
		(char*[]){"border_color","#e0e0e0", "border_style",(is_raised?"outset":"inset"), NULL},
		"overflow:hidden; position:absolute;");
	//htrAddStylesheetItem_va(s,"\t#tx%POSbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%INT; overflow:hidden; }\n",id,x,y,w-2*box_offset,z);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "rte%POSbase",id);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "text_metric", "null", 0);
	htrAddScriptGlobal(s, "rte_current", "null", 0);
	htrAddScriptGlobal(s, "rte_cur_mainlayer", "null", 0);

	htrAddScriptInclude(s, "/sys/js/htdrv_richtextedit.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_cursor.js", 0);
        htrAddScriptInclude(s, "/sys/thirdparty/ckeditor/ckeditor/ckeditor.js",0);
        htrAddScriptInclude(s, "/sys/thirdparty/ckeditor/ckeditor/adapters/jquery.js",0);

	htrAddEventHandlerFunction(s, "document","MOUSEUP", "rte", "rte_mouseup");
	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "rte","rte_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEOVER", "rte", "rte_mouseover");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "rte", "rte_mousemove");
	htrAddEventHandlerFunction(s, "document","PASTE", "rte", "rte_paste");

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    rte_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), fieldname:\"%STR&JSSTR\", form:\"%STR&JSSTR\", isReadonly:%INT, mode:%INT});\n",
	    name, fieldname,form, is_readonly, mode);
        htrAddScriptInit(s, "$('body').on('click', function() { setTimeout( function() { $('.cke_panel').css({'z-index': '20000'}); }, 100) } );\n");
        //htrAddScriptInit_va(s, "var rtobj%POS = CKEDITOR.replace(\"rte%POSbase\",{customConfig:'/sys/thirdparty/ckeditor/ckeditor/config.js'});\n",id,id);
        //htrAddScriptInit_va(s, "rtobj%POS.on('change',function(){rte_action_set_value(rtobj%POS.getData());});",id,id);
        //htrAddScriptInit_va(s,"var ckei_%POSbase = $('ckei_%POSbase').ckeditor().editor;\n",id,id);
        //htrAddScriptInit_va(s," $('%s').on('change',function(){rte.SetValue(this.val());});\n","ckei_"+id);


        /** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<div id=\"rte%POSbase\"><textarea name = \"rte%POSbasetx\", style=\"width:100%%; height:100%%; border:none; outline:none;\">\n",id,id);

	/** Use CSS border or table for drawing? **/
	/*if (is_raised)
	    htrAddStylesheetItem_va(s, "\t#tx%POSbase { border-style:solid; border-width:1px; border-color: white gray gray white; %STR }\n", id, main_bg);
	else
	    htrAddStylesheetItem_va(s, "\t#tx%POSbase { border-style:solid; border-width:1px; border-color: gray white white gray; %STR }\n", id, main_bg);
	if (h >= 0)
	    htrAddStylesheetItem_va(s,"\t#tx%POSbase { height:%POSpx; }\n", id, h-2*box_offset);*/



	/** Check for more sub-widgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

	/** End the containing layer. **/
	htrAddBodyItem(s, "</textarea>\n");


        htrAddBodyItem(s, "</div>\n");
        return 0;
            }


/*** httxInitialize - register with the ht_render module.
 ***/
int
htrteInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML richtexteditor Driver");
	strcpy(drv->WidgetName,"richtextedit");
	drv->Render = htrteRender;

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
