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
/* Creation:	April 28th, 2000					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/*									*/
/*		File: prtmgmt_layout.c - Contains routines and such to	*/
/*		perform layout-oriented operations on the structured	*/
/*		objectstream.  This does not actually GENERATE a page;	*/
/*		see the prtmgmt_generator file for that.		*/
/************************************************************************/



/*** prt_internal_CheckPageBreak - checks the objectstream to see if a 
 *** page break is necessary, and if so, calls PageBreak.
 ***/
int
prt_internal_CheckPageBreak(pPrtSession this)
    {
    pPrtObjStream os = this->ObjStreamPtr;

	/** Compare height of last row... **/
	if (os->RelativeY + os->Height > os->Parent->AvailableHeight)
	    {
	    prt_internal_PageBreak(this);
	    }

    return 0;
    }


/*** prt_internal_PageBreak - performs a page break operation.
 ***/
int
prt_internal_PageBreak(pPrtSession this)
    {
    pPrtObjStream old_os, new_os, brkpt;

    	/** Find the first element that will go on the new page. **/
	brkpt = this->ObjStreamPtr;
	while (brkpt->Prev && brkpt->Prev->RelativeY + brkpt->Prev->Height > brkpt->Prev->Parent->AvailableHeight)
	    {
	    brkpt = brkpt->Prev;
	    }

    return 0;
    }


/*** prt_internal_DetermineTextWidth - calls the session's content driver
 *** to determine what the printed width of the given text is.  The 
 *** computations are done in accordance with what the *current* font and
 *** size are.
 ***/
double
prt_internal_DetermineTextWidth(pPrtSession this, char* text, int len)
    {
    }


/*** prt_internal_CheckTextFlow - this routine is a generic layout function
 *** that does wrap, justify, picture flow, and break checking whenever a
 *** new element is added to the objstream of text/pictures/areas.
 ***/
int
prt_internal_CheckTextFlow(pPrtSession this, pPrtObjStream element)
    {
    }


/*** prt_internal_DetermineYIncrement - this function determines what the 
 *** height of the current line is so that the next line can be properly
 *** positioned below it.  It scans backwards from 'element' to the beginning
 *** of the line containing 'element', and finds the maximum height of an
 *** element on the line (except for pictures/areas flagged as "flowaround").
 ***/
double
prt_internal_DetermineYIncrement(pPrtSession this, pPrtObjStream element)
    {
    }

