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
/* Centrallix Application Server System 								*/
/* Centrallix Core	   													*/
/* 																		*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.			*/
/* 																		*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.									*/
/* 																		*/
/* This program is distributed in the hope that it will be useful,		*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of		*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		*/
/* GNU General Public License for more details.							*/
/* 																		*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software			*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  			*/
/* 02111-1307  USA														*/
/*																		*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".									*/
/* 																		*/
/* Module:	prtmgmt_v3_fm_html_lm_text.c								*/
/* Author:	Greg Beeley											 		*/
/* Date:	April 9th, 2003										 		*/
/*																		*/
/* Description:	This is the component of the HTML formatter which has	*/
/*		intelligence about textflow layout areas.						*/
/************************************************************************/



/*** prt_htmlfm_GenerateArea() - generates the html to represent a
 *** textflow area.
 ***/
int
prt_htmlfm_GenerateArea(pPrtHTMLfmInf context, pPrtObjStream area) {
	int n_xset;
	double xset[PRT_HTMLFM_MAX_TABSTOP];
	double widths[PRT_HTMLFM_MAX_TABSTOP];
	pPrtObjStream scan, linetail, next_xset_obj;
	int i,j,cur_xset,next_xset;
	double w;
	int last_needed_cols, cur_needs_cols, need_new_row, in_td, in_tr;
	PrtTextStyle oldstyle;
	char* justifytypes[] = { "left", "right", "center", "justify" };
	pPrtTextLMData lm_inf = (pPrtTextLMData)(area->LMData);

	/** First we need to scan the area looking for objects which
	 ** were positioned with PRT_OBJ_F_XSET, so we can build a table
	 ** of sorted 'tabstops'.
	 **/
	xset[0] = 0.0;
	n_xset = 1;
	for (scan = area->ContentHead; scan; scan = scan->Next) {
		if (scan->Flags & PRT_OBJ_F_XSET) {
			for (i = 0; i <= n_xset; i++) {
				if (i!=n_xset && xset[i] == scan->X) break;
				if (n_xset < PRT_HTMLFM_MAX_TABSTOP && (i == n_xset || xset[i] > scan->X)) {
					for (j = n_xset; j > i; j--) xset[j] = xset[j - 1];
					xset[i] = scan->X;
					n_xset++;
					break;
				}
			}
		}
	}

	prt_htmlfm_SaveStyle(context, &oldstyle);

	prt_htmlfm_OutputPrintf(context, "<div #area style=\"border: %d solid %d; display: flex; flex-direction: column; width: 100%%\">", lm_inf->AreaBorder.Width[0], lm_inf->AreaBorder.Color[0]);

	for (scan = area->ContentHead; scan != NULL; scan = scan->Next) {
		
		// prt_htmlfm_Output(context, "<div style=\"display: flex\">", -1);

		prt_htmlfm_Generate_r(context, scan);

		// justified text is split into individual objects... skip to next actual object
		while (scan->ObjType->TypeID == PRT_OBJ_T_STRING
				&& scan->Justification == 3
				&& scan->Next
				&& scan->Next->ObjType->TypeID == PRT_OBJ_T_STRING
				&& scan->Next->Justification == 3) {
			scan = scan->Next;
		}

		// prt_htmlfm_Output(context, "</div>", -1);
		
	}

	prt_htmlfm_Output(context, "</div>", -1);

	prt_htmlfm_ResetStyle(context, &oldstyle);

	return 0;

	/** Output the area prologue **/
	prt_htmlfm_SaveStyle(context, &oldstyle);
	prt_htmlfm_Border(context, &(lm_inf->AreaBorder), area);
	prt_htmlfm_Output(context, "<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">\n", -1);
	in_tr = 0;
	in_td = 0;

	/** Issue column width info **/
	for(i=0;i<n_xset;i++)
		{
		if (i == n_xset-1)
		widths[i] = area->Width - area->MarginLeft - area->MarginRight - xset[i];
		else 
		widths[i] = xset[i+1] - xset[i];

		/** We could use relative 'n*' formatting; older browsers will interpret as pixel
		 ** width, newer ones as relative width, but doesn't seem to work right
		 ** with newer browsers.
		 **/
		prt_htmlfm_OutputPrintf(context,"<col width=\"%d\">\n",(int)(widths[i]*PRT_HTMLFM_XPIXEL+0.0001));
		}
	prt_htmlfm_Output(context,"<tr>",4);
	for(i=0;i<n_xset;i++)
		{
		prt_htmlfm_OutputPrintf(context,"<td width=\"%d\"></td>",(int)(widths[i]*PRT_HTMLFM_XPIXEL+0.0001));
		}
	prt_htmlfm_Output(context,"</tr>\n",6);

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
		if ((linetail->Flags & (PRT_OBJ_F_SOFTNEWLINE | PRT_OBJ_F_NEWLINE)) || !linetail->Next) break;
		linetail = linetail->Next;
		}
		need_new_row = (cur_needs_cols || last_needed_cols || scan->Justification != PRT_JUST_T_LEFT);
		if (need_new_row)
		{
		if (in_td)
			{
			prt_htmlfm_EndStyle(context);
			prt_htmlfm_Output(context,"</td>", 5);
			in_td = 0;
			}
		if (in_tr)
			{
			prt_htmlfm_Output(context,"</tr>\n", 6);
			in_tr = 0;
			}
		}
		if (!in_tr)
		{
		prt_htmlfm_Output(context,"<tr>", 4);
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
			prt_htmlfm_OutputPrintf(context, "<td colspan=\"%d\" width=\"%d\">&nbsp;</td>", 
				cur_xset, (int)(widths[cur_xset]*PRT_HTMLFM_XPIXEL+0.001));
			}
			}
		}

		/** Ok, scan through the line now **/
		while(scan != linetail->Next)
		{
		/** Find next xset location **/
		if (cur_needs_cols)
			{
			next_xset_obj = scan->Next;
			while(next_xset_obj != linetail->Next && !(next_xset_obj->Flags & PRT_OBJ_F_XSET)) next_xset_obj=next_xset_obj->Next;
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
			for(w=0.0,i=cur_xset;i<next_xset;i++) w += widths[i];
			prt_htmlfm_OutputPrintf(context, "<td align=\"%s\" valign=\"top\" colspan=\"%d\" width=\"%d\">",
				justifytypes[scan->Justification], next_xset - cur_xset,
				(int)(w*PRT_HTMLFM_XPIXEL+0.001));
			prt_htmlfm_InitStyle(context, &(scan->TextStyle));
			in_td = 1;
			}

		/** print the child objects **/
		w = 0.0;
		while((!next_xset_obj || scan != next_xset_obj->Next) && scan != linetail->Next)
			{
			prt_htmlfm_Generate_r(context, scan);
			w += scan->Width;
			scan = scan->Next;
			}

		/** Nothing printed? **/
		if (w == 0.0)
			{
			prt_htmlfm_Output(context, "&nbsp;", 6);
			}

		/** Emit the closing td? **/
		if (cur_needs_cols && in_td)
			{
			prt_htmlfm_EndStyle(context);
			prt_htmlfm_Output(context, "</td>", 5);
			in_td = 0;
			}
		else if (in_td && scan)
			{
			prt_htmlfm_Output(context,"<br>\n",5);
			}
		cur_xset = next_xset;
		}
		last_needed_cols = cur_needs_cols;
		}

	/** Close the td and tr? **/
	if (in_td)
		{
		prt_htmlfm_EndStyle(context);
		prt_htmlfm_Output(context, "</td>", 5);
		in_td = 0;
		}
	if (in_tr)
		{
		prt_htmlfm_Output(context,"</tr>\n", 6);
		in_tr = 0;
		}

	/** Output the area epilogue **/
	prt_htmlfm_Output(context,"</table>\n", -1);
	prt_htmlfm_EndBorder(context, &(lm_inf->AreaBorder), area);
	prt_htmlfm_ResetStyle(context, &oldstyle);

	return 0;
	}


