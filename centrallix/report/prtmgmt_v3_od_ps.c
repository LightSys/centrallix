#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <zlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/expect.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
#include "prtmgmt_v3/hp_font_metrics.h"
#include "config.h"
#include "assert.h"
#include "cxss/cxss.h"

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
/* Module:	prtmgmt_v3_od_ps.c                                      */
/* Author:	Greg Beeley                                             */
/* Date:	February 5th, 2005                                      */
/*									*/
/* Description:	This module is an output driver for the strict		*/
/*		formatter module which outputs generic postscript       */
/************************************************************************/


/* Postscript Language Reference Manual: http://partners.adobe.com/public/developer/en/ps/PLRM.pdf */
/* PLRM quick ref: http://www.cs.indiana.edu/docproject/programming/postscript/operators.html */
/* basic overview with examples: http://www.eecs.tufts.edu/g/15/postscript/postscript.html */

/*** our list of resolutions ***/
PrtResolution prt_psod_resolutions[] =
    {
    {5,5,PRT_COLOR_T_FULL},
    {10,10,PRT_COLOR_T_FULL},
    {20,20,PRT_COLOR_T_FULL},
    {30,30,PRT_COLOR_T_FULL},
    {40,40,PRT_COLOR_T_FULL},
    {50,50,PRT_COLOR_T_FULL},
    {60,60,PRT_COLOR_T_FULL},
    {75,75,PRT_COLOR_T_FULL},
    {100,100,PRT_COLOR_T_FULL},
    {150,150,PRT_COLOR_T_FULL},
    {300,300,PRT_COLOR_T_FULL},
    {600,600,PRT_COLOR_T_FULL},
    {1200,1200,PRT_COLOR_T_FULL},
    };
#define PRT_PSOD_DEFAULT_RES	10


/*** list of output formats ***/
typedef struct _PPSF
    {
    char*		Name;
    char*		ContentType;
    void*		(*OpenFn)(pPrtSession);
    int			MaxPages;
    char*		Command;
    }
    PrtPsodFormat, *pPrtPsodFormat;

void* prt_psod_OpenPDF(pPrtSession);

static PrtPsodFormat PsFormats[] =
    {
	{ "png",	"image/png",		prt_psod_OpenPDF,	1,	"/usr/bin/gs -q -dSAFER -dNOPAUSE -dBATCH -dFirstPage=1 -dLastPage=1 -sDEVICE=png16m -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -sOutputFile=- -" },
	{ "pdf",	"application/pdf",	prt_psod_OpenPDF,	999999,	"cat | /usr/bin/ps2pdf -dCompatibilityLevel=1.4 -dPDFSETTINGS=/prepress /dev/stdin -" },
	{ NULL,		NULL,			NULL,			0,	NULL }
    };


/*** context structure for this driver ***/
typedef struct _PPS
    {
    XArray		SupportedResolutions;
    pPrtResolution	SelectedResolution;
    pPrtSession		Session;
    PrtTextStyle	SelectedStyle;
    double		CurHPos;
    double		CurVPos;
    int			PageWidth;	/* in points */
    int			PageHeight;	/* in points */
    int			MarginTop;
    int			MarginBottom;
    int			MarginLeft;
    int			MarginRight;
    int			PageNum;
    int			MaxPages;
    char*		Buf;
    int			BufSize;
    int			Flags;		/* PRT_PSOD_F_xxx, see below */
    pPrtPsodFormat	Format;		/* NULL if straight postscript */
    int			(*PrtmgmtWriteFn)();
    void*		PrtmgmtWriteArg;
    int			(*IntWriteFn)();
    void*		IntWriteArg;
    pFile		TransWPipe;
    pFile		TransRPipe;
    pSemaphore		CompletionSem;
    int			ChildPID;
    char		Buffer[512];
    }
    PrtPsodInf, *pPrtPsodInf;

#define PRT_PSOD_F_SENTSETUP	1	/* sent Setup section of PS document */
#define PRT_PSOD_F_SENTPAGE	2	/* sent PageSetup */
#define PRT_PSOD_F_USEGS	4	/* send content through GhostScript */

#define PRT_PSOD_HEX_LINE	64	/* number of compressed image bytes per line of hex */


/*** prt_psod_Output() - outputs the given snippet of text or PS code
 *** to the output channel for the printing session.  If length is set
 *** to -1, it is calculated a la strlen().
 ***/
int
prt_psod_Output(pPrtPsodInf context, char* str, int len)
    {

	/** Calculate the length? **/
	if (len < 0) len = strlen(str);
	if (!len) return 0;

	/** Write the data. **/
	context->IntWriteFn(context->IntWriteArg, str, len, 0, FD_U_PACKET);

    return 0;
    }


/*** prt_psod_Output_va() - output via varargs semantics.
 ***/
int
prt_psod_Output_va(pPrtPsodInf context, char* fmt, ...)
    {
    va_list va;
    int rval;

	va_start(va, fmt);
	rval = xsGenPrintf_va(context->IntWriteFn, context->IntWriteArg, 
		&(context->Buf), &(context->BufSize), fmt, va);
	va_end(va);

    return rval;
    }


