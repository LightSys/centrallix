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
/* Module:	prtmgmt_v3_fm_html.c                                    */
/* Author:	Greg Beeley                                             */
/* Date:	April 4th, 2003                                         */
/*									*/
/* Description:	This module is the HTML formatter, which takes a page	*/
/*		structure and outputs structured HTML.  This is made	*/
/*		separate from the html formatter because HTML is not	*/
/*		a html formatting language.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_html.c,v 1.1 2003/04/04 22:38:27 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_fm_html.c,v $

    $Log: prtmgmt_v3_fm_html.c,v $
    Revision 1.1  2003/04/04 22:38:27  gbeeley
    Added HTML formatter for new print subsystem, with just basic output
    capabilities at present.

 **END-CVSDATA***********************************************************/


#define	PRT_HTMLFM_HEADER	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" \
				"<html>\n" \
				"<head>\n" \
				"    <title>Centrallix HTML Document</title>\n" \
				"    <meta name=\"Generator\" content=\"Centrallix PRTMGMT v3.0\">\n" \
				"</head>\n" \
				"<body bgcolor=\"#c0c0c0\">\n"


#define PRT_HTMLFM_FOOTER	"</body>\n" \
				"</html>\n"


#define PRT_HTMLFM_PAGEHEADER	"    <table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#606060\">\n" \
				"        <tr bgcolor=\"#c0c0c0\"><td width=\"8\"><small>&nbsp;</small></td><td><small>&nbsp;&nbsp;&nbsp;&nbsp;</small></td><td width=\"8\"><small>&nbsp;</small></td></tr>\n" \
				"        <tr>\n" \
				"            <td colspan=\"2\" rowspan=\"2\" bgcolor=\"#000000\">\n" \
				"            <table border=\"0\" cellspacing=\"1\" cellpadding=\"2\" width=\"100%\">\n" \
				"                <tr><td width=\"100%\" bgcolor=\"#ffffff\">\n" \
				"<!------------------------------PAGE BEGIN------------------------------>\n" \
				"\n"


#define PRT_HTMLFM_PAGEFOOTER	"\n" \
				"<!------------------------------PAGE END-------------------------------->\n" \
				"                </td></tr>\n" \
				"            </table>\n" \
				"            </td><td valign=\"top\" align=\"left\" colspan=\"1\" width=\"8\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
				"        </tr><tr>\n" \
				"            <td colspan=\"1\" width=\"8\" bgcolor=\"#606060\"><small>&nbsp;</small></td>\n" \
				"        </tr><tr>\n" \
				"            <td colspan=\"1\" width=\"8\" align=\"left\" valign=\"top\"><table width=\"8\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#c0c0c0\"><tr><td height=\"8\" width=\"8\">&nbsp;</td></tr></table></td>\n" \
				"            <td colspan=\"2\" bgcolor=\"#606060\"><small>&nbsp;</small></td>\n" \
				"        </tr>\n" \
				"    </table>\n"

/*** GLOBAL DATA FOR THIS MODULE ***/
typedef struct _PSF
    {
    }
    PRT_HTMLFM_t;

PRT_HTMLFM_t PRT_HTMLFM;


/*** formatter internal structure. ***/
typedef struct _PSFI
    {
    pPrtSession		Session;
    pPrtResolution	SelectedRes;
    }
    PrtHTMLfmInf, *pPrtHTMLfmInf;


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
	    badcharpos = strpbrk(str+offset, "<>&");
	    if (badcharpos)
		endoffset = badcharpos - str;
	    else
		endoffset = len;
	    if (endoffset - offset > 0)
		prt_htmlfm_Output(context, str+offset, endoffset - offset);
	    if (badcharpos)
		{
		switch(*badcharpos)
		    {
		    case '<': repl = "&lt;"; break;
		    case '>': repl = "&gt;"; break;
		    case '&': repl = "&amp;"; break;
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

	/** Is it html? **/
	if (strcasecmp(output_type,"text/html") != 0) return NULL;

	/** Allocate our context inf structure **/
	context = (pPrtHTMLfmInf)nmMalloc(sizeof(PrtHTMLfmInf));
	if (!context) return NULL;
	context->Session = s;

	/** Write the document header **/
	prt_htmlfm_Output(context, PRT_HTMLFM_HEADER, -1);

    return (void*)context;
    }


/*** prt_htmlfm_GetNearestFontSize - return the nearest font size that this
 *** driver supports.  In this case, this just queries the underlying output
 *** driver for the information.
 ***/
int
prt_htmlfm_GetNearestFontSize(void* context_v, int req_size)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    return 12;
    }


/*** prt_htmlfm_GetCharacterMetric - return the sizing information for a given
 *** character, in standard units.
 ***/
double
prt_htmlfm_GetCharacterMetric(void* context_v, char* str, pPrtTextStyle style)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    return 1.0;
    }


