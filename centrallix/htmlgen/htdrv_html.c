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
/* Module: 	htdrv_html.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 3, 1998 					*/
/* Description:	HTML Widget driver for HTML text, either given as an	*/
/*		immediate string or pulled from an ObjectSystem source	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTHTML;


/*** hthtmlRender - generate the HTML code for the page.
 ***/
int
hthtmlRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char sbuf[320];
    char src[128] = "";
    int x=-1,y=-1,w,h;
    int id,cnt, i;
    int mode = 0;
    pObject content_obj;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTHTML","NS4 or W3C DOM support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTHTML.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=-1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=-1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTHTML","HTML widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;

	/** Get source html objectsystem entry. **/
	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(src,ptr,sizeof(src));
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    /** Only give x and y if supplied. **/
	    if (x < 0 || y < 0)
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { POSITION:relative; VISIBILITY:hidden; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,w,z+1);
	        }
	    else
	        {
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,w,z+1);
	        }

	    if (s->Capabilities.CSS1)
		{
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane { overflow:hidden; }\n",id);
	        htrAddStylesheetItem_va(s,"\t#ht%POSpane2 { overflow:hidden; }\n",id);
	        htrAddStylesheetItem_va(s,"\t#ht%POSfader { overflow:hidden; }\n",id);
		htrAddStylesheetItem_va(s,"\t#ht%POSloader { overflow:hidden; visibility:hidden; position:absolute; top:0px; left:0px; width:0px; height:0px; }\n", id);
		}

            /** Write named global **/
	    htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"ht%POSpane\")",id);

            htrAddScriptGlobal(s, "ht_fadeobj", "null", 0);
    
	    htrAddScriptInclude(s, "/sys/js/htdrv_html.js", 0);
	    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	    /** Event handler for click-on-link. **/
	    htrAddEventHandlerFunction(s, "document","CLICK","ht","ht_click");
	    htrAddEventHandlerFunction(s,"document","MOUSEOVER","ht","ht_mouseover");
	    htrAddEventHandlerFunction(s,"document","MOUSEOUT", "ht", "ht_mouseout");
	    htrAddEventHandlerFunction(s,"document","MOUSEMOVE","ht", "ht_mousemove");
	    htrAddEventHandlerFunction(s,"document","MOUSEDOWN","ht", "ht_mousedown");
	    htrAddEventHandlerFunction(s,"document","MOUSEUP",  "ht", "ht_mouseup");

            /** Script initialization call. **/
	    if (s->Capabilities.Dom0NS)
		htrAddScriptInit_va(s,"    ht_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), layer2:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")),\"ht%POSpane2\"), faderLayer:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")),\"ht%POSfader\"), source:\"%STR&JSSTR\", width:%INT, height:%INT, loader:null});\n",
                    name, name, id, name, id, 
		    src, w,h);
	    else
		htrAddScriptInit_va(s,"    ht_init({layer:wgtrGetNodeRef(ns,\"%STR&SYM\"), layer2:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")),\"ht%POSpane2\"), faderLayer:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")),\"ht%POSfader\"), source:\"%STR&JSSTR\", width:%INT, height:%INT, loader:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")), \"ht%POSloader\")});\n",
                    name, name, id, name, id, 
		    src, w,h, name, id);
    
            /** HTML body <DIV> element for the layer. **/
            htrAddBodyItem_va(s,"<DIV background=\"/sys/images/fade_lrwipe_01.gif\" ID=\"ht%POSfader\"></DIV>",id);
	    htrAddBodyItemLayer_va(s, 0, "ht%POSpane2", id, "");
	    if (!s->Capabilities.Dom0NS)
		htrAddBodyItemLayer_va(s, HTR_LAYER_F_DYNAMIC, "ht%POSloader", id, "");
	    htrAddBodyItemLayerStart(s, 0, "ht%POSpane", id);
	    }
	else
	    {
	    tree->RenderFlags |= HT_WGTF_NOOBJECT;
	    }

        /** If prefix text given, put it. **/
        if (wgtrGetPropertyValue(tree, "prologue", DATA_T_STRING,POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If full text given, put it. **/
        if (wgtrGetPropertyValue(tree, "content", DATA_T_STRING,POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If source is an objectsystem entry... **/
        if (src[0] && strncmp(src,"http:",5) && strncmp(src,"debug:",6))
            {
            content_obj = objOpen(s->ObjSession,src,O_RDONLY,0600,"text/html");
            if (content_obj)
                {
                while((cnt = objRead(content_obj, sbuf, 159,0,0)) > 0)
                    {
                    sbuf[cnt]=0;
                    htrAddBodyItem(s, sbuf);
                    }
                objClose(content_obj);
                }
            }

        /** If post text given, put it. **/
        if (wgtrGetPropertyValue(tree, "epilogue", DATA_T_STRING, POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

	/** render subwidgets **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+3);

        /** End the containing layer. **/
        if (mode == 1) htrAddBodyItemLayerEnd(s, 0);

    return 0;
    }


/*** hthtmlInitialize - register with the ht_render module.
 ***/
int
hthtmlInitialize()
    {
    pHtDriver drv;

        /** Allocate the driver **/
        drv = htrAllocDriver();
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"HTML Textual Source Driver");
        strcpy(drv->WidgetName,"html");
        drv->Render = hthtmlRender;

	htrAddEvent(drv, "MouseUp");
	htrAddEvent(drv, "MouseDown");
	htrAddEvent(drv, "MouseOver");
	htrAddEvent(drv, "MouseOut");
	htrAddEvent(drv, "MouseMove");

        /** Add the 'load page' action **/
	htrAddAction(drv,"LoadPage");
	htrAddParam(drv,"LoadPage","Mode",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Source",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Transition",DATA_T_STRING);

        /** Register. **/
        htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

        HTHTML.idcnt = 0;

    return 0;
    }