/*** prt_psod_OutputHeader() - print the PostScript header.
 ***/
int
prt_psod_OutputHeader(pPrtPsodInf context)
    {

	prt_psod_Output(context,"%!PS-Adobe-3.0\n"
				"%%Creator: Centrallix/" PACKAGE_VERSION " PRTMGMTv3\n"
				"%%Title: Centrallix/" PACKAGE_VERSION " Generated Document\n"
				"%%Pages: (atend)\n"
				"%%DocumentData: Clean7Bit\n"
				"%%LanguageLevel: 3\n"
				"%%EndComments\n"
				"/pdfmark where {pop} {userdict /pdfmark /cleartomark load put} ifelse\n"
				"[ {Catalog} << /ViewerPreferences << /PrintScaling /None >> >> /PUT pdfmark\n"
				, -1);

    return 0;
    }


/*** prt_psod_OutputSetup() - output the document Setup section.
 ***/
int
prt_psod_OutputSetup(pPrtPsodInf context)
    {

	/** Send the document init string to the output channel.
	 ** This includes a reset (%1) and newpath command
	 **/
	prt_psod_OutputHeader(context);

	/** Page prolog **/
	prt_psod_Output_va(context,	"%%%%BeginProlog\n"
					"111 dict begin\n"
					"/FS { findfont exch scalefont setfont } bind def\n"
					"/RGB { /DeviceRGB setcolorspace setcolor } bind def\n"
					"/XY { %d exch sub moveto } bind def\n"
					"/NXY { newpath XY } bind def\n"
					"/LXY { %d exch sub lineto } bind def\n"
					"/UL { dup show stringwidth pop neg gsave 0 currentfont dup /FontInfo get /UnderlineThickness get exch /FontMatrix get dtransform setlinewidth 0 currentfont dup /FontInfo get /UnderlinePosition get exch /FontMatrix get dtransform rmoveto rlineto stroke grestore } bind def\n"
					"12 /Courier FS\n"
					"%%%%EndProlog\n",
				context->PageHeight,
				context->PageHeight);

	/** Setup **/
	prt_psod_Output_va(context,	"%%%%BeginSetup\n");

	/** Resolution **/
	prt_psod_Output_va(context,	"%%%%BeginFeature: *Resolution %ddpi\n"
					"<</HWResolution [%d %d]>> setpagedevice\n"
					"%%%%EndFeature\n",
				context->SelectedResolution->Xres,
				context->SelectedResolution->Xres,
				context->SelectedResolution->Yres);

	/** Page Size **/
	prt_psod_Output_va(context,	"%%%%BeginFeature: *PageSize Custom\n"
					"<</PageSize [%d %d] /ImagingBBox null>> setpagedevice\n"
					"%%%%EndFeature\n",
				context->PageWidth,
				context->PageHeight);

	/** End setup **/
	prt_psod_Output_va(context,	"%%%%EndSetup\n");

    return 0;
    }


/*** prt_psod_OutputPageSetup() - generate the page prologue
 ***/
int
prt_psod_OutputPageSetup(pPrtPsodInf context)
    {

	prt_psod_Output_va(context,	"%%%%Page: %d %d\n"
					"%%%%BeginPageSetup\n"
					"%%%%EndPageSetup\n"
					"%%%%PageBoundingBox: %d %d %d %d\n",
				context->PageNum+1,
				context->PageNum+1,
				context->MarginLeft,
				context->MarginTop,
				context->PageWidth - context->MarginRight,
				context->PageHeight - context->MarginBottom);
    return 0;
    }


/*** prt_psod_BeforeDraw() - this is called by the various functions that
 *** want to draw in the PS page.  Here we output the document or page
 *** prologues as needed.
 ***/
int
prt_psod_BeforeDraw(pPrtPsodInf context)
    {

	/** Send document setup? **/
	if (!(context->Flags & PRT_PSOD_F_SENTSETUP))
	    {
	    prt_psod_OutputSetup(context);
	    context->Flags |= PRT_PSOD_F_SENTSETUP;
	    }

	/** Send page setup? **/
	if (!(context->Flags & PRT_PSOD_F_SENTPAGE))
	    {
	    prt_psod_OutputPageSetup(context);
	    context->Flags |= PRT_PSOD_F_SENTPAGE;
	    }

    return 0;
    }


/*** prt_psod_EndPage() - this is called when a page is finished, either
 *** in WriteFF or in Close.
 ***/
int
prt_psod_EndPage(pPrtPsodInf context)
    {

	/** send page epilogue if a page was emitted. **/
	if (context->Flags & PRT_PSOD_F_SENTPAGE)
	    {
	    context->Flags &= ~PRT_PSOD_F_SENTPAGE;
	    prt_psod_Output_va(context,	"%%%%PageTrailer\n"
					"showpage\n");
	    context->PageNum++;
	    }

    return 0;
    }


/*** prt_psod_WriteTrans() - a writefn that sends the data out to the
 *** subprocess that is translating the postscript to something else.
 ***/
