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
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_autolayout.c      				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 21, 2007   					*/
/* Description:	HTML Widget driver for autolayout of its subwidgets.	*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTAL;


int
htalRenderSpacer(pHtSession s, pWgtrNode tree, int z)
    {

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

    return 0;
    }

/*** htalRender - generate the HTML code for the page.
 ***/
int
htalRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    int x=-1,y=-1,w,h;
    int id;
    pWgtrNode subtree;
    int i;

	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTAL","Netscape DOM or W3C DOM1 HTML and CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTAL.idcnt++);

    	/** Get x,y,w,h of this object.  X and Y can be assumed to be zero if unset,
	 ** but the wgtr Verify routine should have taken care of width/height for us
	 ** if those were unspecified.  Bark!
	 **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(1,"HTAL","Bark! Autolayout widget must have a 'width' property");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(1,"HTAL","Bark! Autolayout widget must have a 'height' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Add the stylesheet for the layer **/
	htrAddStylesheetItem_va(s,"\t#al%POSbase { POSITION:absolute; VISIBILITY:inherit; OVERFLOW:visible; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; CLIP:rect(%INTpx,%INTpx,%INTpx,%INTpx); Z-INDEX:%POS; }\n",
		id,x,y,w,h,
		-1, w+1, h+1, -1,
		z);

	/** Linkage **/
	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"al%POSbase\")",id);

	/** Script include call **/
	htrAddScriptInclude(s, "/sys/js/htdrv_autolayout.js", 0);

	/** Script initialization call. **/
	htrAddScriptInit_va(s, "    al_init(nodes['%STR&SYM'], {});\n", name);

	/** Start of container **/
	htrAddBodyItemLayerStart(s, 0, "al%POSbase", id);

	/** Check for objects within this autolayout widget. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    subtree = xaGetItem(&(tree->Children), i);
	    if (!strcmp(subtree->Type, "widget/autolayoutspacer")) 
		subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	    /*
	    else if(!strcmp(subtree->Type, "widget/repeat"))
		{
		for(rpti=0;rpti<xaCount(&(subtree->Children));rpti++)
		    {
		    rptsubtree = xaGetItem(&(subtree->Children),rpti);
		    htrRenderWidget(s,rptsubtree, z+1);
		    //mssError(1,"HTAL","Found a subwidget to a repeat");
		    }
		    htrRenderWidget(s, subtree, z+1);
		}*/
	    else
		{
		htrRenderWidget(s, subtree, z+1);
		//mssError(1,"HTAL","Found a subwidget");
		}
	    }
	/** End of container **/
	htrAddBodyItemLayerEnd(s, 0);

    return 0;
    }


/*** htalInitialize - register with the ht_render module.
 ***/
int
htalInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver");
	strcpy(drv->WidgetName,"autolayout");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - VBox");
	strcpy(drv->WidgetName,"vbox");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - HBox");
	strcpy(drv->WidgetName,"hbox");
	drv->Render = htalRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Autolayout Driver - Spacer");
	strcpy(drv->WidgetName,"autolayoutspacer");
	drv->Render = htalRenderSpacer;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTAL.idcnt = 0;

    return 0;
    }
