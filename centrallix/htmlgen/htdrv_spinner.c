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
/* Module: 	htdrv_spinner.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line editbox.		*/
/************************************************************************/

/*This file was based on the edit box file revision 1.2*/



/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTSPNR;

int htspnrSetup(pHtSession s)
	{
	htrAddStylesheetItem_va(s,"\t.spnrAbsInh{ POSITION:absolute; VISIBILITY:inherit; }\n");
	htrAddStylesheetItem_va(s,"\t.spnrAbsHid{ POSITION:absolute; VISIBILITY:hidden; }\n");
	return 0;
	}

/*** htspnrRender - generate the HTML code for the spinner widget.
 ***/
int
htspnrRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_raised = 1;
    char* c1;
    char* c2;
    int maxchars;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTSPNR","Netscape DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTSPNR.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTSPNR","Spinner widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTSPNR","Spinner widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (wgtrGetPropertyValue(tree,"maxchars",DATA_T_INTEGER,POD(&maxchars)) != 0) maxchars=255;

	/** Background color/image? **/
	htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS, main_bg, sizeof(main_bg));

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Style of editbox - raised/lowered **/
	if (wgtrGetPropertyValue(tree,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
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
	htrAddStylesheetItem_va(s,"\t#spnr%POSmain { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);
	htrAddStylesheetItem_va(s,"\t#spnr%POSbase { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,1,1,w-12,z);
	htrAddStylesheetItem_va(s,"\t#spnr%POScon1 { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,1,1,w-2-12,z+1);
	htrAddStylesheetItem_va(s,"\t#spnr%POScon2 { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,1,1,w-2-12,z+1);
	htrAddStylesheetItem_va(s,"\t#spnr_button_up { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",1+w-12,1,w,z);
	htrAddStylesheetItem_va(s,"\t#spnr_button_down { LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",1+w-12,1+9,w,z);

	/** DOM Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "spnr%POSmain",id);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "spnr_ibeam", "null", 0);
	htrAddScriptGlobal(s, "spnr_metric", "null", 0);
	htrAddScriptGlobal(s, "spnr_current", "null", 0);

   	htrAddScriptInclude(s,"/sys/js/htdrv_spinner.js",0);

	htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "spnr", "spnr_mousedown");

	/** Script initialization call. **/
	htrAddScriptInit_va(s,
		"    var spnr = wgtrGetNodeRef(ns, \"%STR&SYM\");\n"
		"    spnr_init({main:spnr, layer:htr_subel(spnr,\"spnr%POSbase\"), c1:htr_subel(htr_subel(spnr,\"spnr%POSbase\"),\"spnr%POScon1\"), c2:htr_subel(htr_subel(spnr,\"spnr%POSbase\"),\"spnr%POScon2\")});\n",
		name,
                id,
		id, id, 
		id, id);

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%POSmain\" class=\"spnrAbsInh\">\n",id);
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%POSbase\" class=\"spnrAbsInh\">\n",id);
	htrAddBodyItem_va(s, "    <TABLE width=%POS cellspacing=0 cellpadding=0 border=0 %STR>\n",w-12,main_bg);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c1);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c1,w-2-12);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n",c1);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD>\n",c1,h-2);
	htrAddBodyItem(s,    "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=%POS width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%STR></TD>\n",c2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR height=1 width=%POS></TD>\n",c2,w-2-12);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%STR></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%POScon1\" class=\"spnrAbsInh\"></DIV>\n",id);
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%POScon2\" class=\"spnrAbsHid\"></DIV>\n",id);
	htrAddBodyItem(s,    "</DIV>\n");
	/*Add the spinner buttons*/
	htrAddBodyItem(s,    "<DIV ID=\"spnr_button_up\" class=\"spnrAbsInh\"><IMG SRC=\"/sys/images/spnr_up.gif\"></DIV>\n");
	htrAddBodyItem(s,    "<DIV ID=\"spnr_button_down\" class=\"spnrAbsInh\"><IMG SRC=\"/sys/images/spnr_down.gif\"></DIV>\n");
  	htrAddBodyItem(s,    "</DIV>\n"); 

	return 0;
    }


/*** htspnrInitialize - register with the ht_render module.
 ***/
int
htspnrInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Spinner Box Driver");
	strcpy(drv->WidgetName,"spinner");
	drv->Render = htspnrRender;
	drv->Setup = htspnrSetup;


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

	HTSPNR.idcnt = 0;

    return 0;
    }
