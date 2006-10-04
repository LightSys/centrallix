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
/* Module: 	wgtr/wgtdrv_tab.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/

/**CVSDATA***************************************************************
 

 **END-CVSDATA***********************************************************/


/*** wgttabVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgttabVerify(pWgtrVerifySession s)
    {
    /*** Loops through the tabpage children of the tab widget and
     *** initializes the requested and actual geometry of each 
     *** one to match that of the tab widget itself, minus a few
     *** pixels to account for the border. Necessary for auto-
     *** positioning.
     ***/
    pWgtrNode tab = s->CurrWidget;
    pWgtrNode tabpage;
    int i=0;
    int count = xaCount(&(tab->Children));
    
	if(!strcmp(tab->Type, "widget/tab"))
	    for(i=0; i<count; ++i)	//loop through tabpage children
	        {
		    tabpage = (pWgtrNode)(xaGetItem(&(tab->Children), i));
		    if(!strcmp(tabpage->Type, "widget/tabpage"))
		        {
			    tabpage->r_x = tabpage->r_y = 0;
			    tabpage->r_width = tab->r_width;
			    tabpage->r_height = tab->r_height;
			    tabpage->pre_width = tab->pre_width;
			    tabpage->pre_height = tab->pre_height;
			    tabpage->min_width = tab->min_width;
			    tabpage->min_height = tab->min_height;
			
			    tabpage->x = tabpage->y = 0;
			    tabpage->width = tab->width;
			    tabpage->height = tab->height;
		        }
		}
	    
    return 0;
    }


/*** wgttabNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgttabNew(pWgtrNode node)
    {
	node->Flags |= WGTR_F_CONTAINER;
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;
	
    return 0;
    }


int
wgttabInitialize()
    {
    char* name = "Tab Control / Tab Page Driver";

	wgtrRegisterDriver(name, wgttabVerify, wgttabNew);
	wgtrAddType(name, "tab");
	wgtrAddType(name, "tabpage");

	return NULL;
    }
