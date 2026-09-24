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
#include "cxlib/expect.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_fm_html.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_table.h"
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
/* Module:	prtmgmt_v3_fm_html_lm_table.c                           */
/* Author:	Greg Beeley                                             */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This provides the generator for tabular objects in the	*/
/*		html formatter which has intelligence about the tables.	*/
/************************************************************************/



/*** prt_htmlfm_OutputBorder() - Writes a single CSS "border-<side>" declaration
 *** with an integer pixel width.  Rounding widths to the nearest integer reduces
 *** HTML bloat and improves rendering consistency (especially for some email
 *** HTML).  Also, 0-width borders are skipped to reduce HTML bloat.
 *** 
 *** @param context The report formatter context in which to print.
 *** @param side The side to print (one of top, right, bottom, left).
 *** @param width_units The unrounded border width.
 *** @param color The color of the border, -1 to not use color.
 *** @returns 0 on success, or -1 on failure.
 ***/
static int
prt_htmlfm_OutputBorder(pPrtHTMLfmInf context, const char* side, double width_units, int color)
    {
    int border_width;

	if (width_units == 0.0) return 0;
	border_width = (int)(width_units * PRT_HTMLFM_XPIXEL + 0.5);
	if (border_width < 1) border_width = 1;
	const int rval = (color == -1)
	    ? prt_htmlfm_OutputPrintf(context, " border-%s: %dpx solid;", side, border_width)
	    : prt_htmlfm_OutputPrintf(context, " border-%s: %dpx solid #%6.6X;", side, border_width, color);
	if (UNLIKELY(rval < 0))
	    {
	    mssError(0, "PRT", "Failed to write %dpx %s border.", border_width, side);
	    return -1;
	    }

    return 0;
    }


/*** prt_htmlfm_GenerateTable() - output a tabular data section,
 *** including its rows and cells, into html
 ***/
