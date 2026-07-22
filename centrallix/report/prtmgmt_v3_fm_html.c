/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_fm_html.c                                    */
/* Author:	Greg Beeley                                             */
/* Date:	April 4th, 2003                                         */
/*									*/
/* Description:	This module is the HTML formatter, which takes a page	*/
/*		structure and outputs structured HTML.  This is made	*/
/*		separate from the html formatter because HTML is not	*/
/*		a html formatting language.				*/
/************************************************************************/

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "barcode.h"
#include "centrallix.h"
#include "cxlib/check.h"
#include "cxlib/expect.h"
#include "cxlib/magic.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "double.h"
#include "htmlparse.h"
#include "prtmgmt_v3/ht_font_metrics.h"
#include "prtmgmt_v3/prtmgmt_v3_fm_html.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_text.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "report.h"


/*** The following are for layout purposes at the page level, not for
 *** tables themselves!
 ***/
#define PRT_HTMLFM_MAXCOLS	(64)
#define PRT_HTMLFM_MAXROWS	(64)


/*** Note: Email reports are HTML-only email (no text/plain alternative).
 *** Every mainstream client (Thunderbird, Gmail, Outlook, Apple Mail) renders
 *** HTML natively, and clients forced to plain text down-convert the HTML on
 *** their own, so a text/plain isn't needed.  In fact, some spam filters may
 *** flag messages with a "mismatched-alternative" (where the fallback has
 *** different content than the email) as junk. HTML-only avoids this trap.
 ***/

/** HTML email headers. **/
#define PRT_HTMLFM_EMAIL_BOUNDARY "cx-email-boundary"

// TODO: Israel - Should some of these come from the mime type driver?
#define PRT_HTMLFM_EMAIL_HEADER \
    /** Email file header. **/ \
    "MIME-Version: 1.0\n" \
    /** multipart/related allows HTML to use "cid:" for inline images. **/ \
    "Content-Type: multipart/related; type=\"text/html\"; boundary="PRT_HTMLFM_EMAIL_BOUNDARY"\n"

#define PRT_HTMLFM_EMAIL_CONTENT_HEADER "\n" \
    "--"PRT_HTMLFM_EMAIL_BOUNDARY"\n" \
    "Content-Type: text/html; charset=utf-8\n" \
    "Content-Transfer-Encoding: 7bit\n" \
    "\n"

/*** The HTML part is closed by the next --cx-email-boundary delimiter (in an
 *** inline image part, or the email footer), so no explicit footer is needed.
 ***/
#define PRT_HTMLFM_EMAIL_CONTENT_FOOTER ""

#define PRT_HTMLFM_IMG_HEADER_FORMAT "\n" \
    "--"PRT_HTMLFM_EMAIL_BOUNDARY"\n" \
    "Content-Type: image/png\n" \
    "Content-Transfer-Encoding: base64\n" \
    "Content-Disposition: inline; filename=image_%d.png\n" \
    "Content-ID: <image_%d>\n" \
    "\n"

#define PRT_HTMLFM_IMG_HEADER_VALUES(id) id, id

#define PRT_HTMLFM_IMG_FOOTER ""

/** Max base64 characters per line in a MIME part.  RFC 2045 caps encoded
 ** lines at 76 chars.
 **/
#define PRT_HTMLFM_B64_LINE_LEN 76

#define PRT_HTMLFM_EMAIL_FOOTER \
    "--"PRT_HTMLFM_EMAIL_BOUNDARY"--\n"


/** HTML document headers. **/
/* CLS 2025-03-28: Note that changing the HTML version may change spacing between lines/wrapped text.*/
#define PRT_HTMLFM_HEADER \
    "<!DOCTYPE html>\n" \
    "<html lang=\"en\">\n" \
    "<head>\n" \
	"<title>Centrallix HTML Document</title>\n" \
	"<meta charset=\"utf-8\">\n" \
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n" \
	"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
	"<meta name=\"pragma\" content=\"no-cache\">\n" \
	"<meta name=\"referrer\" content=\"same-origin\">\n" \
	"<meta name=\"generator\" content=\"Centrallix PRTMGMT v3.0\">\n" \
	/** Universal email styles. **/ \
	"<style>\n" \
	    "body { font-family: \"Courier New\",Courier,fixed; }\n" \
	    "td { vertical-align: top; line-height: 1; }\n" \
	"</style>\n" \
    "</head>\n" \
    "<body style=\"background-color: %s;\">\n"

/*** Document footer ***/
#define PRT_HTMLFM_FOOTER	"</body>\n" \
				"</html>\n"


/*** Page header - build the graphical layout showing the 'page'
 ***
 *** Params:
 ***    (1) %d	Width of table, pixels
 ***/
#define PRT_HTMLFM_PAGEHEADER \
	"<center>\n" \
	"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#606060\">\n" \
	    "<tr>\n" \
		"<td bgcolor=\"#000000\">\n" \
		"<table width=\"%d\" border=\"0\" cellspacing=\"1\" cellpadding=\"16\">\n" \
		    "<tr><td width=\"100%\" bgcolor=\"#ffffff\">\n\n"


/*** Page footer - end the page ***/
#define PRT_HTMLFM_PAGEFOOTER "\n" \
		    "</td></tr>\n" \
		"</table>\n" \
		"</td><td valign=\"top\" align=\"left\" width=\"8\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
	    "</tr><tr>\n" \
		"<td width=\"8\" align=\"left\" valign=\"top\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
		"<td><small>&nbsp;</small></td>\n" \
	    "</tr>\n" \
	"</table>\n" \
	"</center>\n" \
	"<br>\n"


/*** this puts the min size at 9 (1), max size at 26 (7), and standard size at 12 (3) ***/
static int prt_htmlfm_fontsize_to_htmlsize[] = {8,9,10,12,15,19,22,26};
#define PRT_HTMLFM_MINFONTSIZE	(1)
#define	PRT_HTMLFM_MAXFONTSIZE	(sizeof(prt_htmlfm_fontsize_to_htmlsize) / sizeof(prt_htmlfm_fontsize_to_htmlsize[0]) - 1)

