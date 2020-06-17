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
#include "wgtr.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_formstatus.c                                      */
/* Author:      Matt McGill (MJM)                                       */
/* Creation:    July 19, 2004                                           */
/* Description: HTML Widget driver for a form bar, basically a tech     */
/*		    test for widgets adding subwidgets on the fly.      */
/************************************************************************/



/* 
   htfbRender - generate the HTML code for the page.
*/
int htfbRender(pHtSession s, pWgtrNode tree, int z) {
   int i;

   if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
       {
       mssError(1,"HTFS","Netscape DOM or W3C DOM1 HTML and CSS1 support required");
       return -1;
       }

    htrAddWgtrCtrLinkage(s, tree, "_parentctr");

    /** mark this node as not being associated with a DHTML object **/
    tree->RenderFlags |= HT_WGTF_NOOBJECT;

    for (i=0;i<xaCount(&(tree->Children));i++)
	htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2);

   return 0;
}


/* 
   htfsInitialize - register with the ht_render module.
*/
int htfbInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Form Bar Driver");
   strcpy(drv->WidgetName,"formbar");
   drv->Render = htfbRender;

/*
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
*/
   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   return 0;
}
