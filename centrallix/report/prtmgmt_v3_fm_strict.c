#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "magic.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_v3.h"
#include "htmlparse.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2002 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_fm_strict.c                                  */
/* Author:	Greg Beeley                                             */
/* Date:	January 21st, 2002                                      */
/*									*/
/* Description:	This module is the "strict" formatter, which interfaces	*/
/*		with the strict-format output drivers to generate	*/
/*		precisely formatted content, such as PCL, FX, text, and	*/
/*		so forth.  This module is *not* used for HTML output.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_strict.c,v 1.3 2002/10/18 22:01:38 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_fm_strict.c,v $

    $Log: prtmgmt_v3_fm_strict.c,v $
    Revision 1.3  2002/10/18 22:01:38  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.2  2002/04/25 04:30:14  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** GLOBAL DATA FOR THIS MODULE ***/
typedef struct _PSF
    {
    XArray	DriverList;
    }
    PRT_STRICTFM_t;

PRT_STRICTFM_t PRT_STRICTFM;


/*** formatter internal structure. ***/
typedef struct _PSFI
    {
    pPrtSession		Session;
    pPrtOutputDriver	OutputDriver;
    void*		OutputDriverData;
    pPrtResolution	SelectedRes;
    }
    PrtStrictfmInf, *pPrtStrictfmInf;


/*** prt_strictfm_RegisterDriver() - register an output content driver with
 *** the strict formatter.
 ***/
int
prt_strictfm_RegisterDriver(pPrtOutputDriver drv)
    {

	/** Add to the list **/
	ASSERTMAGIC(drv,MGK_PRTOUTDRV);
	xaAddItem(&PRT_STRICTFM.DriverList, (void*)drv);

    return 0;
    }


/*** prt_strictfm_AllocDriver() - allocate a new driver structure.
 ***/
pPrtOutputDriver
prt_strictfm_AllocDriver()
    {
    pPrtOutputDriver drv;

	drv = (pPrtOutputDriver)nmMalloc(sizeof(PrtOutputDriver));
	if (!drv) return NULL;
	SETMAGIC(drv,MGK_PRTOUTDRV);

    return drv;
    }


/*** prt_strictfm_Probe() - this function is called when a new printmanagement
 *** session is opened and this driver is being asked whether or not it can
 *** print the given content type.
 ***/
void*
prt_strictfm_Probe(pPrtSession s, char* output_type)
    {
    pPrtStrictfmInf context;
    pPrtOutputDriver drv,found_drv;
    int i;

	/** Scan the list of drivers to see if we have a match. **/
	found_drv = NULL;
	for(i=0;i<PRT_STRICTFM.DriverList.nItems;i++)
	    {
	    drv = (pPrtOutputDriver)(PRT_STRICTFM.DriverList.Items[i]);
	    if (!strcmp(output_type, drv->ContentType))
		{
		found_drv = drv;
		break;
		}
	    }
	if (!found_drv) return NULL;

	/** Allocate our context inf structure **/
	context = (pPrtStrictfmInf)nmMalloc(sizeof(PrtStrictfmInf));
	if (!context) return NULL;
	context->Session = s;
	context->OutputDriver = found_drv;
	context->OutputDriverData = context->OutputDriver->Open(s);
	if (!context->OutputDriverData)
	    {
	    nmFree(context,sizeof(PrtStrictfmInf));
	    return NULL;
	    }

    return (void*)context;
    }


/*** prt_strictfm_GetNearestFontSize - return the nearest font size that this
 *** driver supports.  In this case, this just queries the underlying output
 *** driver for the information.
 ***/
int
prt_strictfm_GetNearestFontSize(void* context_v, int req_size)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;
    return context->OutputDriver->GetNearestFontSize(context->OutputDriverData, req_size);
    }


/*** prt_strictfm_GetCharacterMetric - return the sizing information for a given
 *** character, in standard units.
 ***/
double
prt_strictfm_GetCharacterMetric(void* context_v, char* str, pPrtTextStyle style)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;
    return context->OutputDriver->GetCharacterMetric(context->OutputDriverData,  str, style);
    }


/*** prt_strictfm_GetCharacterBaseline - return the distance from the upper
 *** left corner of the character cell to the left baseline point of the 
 *** character cell, in standard units.
 ***/
double
prt_strictfm_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;
    return context->OutputDriver->GetCharacterBaseline(context->OutputDriverData, style);
    }


/*** prt_strictfm_Close() - end a printing session and destroy the context
 *** structure.
 ***/
int
prt_strictfm_Close(void* context_v)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;

	/** Close the underlying driver **/
	context->OutputDriver->Close(context->OutputDriverData);

	/** Free memory used **/
	nmFree(context, sizeof(PrtStrictfmInf));

    return 0;
    }


