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

    $Id: prtmgmt_v3_od_text.c,v 1.7 2005/02/26 06:42:40 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_od_text.c,v $

    $Log: prtmgmt_v3_od_text.c,v $
    Revision 1.7  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.6  2005/02/24 05:44:32  gbeeley
    - Adding PostScript and PDF report output formats.  (pdf is via ps2pdf).
    - Special Thanks to Tim Irwin who participated in the Apex NC CODN
      Code-a-Thon on Feb 5, 2005, for much of the initial research on the
      PostScript support!!  See http://www.codn.net/
    - More formats (maybe PNG?) should be easy to add.
    - TODO: read the *real* font metric files to get font geometries!
    - TODO: compress the images written into the .ps file!

    Revision 1.5  2003/04/21 21:00:48  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

    Revision 1.4  2003/03/21 22:41:21  gbeeley
    Enhancement to text output driver to buffer entire page before writing
    it to the output, thus allowing borders and boxes to look nicer in
    the text only output.

    Revision 1.3  2003/03/18 04:06:25  gbeeley
    Added basic image (picture/bitmap) support; only PNG images supported
    at present.  Moved image and border (rectangles) functionality into a
    new file prtmgmt_v3_graphics.c.  Graphics are only monochrome at the
    present and work only under PCL (not plain text!!!).  PNG support is
    via libpng, so libpng was added to configure/autoconf.

    Revision 1.2  2003/03/06 02:52:36  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.1  2003/02/27 22:03:39  gbeeley
    Added text/plain output driver, which will among other things allow me
    to test this beast without wasting so much paper printing PCL stuff ;)


 **END-CVSDATA***********************************************************/


#define PRT_TEXTOD_MAXROWS	256	/* maximum lines per page */
#define PRT_TEXTOD_MAXCOLS	256	/* maximum columns per page */

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
    unsigned char	PageBuf[PRT_TEXTOD_MAXROWS][PRT_TEXTOD_MAXCOLS+1];
    double		LineY[PRT_TEXTOD_MAXROWS];
    int			MaxLine;
    double		MarginTop;
    double		MarginLeft;
    }
    PrtTextodInf, *pPrtTextodInf;


/*** prt_textod_Output() - outputs the given snippet of text 
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


/*** prt_textod_BufWrite() - write data into the page buffer, adjusting
 *** the line end markers, line positions, and line-page-Y locations,
 *** as needed.
 ***/
int
prt_textod_BufWrite(pPrtTextodInf context, double x, double y, char* text)
    {
    int row,col,i,len;

	/** Which row to write into? **/
	row = 0;
	while(row < y && row < PRT_TEXTOD_MAXROWS)
	    {
	    if (context->LineY[row] == 0.0) context->LineY[row] = row;
	    row++;
	    }
	if (row >= PRT_TEXTOD_MAXROWS) return -1; /* offpage */
	if (context->LineY[row] == 0.0)
	    {
	    context->LineY[row] = y;
	    if (context->MaxLine < row) context->MaxLine = row;
	    }
	else if (context->LineY[row] > y)
	    {
	    /** move rows **/
	    if (context->MaxLine >= PRT_TEXTOD_MAXROWS-1) return -1;
	    for(i=context->MaxLine; i>=row; i--)
		{
		memcpy(context->PageBuf[i+1], context->PageBuf[i], PRT_TEXTOD_MAXCOLS+1);
		context->LineY[i+1] = context->LineY[i];
		}
	    context->LineY[row] = y;
	    memset(context->PageBuf[row], 0, PRT_TEXTOD_MAXCOLS+1);
	    }

	/** Ok, found a row.  Now point to the col, padding with spaces if needed. **/
	col = x;
	if (col >= PRT_TEXTOD_MAXCOLS) return -1;
	for(i=0;i<col;i++) if (context->PageBuf[row][i] == '\0') context->PageBuf[row][i] = ' ';

	/** Write the text, adjusting for line drawing if need be. **/
	len = strlen(text);
	for(i=0;i<len;i++)
	    {
	    if (i+col >= PRT_TEXTOD_MAXCOLS) break;
	    if (text[i] == '|' && context->PageBuf[row][col+i] == '-')
		context->PageBuf[row][col+i] = '+';
	    else if (text[i] == '-' && context->PageBuf[row][col+i] == '|')
		context->PageBuf[row][col+i] = '+';
	    else if (text[i] == '-' && context->PageBuf[row][col+i] == '-')
		context->PageBuf[row][col+i] = '=';
	    else if (context->PageBuf[row][col+i] == ' ' || context->PageBuf[row][col+i] == '\0')
		context->PageBuf[row][col+i] = text[i];
	    }

    return 0;
    }


/*** prt_textod_OutputPage() - writes the buffered page to the output.
 ***/
int
prt_textod_OutputPage(pPrtTextodInf context)
    {
    int row;

	/** Output all lines that were written **/
	for(row=0;row<=context->MaxLine;row++)
	    {
	    prt_textod_Output(context, context->PageBuf[row], -1);
	    prt_textod_Output(context, "\r\n", 2);
	    }

	/** form feed **/
	prt_textod_Output(context, "\14", 1);

	/** Clear the page **/
	context->MaxLine = -1;
	for(row=0;row<PRT_TEXTOD_MAXROWS;row++) 
	    {
	    memset(context->PageBuf[row], 0, PRT_TEXTOD_MAXCOLS+1);
	    context->LineY[row] = 0.0;
	    }

    return 0;
    }


/*** prt_textod_Open() - open a new printing session with this driver.
 ***/
