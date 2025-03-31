#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_fm_html.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_text.h"
#include "prtmgmt_v3/ht_font_metrics.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "double.h"

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
/* Module:	prtmgmt_v3_fm_html.c                                    */
/* Author:	Greg Beeley                                             */
/* Date:	April 4th, 2003                                         */
/*									*/
/* Description:	This module is the HTML formatter, which takes a page	*/
/*		structure and outputs structured HTML.  This is made	*/
/*		separate from the html formatter because HTML is not	*/
/*		a html formatting language.				*/
/************************************************************************/



/*** The following are for layout purposes at the page level, not for
 *** tables themselves!
 ***/
#define PRT_HTMLFM_MAXCOLS	(64)
#define PRT_HTMLFM_MAXROWS	(64)


/*** Document header ***/
/* CLS 2025-03-28: Note that changing the HTML version may change spacing between lines/wrapped text.*/
#define PRT_HTMLFM_HEADER       "<!DOCTYPE html>\n" \
				"<html lang=\"en\" width=\"100%\">\n" \
				"<head>\n" \
				"    <title>Centrallix HTML Document</title>\n" \
				"    <meta name=\"Generator\" content=\"Centrallix PRTMGMT v3.0\">\n" \
				"</head>\n" \
				"<body bgcolor=\"%s\">\n"


/*** Document footer ***/
#define PRT_HTMLFM_FOOTER	"</body>\n" \
				"</html>\n"


/*** Page header - build the graphical layout showing the 'page'
 ***
 *** Params:
 ***    (1) %d	Width of table, pixels
 ***/
#define PRT_HTMLFM_PAGEHEADER	"    <center>\n" \
				"    <table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#606060\">\n" \
				"        <tr>\n" \
				"            <td bgcolor=\"#000000\">\n" \
				"            <table width=\"%d\" border=\"0\" cellspacing=\"1\" cellpadding=\"16\">\n" \
				"                <tr><td width=\"100%\" bgcolor=\"#ffffff\">\n" \
				"<!------------------------------PAGE BEGIN------------------------------>\n" \
				"\n"


/*** Page footer - end the page ***/
#define PRT_HTMLFM_PAGEFOOTER	"\n" \
				"<!------------------------------PAGE END-------------------------------->\n" \
				"                </td></tr>\n" \
				"            </table>\n" \
				"            </td><td valign=\"top\" align=\"left\" width=\"8\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
				"        </tr><tr>\n" \
				"            <td width=\"8\" align=\"left\" valign=\"top\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
				"            <td><small>&nbsp;</small></td>\n" \
				"        </tr>\n" \
				"    </table>\n" \
				"    </center>\n" \
				"    <br>\n"


/*** this puts the min size at 9 (1), max size at 26 (7), and standard size at 12 (3) ***/
static int prt_htmlfm_fontsize_to_htmlsize[] = {8,9,10,12,15,19,22,26};
#define PRT_HTMLFM_MINFONTSIZE	(1)
#define	PRT_HTMLFM_MAXFONTSIZE	(7)

static char* prt_htmlfm_fontstyles[3] = { "Courier New,Courier,fixed", "Arial,Helvetica,MS Sans Serif", "Times New Roman,Times,MS Serif"};
#define PRT_HTMLFM_MINFONTSTYLE	(0)
#define PRT_HTMLFM_MAXFONTSTYLE	(2)

/*** Session flags ***/
#define PRT_HTMLFM_F_PAGINATED		1

/*** Style Flags ***/
#define PRT_HTMLFM_F_KEEPSPACES		1 //used after newlines to keep space-padding
#define PRT_HTMLFM_F_FONTDIRTY		2
#define PRT_HTMLFM_F_UNDERLINEDIRTY	4
#define PRT_HTMLFM_F_ITALICDIRTY	8
#define PRT_HTMLFM_F_BOLDDIRTY		16

/*** MIME media types ***/
typedef struct
    {
    char*		MimeType;
    char*		OutputMimeType;
    int			SessionFlags;
    }
    PrtHTMLfmSubtype, *pPrtHTMLfmSubtype;

static PrtHTMLfmSubtype prt_htmlfm_subtypes[] =
    {
    { "text/vnd.cx.paginated+html", "text/html", PRT_HTMLFM_F_PAGINATED },
    { "text/html", "text/html", 0 },
    };


/*** GLOBAL DATA FOR THIS MODULE ***/
typedef struct _PSF
    {
    unsigned long	ImageID;
    }
    PRT_HTMLFM_t;

