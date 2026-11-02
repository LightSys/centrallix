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
/* Module: 	wgtdrv_button.c						*/
/* Author:	D. Kasper			 			*/
/* Creation:	June 21, 2007						*/
/* Description:	Widget driver for a 'generic' button widget to replace	*/
/* 		the (now deprecated) imagebutton/textbutton widgets.	*/
/************************************************************************/

#include <string.h>

#include "cxlib/datatypes.h"
#include "wgtr.h"


/*** wgtbtnVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtbtnVerify(pWgtrVerifySession s)
    {
    pWgtrNode this = s->CurrWidget;
    int min_height = s->ClientInfo->ParagraphHeight + 4;
    int est_height;
    int line_count = 1;
    char* text;

	/** 'image_margin' is the deprecated textbutton spelling of 'spacing'. **/
	wgtrRenameProperty(this, "image_margin", "spacing");

	/*** Only a button carrying text sizes itself to that text.  An image-only
	 *** button is as big as its image, and forcing it up to a line of text
	 *** would stretch every icon-sized button on the page.
	 ***/
	if (wgtrGetPropertyType(this, "text") <= 0) return 0;

	if (this->min_height < min_height) this->min_height = min_height;

	/*** A button with no height sizes itself to its text in the browser, so
	 *** estimate how many lines that text wraps onto at the button's width.
	 ***/
	if (this->height < 0)
	    {
	    if (this->width > 0 && wgtrGetPropertyValue(this, "text", DATA_T_STRING, POD(&text)) == 0)
		/** Guess line count, adding `this->width - 1` so the int division rounds up. **/
		line_count = (strlen(text) * s->ClientInfo->CharWidth + this->width - 1) / this->width;

	    est_height = min_height + (line_count - 1) * s->ClientInfo->ParagraphHeight;
	    this->Flags |= WGTR_F_AUTOHEIGHT;
	    this->height = this->pre_height = (est_height > min_height) ? est_height : min_height;
	    }

    return 0;
    }


/*** wgtbtnNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtbtnNew(pWgtrNode node)
    {
	/*** widget/textbutton has always defaulted to a flexible width, and
	 *** existing layouts depend on it.  The other two names default rigid.
	 ***/
	if (strcmp(node->Type, "widget/textbutton") == 0)
	    {
	    if(node->fl_width < 0) node->fl_width = 5;
	    if(node->fl_height < 0) node->fl_height = 1;
	    }
	else
	    {
	    if(node->fl_width < 0) node->fl_width = 0;
	    if(node->fl_height < 0) node->fl_height = 0;
	    }
	
	return wgtrImplementsInterface(node, "net/centrallix/button.ifc?cx__version=1.1");
	
    }


int
wgtbtnInitialize()
    {
    char* name = "Button Widget Driver";
    
	wgtrRegisterDriver(name, wgtbtnVerify, wgtbtnNew);
	wgtrAddType(name, "button");

	/*** The deprecated textbutton and imagebutton names remain part of the
	 *** language and use this driver.  Their own drivers are gone.
	 ***/
	wgtrAddType(name, "textbutton");
	wgtrAddType(name, "imagebutton");

	return 0;
    }
