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
/* Module: 	wgtr/wgtdrv_menu.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/



/*** wgtmenuVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtmenuVerify(pWgtrVerifySession s)
    {
    pWgtrNode menu = s->CurrWidget;
    char* str;
    int i;
    int min_height = s->ClientInfo->ParagraphHeight + 4;

	if (menu->min_height < min_height) menu->min_height = min_height;

	if (menu->Parent && !strcmp(menu->Parent->Type, "widget/menu"))
	    {
	    menu->Flags |= WGTR_F_FLOATING;
	    if (wgtrGetPropertyType(menu, "popup") < 0)
		{
		str = "yes";
		wgtrAddProperty(menu, "popup", DATA_T_STRING, POD(&str), 0);
		}
	    if (wgtrGetPropertyType(menu, "direction") < 0)
		{
		str = "vertical";
		wgtrAddProperty(menu, "direction", DATA_T_STRING, POD(&str), 0);
		}
	    }
	if (wgtrGetPropertyValue(menu, "popup", DATA_T_STRING, POD(&str)) == 0)
	    {
	    if (!strcasecmp(str,"yes") || !strcasecmp(str,"true") || !strcasecmp(str,"on"))
		menu->Flags |= WGTR_F_FLOATING;
	    }
	else if (wgtrGetPropertyValue(menu, "popup", DATA_T_INTEGER, POD(&i)) == 0 && i)
	    menu->Flags |= WGTR_F_FLOATING;

    return 0;
    }


/*** wgtmenuNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtmenuNew(pWgtrNode node)
    {

	if(node->fl_width < 0) node->fl_width = 25;
	if(node->fl_height < 0) node->fl_height = 1;
	
    return 0;
    }


int
wgtmenuInitialize()
    {
    char* name = "Menu Widget Driver";

	wgtrRegisterDriver(name, wgtmenuVerify, wgtmenuNew);
	wgtrAddType(name, "menu");
	wgtrAddType(name, "menuitem");
	wgtrAddType(name, "menusep");
	wgtrAddType(name, "menutitle");

	return 0;
    }
