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
/* Module: 	wgtr/wgtdrv_scrollbar.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/



/*** wgtsbVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtsbVerify(pWgtrVerifySession s)
    {
    char* str;
    pWgtrNode sb;

	sb = s->CurrWidget;
	if (wgtrGetPropertyValue(sb, "direction", DATA_T_STRING, POD(&str)) == 0)
	    {
	    if (!strcmp(str, "vertical")) 
		{
		sb->pre_width = sb->width = sb->r_width = 18;
		sb->fl_width = 0;
		}
	    if (!strcmp(str, "horizontal")) 
		{
		sb->pre_height = sb->height = sb->r_height = 18;
		sb->fl_height = 0;
		}
	    }

    return 0;
    }


/*** wgtsbNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtsbNew(pWgtrNode node)
    {
        if(node->fl_width < 0) node->fl_width = 100;
        if(node->fl_height < 0) node->fl_height = 100;
    
    return 0;
    }


int
wgtsbInitialize()
    {
    char* name = "Scrollbar Widget Driver";

	wgtrRegisterDriver(name, wgtsbVerify, wgtsbNew);
	wgtrAddType(name, "scrollbar");

	return 0;
    }
