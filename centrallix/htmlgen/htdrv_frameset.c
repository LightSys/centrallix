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
#include "cxlib/qprintf.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_frameset.c           				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 17, 1999   					*/
/* Description:	HTML Widget driver for a frameset; use instead of a	*/
/*		widget/page item at the top level.			*/
/************************************************************************/



/*** htsetRender - generate the HTML code for the page.
 ***/
int
htsetRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    pWgtrNode sub_tree;
    char geom_str[64] = "";
    int t,n,bdr=0,direc=0, i;
    char nbuf[16];

    	/** Check for a title. **/
	if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s,"    <TITLE>%STR&HTE</TITLE>\n",ptr);
	    }

	/** Loop through the frames (widget/page items) for geometry data **/
	htrDisableBody(s);
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    t = wgtrGetPropertyType(sub_tree, "framesize");
	    if (t < 0) 
		{
		strcpy(nbuf,"*");
		}
	    else if (t == DATA_T_INTEGER)
		{
		wgtrGetPropertyValue(sub_tree, "framesize", DATA_T_INTEGER,POD(&n));
		qpfPrintf(NULL, nbuf,sizeof(nbuf),"%INT",n);
		}
	    else if (t == DATA_T_STRING)
		{
		wgtrGetPropertyValue(sub_tree, "framesize", DATA_T_STRING,POD(&ptr));
		strtcpy(nbuf, ptr, sizeof(nbuf));
		}
	    if (geom_str[0] != '\0')
		{
		strcat(geom_str,",");
		}
	    if (strlen(geom_str) + strlen(nbuf) + 2 >= sizeof(geom_str))
		{
		mssError(1,"HTSET","Internal storage exhausted");
		return -1;
		}
	    strcat(geom_str,nbuf);
	    }

	/** Check for some optional params **/
	if (wgtrGetPropertyValue(tree,"direction",DATA_T_STRING,POD(&ptr)) != 0)
	    {
	    if (!strcmp(ptr,"rows")) direc=1;
	    else if (!strcmp(ptr,"columns")) direc=0;
	    }
	if (wgtrGetPropertyValue(tree,"borderwidth",DATA_T_INTEGER,POD(&n)) != 0)
	    { 
	    bdr = n;
	    }

	/** Build the frameset tag. **/
	htrAddBodyItem_va(s, "<FRAMESET %STR=%STR&HTE border=%POS>\n", direc?"rows":"cols", geom_str, bdr);

	/** Check for more sub-widgets within the page. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    wgtrGetPropertyValue(sub_tree,"name",DATA_T_STRING,POD(&ptr));
	    if (wgtrGetPropertyValue(sub_tree,"marginwidth",DATA_T_INTEGER,POD(&n)) != 0)
		htrAddBodyItem_va(s,"    <FRAME SRC=\"./%STR&HTE\">\n",ptr);
	    else
		htrAddBodyItem_va(s,"    <FRAME SRC=\"./%STR&HTE\" MARGINWIDTH=%POS>\n",ptr,n);
	    }

	/** End the framset. **/
	htrAddBodyItem(s, "</FRAMESET>\n");

    return 0;
    }


/*** htsetInitialize - register with the ht_render module.
 ***/
int
htsetInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Frameset Driver");
	strcpy(drv->WidgetName,"frameset");
	drv->Render = htsetRender;
	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    return 0;
    }