/*** prt_strictfm_Generate() - this is the workhorse routine that generates
 *** a page's worth of output through the output driver.  This is called
 *** after the page has been sorted a la YSort, etc.
 ***/
int
prt_strictfm_Generate(void* context_v, pPrtObjStream page_obj)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;
    pXArray resolutions;
    pPrtResolution res, best_res=NULL, best_greater_res=NULL;
    double dist, best_dist=0.0, best_greater_dist=0.0, xratio, yratio;
    pPrtObjStream cur_obj;
    pPrtOutputDriver drv;
    void* drvdata;
    PrtTextStyle cur_style;
    double cur_y;
    /*XArray cur_objlist;*/
    int i;

	/** First, determine what resolution the graphics will be rendered at **/
	drv = context->OutputDriver;
	drvdata = context->OutputDriverData;
	resolutions = (pXArray)(drv->GetResolutions(drvdata));
	if (!resolutions || resolutions->nItems == 0)
	    {
	    /** Graphics not supported (such as a textonly driver) **/
	    context->SelectedRes = NULL;
	    }
	else
	    {
	    /** Search through the resolutions... **/
	    for(i=0;i<resolutions->nItems;i++)
		{
		res = (pPrtResolution)(resolutions->Items[i]);
		xratio = (double)(context->Session->ResolutionX) / (double)(res->Xres);
		if (xratio > 1.0) xratio = 1/xratio;
		yratio = (double)(context->Session->ResolutionY) / (double)(res->Yres);
		if (yratio > 1.0) yratio = 1/yratio;
		dist = xratio*yratio;
		if (dist > best_dist)
		    {
		    best_dist = dist;
		    best_res = res;
		    }
		if (dist > best_greater_dist && context->Session->ResolutionX < res->Xres && context->Session->ResolutionY < res->Yres)
		    {
		    best_greater_dist = dist;
		    best_greater_res = res;
		    }
		}
	    }

	/** Select a resolution. **/
	if (best_greater_res) 
	    res = best_greater_res;
	else
	    res = best_res;
	drv->SetResolution(drvdata, res);

	/** Ok, follow the object chain on the page, generating the bits and pieces **/
	memcpy(&cur_style, &(page_obj->TextStyle), sizeof(PrtTextStyle));
	cur_y = -1.0;
	drv->SetTextStyle(drvdata, &cur_style);
	cur_obj = page_obj;
	while((cur_obj = cur_obj->YNext))
	    {
	    /** Style change? **/
	    if (memcmp(&cur_style, &(page_obj->TextStyle), sizeof(PrtTextStyle)))
		{
		memcpy(&cur_style, &(page_obj->TextStyle), sizeof(PrtTextStyle));
		drv->SetTextStyle(drvdata, &cur_style);
		}

	    /** Set position. **/
	    if (cur_obj->PageY != cur_y)
		{
		cur_y = cur_obj->PageY;
		drv->SetVPos(drvdata, cur_y);
		}
	    drv->SetHPos(drvdata, cur_obj->PageX);

	    /** Do the specific object. **/
	    switch(cur_obj->ObjType->TypeID)
		{
		case PRT_OBJ_T_STRING:
		    if (*(cur_obj->Content)) drv->WriteText(drvdata, cur_obj->Content);
		    break;

		case PRT_OBJ_T_IMAGE:
		    break;

		default:
		    break;
		}
	    }

	/** End the page with a form feed. **/
	drv->WriteFF(drvdata);

    return 0;
    }


/*** prt_strictfm_Initialize() - init this module and register with the main
 *** print management system.
 ***/
int
prt_strictfm_Initialize()
    {
    pPrtFormatter fmtdrv;

	/** Init our globals **/
	memset(&PRT_STRICTFM, 0, sizeof(PRT_STRICTFM));
	xaInit(&PRT_STRICTFM.DriverList, 16);

	/** Allocate the formatter structure, and init it **/
	fmtdrv = prtAllocFormatter();
	if (!fmtdrv) return -1;
	strcpy(fmtdrv->Name, "strict");
	fmtdrv->Probe = prt_strictfm_Probe;
	fmtdrv->Generate = prt_strictfm_Generate;
	fmtdrv->GetNearestFontSize = prt_strictfm_GetNearestFontSize;
	fmtdrv->GetCharacterMetric = prt_strictfm_GetCharacterMetric;
	fmtdrv->GetCharacterBaseline = prt_strictfm_GetCharacterBaseline;
	fmtdrv->Close = prt_strictfm_Close;

	/** Register with the main prtmgmt system **/
	prtRegisterFormatter(fmtdrv);

    return 0;
    }