PRT_HTMLFM_t PRT_HTMLFM;


/*** Formatter internal structure (pPrtHTMLfmInf).  Typedef incomplete
 *** def'n is in the header file.  This completes it. 
 ***/
struct _PSFI
    {
    pPrtSession		Session;
    pPrtResolution	SelectedRes;
    PrtTextStyle	CurStyle;
    int			InitStyle;
    int			ExitStyle;
    pPrtHTMLfmSubtype	Subtype;
    int			Flags;			/* PRT_HTMLFM_F_xxx */
    int			StyleFlags;
    };



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
	while(str[offset] && offset < len)
	    {
	    badcharpos = strpbrk(str+offset, "<>& ");
	    if (badcharpos)
		endoffset = badcharpos - str;
	    else
		endoffset = len;
	    if (endoffset - offset > 0)
		prt_htmlfm_Output(context, str+offset, endoffset - offset);

	    if(str[offset] != ' ')
	    {
	    /* we are no longer in our leading spaces, so don't &nbsp them */
		context->Flags &= (! PRT_HTMLFM_F_KEEPSPACES);
	    }
	    if (badcharpos)
		{
		switch(*badcharpos)
		    {
		    case '<': repl = "&lt;"; break;
		    case '>': repl = "&gt;"; break;
		    case '&': repl = "&amp;"; break;
		    case ' ': repl = ( context->Flags & PRT_HTMLFM_F_KEEPSPACES ) ? "&nbsp" : " "; break;
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
    pPrtHTMLfmInf context;
    int i;

	/** Allocate our context inf structure **/
	context = (pPrtHTMLfmInf)nmMalloc(sizeof(PrtHTMLfmInf));
	if (!context) goto error;
	memset(context, 0, sizeof(PrtHTMLfmInf));
	context->Session = s;

	/** Is it an html type we can handle? **/
	for(i=0; i<sizeof(prt_htmlfm_subtypes)/sizeof(PrtHTMLfmSubtype); i++)
	    {
	    if (strcasecmp(output_type, prt_htmlfm_subtypes[i].MimeType) == 0)
		{
		context->Subtype = &(prt_htmlfm_subtypes[i]);
		context->Flags = context->Subtype->SessionFlags;
		break;
		}
	    }
	if (!context->Subtype)
	    goto error;

	/** Write the document header **/
	prt_htmlfm_OutputPrintf(context, PRT_HTMLFM_HEADER, (context->Flags & PRT_HTMLFM_F_PAGINATED)?"#c0c0c0":"#ffffff");

	return (void*)context;

    error:
	if (context) nmFree(context, sizeof(PrtHTMLfmInf));

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
	for(i=PRT_HTMLFM_MINFONTSIZE;i<=PRT_HTMLFM_MAXFONTSIZE;i++)
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
	while(*str)
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

	/** Write the document footer **/
	prt_htmlfm_Output(context, PRT_HTMLFM_FOOTER, -1);

	/** Free memory used **/
	nmFree(context, sizeof(PrtHTMLfmInf));

    return 0;
    }

const char *
prt_htmlfm_GetFont(pPrtTextStyle style) {
    int fontid = style->FontID - 1;
    if (fontid < PRT_HTMLFM_MINFONTSTYLE || fontid > PRT_HTMLFM_MAXFONTSTYLE) fontid = PRT_HTMLFM_MINFONTSTYLE;
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
		!(context->StyleFlags & PRT_HTMLFM_F_BOLDDIRTY)) prt_htmlfm_Output(context, "</b>", 4);
	    if (context->ExitStyle || italicchanged || underlinechanged || fontchanged)
		{
		if (context->CurStyle.Attr & PRT_OBJ_A_ITALIC &&
		    !(context->StyleFlags & PRT_HTMLFM_F_ITALICDIRTY)) prt_htmlfm_Output(context, "</i>", 4);
		if (context->ExitStyle || underlinechanged || fontchanged)
		    {
		    if (context->CurStyle.Attr & PRT_OBJ_A_UNDERLINE && 
			!(context->StyleFlags & PRT_HTMLFM_F_UNDERLINEDIRTY)) prt_htmlfm_Output(context, "</u>", 4);
		    if ((context->ExitStyle || fontchanged) &&
			!(context->StyleFlags & PRT_HTMLFM_F_FONTDIRTY))
			{
			prt_htmlfm_Output(context, "</font>",7);
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
			context->StyleFlags |= PRT_HTMLFM_F_FONTDIRTY;
		    }
		    if (style->Attr & PRT_OBJ_A_UNDERLINE)
			context->StyleFlags |= PRT_HTMLFM_F_UNDERLINEDIRTY;
		}
		if (style->Attr & PRT_OBJ_A_ITALIC)
		    context->StyleFlags |= PRT_HTMLFM_F_ITALICDIRTY;
	    }
	    if (style->Attr & PRT_OBJ_A_BOLD)
		context->StyleFlags |= PRT_HTMLFM_F_BOLDDIRTY;

	    memcpy(&(context->CurStyle), style, sizeof(PrtTextStyle));
	}
    return 0;
    }

