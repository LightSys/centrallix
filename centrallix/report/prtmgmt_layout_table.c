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
/* Creation:	May 26th, 2000  					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_layout_table.c - provides layout-specific	*/
/*		functionality for table objstream elements.		*/
/************************************************************************/



/*** prtTableContentChanged - this function is a driver API routine which
 *** is called by the print management layout module whenever the content
 *** of the table is changed.  For tables, this is called when a row's 
 *** height changes; row widths are fixed.
 ***
 *** Return = 0 if the height of the table did NOT change, or
 ***          1 if the table's height DID change (this, BTW, will always be
 ***            the case for a table).
 ***/
int
prtTableContentChanged(pPrtSession s, pPrtObjStream os, pPrtObjStream changed)
    {
    double h;
    pPrtObjStream tmp;

	/** Total up the new height measurements **/
	for(h=0.0,tmp=os->Rows;tmp;tmp=tmp->Next) h += tmp->Height;
	if (os->HdrRow) h += os->HdrRow->Height;
	os->Height = h;
    	
    return 1;
    }


/*** prtTableInit - driver API routine which initializes a table element
 ***/
int
prtTableInit(pPrtObjStream os)
    {
    os->Table.Rows = NULL;
    os->Table.HdrRow = NULL;
    return 0;
    }


/*** prtTableRelease - driver API routine which releases (oppposite of
 *** Init) a table element.  Doesn't actually free the element itself, that
 *** is done by the print management module's routines.
 ***/
int
prtTableRelease(pPrtObjStream os)
    {
    prt_internal_FreeStream(os->HdrRow);
    prt_internal_FreeStream(os->Rows);
    return 0;
    }


/*** Module initialization function, called by the print management module's
 *** initialization routine.
 ***/
int
prtTableInitialize()
    {
    pPrtLayoutDriver ldrv;

    	/** Allocate driver structure **/
	ldrv = (pPrtLayoutDriver)nmMalloc(sizeof(PrtLayoutDriver));
	if (!ldrv) return -1;
	strcpy(ldrv->Name,"Table");
	ldrv->Type = PRT_OS_T_TABLE;

	/** Function table **/
	ldrv->ContentChanged = prtTableContentChanged;
	ldrv->Init = prtTableInit;
	ldrv->Release = prtTableRelease;

	prtRegisterLayoutDriver(ldrv);

    return 0;
    }