/*** Declare supported font family styles.
 *** 
 *** PRT_HTMLFM_DEFAULT_FONTSTYLE is an index into prt_htmlfm_fontstyles[] for
 *** the report's most common font. This is set in the head tag, so that later
 *** styling can skip setting it, reducing HTML size.
 ***/
static char* prt_htmlfm_fontstyles[3] = { "Courier New,Courier,fixed", "Arial,Helvetica,MS Sans Serif", "Times New Roman,Times,MS Serif"};
#define PRT_HTMLFM_MINFONTSTYLE	(0)
#define PRT_HTMLFM_MAXFONTSTYLE	(sizeof(prt_htmlfm_fontstyles) / sizeof(prt_htmlfm_fontstyles[0]) - 1)
#define PRT_HTMLFM_DEFAULT_FONTSTYLE	(0)

static PrtHTMLfmSubtype prt_htmlfm_subtypes[] =
    {
    { "text/vnd.cx.paginated+html", "text/html", PRT_HTMLFM_F_PAGINATED },
    { "text/html", "text/html", 0 },
    { "multipart/vnd.cx.htmlemail+mixed", "multipart/related", PRT_HTMLFM_F_EMAIL },
    };
#define PRT_HTMLFM_N_SUBTYPES (sizeof(prt_htmlfm_subtypes) / sizeof(prt_htmlfm_subtypes[0]))

/*** GLOBAL DATA FOR THIS MODULE ***/
struct _PSF
    {
    unsigned long	ImageID;
    }
    PRT_HTMLFM;


#define MAX_IMAGE_SIZE (10 * 1024 * 1024) // 10 MB for image buffer

/*** Struct that holds a raw file and its size
 ***/
typedef struct
    {
    char*      buffer;
    size_t     size;
    size_t     capacity;
    } ImageBuffer;


/*** prt_htmlfm_Output() - outputs a string of text into the HTML
 *** document.
 ***/
int
prt_htmlfm_Output(pPrtHTMLfmInf context, char* str, int len)
    {

	/** Check length **/
	if (len < 0) len = strlen(str);

    return context->Session->WriteFn(context->Session->WriteArg, str, len, 0, FD_U_PACKET);
    }


/*** prt_htmlfm_OutputPrintf() - outputs a string of text into the
 *** HTML document, using "printf" semantics.
 ***/
int
prt_htmlfm_OutputPrintf(pPrtHTMLfmInf context, char* fmt, ...)
    {
    va_list va;
    int rval;

	va_start(va, fmt);
	rval = xsGenPrintf_va(context->Session->WriteFn, context->Session->WriteArg, NULL, NULL, fmt, va);
	va_end(va);

    return rval;
    }


/*** prt_htmlfm_OutputEncoded() - outputs a string of text into
 *** the html document, escaping the appropriate characters to
 *** avoid unintentional html or script commands.
 ***/
int
prt_htmlfm_OutputEncoded(pPrtHTMLfmInf context, char* str, int len)
    {
    char* badcharpos;
    char* repl;
    int offset = 0, endoffset;

	/** Check length **/
	if (len < 0) len = strlen(str);

	/** Output with care... **/
	while (str[offset] && offset < len)
	    {
	    badcharpos = strpbrk(str+offset, "<>& ");
	    if (badcharpos)
		endoffset = badcharpos - str;
	    else
		endoffset = len;
	    if (endoffset - offset > 0)
		prt_htmlfm_Output(context, str+offset, endoffset - offset);

	    if (str[offset] != ' ')
		{
		/* we are no longer in our leading spaces, so don't &nbsp them */
		context->StyleFlags &= (~PRT_HTMLFM_SF_KEEPSPACES);
		}
	    if (badcharpos)
		{
		switch (*badcharpos)
		    {
		    case '<': repl = "&lt;"; break;
		    case '>': repl = "&gt;"; break;
		    case '&': repl = "&amp;"; break;
		    case ' ': repl = ( context->StyleFlags & PRT_HTMLFM_SF_KEEPSPACES ) ? "&nbsp;" : " "; break;
		    default: repl = ""; break;
		    }
		prt_htmlfm_Output(context, repl, -1);
		endoffset++;
		}
	    offset = endoffset;
	    }

    return len;
    }


/*** prt_htmlfm_Probe() - this function is called when a new printmanagement
 *** session is opened and this driver is being asked whether or not it can
 *** print the given content type.
 ***/
void*
prt_htmlfm_Probe(pPrtSession s, char* output_type)
    {
	/** Allocate our context inf structure **/
	pPrtHTMLfmInf context = check_ptr(nmMalloc(sizeof(PrtHTMLfmInf)));
	if (context == NULL) goto reject;
	memset(context, 0, sizeof(PrtHTMLfmInf));
	context->Session = s;

	/** Is it an html type we can handle? **/
	for (int i = 0; i < PRT_HTMLFM_N_SUBTYPES; i++)
	    {
	    if (strcasecmp(output_type, prt_htmlfm_subtypes[i].MimeType) == 0)
		{
		context->Subtype = &(prt_htmlfm_subtypes[i]);
		context->Flags = context->Subtype->SessionFlags;
		break;
		}
	    }
	if (context->Subtype == NULL)
	    goto reject;

	/** Allocate attachments. */
	context->Attachments = check_ptr(xaNew(10));
	if (context->Attachments == NULL) goto reject;

	/** Write email headers. **/
	if (context->Flags & PRT_HTMLFM_F_EMAIL)
	    {
	    prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_EMAIL_HEADER);
	    prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_EMAIL_CONTENT_HEADER);
	    }

	/*** Write HTML header.  Report content always sits on a white page area
	 *** (white body, or the white page cell in paginated mode), so white is
	 *** the current background, letting us skip setting the background on
	 *** white cells, reducing the HTML size.
	 ***/
	context->BGColor = 0xFFFFFF;
	const char* background_color = (context->Flags & PRT_HTMLFM_F_PAGINATED) ? "#c0c0c0" : "#ffffff";
	prt_htmlfm_OutputPrintf(context, PRT_HTMLFM_HEADER, background_color);

	/** Success, we can print this content type. **/
	return (void*)context;

    reject: /* We cannot print this content type. */
	if (context->Attachments != NULL) xaFree(context->Attachments);
	if (LIKELY(context != NULL)) nmFree(context, sizeof(PrtHTMLfmInf));

	return NULL;
    }


