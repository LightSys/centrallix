#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
#include "prtmgmt_v3/fx_font_metrics.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2009 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_od_epsonfx.c                                 */
/* Author:	Greg Beeley                                             */
/* Date:	November 19th, 2007                                     */
/*									*/
/* Description:	This module is an output driver for the strict		*/
/*		formatter module which outputs generic Epson FX ESC/P,	*/
/*		compatible with many Epson dot matrix (and clone) 	*/
/*		9-pin and 24-pin printers.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_od_epsonfx.c,v 1.1 2009/06/24 15:49:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_od_epsonfx.c,v $

    $Log: prtmgmt_v3_od_epsonfx.c,v $
    Revision 1.1  2009/06/24 15:49:13  gbeeley
    - (feature) adding EpsonFX output driver support for continuous form
      printers.
    - (feature) adding DELETE sql statement support


 **END-CVSDATA***********************************************************/


/*** dither table; make it *very* simple for now ***/
unsigned char prt_fxod_dithertable2x2[2][2] =
    {
	{1, 7},
	{5, 3},
    };
unsigned char prt_fxod_dithertable2x3[2][3] =
    {
	{1,   9,   5  },
	{11,  3,   7  },
    };
unsigned char prt_fxod_dithertable3x2[3][2] =
    {
	{1, 11},
	{9, 3},
	{5, 7},
    };
unsigned char prt_fxod_dithertable3x3[3][3] =
    {
	{1,   11,  7  },
	{13,  3,   17 },
	{9,   15,  5  },
    };
unsigned char prt_fxod_dithertable1x3[1][3] =
    {
	{1,  3,  5 },
    };
unsigned char prt_fxod_dithertable3x1[3][1] =
    {
	{1},
	{3},
	{5},
    };
unsigned char prt_fxod_dithertable1x2[1][2] =
    {
	{1,  3, },
    };
unsigned char prt_fxod_dithertable2x1[2][1] =
    {
	{1},
	{3},
    };

/*** our list of resolutions ***/
PrtResolution prt_fxod_resolutions24[] =
    {
    {60,60,PRT_COLOR_T_MONO},		/* 60x60 native */
    {60,60,PRT_COLOR_T_GREY},		/* 180x180 with 3x3 dither table */
    {60,180,PRT_COLOR_T_MONO},		/* 60x180 native */
    {90,90,PRT_COLOR_T_GREY},		/* 90x180 with 1x2 dither table */
    {90,180,PRT_COLOR_T_MONO},		/* 90x180 native */
    {180,180,PRT_COLOR_T_MONO},		/* 180x180 native */
    };


/*** context structure for this driver ***/
typedef struct _PFX
    {
    XArray		SupportedResolutions;
    pPrtResolution	SelectedResolution;
    pPrtSession		Session;
    PrtTextStyle	SelectedStyle;
    int			Quality;
    int			PrinterType;
    double		CurVPos;
    double		MarginTop;
    double		MarginBottom;
    double		MarginLeft;
    double		MarginRight;
    double		PageWidth;
    double		PageHeight;
    double		Xadj;
    }
    PrtFXodInf, *pPrtFXodInf;

#define PRT_FXOD_T_LQ2550	1	/* 24-pin LQ2550 and compatibles */
#define PRT_FXOD_T_FX870	2	/* 9-pin FX870/DFX5000 and compatibles */


