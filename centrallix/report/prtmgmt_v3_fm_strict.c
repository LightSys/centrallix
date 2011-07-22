#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"

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


int
prt_strictfm_GetType(void* drv_v, char* objname, char* attrname, void* val_v)
    {
    POD(val_v)->String = ((pPrtOutputDriver)drv_v)->ContentType;
    return 0;
    }


/*** prt_strictfm_RegisterDriver() - register an output content driver with
 *** the strict formatter.
 ***/
int
prt_strictfm_RegisterDriver(pPrtOutputDriver drv)
    {
    char* ptr;
    char buf[64];
    pSysInfoData si;

	/** Add to the list **/
	ASSERTMAGIC(drv,MGK_PRTOUTDRV);
	xaAddItem(&PRT_STRICTFM.DriverList, (void*)drv);

	/** Add the driver's type to the /sys/cx.sysinfo directory **/
	ptr = strchr(drv->ContentType,'/');
	if (!ptr)
	    ptr = drv->ContentType;
	else
	    ptr = ptr + 1;
	snprintf(buf, sizeof(buf), "/prtmgmt/output_types/%s", ptr);
	si = sysAllocData(buf, NULL, NULL, NULL, NULL, prt_strictfm_GetType, NULL, 0);
	sysAddAttrib(si, "type", DATA_T_STRING);
	sysRegister(si, (void*)drv);

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
void
prt_strictfm_GetCharacterMetric(void* context_v, char* str, pPrtTextStyle style, double* width, double* height)
    {
    pPrtStrictfmInf context = (pPrtStrictfmInf)context_v;
    return context->OutputDriver->GetCharacterMetric(context->OutputDriverData,  str, style, width, height);
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
    pPrtObjStream cur_obj, rect_obj, search_obj;
    pPrtOutputDriver drv;
    void* drvdata;
    PrtTextStyle cur_style;
    double cur_y;
    /*XArray cur_objlist;*/
    int i;
    double end_y;
    double next_y;

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

	/** Tell output driver the size of the page **/
	drv->SetPageGeom(drvdata, page_obj->Width, page_obj->Height, 
		page_obj->MarginTop, page_obj->MarginBottom, 
		page_obj->MarginLeft, page_obj->MarginRight);

	/** Ok, follow the object chain on the page, generating the bits and pieces **/
	memcpy(&cur_style, &(page_obj->TextStyle), sizeof(PrtTextStyle));
	cur_y = -1.0;
	next_y = -1.0;
	drv->SetTextStyle(drvdata, &cur_style);
	cur_obj = page_obj;
	while((cur_obj = cur_obj->YNext))
	    {
	    /** Set position. **/
	    if (cur_obj->PageY != cur_y)
		{
		cur_y = cur_obj->PageY;
		drv->SetVPos(drvdata, cur_y);
		}
	    drv->SetHPos(drvdata, cur_obj->PageX);

	    /** Figure out next Y position? **/
	    if (next_y <= cur_y)
		{
		search_obj = cur_obj;
		while(search_obj && search_obj->PageY <= cur_y) search_obj = search_obj->YNext;
		if (search_obj)
		    next_y = search_obj->PageY;
		else
		    next_y = page_obj->Height;
		}

	    /** Style change? **/
	    if (memcmp(&cur_style, &(cur_obj->TextStyle), sizeof(PrtTextStyle)))
		{
		memcpy(&cur_style, &(cur_obj->TextStyle), sizeof(PrtTextStyle));
		drv->SetTextStyle(drvdata, &cur_style);
		}

	    /** Do the specific object. **/
	    switch(cur_obj->ObjType->TypeID)
		{
		case PRT_OBJ_T_STRING:
		    if (*(cur_obj->Content)) drv->WriteText(drvdata, cur_obj->Content);
		    break;

		case PRT_OBJ_T_IMAGE:
		case PRT_OBJ_T_RECT:
		    /** Ask output driver to print as much of the rectangle/image as it can **/
		    if (cur_obj->ObjType->TypeID == PRT_OBJ_T_IMAGE)
			end_y = drv->WriteRasterData(drvdata, cur_obj->Content, cur_obj->Width, cur_obj->Height, next_y);
		    else
			end_y = drv->WriteRect(drvdata, cur_obj->Width, cur_obj->Height, next_y);

		    /** Adjust the rectangle to remove what was already printed **/
		    if (end_y < (cur_obj->PageY + cur_obj->Height - PRT_FP_FUDGE) && end_y > (cur_obj->PageY - PRT_FP_FUDGE))
			{
			cur_obj->Height -= (end_y - cur_obj->PageY);
			cur_obj->PageY = end_y;

			/** Remove from YList and re-add in a sorted manner.  This way, it
			 ** comes back up when we need to print the next segment of
			 ** the rectangle or image.
			 **/
			rect_obj = cur_obj;
			cur_obj = cur_obj->YPrev;
			rect_obj->YPrev->YNext = rect_obj->YNext;
			if (rect_obj->YNext) rect_obj->YNext->YPrev = rect_obj->YPrev;
			prt_internal_AddYSorted(cur_obj, rect_obj);
			}
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


