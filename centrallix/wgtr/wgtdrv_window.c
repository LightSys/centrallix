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
    int has_titlebar = 1, is_dialog_style = 0;
    int title_bar_height, inset_left;
    char* ptr;

	node->Flags |= WGTR_F_CONTAINER | WGTR_F_FLOATING;
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;
	
        /** No titlebar? **/
        if (wgtrGetPropertyValue(node,"titlebar",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"no"))
            has_titlebar = 0;

        /** Dialog or node style? **/
        if (wgtrGetPropertyValue(node,"style",DATA_T_STRING,POD(&ptr)) == 0 && !strcmp(ptr,"dialog"))
            is_dialog_style = 1;

	/*** Declare the insets around our client area (#wnNmain) that holds
	 *** child widgets in the window.  These values must match the geometry
	 *** that htdrv_window.c gives this layer.
	 ***
	 *** Vertically, the layer starts just below the titlebar and stops one
	 *** or two pixels short of the bottom edge.  Horizontally it is always
	 *** two pixels narrower than the window. (For window style: it is inset
	 *** by its own 1px border on each side. For dialog style: it has no
	 *** side borders and sits flush against the left edge.)
	 ***/
	title_bar_height = (has_titlebar) ? ((is_dialog_style) ? 24 : 23) : 0;
	inset_left = (is_dialog_style) ? 0 : 1;
	wgtrSetInsets(node,
	    title_bar_height,				/* top */
	    (is_dialog_style || has_titlebar) ? 1 : 2,	/* bottom */
	    inset_left,					/* left */
	    2 - inset_left				/* right */
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
