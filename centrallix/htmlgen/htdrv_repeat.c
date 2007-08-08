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
/* Module: 	htdrv_repeat.c	         				*/
/* Author:	David Kasper (DRK)					*/
/* Creation:	July 26, 2007	 					*/
/* Description:	Simple Driver for the repeat widget.  All it does is	*/
/*		render its subwidgets.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_repeat.c,v $
    Revision 1.2  2007/08/08 23:17:09  dkasper
    - Added the correct widget tree linkage so that subobjects can find the
      correct container.

    Revision 1.1  2007/07/27 00:24:50  dkasper
    - Simple driver for the repeat widget.  All it does is register itself
      properly and render all its subwidgets since it is not a visual widget.


 **END-CVSDATA***********************************************************/

/** Globals **/

static struct
    {
    int idcnt;
    } HTRPT;

int
htrptRender(pHtSession s, pWgtrNode tree, int z)
    {
	/** Render Subwidgets **/
	htrAddWgtrCtrLinkage(s, tree, "_parentctr");
	htrRenderSubwidgets(s,tree,z);
    }

int
htrptInitialize()
    {
    pHtDriver drv;
    
    /** Allocate the driver **/
    drv = htrAllocDriver();
    if (!drv) return -1;
    
    /** Fill in structure **/
    strcpy(drv->Name,"Repeat Object Driver");
    strcpy(drv->WidgetName,"repeat");
    drv->Render = htrptRender;

    /** Register **/
    htrRegisterDriver(drv);

    htrAddSupport(drv, "dhtml");

    return 0;
    }