/*** prt_htmlfm_GetOutputType - get the content type for the output of this
 *** formatter session.  This may vary from the requested type, which may
 *** be more specific in some cases.
 ***/
char*
prt_htmlfm_GetOutputType(void* context_v)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    return context->Subtype->OutputMimeType;
    }


/*** prt_htmlfm_GetNearestFontSize - return the nearest font size that this
 *** driver supports.  In this case, this just queries the underlying output
 *** driver for the information.
 ***/
double
prt_htmlfm_GetNearestFontSize(void* context_v, double req_size)
    {
    /*pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;*/
    int i;

	/** Check min/max **/
	if (req_size > prt_htmlfm_fontsize_to_htmlsize[PRT_HTMLFM_MAXFONTSIZE])
	    return prt_htmlfm_fontsize_to_htmlsize[PRT_HTMLFM_MAXFONTSIZE];
	if (req_size < prt_htmlfm_fontsize_to_htmlsize[PRT_HTMLFM_MINFONTSIZE])
	    return prt_htmlfm_fontsize_to_htmlsize[PRT_HTMLFM_MINFONTSIZE];

	/** Grab size from the list **/
	for (i=PRT_HTMLFM_MINFONTSIZE;i<=PRT_HTMLFM_MAXFONTSIZE;i++)
	    {
	    if (req_size <= prt_htmlfm_fontsize_to_htmlsize[i])
		return prt_htmlfm_fontsize_to_htmlsize[i];
	    }

    return req_size;
    }


/*** prt_htmlfm_GetCharacterMetric - return the sizing information for a given
 *** character, in standard units.
 ***/
void
prt_htmlfm_GetCharacterMetric(void* context_v, char* str, pPrtTextStyle style, double* width, double* height)
    {
    /*pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;*/
    double n;
    int a;
    
	/** Based on font, style, and size... **/
	if (style->FontID == PRT_FONT_T_MONOSPACE)
	    {
	    *width = strlen(str)*style->FontSize/12.0;
	    *height = style->FontSize/12.0;
	    return;
	    }

	/** Figure based on attribute **/
	a = 0;
	if (style->Attr & PRT_OBJ_A_ITALIC) a += 1;
	if (style->Attr & PRT_OBJ_A_BOLD) a += 2;

	/** Ok, using times or helvetica. **/
	n = 0.0;
	while (*str)
	    {
	    if (*str < 0x20 || *str > 0x7E)
		n += 1.0;
	    else if (style->FontID == PRT_FONT_T_SANSSERIF)
		n += prt_htmlfm_helvetica_font_metrics[(*str) - 0x20][a]/60.0;
	    else if (style->FontID == PRT_FONT_T_SERIF)
		n += prt_htmlfm_times_font_metrics[(*str) - 0x20][a]/60.0;
	    else
		n += 1.0;
	    str++;
	    }

	*width = n*style->FontSize/12.0;
	*height = style->FontSize/12.0;

    return;
    }


/*** prt_htmlfm_GetCharacterBaseline - return the distance from the upper
 *** left corner of the character cell to the left baseline point of the 
 *** character cell, in standard units.
 ***/
double
prt_htmlfm_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    /*pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;*/
    return 0.75*style->FontSize/12.0;
    }


/*** prt_htmlfm_Close() - end a printing session and destroy the context
 *** structure.
 ***/
int
prt_htmlfm_Close(void* context_v)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    int rval = -1;

	/** Write HTML footer. **/
	prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_FOOTER);

	/** Write the email content footer (for email reports). **/
	if (context->Flags & PRT_HTMLFM_F_EMAIL)
	    prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_EMAIL_CONTENT_FOOTER);

	/** Write attachments for emails. **/
	if (context->Flags & PRT_HTMLFM_F_EMAIL)
	    {
	    if (UNLIKELY(context->Attachments == NULL))
		print_fail("Warning: Attachments array missing for email.");

	    for (int i = 0; i < xaCount(context->Attachments); i++)
		{
		char* attachment_str = check_ptr(xsString(xaGetItem(context->Attachments, i)));
		if (attachment_str == NULL) goto end;
		prt_htmlfm_Output(context, attachment_str, -1);
		}
	    }

	/** Write email footer. **/
	if (context->Flags & PRT_HTMLFM_F_EMAIL)
	    prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_EMAIL_FOOTER);

	/** Success. **/
	rval = 0;

    end:
	if (UNLIKELY(rval != 0))
	    mssError(1, "PRT", "Failed to close HTML report formatter.");

	/** Free memory used **/
	if (LIKELY(context != NULL))
	    {
	    if (LIKELY(context->Attachments != NULL))
		{
		xaClear(context->Attachments, (void*)xsFree, NULL);
		xaFree(context->Attachments);
		}
	    nmFree(context, sizeof(PrtHTMLfmInf));
	    }

    return rval;
    }

const char*
prt_htmlfm_GetFont(pPrtTextStyle style)
    {
    int fontid = style->FontID - 1;
    if (fontid < PRT_HTMLFM_MINFONTSTYLE || fontid > PRT_HTMLFM_MAXFONTSTYLE)
	fontid = PRT_HTMLFM_MINFONTSTYLE;
    
    return prt_htmlfm_fontstyles[fontid];
    }


/*** prt_htmlfm_SetStyle() - output the html to change the text style
 ***/
