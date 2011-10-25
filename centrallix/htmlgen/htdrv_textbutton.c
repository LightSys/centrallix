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
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_textbutton.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 5, 1998  					*/
/* Description:	HTML Widget driver for a 'text button', which frames a	*/
/*		text string in a 3d-like box that simulates the 3d	*/
/*		clicking action when the user points and clicks.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBTN;


/*** httbtnRender - generate the HTML code for the page.
 ***/
int
httbtnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char text[64];
    char fgcolor1[64];
    char fgcolor2[64];
    char bgcolor[128];
    char bgstyle[128];
    char disable_color[64];
    int x,y,w,h;
    int id, i;
    int is_ts = 1;
    char* dptr;
    int is_enabled = 1;
    pExpression code;
    int box_offset;
    int clip_offset;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom0IE && !(s->Capabilities.Dom1HTML && s->Capabilities.Dom2CSS))
	    {
	    mssError(1,"HTTBTN","Netscape DOM or (W3C DOM1 HTML and W3C DOM2 CSS) support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTBTN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(1,"HTTBTN","TextButton widget must have an 'x' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'y' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_STRING && wgtrGetPropertyValue(tree,"enabled",DATA_T_STRING,POD(&ptr)) == 0 && ptr)
	    {
	    if (!strcasecmp(ptr,"false") || !strcasecmp(ptr,"no")) is_enabled = 0;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** box adjustment... arrgh **/
	if (s->Capabilities.CSSBox)
	    box_offset = 1;
	else
	    box_offset = 0;
	clip_offset = s->Capabilities.CSSClip?1:0;

	/** User requesting expression for enabled? **/
	if (wgtrGetPropertyType(tree,"enabled") == DATA_T_CODE)
	    {
	    wgtrGetPropertyValue(tree,"enabled",DATA_T_CODE,POD(&code));
	    is_enabled = 0;
	    htrAddExpression(s, name, "enabled", code);
	    }

	/** Threestate button or twostate? **/
	if (wgtrGetPropertyValue(tree,"tristate",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no")) is_ts = 0;

	/** Get normal, point, and click images **/
	ptr = "-";
	if (!htrCheckAddExpression(s, tree, name, "text") && wgtrGetPropertyValue(tree,"text",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    mssError(1,"HTTBTN","TextButton widget must have a 'text' property");
	    return -1;
	    }
	strtcpy(text,ptr,sizeof(text));

	/** Get fgnd colors 1,2, and background color **/
	htrGetBackground(tree, NULL, 0, bgcolor, sizeof(bgcolor));
	htrGetBackground(tree, NULL, 1, bgstyle, sizeof(bgstyle));

	if (wgtrGetPropertyValue(tree,"fgcolor1",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor1,ptr,sizeof(fgcolor1));
	else
	    strcpy(fgcolor1,"white");
	if (wgtrGetPropertyValue(tree,"fgcolor2",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(fgcolor2,ptr,sizeof(fgcolor2));
	else
	    strcpy(fgcolor2,"black");
	if (wgtrGetPropertyValue(tree,"disable_color",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(disable_color,ptr,sizeof(disable_color));
	else
	    strcpy(disable_color,"#808080");

	htrAddScriptGlobal(s, "tb_current", "null", 0);

	/** DOM Linkages **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tb%POSpane\")",id);
	htrAddWgtrCtrLinkage(s, tree, "_obj");

	/** Include the javascript code for the textbutton **/
	htrAddScriptInclude(s, "/sys/js/htdrv_textbutton.js", 0);

	dptr = wgtrGetDName(tree);
	htrAddScriptInit_va(s, "    %STR&SYM = nodes['%STR&SYM'];\n", dptr, name);

	if(s->Capabilities.Dom0NS)
	    {
	    /** Ok, write the style header items. **/
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INT; TOP:%INT; WIDTH:%POS; Z-INDEX:%POS; }\n",id,x,y,w,z);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane2 { POSITION:absolute; VISIBILITY:%STR; LEFT:-1; TOP:-1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"inherit":"hidden",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane3 { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_enabled?"hidden":"inherit",w-1,z+1);
	    htrAddStylesheetItem_va(s,"\t#tb%POStop { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSbtm { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:%POS; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",w,z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSrgt { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);
	    htrAddStylesheetItem_va(s,"\t#tb%POSlft { POSITION:absolute; VISIBILITY:%STR; LEFT:0; TOP:0; HEIGHT:1; WIDTH:1; Z-INDEX:%POS; }\n",id,is_ts?"hidden":"inherit",z+2);

	    /** Script initialization call. **/
	    htrAddScriptInit_va(s, "    tb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"tb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"tb%POSpane3\"), top:htr_subel(%STR&SYM, \"tb%POStop\"), bottom:htr_subel(%STR&SYM, \"tb%POSbtm\"), right:htr_subel(%STR&SYM, \"tb%POSrgt\"), left:htr_subel(%STR&SYM, \"tb%POSlft\"), width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		    dptr, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, dptr, id, w, h, is_ts, name, text);

	    /** HTML body <DIV> elements for the layers. **/
	    if (h >= 0)
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=0 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text,h);
		htrAddBodyItem_va(s, "<DIV ID=\"tb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=0 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD><TD><IMG SRC=/sys/images/trans_1.gif width=1 height=%POS></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text,h);
		}
	    else
		{
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><TABLE border=0 cellspacing=0 cellpadding=3 %STR width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n",id,bgcolor,w,fgcolor2,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane2\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,fgcolor1,text);
		htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane3\"><TABLE border=0 cellspacing=0 cellpadding=3 width=%POS><TR><TD align=center valign=middle><FONT COLOR='%STR&HTE'><B>%STR&HTE</B></FONT></TD></TR></TABLE>\n</DIV>",id,w,disable_color,text);
		}
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POStop\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSbtm\"><IMG SRC=/sys/images/trans_1.gif height=1 width=%POS></DIV>\n",id,w);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSrgt\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSlft\"><IMG SRC=/sys/images/trans_1.gif height=%POS width=1></DIV>\n",id,(h<0)?1:h);
	    htrAddBodyItem(s,   "</DIV>\n");
	    }
	else if(s->Capabilities.CSS2)
	    {
	    if(h >=0 )
		{
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx %INTpx %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,h-1-2*box_offset+2*clip_offset,0);
		htrAddStylesheetItem_va(s,"\t#tb%POSpane2, #tb%POSpane3 { height: %POSpx;}\n",id,id,h-3);
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { height: %POSpx;}\n",id,h-1-2*box_offset);
		}
	    else
		{
		htrAddStylesheetItem_va(s,"\t#tb%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; OVERFLOW:hidden; clip:rect(%INTpx %INTpx auto %INTpx)}\n",id,x,y,w-1-2*box_offset,z,0,w-1-2*box_offset+2*clip_offset,0);
		}
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane, #tb%POSpane2, #tb%POSpane3 { cursor:default; text-align: center; }\n",id,id,id);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane { %STR border-width: 1px; border-style: solid; border-color: white gray gray white; }\n",id,bgstyle);
	    /*htrAddStylesheetItem_va(s,"\t#tb%dpane { color: %s; }\n",id,fgcolor2);*/
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane2 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:-1px; top: -1px; width:%POSpx; }\n",id,is_enabled?"inherit":"hidden",z+1,w-3);
	    htrAddStylesheetItem_va(s,"\t#tb%POSpane3 { VISIBILITY: %STR; Z-INDEX: %INT; position: absolute; left:0px; top: 0px; width:%POSpx; }\n",id,is_enabled?"hidden":"inherit",z+1,w-3);

	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center>\n",id,h-3,fgcolor2,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane2\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,fgcolor1,text);
	    htrAddBodyItem_va(s,"<DIV ID=\"tb%POSpane3\"><center><table cellspacing=0 cellpadding=1 border=0><tr><td height=%POS valign=middle align=center><font color=\"%STR&HTE\"><b>%STR&HTE</b></font></td></tr></table></center></DIV>\n",id,h-3,disable_color,text);
	    htrAddBodyItem(s,   "</DIV>");

	    /** Script initialization call. **/
	    htrAddScriptInit_va(s, "    tb_init({layer:%STR&SYM, layer2:htr_subel(%STR&SYM, \"tb%POSpane2\"), layer3:htr_subel(%STR&SYM, \"tb%POSpane3\"), top:null, bottom:null, right:null, left:null, width:%INT, height:%INT, tristate:%INT, name:\"%STR&SYM\", text:'%STR&JSSTR'});\n",
		    dptr, dptr, id, dptr, id, w, h, is_ts, name, text);
	    }
	else
	    {
	    mssError(0,"HTTBTN","Unable to render for this browser");
	    return -1;
	    }

	/** Add the event handling scripts **/
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "tb", "tb_mousedown");
	htrAddEventHandlerFunction(s, "document", "MOUSEUP", "tb", "tb_mouseup");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "tb", "tb_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "tb", "tb_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "tb", "tb_mousemove");

	/** IE handles dblclick strangely **/
	if (s->Capabilities.Dom0IE)
	    htrAddEventHandlerFunction(s, "document", "DBLCLICK", "tb", "tb_dblclick");

	/** Check for more sub-widgets within the textbutton. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

    return 0;
    }


/*** httbtnInitialize. - register with the ht_render module.
 ***/
int
httbtnInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Text Button Widget Driver");
	strcpy(drv->WidgetName,"textbutton");
	drv->Render = httbtnRender;

	/** Add the 'click' event **/
	htrAddEvent(drv, "Click");
	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBTN.idcnt = 0;

    return 0;
    }
