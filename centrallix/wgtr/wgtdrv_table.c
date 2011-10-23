#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "wgtr.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2008 LightSys Technology Services, Inc.		*/
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
/* Module: 	wgtr/wgtdrv_table.c					*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/



/*** wgttblVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgttblVerify(pWgtrVerifySession s)
    {
    pWgtrNode tbl = s->CurrWidget;

	if (!strcmp(tbl->Type, "widget/table-row-detail"))
	    {
	    tbl->width = tbl->r_width = tbl->pre_width = tbl->Parent->r_width;
	    tbl->x = tbl->r_x = tbl->pre_x = 0;
	    tbl->y = tbl->r_y = tbl->pre_y = 0;
	    }

    return 0;
    }


/*** wgttblNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgttblNew(pWgtrNode node)
    {   

	if (!strcmp(node->Type, "widget/table"))
	    {
	    if(node->fl_width < 0) node->fl_width = 100;
	    if(node->fl_height < 0) node->fl_height = 100;
	    node->Flags |= WGTR_F_CONTAINER;
	    }
	else if (!strcmp(node->Type, "widget/table-row-detail"))
	    {
	    if(node->fl_width < 0) node->fl_width = 100;
	    if(node->fl_height < 0) node->fl_height = 0;
	    node->Flags |= WGTR_F_CONTAINER;
	    }
	else if (!strcmp(node->Type, "widget/table-column"))
	    {
	    node->Flags |= WGTR_F_NONVISUAL;
	    }
	
    return 0;
    }


int
wgttblInitialize()
    {
    char* name = "DataTable Driver";

	wgtrRegisterDriver(name, wgttblVerify, wgttblNew);
	wgtrAddType(name, "table");
	wgtrAddType(name, "table-column");
	wgtrAddType(name, "table-row-detail");

	return 0;
    }