int
prt_psod_WriteTrans(pPrtPsodInf context, char* buf, int buflen, int offset, int flags)
    {
    int rval;

	/** try to send it to the subprocess **/
	if (context->TransWPipe)
	    {
	    rval = fdWrite(context->TransWPipe, buf, buflen, offset, flags);
	    if (rval <= 0)
		mssError(1, "PRT", "Translator subprocess died unexpectedly!");
	    }
	else
	    {
	    return -1;
	    }

    return rval;
    }


/*** prt_psod_Worker() - the worker thread passing the translated info
 *** back to the prtmgmt client.
 ***/
void
prt_psod_Worker(void* v)
    {
    pPrtPsodInf context = (pPrtPsodInf)v;
    char buf[256];
    int rcnt;
    pFile transpipe_fd;
    int (*fn)();
    void* arg;
    int status, rval;
    pSemaphore sem;

	thSetName(NULL, "PRTMGMTv3 PSOD Worker");

	/** Nab these, context structure might go away while we are working **/
	transpipe_fd = context->TransRPipe;
	fn = context->PrtmgmtWriteFn;
	arg = context->PrtmgmtWriteArg;
	sem = context->CompletionSem;

	/** Loop reading data **/
	while((rcnt = fdRead(transpipe_fd, buf, sizeof(buf), 0, 0)) > 0)
	    {
	    if (fn(arg, buf, rcnt, 0, FD_U_PACKET) != rcnt)
		{
		/** Drain out any remaining data, and tail on outta here **/
		while((rcnt = fdRead(transpipe_fd, buf, sizeof(buf), 0, 0)) > 0);
		break;
		}
	    }
	fdClose(transpipe_fd, 0);

	/** Clean up after the child process **/
	while(1)
	    {
	    rval = waitpid(context->ChildPID, &status, WNOHANG);
	    if (rval < 0) break;
	    if (rval == context->ChildPID && WIFEXITED(status)) break;
	    thSleep(500);
	    }

	/** Let parent thread know we're ready **/
	syPostSem(sem, 1, 0);

    thExit();
    }


/*** prt_psod_Open() - open a new printing session with this driver.
 ***/
void*
prt_psod_Open(pPrtSession s)
    {
    pPrtPsodInf context;
    int i;

	/** Set up the context structure for this printing session **/
	context = (pPrtPsodInf)nmMalloc(sizeof(PrtPsodInf));
	if (!context) return NULL;
	context->BufSize = 256;
	context->Buf = nmSysMalloc(context->BufSize);
	if (!context->Buf)
	    {
	    nmFree(context, sizeof(PrtPsodInf));
	    return NULL;
	    }
	context->Session = s;
	context->SelectedResolution = NULL;
	xaInit(&(context->SupportedResolutions), 16);
	context->Flags = 0;
	context->PageWidth = 612;	/* US-Letter */
	context->PageHeight = 792;	/* US-Letter */
	context->MarginTop = 0;
	context->MarginBottom = 0;
	context->MarginLeft = 0;
	context->MarginRight = 0;
	context->PageNum = 0;
	context->MaxPages = 999999;

	/** Set up data path **/
	context->IntWriteFn = s->WriteFn;
	context->IntWriteArg = s->WriteArg;
	context->TransRPipe = NULL;
	context->TransWPipe = NULL;

	/** Right now, just support 75, 100, 150, and 300 dpi **/
	for(i=0;i<sizeof(prt_psod_resolutions)/sizeof(PrtResolution);i++)
	    xaAddItem(&(context->SupportedResolutions), (void*)(prt_psod_resolutions+i));
	/*xaAddItem(&(context->SupportedResolutions), (void*)(prt_psod_resolutions+0));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_psod_resolutions+1));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_psod_resolutions+2));
	xaAddItem(&(context->SupportedResolutions), (void*)(prt_psod_resolutions+3));*/

	/** Setup base text style **/
	context->SelectedStyle.Attr = 0;
	context->SelectedStyle.FontSize = 12.0;
	context->SelectedStyle.FontID = PRT_FONT_T_MONOSPACE;
	context->SelectedStyle.Color = 0x00000000; /* black */

    return (void*)context;
    }


/*** prt_psod_OpenPDF() - opens the printing session for generating
 *** PDF output.
 ***/