/*** Escape Sequence Definitions ***/
#define ESC			"\33"
#define ESCP_RESET		ESC "@"
#define ESCP_10CPI		ESC "P"
#define ESCP_12CPI		ESC "M"
#define ESCP_15CPI		ESC "g"
#define ESCP_ITALIC_ON		ESC "4"
#define ESCP_ITALIC_OFF		ESC "5"
#define ESCP_UNDERLINE_ON	ESC "-1"
#define ESCP_UNDERLINE_OFF	ESC "-0"
#define ESCP_BOLD_ON		ESC "G" ESC "E"
#define ESCP_BOLD_OFF		ESC "H" ESC "F"
#define ESCP_MARGIN_RIGHT	ESC "Q"
#define ESCP_MARGIN_LEFT	ESC "l"
#define ESCP_FORM_LENGTH	ESC "C"
#define ESCP_PERF_SKIP		ESC "N"
#define ESCP_PROPORTIONAL	ESC "p1"
#define ESCP_MONOSPACE		ESC "p0"
#define ESCP_HORIZPOS		ESC "$"
#define ESCP_FEED180		ESC "J"
#define ESCP_QUALDRAFT		ESC "x0"


/*** prt_fxod_Output() - outputs the given snippet of text or ESC/P code
 *** to the output channel for the printing session.  If length is set
 *** to -1, it is calculated a la strlen().
 ***/
int
prt_fxod_Output(pPrtFXodInf context, char* str, int len)
    {

	/** Calculate the length? **/
	if (len < 0) len = strlen(str);
	if (!len) return 0;

	/** Write the data. **/
	context->Session->WriteFn(context->Session->WriteArg, str, len, 0, FD_U_PACKET);

    return 0;
    }


/*** prt_fxod_Open() - open a new printing session with this driver.
 ***/
void*
prt_fxod_Open24(pPrtSession s)
    {
    pPrtFXodInf context;

	/** Set up the context structure for this printing session **/
	context = (pPrtFXodInf)nmMalloc(sizeof(PrtFXodInf));
	if (!context) return NULL;
	context->Session = s;
	context->SelectedResolution = NULL;
	context->PrinterType = PRT_FXOD_T_LQ2550;
	/*context->Quality = PRT_QUALITY_T_NORMAL;*/
	xaInit(&(context->SupportedResolutions), 16);

	/** Right now, just support 75, 100, 150, and 300 dpi **/
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+0));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+1));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+2));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+3));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+4));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_fxod_resolutions24+5));

	/** Setup base text style **/
	context->SelectedStyle.Attr = 0;
	context->SelectedStyle.FontSize = 12.0;
	context->SelectedStyle.FontID = PRT_FONT_T_MONOSPACE;
	context->SelectedStyle.Color = 0x00FFFFFF; /* black */

	/** Send the document init string to the output channel.
	 ** This includes a reset (esc @).
	 **/
	prt_fxod_Output(context, ESCP_RESET, -1);

	/** Initialize the type style settings **/
	prt_fxod_Output(context, ESCP_UNDERLINE_OFF, -1);
	prt_fxod_Output(context, ESCP_ITALIC_OFF, -1);
	prt_fxod_Output(context, ESCP_BOLD_OFF, -1);
	prt_fxod_Output(context, ESCP_MONOSPACE, -1);
	prt_fxod_Output(context, ESCP_10CPI, -1);
	prt_fxod_Output(context, ESCP_QUALDRAFT, -1);

    return (void*)context;
    }


/*** prt_fxod_Close() - closes a printing session with this driver.
 ***/
int
prt_fxod_Close(void* context_v)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	/** Send document end string to the output channel (esc @ reset) **/
	prt_fxod_Output(context, ESCP_RESET, -1);

	/** Free the context structure **/
	xaDeInit(&(context->SupportedResolutions));
	nmFree(context, sizeof(PrtFXodInf));

    return 0;
    }


/*** prt_fxod_GetResolutions() - return the list of supported resolutions
 *** for this printing session.
 ***/
pXArray
prt_fxod_GetResolutions(void* context_v)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    return &(context->SupportedResolutions);
    }


/*** prt_fxod_SetResolution() - set the x/y/color resolution that will be
 *** used for raster data in this printing session.
 ***/
int
prt_fxod_SetResolution(void* context_v, pPrtResolution r)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	/** Set it. **/
	context->SelectedResolution = r;

    return 0;
    }


/*** prt_fxod_SetQuality() - select a print quality.
 ***/
