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
/* Module: 	wgtr/wgtdrv_page.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/

/**CVSDATA***************************************************************
 

 **END-CVSDATA***********************************************************/


/*** wgtpageVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtpageVerify(pWgtrVerifySession s)
    {
    int i;
    pWgtrNode cld;
    int maxw, maxh;

	/** force these for now **/
	s->CurrWidget->r_x = 0;
	s->CurrWidget->r_y = 0;

	if((s->CurrWidget->r_width < 0) || (s->CurrWidget->r_height < 0))
	    {
	    /** Absolute minimum size **/
	    maxw = 1;
	    maxh = 1;

	    /** Scan child widgets to determine width and height **/
	    for(i=0;i<s->CurrWidget->Children.nItems;i++)
		{
		cld = (pWgtrNode)(s->CurrWidget->Children.Items[i]);
		if (cld->r_x + cld->r_width > maxw)
		    maxw = cld->r_x + cld->r_width;
		if (cld->r_y + cld->r_height > maxh)
		    maxh = cld->r_y + cld->r_height;
		}

	    /*mssError(1, "WGTR", "wgtpageVerify(): The design 'width' and/or 'height' of the page '%s' were not specified", s->CurrWidget->Name);
	    return -1;*/
	    if (s->CurrWidget->r_width < 0) s->CurrWidget->r_width = maxw;
	    if (s->CurrWidget->r_height < 0) s->CurrWidget->r_height = maxh;
	    }
	
    return 0;
    }


/*** wgtpageNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtpageNew(pWgtrNode node)
    {
	node->Flags |= WGTR_F_CONTAINER;   
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;
	
    return 0;
    }


int
wgtpageInitialize()
	{
	char* name="Page Driver";

	    wgtrRegisterDriver(name, wgtpageVerify, wgtpageNew);
	    wgtrAddType(name, "page");

	    return 0;
	}
