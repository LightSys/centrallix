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
#include "hp_font_metrics.h"

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
/* Module:	prtmgmt_v3_od_pcl.c                                     */
/* Author:	Greg Beeley                                             */
/* Date:	January 18th, 2002                                      */
/*									*/
/* Description:	This module is an output driver for the strict		*/
/*		formatter module which outputs generic HP PCL, which	*/
/*		is compatible with many HP (and clone) laserjet and	*/
/*		inkjet printers.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_od_pcl.c,v 1.6 2002/10/21 22:55:11 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_od_pcl.c,v $

    $Log: prtmgmt_v3_od_pcl.c,v $
    Revision 1.6  2002/10/21 22:55:11  gbeeley
    Added font/size test in test_prt to test the alignment of different fonts
    and sizes on one line or on separate lines.  Fixed lots of bugs in the
    font baseline alignment logic.  Added prt_internal_Dump() to debug the
    document's structure.  Fixed a YSort bug where it was not sorting the
    YPrev/YNext pointers but the Prev/Next ones instead, and had a loop
    condition problem causing infinite looping as well.  Fixed some problems
    when adding an empty obj to a stream of objects and then modifying
    attributes which would change the object's geometry.

    There are still some glitches in the line spacing when different font
    sizes are used, however.

    Revision 1.5  2002/10/21 20:22:12  gbeeley
    Text foreground color attribute now basically operational.  Range of
    colors is limited however.  Tested on PCL output driver, on hp870c
    and hp4550 printers.  Also tested on an hp3si (black&white) to make
    sure the color pcl commands didn't garble things up there.  Use the
    "colors" test_prt command to test color output (and "output" to
    "/dev/lp0" if desired).

    Revision 1.4  2002/10/18 22:01:39  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.3  2002/10/17 20:23:18  gbeeley
    Got printing v3 subsystem open/close session working (basically)...

    Revision 1.2  2002/04/25 04:30:14  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** our list of resolutions ***/
PrtResolution prt_pcl_resolutions[] =
    {
    {75,75,PRT_COLOR_T_FULL},
    {100,100,PRT_COLOR_T_FULL},
    {150,150,PRT_COLOR_T_FULL},
    {300,300,PRT_COLOR_T_FULL},
    {600,600,PRT_COLOR_T_FULL},
    {1200,1200,PRT_COLOR_T_FULL},
    };


/*** context structure for this driver ***/
typedef struct _PPCL
    {
    XArray		SupportedResolutions;
    pPrtResolution	SelectedResolution;
    pPrtSession		Session;
    PrtTextStyle	SelectedStyle;
    }
    PrtPclodInf, *pPrtPclodInf;


/*** prt_pclod_RGBtoCMYK() - take an RGB color, in the form 0x00RRGGBB,
 *** and convert it to CMYK ink values, in the form 0xCCMMYYKK.
 ***/
unsigned int
prt_pclod_RGBtoCMYK(int rgb_color)
    {
    unsigned int c,m,y,k;
    int r,g,b;

	/** Separate the rgb components **/
	r = (rgb_color>>16)&0xFF;
	g = (rgb_color>>8)&0xFF;
	b = (rgb_color)&0xFF;

	/** black (k) level based on common lightness **/
	k = (r>g)?r:g;
	k = (k>b)?k:b;
	k = 255 - k;

	/** Remove the black component from the r,g, and b. **/
	r += k;
	g += k;
	b += k;

	/** compute the c,m, and y components as inverses of r,g, and b. **/
	c = 255 - r;
	m = 255 - g;
	y = 255 - b;

    return (c<<24 | m<<16 | y<<8 | k);
    }


/*** prt_pclod_Output() - outputs the given snippet of text or PCL code
 *** to the output channel for the printing session.  If length is set
 *** to -1, it is calculated a la strlen().
 ***/
int
prt_pclod_Output(pPrtPclodInf context, char* str, int len)
    {

	/** Calculate the length? **/
	if (len < 0) len = strlen(str);
	if (!len) return 0;

	/** Write the data. **/
	context->Session->WriteFn(context->Session->WriteArg, str, len, 0, 0);

    return 0;
    }


/*** prt_pclod_Open() - open a new printing session with this driver.
 ***/
void*
prt_pclod_Open(pPrtSession s)
    {
    pPrtPclodInf context;

	/** Set up the context structure for this printing session **/
	context = (pPrtPclodInf)nmMalloc(sizeof(PrtPclodInf));
	if (!context) return NULL;
	context->Session = s;
	context->SelectedResolution = NULL;
	xaInit(&(context->SupportedResolutions), 16);

	/** Right now, just support 75, 100, 150, and 300 dpi **/
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_pcl_resolutions+0));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_pcl_resolutions+1));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_pcl_resolutions+2));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_pcl_resolutions+3));

	/** Setup base text style **/
	context->SelectedStyle.Attr = 0;
	context->SelectedStyle.FontSize = 12.0;
	context->SelectedStyle.FontID = PRT_FONT_T_MONOSPACE;
	context->SelectedStyle.Color = 0x00FFFFFF; /* black */

	/** Send the document init string to the output channel.
	 ** This includes a reset (esc-e) and a command to select
	 ** the RGB palette.
	 **/
	prt_pclod_Output(context, "\33E\33*r-3U", -1);

    return (void*)context;
    }