void*
prt_psod_OpenPDF(pPrtSession session)
    {
    pPrtPsodInf context;
    int i;
    int maxfiles;
    char* cmd = NULL;
    int wfds[2];
    int rfds[2];
    gid_t gidlist[1];
    int id;
    void (*prevhandler)(int);

	/** Call the main open function to get a template context **/
	context = prt_psod_Open(session);

	/** Now modify & update for the ps2pdf translator **/
	context->PrtmgmtWriteFn = context->IntWriteFn;
	context->PrtmgmtWriteArg = context->IntWriteArg;
	context->IntWriteFn = prt_psod_WriteTrans;
	context->IntWriteArg = (void*)context;

	/** Find our cmd **/
	for(i=0;PsFormats[i].Name;i++)
	    {
	    if (!strcmp(PsFormats[i].ContentType, session->OutputType))
		{
		cmd = PsFormats[i].Command;
		context->MaxPages = PsFormats[i].MaxPages;
		break;
		}
	    }
	assert(cmd);

	/** Fork process **/
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, rfds) < 0)
	    {
	    prt_psod_Close(context);
	    return NULL;
	    }
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, wfds) < 0)
	    {
	    prt_psod_Close(context);
	    return NULL;
	    }
	prevhandler = signal(SIGPROF, SIG_IGN);
	context->ChildPID = fork();
	signal(SIGPROF, prevhandler);
	if (context->ChildPID < 0)
	    {
	    /** error **/
	    prt_psod_Close(context);
	    return NULL;
	    }
	else if (context->ChildPID == 0)
	    {
	    /** in child **/
	    thLock();
	    dup2(wfds[1],0);
	    dup2(rfds[1],1);
	    dup2(rfds[1],2);
	    maxfiles = sysconf(_SC_OPEN_MAX);
	    if (maxfiles <= 0)
		{
		printf("Warning: sysconf(_SC_OPEN_MAX) returned <= 0; using maxfiles=2048.\n");
		maxfiles = 2048;
		}
	    for(i=3;i<maxfiles;i++) close(i);

	    /** Drop privs **/
	    id = geteuid();
	    setuid(0);
	    setgroups(0, gidlist);
	    if (setregid(getegid(), -1) < 0) _exit(1);
	    if (setreuid(id, id) < 0) _exit(1);
	    setsid();

	    /** Exec the cmd **/
	    execl("/bin/sh", "sh", "-c", cmd, NULL);
	    _exit(1);
	    }

	/** in parent **/
	close(rfds[1]);
	close(wfds[1]);

	/** get the file handle thru mtask **/
	context->TransWPipe = fdOpenFD(wfds[0], O_RDWR);
	fdSetOptions(context->TransWPipe, FD_UF_WRBUF);
	context->TransRPipe = fdOpenFD(rfds[0], O_RDWR);
	context->CompletionSem = syCreateSem(0, 0);

	/** Start the worker thread **/
	thCreate(prt_psod_Worker, 0, context);

    return context;
    }


/*** prt_psod_Close() - closes a printing session with this driver.
 ***/
int
prt_psod_Close(void* context_v)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;

	/** Send document end string to the output channel (showpage) **/
	prt_psod_Output_va(context,	"%%%%Trailer\n"
					"%%%%Pages: %d\n"
					"end\n"
					"%%%%EOF\n",
				context->PageNum);

	/** Free the context structure **/
	if (context->TransWPipe) 
	    {
	    fdClose(context->TransWPipe, 0);
	    syGetSem(context->CompletionSem, 1, 0);
	    syDestroySem(context->CompletionSem, 0);
	    }
	nmSysFree(context->Buf);
	xaDeInit(&(context->SupportedResolutions));
	nmFree(context, sizeof(PrtPsodInf));

    return 0;
    }


/*** prt_psod_eetResolutions() - return the list of supported resolutions
 *** for this printing session.
 ***/
pXArray
prt_psod_GetResolutions(void* context_v)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    return &(context->SupportedResolutions);
    }


/*** prt_psod_SetResolution() - set the x/y/color resolution that will be
 *** used for raster data in this printing session.
 ***/
int
prt_psod_SetResolution(void* context_v, pPrtResolution r)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;

	/** Set it. **/
	if (!r)
	    r = &(prt_psod_resolutions[PRT_PSOD_DEFAULT_RES]);
	context->SelectedResolution = r;

    return 0;
    }


/*** prt_psod_SetPageGeom() - set the page size.
 ***/
int
prt_psod_SetPageGeom(void* context_v, double width, double height, double t, double b, double l, double r)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;

	context->PageWidth = (int)(width*7.2 + 0.5);
	context->PageHeight = (int)(height*12.0 + 0.5);
	context->MarginTop = (int)(t*12.0 + 0.5);
	context->MarginBottom = (int)(b*12.0 + 0.5);
	context->MarginLeft = (int)(l*7.2 + 0.5);
	context->MarginRight = (int)(r*7.2 + 0.5);

    return 0;
    }


/*** prt_psod_GetNearestFontSize() - return the nearest font size to the
 *** requested one.  Most PS printers can scale fonts without any problem,
 *** so we'll allow any font size greater than zero.
 ***/
double
prt_psod_GetNearestFontSize(void* context_v, double req_size)
    {
    /*pPrtPsodInf context = (pPrtPsodInf)context_v;*/
    return (req_size<=0)?1:req_size;
    }


/*** prt_psod_GetCharacterMetric() - return the width of a printed character
 *** in a given font size, attributes, etc., in 10ths of an inch (relative to
 *** 10cpi, or 12point, fonts.
 ***/
