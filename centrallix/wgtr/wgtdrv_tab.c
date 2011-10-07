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



int
wgttab_internal_SetTabpageGeom(pWgtrNode tab, pWgtrNode container)
    {
    int i=0;
    int count = xaCount(&(container->Children));
    pWgtrNode child;

	    for(i=0; i<count; ++i)	//loop through tab children
	        {
		    child = (pWgtrNode)(xaGetItem(&(container->Children), i));
		    if(!strcmp(child->Type, "widget/tabpage"))
		        {
			    /** Set geometry of tabpage, based on the tab control itself **/
			    child->r_x = child->r_y = 0;
			    child->r_width = tab->r_width;
			    child->r_height = tab->r_height;
			    child->pre_width = tab->pre_width;
			    child->pre_height = tab->pre_height;
			    child->min_width = tab->min_width;
			    child->min_height = tab->min_height;
			
			    child->x = child->y = 0;
			    child->width = tab->width;
			    child->height = tab->height;
		        }
		    else if ((child->Flags & WGTR_F_NONVISUAL) && (child->Flags & WGTR_F_CONTAINER))
			{
			    /** Might be more tabpages within a nonvisual container like widget/repeat **/
			    wgttab_internal_SetTabpageGeom(tab, child);
			}
		}

    return 0;
    }


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
    
	if(!strcmp(tab->Type, "widget/tab"))
	    wgttab_internal_SetTabpageGeom(tab, tab);
	    
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
    int tloc;
    char* ptr;
    int tw;

	node->Flags |= WGTR_F_CONTAINER;
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;
	
	/** Set border widths if this is a tab control widget **/
	if(!strcmp(node->Type, "widget/tab"))
	    {
	    if (wgtrGetPropertyValue(node, "tab_location", DATA_T_STRING, POD(&ptr)) == 0)
		{
		if (!strcasecmp(ptr,"top")) tloc = 0;
		else if (!strcasecmp(ptr,"bottom")) tloc = 1;
		else if (!strcasecmp(ptr,"left")) tloc = 2;
		else if (!strcasecmp(ptr,"right")) tloc = 3;
		else if (!strcasecmp(ptr,"none")) tloc = 4;
		else
		    {
		    mssError(1,"WGTTAB","%s: '%s' is not a valid tab_location",node->Name,ptr);
		    return -1;
		    }
		}
	    else
		{
		tloc = 0;
		}

	    if (wgtrGetPropertyValue(node, "tab_width", DATA_T_INTEGER, POD(&tw)) != 0 || tw <= 0)
		{
		if (tloc == 2 || tloc == 3)
		    {
		    mssError(1,"WGTTAB","%s: must specify a valid tab width when tab location is left or right",node->Name);
		    return -1;
		    }
		}

	    node->top = node->bottom = node->left = node->right = 1;
	    switch(tloc)
		{
		case 0:	/* top */
		    node->top = 25;
		    break;
		case 1:	/* bottom */
		    node->bottom = 25;
		    break;
		case 2:	/* left */
		    node->left = tw;
		    break;
		case 3:
		    node->right = tw;
		    break;
		default:
		    break;
		}
	    }

    return 0;
    }


int
wgttabInitialize()
    {
    char* name = "Tab Control / Tab Page Driver";

	wgtrRegisterDriver(name, wgttabVerify, wgttabNew);
	wgtrAddType(name, "tab");
	wgtrAddType(name, "tabpage");

    return 0;
    }