void*
prt_textod_Open(pPrtSession s)
    {
    pPrtTextodInf context;
    int i;

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
	context->MaxLine = -1;
	context->MarginTop = 0.0;
	context->MarginLeft = 0.0;
	for(i=0;i<PRT_TEXTOD_MAXROWS;i++) 
	    {
	    memset(context->PageBuf[i], 0, PRT_TEXTOD_MAXCOLS+1);
	    context->LineY[i] = 0.0;
	    }

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


/*** prt_textod_SetPageGeom() - set the size of the page.  Ignored.
 ***/
int
prt_textod_SetPageGeom(void* context_v, double width, double height, double tm, double bm, double lm, double rm)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;

	context->MarginTop = tm;
	context->MarginLeft = lm;

    return 0;
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

	/** Adjust for left margin **/
	x -= context->MarginLeft;
	if (x < 0) x = 0;

	/** Emit enough spaces to move us to the correct column **/
	t = x + 0.00001;
	if (t < context->CurPhysHPos) 
	    {
	    /*return -1;*/
	    context->CurHPos = t;
	    return 0;
	    }
	while(t > context->CurPhysHPos)
	    {
	    d = t - context->CurPhysHPos;
	    if (d > 16) d = 16;
	    /*prt_textod_Output(context, "                ",d);*/
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

	/** Adjust for top margin **/
	y -= context->MarginTop;
	if (y < 0) y = 0;

	/** Use newlines... **/
	n = y + 0.00001;
	while(context->CurVPos < n)
	    {
	    /*prt_textod_Output(context,"\r\n",2);*/
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
	/*prt_textod_Output(context, str, n);*/
	prt_textod_BufWrite(context, context->CurHPos, context->CurVPos, str);
	context->CurHPos += n;
	context->CurPhysHPos += n;

    return 0;
    }


/*** prt_textod_WriteRasterData() - outputs a block of raster (image) data
 *** at the current printing position on the page, given the selected
 *** pixel and color resolution.  Data[] is an array of 32-bit integers, one
 *** per pixel, containing 0x00RRGGBB values for the pixel.  The text driver
 *** does NOT support writing raster data, so we just ignore it (and pretend
 *** it succeeded).
 ***/
double
prt_textod_WriteRasterData(void* context_v, pPrtImage img, double w, double h, double next_y)
    {
    /*pPrtTextodInf context = (pPrtTextodInf)context_v;*/
    return next_y;
    }


/*** prt_textod_WriteFF() - sends a form feed to end the page.
 ***/
int
prt_textod_WriteFF(void* context_v)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;

	/** Issue a formfeed (ctl-L). **/
	/*prt_textod_Output(context, "\14", 1);*/
	prt_textod_OutputPage(context);
	context->CurVPos = 0;
	context->CurHPos = 0;
	context->CurPhysHPos = 0;

    return 0;
    }


/*** prt_textod_WriteRect() - writes a solid rectangular area into the 
 *** document.  Depending on the geometry and so forth, we write a 
 *** character to represent the line or shaded area, such as - or |
 *** or perhaps = or * for a larger area.  Return the absolute Y point at
 *** which we ended the rectangle.  'next_y' is the next Y position on the
 *** page which will be printed after the current row of objects.
 ***/
double
prt_textod_WriteRect(void* context_v, double width, double height, double next_y)
    {
    pPrtTextodInf context = (pPrtTextodInf)context_v;
    double new_y;
    char rectbuf[33];
    int n,cnt;
    char rectch;

	/** Make sure the physical position matches the logical one. **/
	prt_textod_SetHPos(context_v, context->CurHPos);

	/** Select an appropriate character to use **/
	if (width < 1.0 && height >= 1.0) rectch = '|';
	else if (width >= 1.0 && height < 0.3) rectch = '-';
	else if (width >= 1.0 && height >= 0.3 && height < 1.0) rectch = '=';
	else if (width < 1.0 && height < 1.0) rectch = '+';
	else rectch = '*';

	/** How many? **/
	if (width < 1.0) n = 1;
	else n = (width + 0.0001);

	/** Write em **/
	memset(rectbuf,rectch,(n>32)?32:n);
	while(n > 0)
	    {
	    cnt = n;
	    if (cnt > 32) cnt = 32;
	    rectbuf[cnt] = '\0';
	    /*prt_textod_Output(context, rectbuf, cnt);*/
	    prt_textod_BufWrite(context, context->CurHPos, context->CurVPos, rectbuf);
	    context->CurHPos += cnt;
	    context->CurPhysHPos += cnt;
	    n -= cnt;
	    }

	/** How far did we get? */
	if (height < 1.0) new_y = context->CurVPos + height;
	else new_y = context->CurVPos + 1.0;

    return new_y;
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
	drv->SetPageGeom = prt_textod_SetPageGeom;
	drv->GetNearestFontSize = prt_textod_GetNearestFontSize;
	drv->GetCharacterMetric = prt_textod_GetCharacterMetric;
	drv->GetCharacterBaseline = prt_textod_GetCharacterBaseline;
	drv->SetTextStyle = prt_textod_SetTextStyle;
	drv->SetHPos = prt_textod_SetHPos;
	drv->SetVPos = prt_textod_SetVPos;
	drv->WriteText = prt_textod_WriteText;
	drv->WriteRasterData = prt_textod_WriteRasterData;
	drv->WriteFF = prt_textod_WriteFF;
	drv->WriteRect = prt_textod_WriteRect;

	prt_strictfm_RegisterDriver(drv);

    return 0;
    }