void
prt_psod_GetCharacterMetric(void* context_v, unsigned char* str, pPrtTextStyle style, double* width, double* height)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
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
		n += hp_helvetica_font_metrics[(*str) - 0x20][style_code]/600.0;
		}
	    else if (style->FontID == PRT_FONT_T_SERIF)
		{
		/** metrics based on empirical analysis **/
		n += hp_times_font_metrics[(*str) - 0x20][style_code]/600.0;
		}
	    else
		{
		/** Assume monospace font if unknown **/
		n += 1.0;
		}
	    str++;
	    }

	/** Adjust the width based on the font size.  Base size is 12pt **/
	*width = n*(style->FontSize/12.0);
	*height = style->FontSize/12.0;

    return;
    }


/*** prt_psod_GetCharacterBaseline() - returns the Y offset of the text
 *** baseline from the upper-left corner of the text, for a given style
 *** or for the current style.
 ***/
double
prt_psod_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
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


/*** prt_psod_SetTextStyle() - set the current output text style, including
 *** attributes (bold/italic/underlined), color, size, and typeface.
 ***/
int
prt_psod_SetTextStyle(void* context_v, pPrtTextStyle style)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    int onattr, offattr;
    char* fontname;
    char* fontattr;

	prt_psod_BeforeDraw(context);

	/** Attribute change? **/
	onattr=(style->Attr ^ context->SelectedStyle.Attr)&(style->Attr);
	offattr=(style->Attr ^ context->SelectedStyle.Attr)&(context->SelectedStyle.Attr);

	/** Need to reissue font selection command? **/
	if (onattr || offattr || style->FontID != context->SelectedStyle.FontID || style->FontSize != context->SelectedStyle.FontSize)
	    {
	    /** Select the font face **/
	    switch (style->FontID)
		{
		case PRT_FONT_T_SANSSERIF:
		    fontname = "Helvetica";
		    break;
		case PRT_FONT_T_SERIF:
		    fontname = "Times";
		    break;
		default:
		case PRT_FONT_T_MONOSPACE:
		    fontname = "Courier";
		    break;
		}

	    /** Select bold and italic, since in PS they are a part of the font def'n **/
	    if ((style->Attr & PRT_OBJ_A_ITALIC) && (style->Attr & PRT_OBJ_A_BOLD))
		fontattr = "-BoldOblique";
	    else if (style->Attr & PRT_OBJ_A_ITALIC)
		fontattr = "-Oblique";
	    else if (style->Attr & PRT_OBJ_A_BOLD)
		fontattr = "-Bold";
	    else
		fontattr = "";

	    /** Output our FS command (see %%BeginProlog for FS macro def'n) **/
	    prt_psod_Output_va(context,	"%.1f /%s%s FS\n", style->FontSize, fontname, fontattr);
	    }

	/** Color change? **/
	if (style->Color != context->SelectedStyle.Color)
	    {
	    prt_psod_Output_va(context, "%.3f %.3f %.3f RGB\n", 
		    ((style->Color>>16) & 0xFF) / 255.0,
		    ((style->Color>>8) & 0xFF) / 255.0,
		    ((style->Color) & 0xFF) / 255.0);
	    }

	/** Record the newly selected style **/
	memcpy(&(context->SelectedStyle), style, sizeof(PrtTextStyle));

    return 0;
    }


/*** prt_psod_SetHPos() - sets the horizontal (x) position on the page.
 ***/
int
prt_psod_SetHPos(void* context_v, double x)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    
	context->CurHPos = x;

    return 0;
    }


/*** prt_psod_SetVPos() - sets the vertical (y) position on the page,
 *** independently of the x position (does not emit newlines)
 ***/
int
prt_psod_SetVPos(void* context_v, double y)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    
	context->CurVPos = y;

    return 0;
    }


/*** prt_psod_WriteText() - sends a string of text to the printer.
 ***/
int
prt_psod_WriteText(void* context_v, char* str, char* url, double width, double height)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    double bl;
    int i,psbuflen;
    int do_underline;
    pXString url_xs;

	if (context->PageNum >= context->MaxPages) return 0;

	prt_psod_BeforeDraw(context);

	do_underline = (context->SelectedStyle.Attr & PRT_OBJ_A_UNDERLINE);

	/** Move the starting point and adjust for the baseline before outputting the text. **/
	bl = prt_psod_GetCharacterBaseline(context_v, NULL);
	prt_psod_Output_va(context, "%.1f %.1f NXY\n", 
		(context->CurHPos)*7.2 + 0.000001, 
		(context->CurVPos)*12.0 + bl*12.0 + 0.000001);

	/** output it. **/
	/** wrap content in parentheses and show command **/
	context->Buffer[0] = '\0';
	for(i=psbuflen=0;i<strlen(str);i++)
	    {
	    sprintf(context->Buffer+psbuflen, "%2.2X", str[i]);
	    psbuflen += 2;
	    if (psbuflen >= sizeof(context->Buffer)-1)
		{
		prt_psod_Output_va(context, "<%s> %s\n", context->Buffer, do_underline?"UL":"show");
		psbuflen = 0;
		context->Buffer[0] = '\0';
		}
	    }
	if (psbuflen)
	    {
	    prt_psod_Output_va(context, "<%s> %s\n", context->Buffer, do_underline?"UL":"show");
	    }

	/** URL? **/
	if (url && !strpbrk(url, "() "))
	    {
	    url_xs = cxssLinkSign(url);
	    prt_psod_Output_va(context, "[ /Rect [ %.1f %.1f %.1f %.1f ] /Action << /Subtype /URI /URI (%s) >> /Border [0 0 0] /Color [0 0 .7] /Subtype /Link /ANN pdfmark\n",
		    (context->CurHPos)*7.2 + 0.000001,
		    context->PageHeight - ((context->CurVPos)*12.0 + bl*12.0 - 0.000001 - height*12.0),
		    (context->CurHPos)*7.2 + 0.000001 + width*7.2,
		    context->PageHeight - ((context->CurVPos)*12.0 + bl*12.0 - 0.000001),
		    url_xs?xsString(url_xs):url
		    );
	    if (url_xs)
		xsFree(url_xs);
	    }

    return 0;
    }