int
prt_htmlfm_WriteStyle(pPrtHTMLfmInf context) {
    pPrtTextStyle style = &(context->CurStyle);
    int htmlfontsize;
    int i;
    char stylebuf[128];
    
    /** Figure the size **/
    for(i=PRT_HTMLFM_MINFONTSIZE;i<=PRT_HTMLFM_MAXFONTSIZE;i++)
    {
        if (realComparePrecision(prt_htmlfm_fontsize_to_htmlsize[i], style->FontSize, 0.5) == 0)
	{
	    htmlfontsize = i;
	    break;
	}
    }
    /*htmlfontsize = style->FontSize - PRT_HTMLFM_FONTSIZE_DEFAULT + PRT_HTMLFM_FONTSIZE_OFFSET;*/
    
    if(context->StyleFlags & PRT_HTMLFM_F_FONTDIRTY)
    {
	snprintf(stylebuf, sizeof(stylebuf), "<font face=\"%s\" color=\"#%6.6X\" size=\"%d\">",
	    prt_htmlfm_GetFont(style), style->Color, htmlfontsize);
	prt_htmlfm_Output(context, stylebuf, -1);
    }
    if(context->StyleFlags & PRT_HTMLFM_F_UNDERLINEDIRTY)
    {
	prt_htmlfm_Output(context, "<u>", 3);
    }
    if(context->StyleFlags & PRT_HTMLFM_F_ITALICDIRTY)
    {
	prt_htmlfm_Output(context, "<i>", 3);
    }
    if(context->StyleFlags & PRT_HTMLFM_F_BOLDDIRTY)
    {
	prt_htmlfm_Output(context, "<b>", 3);
    }

    /*Clear the dirty flags*/
    context->StyleFlags &= ! (PRT_HTMLFM_F_FONTDIRTY | PRT_HTMLFM_F_UNDERLINEDIRTY |
	    PRT_HTMLFM_F_ITALICDIRTY | PRT_HTMLFM_F_BOLDDIRTY);
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


/*** prt_htmlfm_ResetStyle() - reset a style setting to that which
 *** was used previously in a container before a subcontainer was 
 *** entered.  This is used when a subcontainer was just closed.
 ***/
int
prt_htmlfm_ResetStyle(pPrtHTMLfmInf context, pPrtTextStyle style)
    {

	/** Set style settings, and do nothing else **/
	memcpy(&(context->CurStyle), style, sizeof(PrtTextStyle));

    return 0;
    }


/*** prt_htmlfm_SaveStyle() - save the current text style in a given
 *** style structure
 ***/
int
prt_htmlfm_SaveStyle(pPrtHTMLfmInf context, pPrtTextStyle style)
    {

	/** Save style settings **/
	memcpy(style, &(context->CurStyle), sizeof(PrtTextStyle));

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
prt_htmlfm_SetKeepSpaces(pPrtHTMLfmInf context) {
    context->Flags |= PRT_HTMLFM_F_KEEPSPACES;
}


/*** prt_htmlfm_Border() - use nested tables to create a border matching
 *** the given border structure, with an appropriate margin setting from
 *** the given prt object
 ***/
int
prt_htmlfm_Border(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj)
    {
    int i;
    int m,bw,iw;

	/** Figure the margins **/
	m = (obj->MarginTop + obj->MarginBottom + obj->MarginLeft + obj->MarginRight)*PRT_HTMLFM_XPIXEL/4;

	/** Construct the border for each element **/
	for(i=0;i<border->nLines;i++)
	    {
	    /** Output border line itself **/
	    bw = border->Width[i]*PRT_HTMLFM_XPIXEL + 0.5;
	    if (bw == 0) bw = 1;
	    iw = ((i==border->nLines-1)?m:(border->Sep*PRT_HTMLFM_XPIXEL)) + 0.5;
	    if (iw == 0 && i!=border->nLines-1) iw = 1;
	    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"%d\"><tr><td bgcolor=\"%6.6X\">",
		    (int)(bw),
		    (int)(border->Color[i]));
	    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"%d\"><tr><td bgcolor=\"%6.6X\">\n",
		    (int)(iw),
		    (int)(obj->BGColor));
	    }
	if (border->nLines == 0)
	    {
	    prt_htmlfm_OutputPrintf(context, "<table border=\"0\" width=\"100%\" cellspacing=\"0\" cellpadding=\"%d\"><tr><td bgcolor=\"%6.6X\">\n",
		    (int)(m),
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
	for(i=0;i<border->nLines;i++)
	    {
	    /** Output border line itself **/
	    prt_htmlfm_Output(context, "</td></tr></table></td></tr></table>\n",-1);
	    }
	if (border->nLines == 0)
	    {
	    prt_htmlfm_Output(context, "</td></tr></table>\n",-1);
	    }

    return 0;
    }


/*** prt_htmlfm_Generate_r() - recursive worker routine to do the bulk
 *** of page generation.
 ***/
int
prt_htmlfm_Generate_r(pPrtHTMLfmInf context, pPrtObjStream obj)
    {
    char* path;
    void* arg;
    int w,h;
    unsigned long id;
    int rval;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Select the type of object we're formatting **/
	switch(obj->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_STRING:
		prt_htmlfm_SetStyle(context, &(obj->TextStyle));

		if(strlen((char*) obj->Content))
		{
		    prt_htmlfm_WriteStyle(context);
    		}

		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_Output(context, "<a href=\"", 9);
		    prt_htmlfm_OutputEncoded(context, obj->URL, -1);
		    prt_htmlfm_Output(context, "\">", 2);
		    }
		prt_htmlfm_OutputEncoded(context, (char*)obj->Content, -1);

		if ((obj->Flags & PRT_OBJ_F_SOFTNEWLINE) && (obj->Flags & PRT_TEXTLM_F_RMSPACE)) {
		    prt_htmlfm_OutputEncoded(context, " ", 1);
		}

		if (obj->URL && !strchr(obj->URL, '"'))
		    {
		    prt_htmlfm_Output(context, "</a>", 4);
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
		/** We need an image store location in order to handle these **/
		if (context->Session->ImageOpenFn)
		    {
		    id = PRT_HTMLFM.ImageID++;
		    w = obj->Width*PRT_HTMLFM_XPIXEL;
		    h = obj->Height*PRT_HTMLFM_YPIXEL;
		    if (w <= 0) w = 1;
		    if (h <= 0) h = 1;
		    path = (char*)nmMalloc(OBJSYS_MAX_PATH);
		    if (!path) 
                        {
                        mssError(1, "PRT", "nmMalloc() failed\n");
                        return -1;
                        }
                    rval = snprintf(path, OBJSYS_MAX_PATH, "%sprt_htmlfm_%8.8lX.png", context->Session->ImageSysDir, id);
		    if (rval < 0 || rval >= OBJSYS_MAX_PATH)
			{
                        mssError(1, "PRT", "Internal representation exceeded for image pathname\n");
			nmFree(path, OBJSYS_MAX_PATH);
                        return -1;
			}
		    arg = context->Session->ImageOpenFn(context->Session->ImageContext, path, O_CREAT | O_WRONLY | O_TRUNC, 0600, "image/png");
		    if (!arg)
			{
			mssError(0,"PRT","Failed to open new linked image '%s'",path);
			nmFree(path, OBJSYS_MAX_PATH);
			return -1;
			}
		    prt_internal_WriteImageToPNG(context->Session->ImageWriteFn, arg, (pPrtImage)(obj->Content), w, h);
		    context->Session->ImageCloseFn(arg);
		    nmFree(path, OBJSYS_MAX_PATH);
		    if (obj->URL && !strchr(obj->URL, '"'))
			{
			prt_htmlfm_Output(context, "<a href=\"", 9);
			prt_htmlfm_OutputEncoded(context, obj->URL, -1);
			prt_htmlfm_Output(context, "\">", 2);
			}
		    prt_htmlfm_OutputPrintf(context, "<img src=\"%sprt_htmlfm_%8.8X.png\" border=\"0\" width=\"%d\" height=\"%d\">", 
			    context->Session->ImageExtDir, id, w, h);
		    if (obj->URL && !strchr(obj->URL, '"'))
			{
			prt_htmlfm_Output(context, "</a>", 4);
			}
		    }
		break;

            case PRT_OBJ_T_SVG:
		/** We need an image store location in order to handle these **/
		if (context->Session->ImageOpenFn)
		    {
		    id = PRT_HTMLFM.ImageID++;
		    w = obj->Width*PRT_HTMLFM_XPIXEL;
		    h = obj->Height*PRT_HTMLFM_YPIXEL;
		    if (w <= 0) w = 1;
		    if (h <= 0) h = 1;
		    path = (char*)nmMalloc(OBJSYS_MAX_PATH);
		    if (!path) 
                        {
                        mssError(1, "PRT", "nmMalloc() failed\n");
                        return -1;
                        }
                    rval = snprintf(path, OBJSYS_MAX_PATH, "%sprt_htmlfm_%8.8lX.svg", context->Session->ImageSysDir, id);
		    if (rval < 0 || rval >= OBJSYS_MAX_PATH)
			{
                        mssError(1, "PRT", "Internal representation exceeded for image pathname\n");
			nmFree(path, OBJSYS_MAX_PATH);
                        return -1;
			}
		    arg = context->Session->ImageOpenFn(context->Session->ImageContext, path, O_CREAT | O_WRONLY | O_TRUNC, 0600, "image/svg+xml");
		    if (!arg)
			{
			mssError(0,"PRT","Failed to open new linked image '%s'",path);
			nmFree(path, OBJSYS_MAX_PATH);
			return -1;
			}
		    prt_internal_WriteSvgToFile(context->Session->ImageWriteFn, arg, (pPrtSvg)(obj->Content), w, h);
		    context->Session->ImageCloseFn(arg);
		    nmFree(path, OBJSYS_MAX_PATH);
		    if (obj->URL && !strchr(obj->URL, '"'))
			{
			prt_htmlfm_Output(context, "<a href=\"", 9);
			prt_htmlfm_OutputEncoded(context, obj->URL, -1);
			prt_htmlfm_Output(context, "\">", 2);
			}
		    prt_htmlfm_OutputPrintf(context, "<img src=\"%sprt_htmlfm_%8.8X.svg\" border=\"0\" width=\"%d\" height=\"%d\">", 
			    context->Session->ImageExtDir, id, w, h);
		    if (obj->URL && !strchr(obj->URL, '"'))
			{
			prt_htmlfm_Output(context, "</a>", 4);
			}
		    }
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
    int rs,cs;

	/** Write the page header **/
	if (context->Flags & PRT_HTMLFM_F_PAGINATED)
	    prt_htmlfm_OutputPrintf(context, PRT_HTMLFM_PAGEHEADER, (int)(page_obj->Width*PRT_HTMLFM_XPIXEL+0.001)+34);

	/** Write a table to handle page margins **/
	//TODO CSMITH I can't figure out if the width* actually does anything (didn't do anything when switched to px either). I don't see any padding in web client. (Also email will often ignore padding on <body> I think). Need to test on email.
	int left_margin = (int)(page_obj->MarginLeft*PRT_HTMLFM_XPIXEL+0.001);
	int center_width = (int)((page_obj->Width - page_obj->MarginLeft - page_obj->MarginRight+0.001)*PRT_HTMLFM_XPIXEL);
	int right_margin = (int)(page_obj->MarginRight*PRT_HTMLFM_XPIXEL+0.001);
	int top_margin = (int)((page_obj->MarginTop+0.001)*PRT_HTMLFM_YPIXEL);
	prt_htmlfm_Output(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n", -1);
	prt_htmlfm_Output(context, "<colgroup>", -1);
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", left_margin);
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", center_width);
	prt_htmlfm_OutputPrintf(context, "<col width=\"%d*\">\n", right_margin);
	prt_htmlfm_Output(context, "</colgroup>", -1);
	/* Print the first row, empty with appropriate margins*/
	//TODO CSMITH the "px" modifier here has no effect, and the element does not actually have that much width. Why?
	prt_htmlfm_OutputPrintf(context, "<tr><td height=\"%dpx\" width=\"%dpx\"></td><td height=\"%dpx\" width=\"%dpx\"></td><td height=\"%dpx\" width=\"%dpx\"></td></tr>", 
		top_margin, left_margin,
		top_margin, center_width,
		top_margin, right_margin);
	/* Start the second row (with appropriate margin) */
	prt_htmlfm_OutputPrintf(context, "<tr><td width=\"%dpx\"></td><td>\n", left_margin);


	/** We need to scan the absolute-positioned content to figure out how many
	 ** "columns" and "rows" we need to put in the "table" used for layout
	 ** purposes.
	 **/
	for(subobj=page_obj->ContentHead; subobj; subobj=subobj->Next)
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
	prt_htmlfm_Output(context, "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n", -1);
	for(i=0;i<n_cols;i++)
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
	prt_htmlfm_Output(context, "<tr>", 4);
	for(subobj=page_obj; subobj; subobj=subobj->YNext)
	    {
	    if (subobj->Parent == page_obj)
		{
		/** Next row? **/
		if (subobj->Y > rowpos[cur_row])
		    {
		    while(subobj->Y > (rowpos[cur_row]+0.001) && cur_row < PRT_HTMLFM_MAXROWS-1) cur_row++;
		    prt_htmlfm_Output(context, "</tr>\n<tr>", 10);
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
		    prt_htmlfm_OutputPrintf(context, "<td colspan=\"%d\">&nbsp;</td>", i);
		    }

		/** Figure rowspan and colspan **/
		cs=1;
		while(cur_col+cs < n_cols && (colpos[cur_col+cs]+0.001) < subobj->X + subobj->Width) cs++;
		rs=1;
		//TODO CSMITH: we need to enter empty rows if we don't have anything in one, but something still spans multiple rows. How to do? Or do we just rowspan everything to 1?? Problem I think might actually be in the recept file def... somehow, the table is showing up as "should be at this Y height" but the other div is extending into that?... TBD.
		while(cur_row+rs < n_rows && (rowpos[cur_row+rs]+0.001) < subobj->Y + subobj->Height) rs++;
		prt_htmlfm_OutputPrintf(context, "<td colspan=\"%d\" rowspan=\"%d\" valign=\"top\" align=\"%s\">", cs, rs, justifytypes[subobj->Justification]);
		prt_htmlfm_Generate_r(context, subobj);
		prt_htmlfm_Output(context, "</td>", 5);
		cur_col += cs;
		if (cur_col >= n_cols) cur_col = n_cols-1;
		}
	    }
	prt_htmlfm_Output(context, "</tr></table>\n", 14);


	/** Write the page footer **/
	prt_htmlfm_OutputPrintf(context, "</td><td></td></tr><tr><td height=\"%d\"></td><td></td><td></td></tr></table>\n", 
		(int)((page_obj->MarginBottom+0.001)*PRT_HTMLFM_YPIXEL));
	if (context->Flags & PRT_HTMLFM_F_PAGINATED)
	    prt_htmlfm_Output(context, PRT_HTMLFM_PAGEFOOTER, -1);

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
    pPrtFormatter fmtdrv;
    pSysInfoData si;
    int i;
    char sbuf[256];
    char* ptr;

	/** Init our globals **/
	memset(&PRT_HTMLFM, 0, sizeof(PRT_HTMLFM));
	PRT_HTMLFM.ImageID = rand();

	/** Allocate the formatter structure, and init it **/
	fmtdrv = prtAllocFormatter();
	if (!fmtdrv) return -1;
	strcpy(fmtdrv->Name, "html");
	fmtdrv->Probe = prt_htmlfm_Probe;
	fmtdrv->GetOutputType = prt_htmlfm_GetOutputType;
	fmtdrv->Generate = prt_htmlfm_Generate;
	fmtdrv->GetNearestFontSize = prt_htmlfm_GetNearestFontSize;
	fmtdrv->GetCharacterMetric = prt_htmlfm_GetCharacterMetric;
	fmtdrv->GetCharacterBaseline = prt_htmlfm_GetCharacterBaseline;
	fmtdrv->Close = prt_htmlfm_Close;

	/** Register with the main prtmgmt system **/
	prtRegisterFormatter(fmtdrv);

	/** Register with the cx.sysinfo /prtmgmt/output_types dir **/
	for(i=0; i<sizeof(prt_htmlfm_subtypes)/sizeof(PrtHTMLfmSubtype); i++)
	    {
	    ptr = strchr(prt_htmlfm_subtypes[i].MimeType, '/');
	    if (!ptr) return -1;
	    snprintf(sbuf, sizeof(sbuf), "/prtmgmt/output_types/%s", ptr+1);
	    si = sysAllocData(sbuf, NULL, NULL, NULL, NULL, prt_htmlfm_GetType, NULL, 0);
	    if (!si) return -1;
	    sysAddAttrib(si, "type", DATA_T_STRING);
	    sysRegister(si, &prt_htmlfm_subtypes[i]);
	    }

    return 0;
    }