/*** prt_htmlfm_GetCharacterBaseline - return the distance from the upper
 *** left corner of the character cell to the left baseline point of the 
 *** character cell, in standard units.
 ***/
double
prt_htmlfm_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;
    return 0.75;
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


/*** prt_htmlfm_StartStyle() - output the html to begin a text style
 ***/
int
prt_htmlfm_StartStyle(pPrtHTMLfmInf context, pPrtObjStream styleobj)
    {
    char* fonts[3] = { "Courier,Courier New,fixed", "Helvetica,Arial,MS Sans Serif", "Times,Times New Roman,MS Serif"};
    int htmlfontsize, fontid;
    char stylebuf[128];

	/** Figure the size **/
	htmlfontsize = styleobj->TextStyle.FontSize - 11;
	fontid = styleobj->TextStyle.FontID - 1;
	if (fontid < 0 || fontid > 2) fontid = 0;
	snprintf(stylebuf, sizeof(stylebuf), "<font face=\"%s\" color=\"#%6.6X\" size=\"%+d\">",
		fonts[fontid], styleobj->TextStyle.Color, htmlfontsize);
	prt_htmlfm_Output(context, stylebuf, -1);

    return 0;
    }


/*** prt_htmlfm_EndStyle() - output the html to end a given text 
 *** style.
 ***/
int
prt_htmlfm_EndStyle(pPrtHTMLfmInf context, pPrtObjStream styleobj)
    {

	/** End font tag **/
	prt_htmlfm_Output(context,"</font>",7);

    return 0;
    }


/*** prt_htmlfm_Generate_r() - recursive worker routine to do the bulk
 *** of page generation.
 ***/
int
prt_htmlfm_Generate_r(pPrtHTMLfmInf context, pPrtObjStream obj)
    {
    pPrtObjStream subobj;

	if (obj) prt_htmlfm_StartStyle(context, obj);
	while(obj)
	    {
	    /** Select the type of object we're formatting **/
	    switch(obj->ObjType->TypeID)
		{
		case PRT_OBJ_T_STRING:
		    prt_htmlfm_OutputEncoded(context, obj->Content, -1);
		    if (obj->Flags & PRT_OBJ_F_NEWLINE) prt_htmlfm_Output(context, "<br>\n",5);
		    break;

		case PRT_OBJ_T_AREA:
		    prt_htmlfm_Output(context,"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr><td>\n", -1);
		    prt_htmlfm_Generate_r(context, obj->ContentHead);
		    prt_htmlfm_Output(context,"</td></tr></table>\n", -1);
		    break;
		}

	    /** Check style change or end of objstream **/
	    if (!(obj->Next) || memcmp(&(obj->TextStyle), &(obj->Next->TextStyle), sizeof(PrtTextStyle)))
		{
		prt_htmlfm_EndStyle(context, obj);
		if (obj->Next) prt_htmlfm_StartStyle(context, obj->Next);
		}
	    obj = obj->Next;
	    }

    return 0;
    }


/*** prt_htmlfm_Generate() - generate the html for the page.  Basically,
 *** walk through the document and generate appropriate html layout to
 *** make the thing look similar to what it should.
 ***/
int
prt_htmlfm_Generate(void* context_v, pPrtObjStream page_obj)
    {
    pPrtHTMLfmInf context = (pPrtHTMLfmInf)context_v;

	/** Write the page header **/
	prt_htmlfm_Output(context, PRT_HTMLFM_PAGEHEADER, -1);

	/** Generate the body of the page **/
	prt_htmlfm_Generate_r(context, page_obj->ContentHead);

	/** Write the page footer **/
	prt_htmlfm_Output(context, PRT_HTMLFM_PAGEFOOTER, -1);

    return 0;
    }


/*** prt_htmlfm_Initialize() - init this module and register with the main
 *** print management system.
 ***/
int
prt_htmlfm_Initialize()
    {
    pPrtFormatter fmtdrv;

	/** Init our globals **/
	memset(&PRT_HTMLFM, 0, sizeof(PRT_HTMLFM));

	/** Allocate the formatter structure, and init it **/
	fmtdrv = prtAllocFormatter();
	if (!fmtdrv) return -1;
	strcpy(fmtdrv->Name, "html");
	fmtdrv->Probe = prt_htmlfm_Probe;
	fmtdrv->Generate = prt_htmlfm_Generate;
	fmtdrv->GetNearestFontSize = prt_htmlfm_GetNearestFontSize;
	fmtdrv->GetCharacterMetric = prt_htmlfm_GetCharacterMetric;
	fmtdrv->GetCharacterBaseline = prt_htmlfm_GetCharacterBaseline;
	fmtdrv->Close = prt_htmlfm_Close;

	/** Register with the main prtmgmt system **/
	prtRegisterFormatter(fmtdrv);

    return 0;
    }