/*** prt_psod_Compress() - compresses data into a zlib stream and append the
 *** compressed bytes to the provided string.
 ***
 *** @param zs The zlib stream, initialized with deflateInit().
 *** @param data The data to compress, or NULL when len is 0.
 *** @param len The length of data.
 *** @param flush Z_NO_FLUSH, or Z_FINISH to end the stream.
 *** @param out The string where compressed bytes are written.
 *** @returns 0 on success, or -1 with mssError() on failure.
 ***/
static int
prt_psod_Compress(z_stream* zs, unsigned char* data, int len, int flush, pXString out)
    {
	/*** Compress until zlib consumes the entire input and doesn't fill
	 *** the output buffer.
	 ***/
	zs->next_in = data;
	zs->avail_in = len;
	do  {
	    unsigned char chunk[4096];
	    zs->next_out = chunk;
	    zs->avail_out = sizeof(chunk);

	    const int z_rval = deflate(zs, flush);
	    if (UNLIKELY(z_rval == Z_STREAM_ERROR))
		{
		mssError(1, "PRT",
		    "Failed to compress %d bytes: zlib error %d.",
		    len, z_rval
		);
		return -1;
		}

	    if (UNLIKELY(xsConcatenate(out, (char*)chunk, sizeof(chunk) - zs->avail_out) < 0))
		{
		mssError(1, "PRT",
		    "Failed to store %zu bytes of compressed data.",
		    sizeof(chunk) - zs->avail_out
		);
		return -1;
		}
	    } while (zs->avail_out == 0);

    return 0;
    }


/*** prt_psod_WriteRasterData() - outputs a block of raster (image) data
 *** at the current printing position on the page, given the selected
 *** pixel and color resolution.  The pixels are Flate-compressed and
 *** hex-encoded, which requires PostScript Level 3.
 ***/