int
prt_fxod_SetQuality(void* context_v, int quality)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	context->Quality = quality;

    return 0;
    }


/*** prt_fxod_SetPageGeom() - ignored for now.
 ***/
int
prt_fxod_SetPageGeom(void* context_v, double width, double height, double t, double b, double l, double r)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    char fxbuf[40];
    int h, bm;

	h = (height + 0.00001);
	bm = (b + 0.00001);

	context->MarginTop = t;
	context->MarginBottom = b;
	context->MarginLeft = l;
	context->MarginRight = r;
	context->PageWidth = width;
	context->PageHeight = height;
	context->Xadj = 2.5;

	snprintf(fxbuf, sizeof(fxbuf), ESCP_FORM_LENGTH "%c" ESCP_PERF_SKIP "%c", h, bm);
	prt_fxod_Output(context, fxbuf, -1);

    return 0;
    }


/*** prt_fxod_GetNearestFontSize() - return the nearest font size to the
 *** requested one.  Right now, we just support a few sizes.
 ***/
int
prt_fxod_GetNearestFontSize(void* context_v, int req_size)
    {
    /*pPrtFXodInf context = (pPrtFXodInf)context_v;*/
    if (req_size >= 11)
	return 12;  /* 10 cpi / 12 point */
    else if (req_size >= 9)
	return 10;  /* 12 cpi / 10 point */
    else
	return 8;   /* 15 cpi / 8 point */
    }


/*** prt_fxod_GetCharacterMetric() - return the width of a printed character
 *** in a given font size, attributes, etc., in 10ths of an inch (relative to
 *** 10cpi, or 12point, fonts.
 ***/
void
prt_fxod_GetCharacterMetric(void* context_v, unsigned char* str, pPrtTextStyle style, double* width, double* height)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    double n;
    int style_code;

	/** Style not specified?  Get it from the context then. **/
	if (!style) style = &(context->SelectedStyle);

	/** Determine style code (0-3 = plain, italic, bold, italic+bold) **/
	style_code = 0;
	if (style->Attr & PRT_OBJ_A_ITALIC) style_code += 1;

	/** Count up the base width values for the characters in the string **/
	n = 0.0;
	while(*str)
	    {
	    if (*str < 0x20 || *str > 0x7E)
		{
		/** No data for characters outside the range; assume 1.0 **/
		n += 1.0;
		}
	    else if (style->FontID == PRT_FONT_T_SANSSERIF || style->FontID == PRT_FONT_T_SERIF)
		{
		/** metrics based on empirical analysis **/
		n += fx_font_metrics[(*str) - 0x20][style_code]/12.0;
		}
	    else
		{
		/** Assume monospace font if unknown **/
		n += 1.0;
		}
	    str++;
	    }

	/** Adjust the width based on the font size.  Base size is 12pt (10cpi) **/
	*width = n*(style->FontSize/12.0);
	*height = 1.0;

    return;
    }


/*** prt_fxod_GetCharacterBaseline() - returns the Y offset of the text
 *** baseline from the upper-left corner of the text, for a given style
 *** or for the current style.
 ***/
double
prt_fxod_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    double bl;

	/** Use current style or specified one? **/
	if (!style) style = &(context->SelectedStyle);

	/** Get the standard baseline height given the font type. **/
	switch(style->FontID)
	    {
	    case PRT_FONT_T_MONOSPACE:	bl = 0.75; break;
	    case PRT_FONT_T_SANSSERIF:	bl = 0.75; break;
	    case PRT_FONT_T_SERIF:	bl = 0.75; break;
	    case PRT_FONT_T_USBARCODE:	bl = 1.00; break;
	    default:			bl = 1.00; break;
	    }

	/** Correct for the font size **/
	bl = (style->FontSize/12.0)*bl;

    return bl;
    }


/*** prt_fxod_SetTextStyle() - set the current output text style, including
 *** attributes (bold/italic/underlined), color, size, and typeface.
 ***/