/*** prt_pclod_Close() - closes a printing session with this driver.
 ***/
int
prt_pclod_Close(void* context_v)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;

	/** Send document end string to the output channel (esc-e reset) **/
	prt_pclod_Output(context, "\33E", -1);

	/** Free the context structure **/
	xaDeInit(&(context->SupportedResolutions));
	nmFree(context, sizeof(PrtPclodInf));

    return 0;
    }


/*** prt_pclod_GetResolutions() - return the list of supported resolutions
 *** for this printing session.
 ***/
pXArray
prt_pclod_GetResolutions(void* context_v)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    return &(context->SupportedResolutions);
    }


/*** prt_pclod_SetResolution() - set the x/y/color resolution that will be
 *** used for raster data in this printing session.
 ***/
int
prt_pclod_SetResolution(void* context_v, pPrtResolution r)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;

	/** Set it. **/
	context->SelectedResolution = r;

    return 0;
    }


/*** prt_pclod_GetNearestFontSize() - return the nearest font size to the
 *** requested one.  Most PCL printers can scale fonts without any problem,
 *** so we'll allow any font size greater than zero.
 ***/
int
prt_pclod_GetNearestFontSize(void* context_v, int req_size)
    {
    /*pPrtPclodInf context = (pPrtPclodInf)context_v;*/
    return (req_size<=0)?1:req_size;
    }


/*** prt_pclod_GetCharacterMetric() - return the width of a printed character
 *** in a given font size, attributes, etc., in 10ths of an inch (relative to
 *** 10cpi, or 12point, fonts.
 ***/
double
prt_pclod_GetCharacterMetric(void* context_v, unsigned char* str, pPrtTextStyle style)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    double n;
    int style_code;

	/** Style not specified?  Get it from the context then. **/
	if (!style) style = &(context->SelectedStyle);

	/** Determine style code (0-3 = plain, italic, bold, italic+bold) **/
	style_code = 0;
	if (style->Attr & PRT_OBJ_A_BOLD) style_code += 2;
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
	    else if (style->FontID == PRT_FONT_T_SANSSERIF)
		{
		/** metrics based on empirical analysis **/
		n += hp_helvetica_font_metrics[(*str) - 0x20][style_code]/60.0;
		}
	    else if (style->FontID == PRT_FONT_T_SERIF)
		{
		/** metrics based on empirical analysis **/
		n += hp_times_font_metrics[(*str) - 0x20][style_code]/60.0;
		}
	    else
		{
		/** Assume monospace font if unknown **/
		n += 1.0;
		}
	    str++;
	    }

	/** Adjust the width based on the font size.  Base size is 12pt **/
	n = n*(style->FontSize/12.0);

    return n;
    }


/*** prt_pclod_GetCharacterBaseline() - returns the Y offset of the text
 *** baseline from the upper-left corner of the text, for a given style
 *** or for the current style.
 ***/
double
prt_pclod_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
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


/*** prt_pclod_SetTextStyle() - set the current output text style, including
 *** attributes (bold/italic/underlined), color, size, and typeface.
 ***/
int
prt_pclod_SetTextStyle(void* context_v, pPrtTextStyle style)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    char pclbuf[64];
    int onattr, offattr;
    int color;

	/** Attribute change? **/
	onattr=(style->Attr ^ context->SelectedStyle.Attr)&(style->Attr);
	offattr=(style->Attr ^ context->SelectedStyle.Attr)&(context->SelectedStyle.Attr);
	if (onattr & PRT_OBJ_A_UNDERLINE)
	    prt_pclod_Output(context, "\33&d0D", -1);
	else if (offattr & PRT_OBJ_A_UNDERLINE)
	    prt_pclod_Output(context, "\33&d@", -1);
	if (onattr & PRT_OBJ_A_ITALIC)
	    prt_pclod_Output(context, "\33(s1S", -1);
	else if (offattr & PRT_OBJ_A_ITALIC)
	    prt_pclod_Output(context, "\33(s0S", -1);
	if (onattr & PRT_OBJ_A_BOLD)
	    prt_pclod_Output(context, "\33(s3B", -1);
	else if (offattr & PRT_OBJ_A_BOLD)
	    prt_pclod_Output(context, "\33(s0B", -1);

	/** Font typeface change? **/
	if (style->FontID != context->SelectedStyle.FontID)
	    {
	    if (style->FontID == PRT_FONT_T_SANSSERIF)
		prt_pclod_Output(context, "\33(s1P\33(s4148T", -1);
	    else if (style->FontID == PRT_FONT_T_SERIF)
		prt_pclod_Output(context, "\33(s1P\33(s4101T", -1);
	    else
		prt_pclod_Output(context, "\33(s0P\33(s3T", -1);
	    }
	
	/** Font size change? **/
	if (style->FontSize != context->SelectedStyle.FontSize)
	    {
	    /** Set pointsize and pitch, and add .000001 to normalize the floats **/
	    snprintf(pclbuf, 64, "\33(s%.3fH\33(s%.3fV", (120.0/(double)style->FontSize)+0.000001, (double)style->FontSize+0.000001);
	    prt_pclod_Output(context, pclbuf, -1);
	    }

	/** Color change? **/
	if (style->Color != context->SelectedStyle.Color)
	    {
	    /** Only limited color support right now **/
	    color = 0;
	    if ((style->Color & 0x000000FF) >= 0x0080) color += 4;
	    if ((style->Color & 0x0000FF00) >= 0x008000) color += 2;
	    if ((style->Color & 0x00FF0000) >= 0x00800000) color += 1;
	    snprintf(pclbuf, 64, "\33*v%dS", 7-color);
	    prt_pclod_Output(context, pclbuf, -1);
	    }

	/** Record the newly selected style **/
	memcpy(&(context->SelectedStyle), style, sizeof(PrtTextStyle));

    return 0;
    }