int
prt_htmlfm_GenerateTable(pPrtHTMLfmInf context, pPrtObjStream table)
    {
    pPrtObjStream row, cell, subobj;
    PrtHTMLfmSavedStyle oldstyle;
    int n_rows = 0, n_cols = 0, cur_row = 0, cur_col = 0;
    pPrtTabLMData lm_data = (pPrtTabLMData)(table->LMData);

	/** Write the table prologue **/
	if (UNLIKELY(prt_htmlfm_SaveStyle(context, &oldstyle) < 0))
	    {
	    mssError(0, "PRT", "Failed to save style.");
	    goto err;
	    }


	/** Write the container HTML with borders. **/
	if (UNLIKELY(prt_htmlfm_OutputPrintf(context,
	    "<table width=\"100%%\" cellpadding=\"0\" style=\"height: %dpx;",
	    (int)(table->Height * PRT_HTMLFM_YPIXEL + 0.5)
	) < 0
	    || prt_htmlfm_OutputBorder(context, "top", lm_data->TopBorder.Width[0], lm_data->TopBorder.Color[0]) < 0
	    || prt_htmlfm_OutputBorder(context, "right", lm_data->RightBorder.Width[0], lm_data->RightBorder.Color[0]) < 0
	    || prt_htmlfm_OutputBorder(context, "bottom", lm_data->BottomBorder.Width[0], lm_data->BottomBorder.Color[0]) < 0
	    || prt_htmlfm_OutputBorder(context, "left", lm_data->LeftBorder.Width[0], lm_data->LeftBorder.Color[0]) < 0
	    || prt_htmlfm_OutputStrLiteral(context, "\">") < 0
	))  {
	    mssError(0, "PRT", "Failed to write table opening tag.");
	    goto err;
	    }

	/* Count rows for style purposes */
	for(row = table->ContentHead; row; row=row->Next) {
	    if (row->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		n_rows++;
	}
	cur_row = 0;

	/** Loop through the subobjects, generating the rows **/
	for(row = table->ContentHead; row; row=row->Next)
	    {
	    if (row->ObjType->TypeID != PRT_OBJ_T_TABLEROW) continue;

	    cur_row++;
	    /*count cols for style purposes */
	    n_cols = 0;
	    for(cell = row->ContentHead; cell; cell=cell->Next) {
		if (cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		    n_cols++;
	    }
	    cur_col = 0;

	    /** Got a row.  Does it contain cells or otherwise? **/
	    cell = row->ContentHead;
	    if (cell && cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		{
		/** Got a cell.  Emit list of cells in the row **/
		/* Arbitrarily specify a restricted height for table header if it has one */
		const int tr_rval = (cur_row == 1 && lm_data->HeaderRow)
		    ? prt_htmlfm_OutputStrLiteral(context, "<tr height=10>")
		    : prt_htmlfm_OutputStrLiteral(context, "<tr>");
		if (UNLIKELY(tr_rval < 0))
		    {
		    mssError(0, "PRT", "Failed to write row #%d/%d opening tag.", cur_row, n_rows);
		    goto err;
		    }
		while(cell)
		    {
		    if (cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
			{
			cur_col++;
			if (UNLIKELY(prt_htmlfm_OutputPrintf(context, "<td width=\"%d\"",
				(int)(cell->Width*PRT_HTMLFM_XPIXEL)) < 0))
			    goto err_cell;
			int saved_bg = prt_htmlfm_OutputBGColor(context, cell->BGColor);
			if (UNLIKELY(saved_bg < 0)) goto err_cell;
			if (UNLIKELY(prt_htmlfm_OutputPrintf(context, " style=\"padding:%dpx;",
				(int)(lm_data->ColSep * PRT_HTMLFM_XPIXEL / 2)) < 0))
			    goto err_cell;

			/* top border */
			if (cell->BorderTop != 0 || row->BorderTop != 0) {
			    if (cell->BorderTop != 0) {
				if (UNLIKELY(prt_htmlfm_OutputBorder(context, "top", cell->BorderTop, -1) < 0)) goto err_cell;
			    } else {
				if (UNLIKELY(prt_htmlfm_OutputBorder(context, "top", row->BorderTop, -1) < 0)) goto err_cell;
			    }
			} else if(cur_row != 1) {
			    if (UNLIKELY(prt_htmlfm_OutputBorder(context, "top", lm_data->InnerBorder.Width[0], lm_data->InnerBorder.Color[0]) < 0)) goto err_cell;
			}
			/* bottom border */
			if (cell->BorderBottom != 0 || row->BorderBottom != 0) {
			    if (cell->BorderBottom != 0) {
				if (UNLIKELY(prt_htmlfm_OutputBorder(context, "bottom", cell->BorderBottom, -1) < 0)) goto err_cell;
			    } else {
				if (UNLIKELY(prt_htmlfm_OutputBorder(context, "bottom", row->BorderBottom, -1) < 0)) goto err_cell;
			    }
			} 
			/* left border */
			if (cell->BorderLeft != 0) {
			    if (UNLIKELY(prt_htmlfm_OutputBorder(context, "left", cell->BorderLeft, -1) < 0)) goto err_cell;
			} else if(cur_col != 1) {
			    if (UNLIKELY(prt_htmlfm_OutputBorder(context, "left", lm_data->InnerBorder.Width[0], lm_data->InnerBorder.Color[0]) < 0)) goto err_cell;
			}
			/* right border */
			if (cell->BorderRight != 0) {
			    if (UNLIKELY(prt_htmlfm_OutputBorder(context, "right", cell->BorderRight, -1) < 0)) goto err_cell;
			}
			
			if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "\">") < 0)) goto err_cell;
			if (UNLIKELY(prt_htmlfm_InitStyle(context, &(cell->TextStyle)) < 0)) goto err_cell;
			for(subobj=cell->ContentHead;subobj;subobj=subobj->Next)
			    {
			    if (UNLIKELY(prt_htmlfm_Generate_r(context, subobj) < 0)) goto err_cell;
			    }
			if (UNLIKELY(prt_htmlfm_EndStyle(context) < 0)) goto err_cell;
			if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</td>") < 0)) goto err_cell;
			context->BGColor = saved_bg;
			}
		    cell=cell->Next;
		    }
		if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</tr>\n") < 0))
		    {
		    mssError(0, "PRT", "Failed to write row #%d/%d closing tag.", cur_row, n_rows);
		    goto err;
		    }
		}
	    else
		{
		/** Write row container opening tags. **/
		if (UNLIKELY(prt_htmlfm_OutputPrintf(context, "<tr><td width=\"%d\"",
		    (int)(row->Width*PRT_HTMLFM_XPIXEL)) < 0))
		    goto err_row;
		int saved_bg = prt_htmlfm_OutputBGColor(context, row->BGColor);
		if (UNLIKELY(saved_bg < 0)) goto err_row;
		if (UNLIKELY(lm_data->nColumns > 1
		    && prt_htmlfm_OutputPrintf(context, " colspan=\"%d\"", lm_data->nColumns) < 0))
		    goto err_row;
		if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, ">") < 0)) goto err_row;
		if (UNLIKELY(prt_htmlfm_InitStyle(context, cell?(&(cell->TextStyle)):(&(row->TextStyle))) < 0)) goto err_row;
		
		/** Write child content. **/
		for(subobj=row->ContentHead;subobj;subobj=subobj->Next)
		    {
		    if (UNLIKELY(prt_htmlfm_Generate_r(context, subobj) < 0)) goto err_row;
		    }
		
		/** Write row container closing tags. */
		if (UNLIKELY(prt_htmlfm_EndStyle(context) < 0)) goto err_row;
		if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</td></tr>\n") < 0)) goto err_row;
		context->BGColor = saved_bg; /* Restore background color. */
		}
	    }

	/** Output the section epilogue **/
	if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</table>\n") < 0))
	    {
	    mssError(0, "PRT", "Failed to write table closing tag.");
	    goto err;
	    }
	if (UNLIKELY(prt_htmlfm_ResetStyle(context, &oldstyle) < 0))
	    {
	    mssError(0, "PRT", "Failed to reset style.");
	    goto err;
	    }

	return 0;

    err_cell:
	mssError(0, "PRT", "Failed to write cell #%d/%d in row #%d/%d.", cur_col, n_cols, cur_row, n_rows);
	goto err;

    err_row:
	mssError(0, "PRT", "Failed to write row #%d/%d.", cur_row, n_rows);
	goto err;

    err:
	mssError(0, "PRT", "Failed to generate table.");
	return -1;
    }
