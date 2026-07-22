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
#include "htmlparse.h"
#include "cxlib/mtsession.h"

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
/* Module:	prtmgmt_v3_fm_html_lm_text.c                            */
/* Author:	Greg Beeley                                             */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This is the component of the HTML formatter which has	*/
/*		intelligence about textflow layout areas.		*/
/************************************************************************/



/*** prt_htmlfm_GenerateArea() - generates the html to represent a
 *** textflow area.
 ***/
int
prt_htmlfm_GenerateArea(pPrtHTMLfmInf context, pPrtObjStream area)
    {
    int n_xset;
    double xset[PRT_HTMLFM_MAX_TABSTOP];
    double widths[PRT_HTMLFM_MAX_TABSTOP];
    pPrtObjStream scan, linetail, next_xset_obj, justif_subscan;
    int i,j,cur_xset,next_xset;
    double w, cur_x;
    int last_needed_cols, cur_needs_cols, need_new_row, in_td, in_tr;
    PrtHTMLfmSavedStyle oldstyle;
    char* justifytypes[] = { "left", "right", "center", "justify" };
    pPrtTextLMData lm_inf = (pPrtTextLMData)(area->LMData);

	/** First we need to scan the area looking for objects which
	 ** were positioned with PRT_OBJ_F_XSET, so we can build a table
	 ** of sorted 'tabstops'.
	 **/
	xset[0] = 0.0;
	n_xset = 1;
	for(scan=area->ContentHead;scan;scan=scan->Next)
	    {
	    if (scan->Flags & PRT_OBJ_F_XSET)
		{
		for(i=0;i<=n_xset;i++)
		    {
		    if (i!=n_xset && xset[i] == scan->X) break;
		    if (n_xset < PRT_HTMLFM_MAX_TABSTOP && (i==n_xset || xset[i] > scan->X))
			{
			for(j=n_xset;j>i;j--) xset[j] = xset[j-1];
			xset[i] = scan->X;
			n_xset++;
			break;
			}
		    }
		}
	    }

	/** Output the area prologue. **/
	prt_htmlfm_SaveStyle(context, &oldstyle);
	int saved_bg = context->BGColor;
	if (lm_inf->AreaBorder.nLines > 0)
	    {
	    /** Draw the border. **/
	    prt_htmlfm_Border(context, &(lm_inf->AreaBorder), area);
	    context->BGColor = area->BGColor;
	    prt_htmlfm_OutputStrLiteral(context, "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">\n");
	    }
	else
	    {
	    /** No border: Draw the padding and background directly. **/
	    const int pad = (area->MarginTop + area->MarginBottom + area->MarginLeft + area->MarginRight) * PRT_HTMLFM_XPIXEL/4;
	    prt_htmlfm_OutputPrintf(context,
		"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"%d\" border=\"0\"",
		pad
	    );
	    prt_htmlfm_OutputBGColor(context, area->BGColor);
	    prt_htmlfm_OutputStrLiteral(context, ">\n");
	    }
	in_tr = 0;
	in_td = 0;

	/*** Issue column width info.  A single column spans the whole area by
	 *** default, so we only need explicit <col> elements when there is more
	 *** than one tabstop.
	 ***/
	if (n_xset == 1)
	    {
	    /*** Note a single column spans the whole area by default,
	     *** so no HTML is needed.
	     ***/
	    widths[0] = area->Width - area->MarginLeft - area->MarginRight;
	    }
	else
	    {
	    for (i=0;i<n_xset;i++)
		{
		if (i == n_xset-1)
		    widths[i] = area->Width - area->MarginLeft - area->MarginRight - xset[i];
		else 
		    widths[i] = xset[i+1] - xset[i];

		/** We could use relative 'n*' formatting; older browsers will interpret as pixel
		 ** width, newer ones as relative width, but doesn't seem to work right
		 ** with newer browsers.
		 **/
		prt_htmlfm_OutputPrintf(context,"<col width=\"%d*\">\n",(int)(widths[i]*PRT_HTMLFM_XPIXEL+0.0001));
		}
	    }

	/** Walk the area's content **/
	scan = area->ContentHead;
	last_needed_cols = 0;
	in_tr = 0;
	while(scan)
	    {
	    /** Find the tail of this line, and figure out if we need to use columns **/
	    cur_xset = 0;
	    linetail = scan;
	    cur_needs_cols = 0;
	    while(1)
		{
		if (linetail->Flags & PRT_OBJ_F_XSET) cur_needs_cols = 1;
		if ((linetail->Flags & PRT_OBJ_F_NEWLINE) || !linetail->Next) break;

		linetail = linetail->Next;
		}
	    need_new_row = (cur_needs_cols || last_needed_cols || scan->Justification != PRT_JUST_T_LEFT);
	    if (need_new_row)
		{
		if (in_td)
		    {
		    prt_htmlfm_EndStyle(context);
		    prt_htmlfm_OutputStrLiteral(context, "</td>");
		    in_td = 0;
		    }
		if (in_tr)
		    {
		    prt_htmlfm_OutputStrLiteral(context, "</tr>\n");
		    in_tr = 0;
		    }
		}
	    if (!in_tr)
		{
		prt_htmlfm_OutputStrLiteral(context, "<tr>");
		in_tr = 1;
		}

	    /** Need to advance to first column/tabstop being used? **/
	    if (cur_needs_cols && (scan->Flags & PRT_OBJ_F_XSET))
		{
		for(cur_xset=0;cur_xset<n_xset && xset[cur_xset] < scan->X;) cur_xset++;
		if (cur_xset != 0)
		    {
		    if (in_tr && !in_td)
			{
			prt_htmlfm_OutputStrLiteral(context, "<td");
			if (cur_xset > 1)
			    prt_htmlfm_OutputPrintf(context, " colspan=\"%d\"", cur_xset);
			prt_htmlfm_OutputPrintf(context,
			    " width=\"%d\">&nbsp;</td>",
			    (int)(widths[cur_xset]*PRT_HTMLFM_XPIXEL+0.001)
			);
			}
		    }
		}

	    /** Ok, scan through the line now **/
	    while(scan != linetail->Next)
		{

		/** Find next xset location that isn't at the current x **/
		if (cur_needs_cols)
		    {
		    cur_x = scan->X; 
		    next_xset_obj = scan->Next;
		    while(next_xset_obj != linetail->Next && 
			(!(next_xset_obj->Flags & PRT_OBJ_F_XSET) ||
			next_xset_obj->X - cur_x < 0.001)) 
		    {
			next_xset_obj=next_xset_obj->Next;
		    }
		    if (next_xset_obj == linetail->Next)
			{
			next_xset_obj = NULL;
			next_xset = n_xset;
			}
		    else
			{
			for(next_xset=cur_xset;next_xset<n_xset && xset[next_xset] < next_xset_obj->X;) next_xset++;
			}
		    }
		else
		    {
		    next_xset_obj = NULL;
		    next_xset = n_xset;
		    }
		if (!in_td)
		    {
		    /* find first non-empty or non-string justification */
		    justif_subscan = scan;
		    while(justif_subscan != linetail && 
			justif_subscan->ObjType->TypeID == PRT_OBJ_T_STRING && ! (strlen((char*) scan->Content)))
		    {
			justif_subscan = justif_subscan->Next;
		    }


		    for(w=0.0,i=cur_xset;i<next_xset;i++) w += widths[i];
		    /*** Write HTML, skipping defaults (align="left", colspan="1")
		     *** to reduce HTML size.  These cells are written very often.
		     **/
		    prt_htmlfm_OutputStrLiteral(context, "<td");
		    if (justif_subscan->Justification != PRT_JUST_T_LEFT)
			prt_htmlfm_OutputPrintf(context, " align=\"%s\"", justifytypes[justif_subscan->Justification]);
		    const int n_cols = next_xset - cur_xset;
		    if (n_cols > 1)
			prt_htmlfm_OutputPrintf(context, " colspan=\"%d\"", n_cols);
		    prt_htmlfm_OutputPrintf(context,
			" width=\"%d\">",
			(int)(w*PRT_HTMLFM_XPIXEL+0.001)
		    );
		    prt_htmlfm_InitStyle(context, &(scan->TextStyle));
		    in_td = 1;
		    }

		/** print the child objects **/
		w = 0.0;
		/* set keepspaces at the start of this line */
		prt_htmlfm_SetKeepSpaces(context);
		while((!next_xset_obj || scan != next_xset_obj) && scan != linetail->Next)
		    {
		    prt_htmlfm_Generate_r(context, scan);
		    w += scan->Width;
		    scan = scan->Next;
		    }

		/** Nothing printed? **/
		if (w == 0.0)
		    {
		    prt_htmlfm_OutputStrLiteral(context, "&nbsp;");
		    }

		/** Emit the closing td? **/
		if (cur_needs_cols && in_td)
		    {
		    prt_htmlfm_EndStyle(context);
		    prt_htmlfm_OutputStrLiteral(context, "</td>");
		    in_td = 0;
		    }
		else if (in_td && scan)
		    {
		    prt_htmlfm_OutputStrLiteral(context, "<br>\n");
		    }
		cur_xset = next_xset;
		}
	    last_needed_cols = cur_needs_cols;
	    }

	/** Close the td and tr? **/
	if (in_td)
	    {
	    prt_htmlfm_EndStyle(context);
	    prt_htmlfm_OutputStrLiteral(context, "</td>");
	    in_td = 0;
	    }
	if (in_tr)
	    {
	    prt_htmlfm_OutputStrLiteral(context, "</tr>\n");
	    in_tr = 0;
	    }
    
	if(area->ContentTail && (area->ContentTail->Y + area->ContentTail->Height + 0.01 < area->Height)) {
	    prt_htmlfm_OutputPrintf(context, "<tr><td style=\"height: %dpx;line-height:0;\">&nbsp;</td></tr>",
		(int) ((area->Height - area->ContentTail->Y - area->ContentTail->Height + 0.001) * PRT_HTMLFM_YPIXEL));
	}

	/** Output the area epilogue **/
	prt_htmlfm_OutputStrLiteral(context, "</table>\n");
	if (lm_inf->AreaBorder.nLines > 0)
	    prt_htmlfm_EndBorder(context, &(lm_inf->AreaBorder), area);
	context->BGColor = saved_bg; /* Restore background color. */
	prt_htmlfm_ResetStyle(context, &oldstyle);

    return 0;
    }