int
prt_fxod_SetTextStyle(void* context_v, pPrtTextStyle style)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    int onattr, offattr;

	/** Attribute change? **/
	onattr=(style->Attr ^ context->SelectedStyle.Attr)&(style->Attr);
	offattr=(style->Attr ^ context->SelectedStyle.Attr)&(context->SelectedStyle.Attr);
	if (onattr & PRT_OBJ_A_UNDERLINE)
	    prt_fxod_Output(context, ESCP_UNDERLINE_ON, -1);
	else if (offattr & PRT_OBJ_A_UNDERLINE)
	    prt_fxod_Output(context, ESCP_UNDERLINE_OFF, -1);
	if (onattr & PRT_OBJ_A_ITALIC)
	    prt_fxod_Output(context, ESCP_ITALIC_ON, -1);
	else if (offattr & PRT_OBJ_A_ITALIC)
	    prt_fxod_Output(context, ESCP_ITALIC_OFF, -1);
	if (onattr & PRT_OBJ_A_BOLD)
	    prt_fxod_Output(context, ESCP_BOLD_ON, -1);
	else if (offattr & PRT_OBJ_A_BOLD)
	    prt_fxod_Output(context, ESCP_BOLD_OFF, -1);

	/** Font typeface change? **/
	if (style->FontID != context->SelectedStyle.FontID)
	    {
	    if (style->FontID == PRT_FONT_T_SANSSERIF)
		prt_fxod_Output(context, ESCP_PROPORTIONAL, -1);
	    else if (style->FontID == PRT_FONT_T_SERIF)
		prt_fxod_Output(context, ESCP_PROPORTIONAL, -1);
	    else
		prt_fxod_Output(context, ESCP_MONOSPACE, -1);
	    }
	
	/** Font size change? **/
	if (style->FontSize != context->SelectedStyle.FontSize)
	    {
	    if (style->FontSize == 12) /* 10cpi pica */
		prt_fxod_Output(context, ESCP_10CPI, -1);
	    else if (style->FontSize == 10) /* 12cpi elite */
		prt_fxod_Output(context, ESCP_12CPI, -1);
	    else if (style->FontSize == 8) /* 15cpi condensed */
		prt_fxod_Output(context, ESCP_15CPI, -1);
	    }

	/** Record the newly selected style **/
	memcpy(&(context->SelectedStyle), style, sizeof(PrtTextStyle));

    return 0;
    }


/*** prt_fxod_SetHPos() - sets the horizontal (x) position on the page.
 ***/
int
prt_fxod_SetHPos(void* context_v, double x)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    char fxbuf[64];
    int xpos;

	/** Generate the horiz positioning command, in 60ths of an inch. **/
	x -= context->Xadj;
	if (x < 0) x = 0;
	xpos = (x * 6 + 0.000001);
	snprintf(fxbuf, sizeof(fxbuf), ESCP_HORIZPOS "%c%c", xpos & 0xFF, (xpos >> 8) & 0xFF);
	prt_fxod_Output(context, fxbuf, 4);

    return 0;
    }


/*** prt_fxod_SetVPos() - sets the vertical (y) position on the page,
 *** independently of the x position (no carriage return)
 ***/
int
prt_fxod_SetVPos(void* context_v, double y)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    char fxbuf[64];
    double incr;
    int rem;

	/** Emit enough newline chars to scroll down some distance **/
	incr = y - context->CurVPos;
	while (incr >= 0.99999)
	    {
	    prt_fxod_Output(context, "\n", 1);
	    incr -= 1;
	    }

	/** Fine line feed for the remainder, in 180ths of an inch **/
	rem = floor(incr * (180 / 6));
	if (rem > 0)
	    {
	    snprintf(fxbuf, sizeof(fxbuf), ESCP_FEED180 "%c", rem);
	    prt_fxod_Output(context, fxbuf, -1);
	    }

	context->CurVPos = y;

    return 0;
    }


