#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/datatypes.h"
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
/* Module: 	wgtr/wgtdrv_html.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/



/*** wgthtmlVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgthtmlVerify(pWgtrVerifySession s)
    {
    /** This sets the flexibility of the html widget based on whether
    *** it is static or dynamic, unless the user has already specified
    *** a flexibility in which case it is left alone. This process
    *** would usually be done in the wgthtmlNew function, but that's 
    *** not possible in this case because the static/dynamic property 
    *** isn't set until after the wgthtmlNew function has been run. 
    **/
    ObjData val;
    int Static;
    wgtrGetPropertyValue(s->CurrWidget, "mode", DATA_T_STRING, &val);
    Static = ((val.String == NULL) || !strcmp(val.String, "static"));
    
    if(s->CurrWidget->fl_width < 0)
        {
            if(Static)
                s->CurrWidget->fl_width = 0;
            else
                s->CurrWidget->fl_width = 100;
	}
    
    if(s->CurrWidget->fl_height < 0)
        {
            if(Static)
                s->CurrWidget->fl_height = 0;
            else
                s->CurrWidget->fl_height = 100;
	}
	
    return 0;
    }


/*** wgthtmlNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgthtmlNew(pWgtrNode node)
    {
    return 0;
    }


int
wgthtmlInitialize()
    {
    char* name = "Textual Source Driver";

	wgtrRegisterDriver(name, wgthtmlVerify, wgthtmlNew);
	wgtrAddType(name, "html");

	return 0;
    }