int
prt_htmlfm_SetStyle(pPrtHTMLfmInf context, pPrtTextStyle style)
    {
    int boldchanged, italicchanged, underlinechanged, fontchanged;

	/** Close out current style settings? **/
	boldchanged = (style->Attr ^ context->CurStyle.Attr) & PRT_OBJ_A_BOLD;
	italicchanged = (style->Attr ^ context->CurStyle.Attr) & PRT_OBJ_A_ITALIC;
	underlinechanged = (style->Attr ^ context->CurStyle.Attr) & PRT_OBJ_A_UNDERLINE;
	fontchanged = (style->FontID != context->CurStyle.FontID || 
		realComparePrecision(style->FontSize, context->CurStyle.FontSize, 0.5) != 0 || 
		style->Color != context->CurStyle.Color);

	if ((!context->InitStyle) && (context->ExitStyle || boldchanged || italicchanged || underlinechanged || fontchanged))
	    {
	    /*For each thing, check dirty flag is clear to ensure opening tag was actually written*/
	    if ((context->CurStyle.Attr & PRT_OBJ_A_BOLD) && 
		!(context->StyleFlags & PRT_HTMLFM_SF_BOLDDIRTY)) prt_htmlfm_OutputStrLiteral(context, "</b>");
	    if (context->ExitStyle || italicchanged || underlinechanged || fontchanged)
		{
		if (context->CurStyle.Attr & PRT_OBJ_A_ITALIC &&
		    !(context->StyleFlags & PRT_HTMLFM_SF_ITALICDIRTY)) prt_htmlfm_OutputStrLiteral(context, "</i>");
		if (context->ExitStyle || underlinechanged || fontchanged)
		    {
		    if (context->CurStyle.Attr & PRT_OBJ_A_UNDERLINE && 
			!(context->StyleFlags & PRT_HTMLFM_SF_UNDERLINEDIRTY)) prt_htmlfm_OutputStrLiteral(context, "</u>");
		    if ((context->ExitStyle || fontchanged) &&
			!(context->StyleFlags & PRT_HTMLFM_SF_FONTDIRTY))
			{
			prt_htmlfm_OutputStrLiteral(context, "</font>");
			}
		    }
		}
	    }
	if (context->ExitStyle) return 0;

	/*Set the dirty flags as appropriate*/
	if (context->InitStyle || boldchanged || italicchanged || underlinechanged || fontchanged)
	    {
	    if (context->InitStyle || italicchanged || underlinechanged || fontchanged)
		{
		if (context->InitStyle || underlinechanged || fontchanged)
		    {
		    if (context->InitStyle || fontchanged)
			{
			context->StyleFlags |= PRT_HTMLFM_SF_FONTDIRTY;
			}
		    if (style->Attr & PRT_OBJ_A_UNDERLINE)
			context->StyleFlags |= PRT_HTMLFM_SF_UNDERLINEDIRTY;
		    }
		if (style->Attr & PRT_OBJ_A_ITALIC)
		    context->StyleFlags |= PRT_HTMLFM_SF_ITALICDIRTY;
		}
	    if (style->Attr & PRT_OBJ_A_BOLD)
		context->StyleFlags |= PRT_HTMLFM_SF_BOLDDIRTY;

	    memcpy(&(context->CurStyle), style, sizeof(PrtTextStyle));
	    }

    return 0;
    }

int
prt_htmlfm_WriteStyle(pPrtHTMLfmInf context)
    {
    pPrtTextStyle style = &(context->CurStyle);
    int htmlfontsize;
    int i;
    char stylebuf[128];
    
    /** Figure the size **/
    for (i=PRT_HTMLFM_MINFONTSIZE;i<=PRT_HTMLFM_MAXFONTSIZE;i++)
	{
        if (realComparePrecision(prt_htmlfm_fontsize_to_htmlsize[i], style->FontSize, 0.5) == 0)
	    {
	    htmlfontsize = i;
	    break;
	    }
	}
    /*htmlfontsize = style->FontSize - PRT_HTMLFM_FONTSIZE_DEFAULT + PRT_HTMLFM_FONTSIZE_OFFSET;*/
    
    if (context->StyleFlags & PRT_HTMLFM_SF_FONTDIRTY)
	{
	/** Write the font, omitting face= if the font is the document default. **/
	const char* face = prt_htmlfm_GetFont(style);
	if (strcmp(face, prt_htmlfm_fontstyles[PRT_HTMLFM_DEFAULT_FONTSTYLE]) == 0)
	    {
	    snprintf(
		stylebuf, sizeof(stylebuf),
	    	"<font color=\"#%6.6X\" size=\"%d\">",
		style->Color, htmlfontsize
	    );
	    }
	else
	    {
	    snprintf(
		stylebuf, sizeof(stylebuf),
		"<font face=\"%s\" color=\"#%6.6X\" size=\"%d\">",
		face, style->Color, htmlfontsize
	    );
	    }
	prt_htmlfm_Output(context, stylebuf, -1);
	}
    if (context->StyleFlags & PRT_HTMLFM_SF_UNDERLINEDIRTY)
	{
	prt_htmlfm_OutputStrLiteral(context, "<u>");
	}
    if (context->StyleFlags & PRT_HTMLFM_SF_ITALICDIRTY)
	{
	prt_htmlfm_OutputStrLiteral(context, "<i>");
	}
    if (context->StyleFlags & PRT_HTMLFM_SF_BOLDDIRTY)
	{
	prt_htmlfm_OutputStrLiteral(context, "<b>");
	}

    /*Clear the dirty flags*/
    context->StyleFlags &= ~ (PRT_HTMLFM_SF_FONTDIRTY | PRT_HTMLFM_SF_UNDERLINEDIRTY |
	    PRT_HTMLFM_SF_ITALICDIRTY | PRT_HTMLFM_SF_BOLDDIRTY);
    
    return 0;
    }

/*** prt_htmlfm_InitStyle() - initialize style settings, as if we are 
 *** entering a new subcontainer.
 ***/