/*** prt_fxod_WriteScreen() - writes on screen (color plane) of text.
 ***/
int
prt_fxod_WriteScreen(pPrtFXodInf context, int color_id, int intensity, char* text)
    {
    return 0;
    }

/*** prt_fxod_WriteText() - sends a string of text to the printer.
 ***/
int
prt_fxod_WriteText(void* context_v, char* str)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	/** output it. **/
	prt_fxod_Output(context, str, -1);

    return 0;
    }


/*** prt_fxod_WriteRasterData() - outputs a block of raster (image) data
 *** at the current printing position on the page, given the selected
 *** pixel and color resolution.  Currently, we just do black+white 
 *** graphics only.
 ***/
double
prt_fxod_WriteRasterData(void* context_v, pPrtImage img, double width, double height, double next_y)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;
    int rows,cols;
    double actual_height;

	/** How many raster rows/cols are we looking at here? **/
	actual_height = (context->CurVPos + height <= next_y)?height:(next_y - context->CurVPos);
	rows = actual_height/6.0*(context->SelectedResolution->Yres);
	cols = width/10.0*(context->SelectedResolution->Xres);

	/** Don't actually print the image for now **/

    return context->CurVPos + actual_height;
    }


/*** prt_fxod_WriteFF() - sends a form feed to end the page.
 ***/
int
prt_fxod_WriteFF(void* context_v)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	/** Create the formfeed/home command. **/
	prt_fxod_Output(context, "\14", -1);
	context->CurVPos = 0.0;

    return 0;
    }


/*** prt_fxod_WriteRect() - write a rectangular area out to the page (or
 *** a fragment of one).  Return the Y coordinate of the bottom of the
 *** fragment that was actually output.  'next_y' is the next Y position
 *** on the page that will be printed after this row of objects.
 ***/
double
prt_fxod_WriteRect(void* context_v, double width, double height, double next_y)
    {
    pPrtFXodInf context = (pPrtFXodInf)context_v;

	/** Make sure width and height meet the minimum required by the
	 ** currently selected resolution
	 **/
	if (width < 10.0/context->SelectedResolution->Xres) width = 10.0/context->SelectedResolution->Xres;
	if (height < 6.0/context->SelectedResolution->Yres) height = 6.0/context->SelectedResolution->Yres;

	/** Don't actually print the rectangle for now **/

    return context->CurVPos + height;
    }


/*** prt_fxod_Initialize() - init this module and register with the main
 *** format driver as a strict output formatter.
 ***/
int
prt_fxod_Initialize()
    {
    pPrtOutputDriver drv;

	/** Register the driver **/
	drv = prt_strictfm_AllocDriver();
	strcpy(drv->Name,"escp24");
	strcpy(drv->ContentType,"text/x-epson-escp-24");
	drv->Open = prt_fxod_Open24;
	drv->Close = prt_fxod_Close;
	drv->GetResolutions = prt_fxod_GetResolutions;
	drv->SetResolution = prt_fxod_SetResolution;
	drv->SetPageGeom = prt_fxod_SetPageGeom;
	drv->GetNearestFontSize = prt_fxod_GetNearestFontSize;
	drv->GetCharacterMetric = prt_fxod_GetCharacterMetric;
	drv->GetCharacterBaseline = prt_fxod_GetCharacterBaseline;
	drv->SetTextStyle = prt_fxod_SetTextStyle;
	drv->SetHPos = prt_fxod_SetHPos;
	drv->SetVPos = prt_fxod_SetVPos;
	drv->WriteText = prt_fxod_WriteText;
	drv->WriteRasterData = prt_fxod_WriteRasterData;
	drv->WriteFF = prt_fxod_WriteFF;
	drv->WriteRect = prt_fxod_WriteRect;

	prt_strictfm_RegisterDriver(drv);

    return 0;
    }