double
prt_psod_WriteRasterData(void* context_v, pPrtImage img, double width, double height, double next_y)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    double rval = -1;
    int rows,cols,x,y,pix,i,n;
    int direct_map = 0;
    unsigned char* img_row = NULL;
    z_stream zs;
    bool zs_initialized = false;
    XString zdata_buf;
    pXString zdata = NULL;
    char drawcmd[256];
    int drawcmd_len;
    long long hex_len;
    char hexline[PRT_PSOD_HEX_LINE*2 + 1];
    unsigned char* zbytes;
    char* hexdigit = "0123456789abcdef";

	if (context->PageNum >= context->MaxPages) return 0;

	/** Determine how many actual rows/cols we are looking at for the
	 ** target resolution.
	 **/
	rows = height/6.0*(context->SelectedResolution->Yres);
	cols = width/10.0*(context->SelectedResolution->Xres);

	/** If image is far lower res than output device, go with image res,
	 ** to save on resources.
	 **/
	if (img->Hdr.Width <= cols/2) cols = img->Hdr.Width;
	if (img->Hdr.Height <= rows/2) rows = img->Hdr.Height;

	/** Use direct pixel mapping to speed things up? **/
	if (cols == img->Hdr.Width && rows == img->Hdr.Height && img->Hdr.ColorMode == PRT_COLOR_T_FULL)
	    direct_map = 1;

	/** Allocate image data row **/
	img_row = (unsigned char*)nmSysMalloc(cols*3 + 1);
	if (UNLIKELY(img_row == NULL))
	    {
	    mssError(1, "PRT", "Failed to allocate a %d-pixel image row.", cols);
	    goto end;
	    }

	/** Start the zlib compression. **/
	if (UNLIKELY(xsInit(&zdata_buf) < 0))
	    {
	    mssError(1, "PRT", "Failed to initialize the compressed image buffer.");
	    goto end;
	    }
	zdata = &zdata_buf;
	memset(&zs, 0, sizeof(zs));
	if (UNLIKELY(deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK))
	    {
	    mssError(1, "PRT",
		"Failed to start compressing a %dx%d image: %s.",
		cols, rows, (zs.msg != NULL) ? zs.msg : "unknown zlib error"
	    );
	    goto end;
	    }
	zs_initialized = true;

	/** Compress the pixels as RGB bytes. **/
	for(y=0;y<rows;y++)
	    {
	    for(x=0;x<cols;x++)
		{
		if (direct_map)
		    pix = img->Data.Word[y*img->Hdr.Width + x] & 0x00FFFFFF;
		else
		    pix = prt_internal_GetPixel(img, ((double)x)/(cols), ((double)y)/(rows));
		img_row[x*3+0] = (pix >> 16) & 0xFF;
		img_row[x*3+1] = (pix >> 8) & 0xFF;
		img_row[x*3+2] = pix & 0xFF;
		}
	    if (UNLIKELY(prt_psod_Compress(&zs, img_row, cols*3, Z_NO_FLUSH, zdata) < 0)) goto end;
	    }
	if (UNLIKELY(prt_psod_Compress(&zs, NULL, 0, Z_FINISH, zdata) < 0)) goto end;

	/** Build the drawing command, which reads the data after it and
	 ** then skips to the data's end marker.
	 **/
	drawcmd_len = snprintf(drawcmd, sizeof(drawcmd),
		"{ /rasterdata currentfile /ASCIIHexDecode filter def "
		"%d %d 8 [ %d %d %d %d %d %d ] rasterdata /FlateDecode filter false 3 colorimage "
		"rasterdata flushfile } exec\n",
		cols, rows,
		cols, 0, 0, -rows, 0, rows
		);
	if (UNLIKELY(drawcmd_len < 0 || (size_t)drawcmd_len >= sizeof(drawcmd)))
	    {
	    mssError(1, "PRT", "Failed to build the drawing command for a %dx%d image.", cols, rows);
	    goto end;
	    }

	/** Hex data: two digits per byte, a newline per line, then ">\n". **/
	n = zdata->Length;
	hex_len = 2LL*n + (n + PRT_PSOD_HEX_LINE - 1)/PRT_PSOD_HEX_LINE + 2;

	/** save graphics context before beginning, 
	 ** then emit the image header.
	 **/
	prt_psod_Output_va(context,	"gsave\n"
					"%.1f %.1f translate\n"
					"%.1f %.1f scale\n"
					"%%%%BeginData: %lld Hex Bytes\n"
					,
				context->CurHPos*7.2 + 0.000001, 
				context->PageHeight - (context->CurVPos*12.0 + height*12.0 + 0.000001),
				width*7.2 + 0.000001,
				height*12.0 + 0.000001,
				drawcmd_len + hex_len
				);
	prt_psod_Output(context, drawcmd, drawcmd_len);

	/** Output the compressed data, in hexadecimal **/
	zbytes = (unsigned char*)zdata->String;
	for(i=0;i<n;i+=PRT_PSOD_HEX_LINE)
	    {
	    for(x=0;x<PRT_PSOD_HEX_LINE && i+x<n;x++)
		{
		hexline[x*2+0] = hexdigit[zbytes[i+x] >> 4];
		hexline[x*2+1] = hexdigit[zbytes[i+x] & 0xF];
		}
	    hexline[x*2] = '\n';
	    prt_psod_Output(context, hexline, x*2+1);
	    }

	/** Restore graphics context when done **/
	prt_psod_Output_va(context,	">\n"
					"%%%%EndData\n"
					"grestore\n");

	rval = context->CurVPos + height;

    end:
	if (zs_initialized) deflateEnd(&zs);
	if (zdata != NULL) xsDeInit(zdata);
	if (img_row != NULL) nmSysFree(img_row);

    return rval;
    }


/*** prt_psod_WriteSvgData() - outputs an svg image at the current
 *** printing position on the page, given the selected pixel and 
 *** color resolution. 
 ***/
double
prt_psod_WriteSvgData(void* context_v, pPrtSvg svg, double width, double height, double next_y)
    {
    pPrtPsodInf context;
    pXString epsXString;
    double dx, dy;    

    context = (pPrtPsodInf)context_v;
    if (context->PageNum >= context->MaxPages) {
        return 0;
    }

    /* Distance (x, y) from lower-left corner */ 
    dx = context->CurHPos * 7.2 + 0.000001;
    dy = context->PageHeight - (context->CurVPos * 12.0 +
                               height * 12.0 + 0.000001);

    /* Width and height in pt (1/72th of an inch) */
    width = width/10.0 * 72;
    height = height/6.0 * 72;

    /* Convert SVG data to postscript */
    epsXString = prtConvertSvgToEps(svg, width, height);
    if (!epsXString) return -1;
    
    /** Save state and embed EPS data **/
    prt_psod_Output_va(context, "save\n"
                                "%.1f %.1f translate\n"
                                "/showpage {} def\n",
                                dx, dy);
    prt_psod_Output(context, epsXString->String, epsXString->Length);   

    /** Restore state and free xstring **/
    prt_psod_Output_va(context, "restore\n");
    xsFree(epsXString);
        
    return context->CurVPos + height;
    }


/*** prt_psod_WriteFF() - sends a form feed to end the page.
 ***/
int
prt_psod_WriteFF(void* context_v)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;

	if (context->PageNum >= context->MaxPages) return 0;

	/** End page if one was started **/
	prt_psod_EndPage(context);

	/** Reset cursor position **/
	context->CurVPos = 0.0;
	context->CurHPos = 0.0;

    return 0;
    }