int
prt_htmlfm_InitStyle(pPrtHTMLfmInf context, pPrtTextStyle style)
    {

	/** Set all style settings, and indicate init mode **/
	context->InitStyle = 1;
	memcpy(&(context->CurStyle), style, sizeof(PrtTextStyle));

	/** Call for a style change **/
	prt_htmlfm_SetStyle(context, style);
	context->InitStyle = 0;

    return 0;
    }


/*** prt_htmlfm_ResetStyle() - reset the rendering state to the saved state,
 *** typically called when closing a container.
 ***/
int
prt_htmlfm_ResetStyle(pPrtHTMLfmInf context, pPrtHTMLfmSavedStyle saved)
    {

	/** Set style settings, and do nothing else **/
	memcpy(&(context->CurStyle), &(saved->Style), sizeof(PrtTextStyle));
	context->StyleFlags = saved->Flags;

    return 0;
    }


/*** prt_htmlfm_SaveStyle() - save the current rendering state to the save
 *** struct so that it can be restored later by prt_htmlfm_ResetStyle().
 *** Commonly used when entering a container so the state can be restored
 *** when exitting the container.
 ***/
int
prt_htmlfm_SaveStyle(pPrtHTMLfmInf context, pPrtHTMLfmSavedStyle saved)
    {

	/** Save style settings **/
	memcpy(&(saved->Style), &(context->CurStyle), sizeof(PrtTextStyle));
	saved->Flags = context->StyleFlags;

    return 0;
    }


/*** prt_htmlfm_EndStyle() - close out a style setting just before
 *** exiting a container.
 ***/
int
prt_htmlfm_EndStyle(pPrtHTMLfmInf context)
    {
    PrtTextStyle dummy_style;

	context->ExitStyle = 1;
	prt_htmlfm_SetStyle(context, &dummy_style);
	context->ExitStyle = 0;

    return 0;
    }

/*** prt_htmlfm_SetKeepSpaces() - turn &nbsp replacement on until 
 *** the next non-space character 
 ***/
void
prt_htmlfm_SetKeepSpaces(pPrtHTMLfmInf context)
    {
    context->StyleFlags |= PRT_HTMLFM_SF_KEEPSPACES;
    }


/*** prt_htmlfm_OutputBGColor() - Write a bgcolor attribute, but only when the
 *** color differs from the background.  The new background is remembered.
 *** This method reduces HTML size.
 *** Returns the previous background color (useful if you return to it later).
 ***/
int
prt_htmlfm_OutputBGColor(pPrtHTMLfmInf context, int bgcolor)
    {
    int prev = context->BGColor;
    if (bgcolor != prev)
	{
	prt_htmlfm_OutputPrintf(context, " bgcolor=\"#%6.6X\"", bgcolor);
	context->BGColor = bgcolor;
	}
    return prev;
    }


/*** prt_htmlfm_Border() - use nested tables to create a border matching
 *** the given border structure, with an appropriate margin setting from
 *** the given prt object.  Zero-width borders (those with no lines) are
 *** skipped to reduce HTML size.
 ***/
int
prt_htmlfm_Border(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj)
    {
    int i;
    int m,bw,iw;

	/** Figure the margins **/
	m = (obj->MarginTop + obj->MarginBottom + obj->MarginLeft + obj->MarginRight)*PRT_HTMLFM_XPIXEL/4;

	/** Construct the border for each element **/
	for (i=0;i<border->nLines;i++)
	    {
	    /** Output border line itself **/
	    bw = border->Width[i]*PRT_HTMLFM_XPIXEL + 0.5;
	    if (bw == 0) bw = 1;
	    iw = ((i==border->nLines-1)?m:(border->Sep*PRT_HTMLFM_XPIXEL)) + 0.5;
	    if (iw == 0 && i!=border->nLines-1) iw = 1;
	    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"%d\"><tr><td bgcolor=\"#%6.6X\">",
		    (int)(bw),
		    (int)(border->Color[i]));
	    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"%d\"><tr><td bgcolor=\"#%6.6X\">\n",
		    (int)(iw),
		    (int)(obj->BGColor));
	    }

    return 0;
    }


/*** prt_htmlfm_EndBorder() - end a nested table structure implementing
 *** a border.
 ***/
int
prt_htmlfm_EndBorder(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj)
    {
    int i;

	/** Construct the end-border for each element **/
	for (i=0;i<border->nLines;i++)
	    {
	    /** Output border line itself **/
	    prt_htmlfm_OutputStrLiteral(context, "</td></tr></table></td></tr></table>\n");
	    }

    return 0;
    }

//TODO CSMITH put in .h
/** Gets size of image file */
int
ImageWriteFn(void *arg, const void *data, size_t len)
    {
	ImageBuffer *imgBuf = (ImageBuffer *)arg;
	if (imgBuf->size + len > imgBuf->capacity)
	    {
	    return -1;  // Buffer overflow
	    }

	memcpy(imgBuf->buffer + imgBuf->size, data, len);
	imgBuf->size += len;
	return len;
    }

//TODO CSMITH put in .h
/** Encodes a char* input to base64 */
char*
base64_encode(const unsigned char *input, size_t len)
    {
	const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t out_len = 4 * ((len + 2) / 3);
	char *output = (char *)nmMalloc(out_len + 1);
	if (!output) return NULL;

	char *p = output;
	for (size_t i = 0; i < len; i += 3)
	    {
	    int val = (input[i] << 16) | ((i + 1 < len ? input[i + 1] : 0) << 8) | (i + 2 < len ? input[i + 2] : 0);
	    *p++ = b64_table[(val >> 18) & 0x3F];
	    *p++ = b64_table[(val >> 12) & 0x3F];
	    *p++ = (i + 1 < len) ? b64_table[(val >> 6) & 0x3F] : '=';
	    *p++ = (i + 2 < len) ? b64_table[val & 0x3F] : '=';
	    }
	*p = '\0';
	return output;
    }


/*** prt_htmlfm_Generate_r() - recursive worker routine to do the bulk
 *** of page generation.
 ***/
