#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_new.h"
#include "prtmgmt_private.h"
#include "htmlparse.h"
#include "cxlib/magic.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_*.c, prtmgmt_new.h				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 25th, 2000  					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_layout_string.c - provides layout-	*/
/*		specific functionality for string objstream elements.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_layout_string.c,v 1.2 2005/02/26 06:42:40 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_layout_string.c,v $

    $Log: prtmgmt_layout_string.c,v $
    Revision 1.2  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1.1.1  2001/08/13 18:01:15  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:18  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** prtStringContentChanged - this function is a driver API routine which
 *** is called by the print management layout module whenever the content
 *** of the string is changed.  For strings, this happens when a string's
 *** text is changed, added to, or deleted from.
 ***
 *** Return = 0 if the geometry of the string did not change, or
 ***          1 if the geometry of the string DID change.
 ***/
int
prtStringContentChanged(pPrtSession s, pPrtObjStream os, pPrtObjStream changed)
    {
    double old_width;
    old_width = os->Width;
    os->Width = prt_internal_DetermineTextWidth(s, os->String.Text, os->String.Length);
    return (old_width != os->Width);
    }


/*** prtStringInit - driver API routine which initializes a string element
 ***/
int
prtStringInit(pPrtObjStream os)
    {
    os->String.Length = 0;
    os->String.Text = NULL;
    os->Width = 0.0;
    return 0;
    }


/*** prtStringRelease - driver API routine which releases (oppposite of
 *** Init) a string element.  Doesn't actually free the element itself, that
 *** is done by the print management module's routines.
 ***/
int
prtStringRelease(pPrtObjStream os)
    {
    if (os->String.Text) nmSysFree(os->String.Text);
    os->String.Text = NULL;
    return 0;
    }


/*** Module initialization function, called by the print management module's
 *** initialization routine.
 ***/
int
prtStringInitialize()
    {
    pPrtLayoutDriver ldrv;

    	/** Allocate driver structure **/
	ldrv = (pPrtLayoutDriver)nmMalloc(sizeof(PrtLayoutDriver));
	if (!ldrv) return -1;
	strcpy(ldrv->Name,"String");
	ldrv->Type = PRT_OS_T_STRING;

	/** Function table **/
	ldrv->ContentChanged = prtStringContentChanged;
	ldrv->Init = prtStringInit;
	ldrv->Release = prtStringRelease;

	prtRegisterLayoutDriver(ldrv);

    return 0;
    }