/*** prt_psod_WriteRect() - write a rectangular area out to the page (or
 *** a fragment of one).  Return the Y coordinate of the bottom of the
 *** fragment that was actually output.  'next_y' is the next Y position
 *** on the page that will be printed after this row of objects.
 ***/
double
prt_psod_WriteRect(void* context_v, double width, double height, double next_y, int color)
    {
    pPrtPsodInf context = (pPrtPsodInf)context_v;
    double x1,x2,y1,y2;

	if (context->PageNum >= context->MaxPages) return 0;

	/** Make sure width and height meet the minimum required by the
	 ** currently selected resolution
	 **/
	if (width < 10.0/context->SelectedResolution->Xres) width = 10.0/context->SelectedResolution->Xres;
	if (height < 6.0/context->SelectedResolution->Yres) height = 6.0/context->SelectedResolution->Yres;

	x1 = context->CurHPos*7.2 + 0.000001;
	x2 = x1 + width*7.2;
	y1 = context->CurVPos*12.0 + 0.000001;
	y2 = y1 + height*12.0;

	/** Color change? **/
	if (color != -1 && color != context->SelectedStyle.Color)
	    {
	    prt_psod_Output_va(context, "%.3f %.3f %.3f RGB\n", 
		    ((color>>16) & 0xFF) / 255.0,
		    ((color>>8) & 0xFF) / 255.0,
		    ((color) & 0xFF) / 255.0);
	    }

	/** Output the rectangle **/
	prt_psod_Output_va(context,	"%.1f %.1f NXY %.1f %.1f LXY %.1f %.1f LXY %.1f %.1f LXY fill\n",
		x1,y1,
		x2,y1,
		x2,y2,
		x1,y2);

	/** Color change? **/
	if (color != -1 && color != context->SelectedStyle.Color)
	    {
	    prt_psod_Output_va(context, "%.3f %.3f %.3f RGB\n", 
		    ((context->SelectedStyle.Color>>16) & 0xFF) / 255.0,
		    ((context->SelectedStyle.Color>>8) & 0xFF) / 255.0,
		    ((context->SelectedStyle.Color) & 0xFF) / 255.0);
	    }

    return context->CurVPos + height;
    }


/*** prt_psod_Initialize() - init this module and register with the main
 *** format driver as a strict output formatter.
 ***/
int
prt_psod_Initialize()
    {
    pPrtOutputDriver drv;
    int i;

	/** Register the PostScript version of the driver **/
	drv = prt_strictfm_AllocDriver();
	strcpy(drv->Name,"ps");
	strcpy(drv->ContentType,"application/postscript");
	drv->Open = prt_psod_Open;
	drv->Close = prt_psod_Close;
	drv->GetResolutions = prt_psod_GetResolutions;
	drv->SetResolution = prt_psod_SetResolution;
	drv->SetPageGeom = prt_psod_SetPageGeom;
	drv->GetNearestFontSize = prt_psod_GetNearestFontSize;
	drv->GetCharacterMetric = prt_psod_GetCharacterMetric;
	drv->GetCharacterBaseline = prt_psod_GetCharacterBaseline;
	drv->SetTextStyle = prt_psod_SetTextStyle;
	drv->SetHPos = prt_psod_SetHPos;
	drv->SetVPos = prt_psod_SetVPos;
	drv->WriteText = prt_psod_WriteText;
	drv->WriteRasterData = prt_psod_WriteRasterData;
	drv->WriteFF = prt_psod_WriteFF;
	drv->WriteRect = prt_psod_WriteRect;
	prt_strictfm_RegisterDriver(drv);

	/** Register the translated version(s) of the driver **/
	for(i=0;PsFormats[i].Name;i++)
	    {
	    drv = prt_strictfm_AllocDriver();
	    strcpy(drv->Name, PsFormats[i].Name);
	    strcpy(drv->ContentType, PsFormats[i].ContentType);
	    drv->Open = PsFormats[i].OpenFn;
	    drv->Close = prt_psod_Close;
	    drv->GetResolutions = prt_psod_GetResolutions;
	    drv->SetResolution = prt_psod_SetResolution;
	    drv->SetPageGeom = prt_psod_SetPageGeom;
	    drv->GetNearestFontSize = prt_psod_GetNearestFontSize;
	    drv->GetCharacterMetric = prt_psod_GetCharacterMetric;
	    drv->GetCharacterBaseline = prt_psod_GetCharacterBaseline;
	    drv->SetTextStyle = prt_psod_SetTextStyle;
	    drv->SetHPos = prt_psod_SetHPos;
	    drv->SetVPos = prt_psod_SetVPos;
	    drv->WriteText = prt_psod_WriteText;
	    drv->WriteRasterData = prt_psod_WriteRasterData;
	    drv->WriteSvgData = prt_psod_WriteSvgData;
            drv->WriteFF = prt_psod_WriteFF;
	    drv->WriteRect = prt_psod_WriteRect;
	    prt_strictfm_RegisterDriver(drv);
	    }

    return 0;
    }
