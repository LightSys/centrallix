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
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_od_text.c                                    */
/* Author:	Greg Beeley                                             */
/* Date:	February 27th, 2003                                     */
/*									*/
/* Description:	This module is an output driver for the strict		*/
/*		formatter module which just outputs plain text.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_od_text.c,v 1.1 2003/02/27 22:03:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_od_text.c,v $

    $Log: prtmgmt_v3_od_text.c,v $
    Revision 1.1  2003/02/27 22:03:39  gbeeley
    Added text/plain output driver, which will among other things allow me
    to test this beast without wasting so much paper printing PCL stuff ;)


 **END-CVSDATA***********************************************************/


/*** our list of resolutions ***/
PrtResolution prt_text_resolutions[] =
    {
    };


/*** context structure for this driver ***/
typedef struct _PTEXT
    {
    XArray		SupportedResolutions;
    pPrtResolution	SelectedResolution;
    pPrtSession		Session;
    PrtTextStyle	SelectedStyle;
    int			CurPhysHPos;
    int			CurHPos;
    int			CurVPos;
    }
    PrtTextodInf, *pPrtTextodInf;


/*** prt_textod_Output() - outputs the given snippet of text or PCL code
 *** to the output channel for the printing session.  If length is set
 *** to -1, it is calculated a la strlen().
 ***/
int
prt_textod_Output(pPrtTextodInf context, char* str, int len)
    {

	/** Calculate the length? **/
	if (len < 0) len = strlen(str);
	if (!len) return 0;

	/** Write the data. **/
	context->Session->WriteFn(context->Session->WriteArg, str, len, 0, FD_U_PACKET);

    return 0;
    }


/*** prt_textod_Open() - open a new printing session with this driver.
 ***/
void*
prt_textod_Open(pPrtSession s)
    {
    pPrtTextodInf context;

	/** Set up the context structure for this printing session **/
	context = (pPrtTextodInf)nmMalloc(sizeof(PrtTextodInf));
	if (!context) return NULL;
	context->Session = s;
	context->SelectedResolution = NULL;
	xaInit(&(context->SupportedResolutions), 16);

	/** Setup base text style **/
	context->SelectedStyle.Attr = 0;
	context->SelectedStyle.FontSize = 12.0;
	context->SelectedStyle.FontID = PRT_FONT_T_MONOSPACE;
	context->SelectedStyle.Color = 0x00FFFFFF; /* black */
	context->CurVPos = 0;
	context->CurHPos = 0;
	context->CurPhysHPos = 0;

    return (void*)context;
    }


/*** prt_textod_Close() - closes a printing session with this driver.
 ***/
int
prt_textod_Close(void* context_v)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;

	/** Free the context structure **/
	xaDeInit(&(context->SupportedResolutions));
	nmFree(context, sizeof(PrtTextodInf));

    return 0;
    }


/*** prt_textod_GetResolutions() - return the list of supported resolutions
 *** for this printing session.
 ***/
pXArray
prt_textod_GetResolutions(void* context_v)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;
    return &(context->SupportedResolutions);
    }


/*** prt_textod_SetResolution() - set the x/y/color resolution that will be
 *** used for raster data in this printing session.  Not supported for the
 *** text output driver - no images are possible.
 ***/
int
prt_textod_SetResolution(void* context_v, pPrtResolution r)
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/

    return -1;
    }


/*** prt_textod_GetNearestFontSize() - return the nearest font size to the
 *** requested one.  We only support one font size - 12 point (10cpi).
 ***/
int
prt_textod_GetNearestFontSize(void* context_v, int req_size)
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/
    return 12;
    }


/*** prt_textod_GetCharacterMetric() - return the width of a printed character
 *** in a given font size, attributes, etc., in 10ths of an inch (relative to
 *** 10cpi, or 12point, fonts.  For the plain text driver, we just return the
 *** length of the string, since there is only one font - fixed 12point courier.
 ***/
double
prt_textod_GetCharacterMetric(void* context_v, unsigned char* str, pPrtTextStyle style)
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/
    return strlen(str);
    }


/*** prt_textod_GetCharacterBaseline() - returns the Y offset of the text
 *** baseline from the upper-left corner of the text, for a given style
 *** or for the current style.  Baseline is rather irrelevant in the
 *** plain text driver since there is only one font size.
 ***/