int
prt_htmlfm_Generate_r(pPrtHTMLfmInf context, pPrtObjStream obj)
    {
    int w,h;
    unsigned long id;
    int justif = 0;
    char* justifytypes[] = { "left", "right", "center", "justify" };

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Select the type of object we're formatting **/
	switch (obj->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_STRING:
		prt_htmlfm_SetStyle(context, &(obj->TextStyle));

		if (strlen((char*) obj->Content))
		    {
		    prt_htmlfm_WriteStyle(context);
		    }

		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_OutputStrLiteral(context, "<a href=\"");
		    prt_htmlfm_OutputEncoded(context, obj->URL, -1);
		    prt_htmlfm_OutputStrLiteral(context, "\">");
		    }
		prt_htmlfm_OutputEncoded(context, (char*)obj->Content, -1);

		if ((obj->Flags & PRT_OBJ_F_SOFTNEWLINE) && (obj->Flags & PRT_TEXTLM_F_RMSPACE))
		    {
		    prt_htmlfm_OutputEncoded(context, " ", 1);
		    }

		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_OutputStrLiteral(context, "</a>");
		    }
		break;
	    case PRT_OBJ_T_AREA:
		prt_htmlfm_GenerateArea(context, obj);
		break;

	    case PRT_OBJ_T_SECTION:
		prt_htmlfm_GenerateMultiCol(context, obj);
		break;

	    case PRT_OBJ_T_RECT:
		/** Don't output rectangles that are container decorations added
		 ** by finalize routines in the layout managers.  We really need a 
		 ** better way to tell this than the conditional below.
		 **/
		if (obj->Parent && obj->Parent->ObjType->TypeID != PRT_OBJ_T_SECTION && !(obj->Flags & PRT_OBJ_F_MARGINRELEASE))
		    {
		    w = obj->Width*PRT_HTMLFM_XPIXEL;
		    h = obj->Height*PRT_HTMLFM_YPIXEL;
		    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr><td bgcolor=\"#%6.6X\" width=\"%d\" height=\"%d\"><table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr><td></td></tr></table></td></tr></table>\n",
			    obj->TextStyle.Color, w, h);
		    }
		break;

	    case PRT_OBJ_T_IMAGE:
	    case PRT_OBJ_T_SVG:
		justif=0;
		if (obj->Parent)
		    {
		    if (obj->X > 0.1 && obj->Parent->Width - obj->Parent->MarginLeft - obj->Parent->MarginRight - obj->Width - 0.1 <= obj->X)
			{
			justif = 1;
			}
		    }
		
		id = PRT_HTMLFM.ImageID++;
		w = obj->Width*PRT_HTMLFM_XPIXEL;
		h = obj->Height*PRT_HTMLFM_YPIXEL;
		if (w <= 0) w = 1;
		if (h <= 0) h = 1;
		
		// lifetime start: buf
		ImageBuffer imgBuf = { (char *)nmMalloc(MAX_IMAGE_SIZE), 0, MAX_IMAGE_SIZE };
		if (!imgBuf.buffer)
		    {
		    mssError(1, "PRT", "nmMalloc() failed\n");
		    return -1;
		    }
		
		// Capture image to buffer
		//TODO we weren't supposed to replace context->Session->ImageWriteFn with ImageWriteFn,
		// except the former references the image store I think which we don't want anymore...
		if(obj->ObjType->TypeID == PRT_OBJ_T_IMAGE)
		    {
		    prt_internal_WriteImageToPNG(ImageWriteFn, &imgBuf, (pPrtImage)(obj->Content), w, h);
		    }
		else
		    {
		    prt_internal_WriteSvgToFile(ImageWriteFn, &imgBuf, (pPrtSvg)(obj->Content), w, h);
		    }

		// Encode image to base64
		// copy out of lifetime: buf into img
		char *base64Image = base64_encode((unsigned char *)imgBuf.buffer, imgBuf.size);
		// lifetime end: buf
		nmFree(imgBuf.buffer, MAX_IMAGE_SIZE);
		if (!base64Image)
		    {
		    mssError(1, "PRT", "Base64 encoding failed\n");
		    return -1;
		    }
		
		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_OutputStrLiteral(context, "<a href=\"");
		    prt_htmlfm_OutputEncoded(context, obj->URL, -1);
		    prt_htmlfm_OutputStrLiteral(context, "\">");
		    }
	
		if (context->Flags & PRT_HTMLFM_F_EMAIL)
		    {
		    prt_htmlfm_OutputPrintf(context, "<img src=\"cid:image_%d\"", id);
		    
		    /*** Add this attachment to the context.  The base64 data
		     *** is wrapped at PRT_HTMLFM_B64_LINE_LEN chars per line.
		     ***/
		    pXString attachment = xsNew();
		    xsConcatPrintf(attachment,
			PRT_HTMLFM_IMG_HEADER_FORMAT,
			PRT_HTMLFM_IMG_HEADER_VALUES(id)
		    );
		    size_t b64_len = strlen(base64Image);
		    for (size_t off = 0; off < b64_len; off += PRT_HTMLFM_B64_LINE_LEN)
			{
			size_t chunk = b64_len - off;
			if (chunk > PRT_HTMLFM_B64_LINE_LEN) chunk = PRT_HTMLFM_B64_LINE_LEN;
			xsConcatenate(attachment, base64Image + off, chunk);
			xsConcatenate(attachment, "\n", 1);
			}
		    xsConcatenate(attachment, PRT_HTMLFM_IMG_FOOTER, -1);
		    xaAddItem(context->Attachments, attachment);
		    }
		else
		    {
		    if (obj->ObjType->TypeID == PRT_OBJ_T_IMAGE)
			{
			prt_htmlfm_OutputPrintf(context, "<img src=\"data:image/png;base64,%s\"", 
			    base64Image, justifytypes[justif], w, h);
			}
		    else
			{
			prt_htmlfm_OutputPrintf(context, "<img src=\"data:image/svg+xml;base64,%s\"", 
			    base64Image, justifytypes[justif], w, h);
			}
		    }
		prt_htmlfm_OutputPrintf(context, " align=\"%s\" border=\"0\" width=\"%d\" height=\"%d\">", 
		    justifytypes[justif], w, h);

		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_OutputStrLiteral(context, "</a>");
		    }
		
		// lifetime end: img
		nmFree(base64Image, strlen(base64Image));
		
		break;
	    
	    case PRT_OBJ_T_TABLE:
		prt_htmlfm_GenerateTable(context, obj);
		break;
	    }

    return 0;
    }