/*** prt_pclod_SetHPos() - sets the horizontal (x) position on the page.
 ***/
int
prt_pclod_SetHPos(void* context_v, double x)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    char pclbuf[64];

	/** Generate the horiz-index positioning command. **/
	snprintf(pclbuf, 64, "\33&a%.1fH", (x)*72 + 0.000001);
	prt_pclod_Output(context, pclbuf, -1);

    return 0;
    }


/*** prt_pclod_SetVPos() - sets the vertical (y) position on the page,
 *** independently of the x position (does not emit newlines)
 ***/
int
prt_pclod_SetVPos(void* context_v, double y)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    char pclbuf[64];

	/** Generate the vertical index positioning command. **/
	snprintf(pclbuf, 64, "\33&a%.1fV", (y)*120 + 0.000001);
	prt_pclod_Output(context, pclbuf, -1);

    return 0;
    }


/*** prt_pclod_WriteText() - sends a string of text to the printer.
 ***/
int
prt_pclod_WriteText(void* context_v, char* str)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;
    char pclbuf[64];
    double bl;

	/** Temporarily move the cursor position to adjust for the baseline. **/
	bl = prt_pclod_GetCharacterBaseline(context_v, NULL);
	snprintf(pclbuf, 64, "\33&a+%.1fV", (bl)*120 + 0.000001);
	prt_pclod_Output(context, pclbuf, -1);

	/** output it. **/
	prt_pclod_Output(context, str, -1);

	/** Put the cursor back **/
	snprintf(pclbuf, 64, "\33&a-%.1fV", (bl)*120 + 0.000001);
	prt_pclod_Output(context, pclbuf, -1);

    return 0;
    }


/*** prt_pclod_WriteRasterData() - outputs a block of raster (image) data
 *** at the current printing position on the page, given the selected
 *** pixel and color resolution.  Data[] is an array of 32-bit integers, one
 *** per pixel, containing 0x00RRGGBB values for the pixel.
 ***/
int
prt_pclod_WriteRasterData(void* context_v, int xpixels, int ypixels, unsigned int data[])
    {
    /*pPrtPclodInf context = (pPrtPclodInf)context_v;*/
    return 0;
    }


/*** prt_pclod_WriteFF() - sends a form feed to end the page.
 ***/
int
prt_pclod_WriteFF(void* context_v)
    {
    pPrtPclodInf context = (pPrtPclodInf)context_v;

	/** Create the formfeed/home command. **/
	prt_pclod_Output(context, "\14\33&a0V\33&a0H", -1);

    return 0;
    }


/*** prt_pclod_Initialize() - init this module and register with the main
 *** format driver as a strict output formatter.
 ***/
int
prt_pclod_Initialize()
    {
    pPrtOutputDriver drv;

	/** Register the driver **/
	drv = prt_strictfm_AllocDriver();
	strcpy(drv->Name,"pcl");
	strcpy(drv->ContentType,"text/x-hp-pcl");
	drv->Open = prt_pclod_Open;
	drv->Close = prt_pclod_Close;
	drv->GetResolutions = prt_pclod_GetResolutions;
	drv->SetResolution = prt_pclod_SetResolution;
	drv->GetNearestFontSize = prt_pclod_GetNearestFontSize;
	drv->GetCharacterMetric = prt_pclod_GetCharacterMetric;
	drv->GetCharacterBaseline = prt_pclod_GetCharacterBaseline;
	drv->SetTextStyle = prt_pclod_SetTextStyle;
	drv->SetHPos = prt_pclod_SetHPos;
	drv->SetVPos = prt_pclod_SetVPos;
	drv->WriteText = prt_pclod_WriteText;
	drv->WriteRasterData = prt_pclod_WriteRasterData;
	drv->WriteFF = prt_pclod_WriteFF;

	prt_strictfm_RegisterDriver(drv);

    return 0;
    }


