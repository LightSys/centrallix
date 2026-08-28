/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	wgtr/wgtdrv_window.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/

#include <string.h>

#include "cxlib/datatypes.h"
#include "wgtr.h"


/*** wgtwinVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtwinVerify(pWgtrVerifySession s)
    {
    /*pWgtrNode window = s->CurrWidget;*/

    return 0;
    }


/*** wgtwinNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtwinNew(pWgtrNode node)
    {
    int has_titlebar = 1, is_dialog_style = 0, border_width = 1;
    int title_bar_height, main_top_width, main_side_width;
    char* ptr;

	node->Flags |= WGTR_F_CONTAINER | WGTR_F_FLOATING | WGTR_F_VISUAL_CONTAINER;
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;
	
        /** No titlebar? **/
        if (wgtrGetPropertyValue(node,"titlebar",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no"))
            has_titlebar = 0;

        /** Dialog or node style? **/
        if (wgtrGetPropertyValue(node,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
            is_dialog_style = 1;

	/** A borderless window draws no outer edge. **/
	if (wgtrGetPropertyValue(node,"border_style",DATA_T_STRING,POD(&ptr)) == 0
	    && (!strcmp(ptr,"none") || !strcmp(ptr,"hidden")))
	    border_width = 0;

	/*** Declare the insets around our client area (#wnNmain) that holds
	 *** child widgets in the window.  These values must match the geometry
	 *** that htdrv_window.c gives this layer.
	 ***
	 *** Both layers are border-box, so every edge is drawn inside the width
	 *** and height the window was given.  The client area is therefore inset
	 *** by the window's own outer edge, by the titlebar above it, and by
	 *** whichever edges htdrv_window.c draws on the client layer itself.
	 ***/
	title_bar_height = (has_titlebar) ? 24 : 0;
	if (is_dialog_style)
	    {
	    main_side_width = 0;
	    main_top_width = (has_titlebar) ? 1 : 0;
	    }
	else
	    {
	    main_side_width = 1;
	    main_top_width = (has_titlebar) ? 0 : 1;
	    }
	wgtrSetInsets(node,
	    border_width + title_bar_height + main_top_width,	/* top */
	    border_width + main_side_width,			/* bottom */
	    border_width + main_side_width,			/* left */
	    border_width + main_side_width			/* right */
	);

    return 0;
    }


int
wgtwinInitialize()
    {
    char* name = "Window Widget Driver";

	wgtrRegisterDriver(name, wgtwinVerify, wgtwinNew);
	wgtrAddType(name, "childwindow");

	return 0;
    }