/*** prt_htmlfm_Generate() - generate the html for the page.  Basically,
 *** walk through the document and generate appropriate html layout to
 *** make the thing look similar to what it should.  Does not yet support
 *** overlapping objects on a page.
 ***/
int
prt_htmlfm_Generate(void* context_v, pPrtObjStream page_obj)
    {
    char* justifytypes[] = { "left", "right", "center", "justify" };
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    pPrtObjStream subobj;
    double colpos[PRT_HTMLFM_MAXCOLS];
    double rowpos[PRT_HTMLFM_MAXROWS];
    int n_cols=0, n_rows=0;
    int found;
    int i;
    int w;
    int cur_row, cur_col;
    double last_height;
    int rs,cs;

	/** Write the page HTML (for paginated reports). **/
	if (context->Flags & PRT_HTMLFM_F_PAGINATED)
	    prt_htmlfm_OutputPrintf(context, PRT_HTMLFM_PAGEHEADER, (int)(page_obj->Width*PRT_HTMLFM_XPIXEL+0.001)+34);

	/** Write a table to handle page margins **/
	int left_margin = (int)(page_obj->MarginLeft*PRT_HTMLFM_XPIXEL+0.001);
	int center_width = (int)((page_obj->Width - page_obj->MarginLeft - page_obj->MarginRight+0.001)*PRT_HTMLFM_XPIXEL);
	int right_margin = (int)(page_obj->MarginRight*PRT_HTMLFM_XPIXEL+0.001);
	int top_margin = (int)((page_obj->MarginTop+0.001)*PRT_HTMLFM_YPIXEL);
	prt_htmlfm_OutputStrLiteral(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n");
	prt_htmlfm_OutputStrLiteral(context, "<colgroup>");
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", left_margin);
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", center_width);
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", right_margin);
	prt_htmlfm_OutputStrLiteral(context, "</colgroup>");
	/* Print the first row, empty with appropriate margins*/
	prt_htmlfm_OutputPrintf(context, "<tr><td style=\"height: %dpx; width: %dpx;\"></td><td style=\"height: %dpx;width: %dpx;\"></td><td style=\"height: %dpx;width: %dpx;\"></td></tr>",
		top_margin, left_margin,
		top_margin, center_width,
		top_margin, right_margin);
	/* Start the second row (with appropriate margin) */
	prt_htmlfm_OutputPrintf(context, "<tr><td style=\"width: %dpx;\"></td><td>\n", left_margin);


	/** We need to scan the absolute-positioned content to figure out how many
	 ** "columns" and "rows" we need to put in the "table" used for layout
	 ** purposes.
	 **/
	for (subobj=page_obj->ContentHead; subobj; subobj=subobj->Next)
	    {
	    if (n_cols < PRT_HTMLFM_MAXCOLS)
		{
		/** Search for the X position in the 'colpos' list **/
		found = n_cols;
		for(i=0;i<n_cols;i++)
		    {
		    if (subobj->X == colpos[i]) 
			{
			found = -1;
			break;
			}
		    if (subobj->X < colpos[i])
			{
			found=i;
			break;
			}
		    }
		if (found != -1)
		    {
		    for(i=n_cols-1;i>=found;i--) colpos[i+1] = colpos[i];
		    colpos[found] = subobj->X;
		    n_cols++;
		    }
		}
	    if (n_rows < PRT_HTMLFM_MAXROWS)
		{
		/** Search for the Y position in the 'rowpos' list **/
		found = n_rows;
		for(i=0;i<n_rows;i++)
		    {
		    if (subobj->Y == rowpos[i]) 
			{
			found = -1;
			break;
			}
		    if (subobj->Y < rowpos[i])
			{
			found=i;
			break;
			}
		    }
		if (found != -1)
		    {
		    for(i=n_rows-1;i>=found;i--) rowpos[i+1] = rowpos[i];
		    rowpos[found] = subobj->Y;
		    n_rows++;
		    }
		}
	    }

	/** Write the layout table **/
	prt_htmlfm_OutputStrLiteral(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n");
	for (i=0;i<n_cols;i++)
	    {
	    if (i == n_cols-1)
		w = (page_obj->Width - page_obj->MarginLeft - page_obj->MarginRight - colpos[i])*PRT_HTMLFM_XPIXEL;
	    else
		w = (colpos[i+1] - colpos[i])*PRT_HTMLFM_XPIXEL;
	    prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", w);
	    }

	/** Generate the body of the page, by selectively walking the YPrev/YNext chain **/
	cur_row = 0;
	cur_col = 0;
	last_height = 0.0;
	prt_htmlfm_OutputStrLiteral(context, "<tr>");
	for (subobj=page_obj; subobj; subobj=subobj->YNext)
	    {
	    if (subobj->Parent == page_obj)
		{
		/** Next row? **/
		if (subobj->Y > rowpos[cur_row])
		    {
		    while(subobj->Y > (rowpos[cur_row]+0.001) && cur_row < PRT_HTMLFM_MAXROWS-1) cur_row++;
		    
		    if(last_height + 0.001 < subobj->Y) {
			prt_htmlfm_OutputPrintf(context, "</tr><tr><td style=\"height: %dpx;line-height:0;\">&nbsp;</td>",
			   (int) ((subobj->Y - last_height) * PRT_HTMLFM_YPIXEL));
		    }
		    prt_htmlfm_OutputStrLiteral(context, "</tr>\n<tr>");
		    cur_col = 0;
		    }

		/** Skip cols? **/
		if (subobj->X > colpos[cur_col])
		    {
		    i=0;
		    while(subobj->X > (colpos[cur_col]+0.001) && cur_col < PRT_HTMLFM_MAXCOLS-1)
			{
			i++;
			cur_col++;
			}
		    if (i > 1)
			prt_htmlfm_OutputPrintf(context, "<td colspan=\"%d\">&nbsp;</td>", i);
		    else
			prt_htmlfm_OutputStrLiteral(context, "<td>&nbsp;</td>");
		    }

		/** Figure rowspan and colspan **/
		cs=1;
		while (cur_col+cs < n_cols && (colpos[cur_col+cs]+0.001) < subobj->X + subobj->Width) cs++;
		rs=1;
		while (cur_row+rs < n_rows && (rowpos[cur_row+rs]+0.001) < subobj->Y + subobj->Height) {
		    if (subobj->Height <= subobj->ConfigHeight+1.5 && 
			(rowpos[cur_row+rs]+0.001) < subobj->Y + subobj->ConfigHeight) rs++;
		    else break;
		}
		
		/** Update the lowest bottom edge for this row. **/
		if (subobj->Y + subobj->Height > last_height)
		    last_height = subobj->Y + subobj->Height;
		
		/*** Write container HTML, skipping default values
		 *** (colspan/rowspan="1", align="left") to reduce HTML size.
		 ***/
		prt_htmlfm_OutputStrLiteral(context, "<td");
		if (cs > 1) prt_htmlfm_OutputPrintf(context, " colspan=\"%d\"", cs);
		if (rs > 1) prt_htmlfm_OutputPrintf(context, " rowspan=\"%d\"", rs);
		if (subobj->Justification != PRT_JUST_T_LEFT)
		    prt_htmlfm_OutputPrintf(context, " align=\"%s\"", justifytypes[subobj->Justification]);
		prt_htmlfm_OutputStrLiteral(context, ">");
		
		/** Write child content. **/
		prt_htmlfm_Generate_r(context, subobj);
		
		/** Close container. **/
		prt_htmlfm_OutputStrLiteral(context, "</td>");
		
		cur_col += cs;
		if (cur_col >= n_cols) cur_col = n_cols-1;
		}
	    }
	prt_htmlfm_OutputStrLiteral(context, "</tr></table>\n");


	/** Write page footer (for paginated reports). **/
	prt_htmlfm_OutputPrintf(context, "</td><td></td></tr><tr><td height=\"%d\"></td><td></td><td></td></tr></table>\n", 
		(int)((page_obj->MarginBottom+0.001)*PRT_HTMLFM_YPIXEL));
	if (context->Flags & PRT_HTMLFM_F_PAGINATED)
	    prt_htmlfm_OutputStrLiteral(context, PRT_HTMLFM_PAGEFOOTER);

    return 0;
    }


int
prt_htmlfm_GetType(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pPrtHTMLfmSubtype type = (pPrtHTMLfmSubtype)ctx;

	if (!type) return -1;

	POD(val_v)->String = type->MimeType;

    return 0;
    }


/*** prt_htmlfm_Initialize() - init this module and register with the main
 *** print management system.
 ***/
int
prt_htmlfm_Initialize()
    {
	/** Init our globals **/
	memset(&PRT_HTMLFM, 0, sizeof(PRT_HTMLFM));
	PRT_HTMLFM.ImageID = rand();

	/** Allocate the formatter structure, and init it **/
	pPrtFormatter fmtdrv = check_ptr(prtAllocFormatter());
	if (fmtdrv == NULL)
	    {
	    mssError(0, "RPT", "Failed to allocate formatter struct.");
	    goto err;
	    }

	strcpy(fmtdrv->Name, "html");
	fmtdrv->Probe = prt_htmlfm_Probe;
	fmtdrv->GetOutputType = prt_htmlfm_GetOutputType;
	fmtdrv->Generate = prt_htmlfm_Generate;
	fmtdrv->GetNearestFontSize = prt_htmlfm_GetNearestFontSize;
	fmtdrv->GetCharacterMetric = prt_htmlfm_GetCharacterMetric;
	fmtdrv->GetCharacterBaseline = prt_htmlfm_GetCharacterBaseline;
	fmtdrv->Close = prt_htmlfm_Close;

	/** Register with the main prtmgmt system **/
	if (prtRegisterFormatter(fmtdrv) != 0)
	    {
	    mssError(0, "RPT", "Failed to register formatter.");
	    goto err;
	    }

	/** Register with the cx.sysinfo /prtmgmt/output_types dir **/
	for (int i = 0; i < PRT_HTMLFM_N_SUBTYPES; i++)
	    {
	    char* subtype = check_ptr(strchr(prt_htmlfm_subtypes[i].MimeType, '/'));
	    if (subtype == NULL) goto err_type;

	    /** Allocate subtype data. **/
	    char path_buf[256];
	    snprintf(path_buf, sizeof(path_buf), "/prtmgmt/output_types/%s", subtype + 1);
	    pSysInfoData si = check_ptr(sysAllocData(path_buf, NULL, NULL, NULL, NULL, prt_htmlfm_GetType, NULL, 0));
	    if (si == NULL) goto err_type;

	    /** Register subtype. */
	    if (sysAddAttrib(si, "type", DATA_T_STRING) != 0)
		{
		mssError(0, "RPT", "Failed to add 'type' attribute.");
		goto err_type;
		}
	    if (sysRegister(si, &prt_htmlfm_subtypes[i]) != 0)
		{
		mssError(0, "RPT", "Failed to register subtype.");
		goto err_type;
		}

	    /** Success. **/
	    continue;

    err_type:
		mssError(0, "RPT",
		    "Failed to add subtype #%d/%d: \"%s\"",
		    i + 1, PRT_HTMLFM_N_SUBTYPES, prt_htmlfm_subtypes[i].MimeType
		);
		goto err;
	    }

	return 0;

    err:
	mssError(0, "RPT", "Failed to initialize HTML formatter.");
	return -1;
    }