double
prt_textod_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/
    return 0.75;
    }


/*** prt_textod_SetTextStyle() - set the current output text style, including
 *** attributes (bold/italic/underlined), color, size, and typeface.  We 
 *** accept the style setting here, but it makes no real difference.
 ***/
int
prt_textod_SetTextStyle(void* context_v, pPrtTextStyle style)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;

	/** Record the newly selected style **/
	memcpy(&(context->SelectedStyle), style, sizeof(PrtTextStyle));
	context->SelectedStyle.FontSize = 12;

    return 0;
    }


/*** prt_textod_SetHPos() - sets the horizontal (x) position on the page.
 ***/
int
prt_textod_SetHPos(void* context_v, double x)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;
    int d,t;

	/** Emit enough spaces to move us to the correct column **/
	t = x + 0.00001;
	if (t < context->CurPhysHPos) return -1;
	while(t > context->CurPhysHPos)
	    {
	    d = t - context->CurPhysHPos;
	    if (d > 16) d = 16;
	    prt_textod_Output(context, "                ",d);
	    context->CurPhysHPos += d;
	    }
	context->CurHPos = t;

    return 0;
    }


/*** prt_textod_SetVPos() - sets the vertical (y) position on the page,
 *** via newlines in this case.  This causes the physical X position to
 *** be reset to zero so that the next time something is printed, the
 *** physical position has to be padded up to the logical position.
 ***/
int
prt_textod_SetVPos(void* context_v, double y)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;
    int n;

	/** Use newlines... **/
	n = y + 0.00001;
	while(context->CurVPos < n)
	    {
	    prt_textod_Output(context,"\r\n",2);
	    context->CurVPos++;
	    context->CurPhysHPos = 0;
	    }

    return 0;
    }


/*** prt_textod_WriteText() - sends a string of text to the printer.
 ***/
int
prt_textod_WriteText(void* context_v, char* str)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;
    int n;

	/** Make sure the physical position matches the logical one. **/
	prt_textod_SetHPos(context_v, context->CurHPos);

	/** output it. **/
	n = strlen(str);
	prt_textod_Output(context, str, n);
	context->CurHPos += n;
	context->CurPhysHPos += n;

    return 0;
    }


/*** prt_textod_WriteRasterData() - outputs a block of raster (image) data
 *** at the current printing position on the page, given the selected
 *** pixel and color resolution.  Data[] is an array of 32-bit integers, one
 *** per pixel, containing 0x00RRGGBB values for the pixel.  The text driver
 *** does NOT support writing raster data, so we just ignore it.
 ***/
int
prt_textod_WriteRasterData(void* context_v, int xpixels, int ypixels, unsigned int data[])
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/
    return 0;
    }


/*** prt_textod_WriteFF() - sends a form feed to end the page.
 ***/
int
prt_textod_WriteFF(void* context_v)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;

	/** Issue a formfeed (ctl-L). **/
	prt_textod_Output(context, "\14", 1);
	context->CurVPos = 0;
	context->CurHPos = 0;
	context->CurPhysHPos = 0;

    return 0;
    }


/*** prt_textod_Initialize() - init this module and register with the main
 *** format driver as a strict output formatter.
 ***/
int
prt_textod_Initialize()
    {
    pPrtOutputDriver drv;

	/** Register the driver **/
	drv = prt_strictfm_AllocDriver();
	strcpy(drv->Name,"text");
	strcpy(drv->ContentType,"text/plain");
	drv->Open = prt_textod_Open;
	drv->Close = prt_textod_Close;
	drv->GetResolutions = prt_textod_GetResolutions;
	drv->SetResolution = prt_textod_SetResolution;
	drv->GetNearestFontSize = prt_textod_GetNearestFontSize;
	drv->GetCharacterMetric = prt_textod_GetCharacterMetric;
	drv->GetCharacterBaseline = prt_textod_GetCharacterBaseline;
	drv->SetTextStyle = prt_textod_SetTextStyle;
	drv->SetHPos = prt_textod_SetHPos;
	drv->SetVPos = prt_textod_SetVPos;
	drv->WriteText = prt_textod_WriteText;
	drv->WriteRasterData = prt_textod_WriteRasterData;
	drv->WriteFF = prt_textod_WriteFF;

	prt_strictfm_RegisterDriver(drv);

    return 0;
    }


