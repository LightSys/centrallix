#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_new.h"
#include "prtmgmt_private.h"
#include "htmlparse.h"
#include "magic.h"

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
/* Creation:	May 26th, 2000  					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_layout_tablecell.c - provides layout	*/
/*		specific functionality for table-cell objstream 	*/
/*		elements.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_layout_tablecell.c,v 1.1 2001/08/13 18:01:15 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_layout_tablecell.c,v $

    $Log: prtmgmt_layout_tablecell.c,v $
    Revision 1.1  2001/08/13 18:01:15  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:18  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** prtTablecellContentChanged - this function is a driver API routine which
 *** is called by the print management layout module whenever the content of
 *** the table cell changes.  A cell is a direct container -- it can contain
 *** text directly.  
 ***
 *** Return = 0 if the height of the table row did NOT change, or
 ***          1 if the table row's height DID change.
 ***/
int
prtTablecellContentChanged(pPrtSession s, pPrtObjStream os, pPrtObjStream changed)
    {
    double h;
    pPrtObjStream tmp;

	/** Find the max height.  A row could DECREASE in height resulting
	 ** from a page split.
	 **/
	for(h=0.0,tmp=os->Columns;tmp;tmp=tmp->Next)
	    {
	    if (h < tmp->Height) h = tmp->Height;
	    }
	if (h == os->Height) return 0;
	os->Height = h;
    	
    return 1;
    }


/*** prtTablecellInit - driver API routine which initializes a table element
 ***/
int
prtTablecellInit(pPrtObjStream os)
    {
    os->Tablecell.Content = NULL;
    return 0;
    }


/*** prtTablecellRelease - driver API routine which releases (oppposite of
 *** Init) a table element.  Doesn't actually free the element itself, that
 *** is done by the print management module's routines.
 ***/
int
prtTablecellRelease(pPrtObjStream os)
    {
    prt_internal_FreeStream(os->Content);
    return 0;
    }


/*** Module initialization function, called by the print management module's
 *** initialization routine.
 ***/
int
prtTablecellInitialize()
    {
    pPrtLayoutDriver ldrv;

    	/** Allocate driver structure **/
	ldrv = (pPrtLayoutDriver)nmMalloc(sizeof(PrtLayoutDriver));
	if (!ldrv) return -1;
	strcpy(ldrv->Name,"Table Cell");
	ldrv->Type = PRT_OS_T_TABLECELL;

	/** Function table **/
	ldrv->ContentChanged = prtTablecellContentChanged;
	ldrv->Init = prtTablecellInit;
	ldrv->Release = prtTablecellRelease;

	prtRegisterLayoutDriver(ldrv);

    return 0;
    }

