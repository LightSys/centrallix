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
#include "prtmgmt_v3/prtmgmt_v3_lm_table.h"
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
/* Module:	prtmgmt_v3_fm_html_lm_table.c                           */
/* Author:	Greg Beeley                                             */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This provides the generator for tabular objects in the	*/
/*		html formatter which has intelligence about the tables.	*/
/************************************************************************/



/*** prt_htmlfm_GenerateTable() - output a tabular data section,
 *** including its rows and cells, into html
 ***/
int
prt_htmlfm_GenerateTable(pPrtHTMLfmInf context, pPrtObjStream table)
    {
    pPrtObjStream row, cell, subobj;
    PrtTextStyle oldstyle;
    int n_rows = 0, n_cols = 0, cur_row = 0, cur_col = 0;
    pPrtTabLMData lm_data = (pPrtTabLMData)(table->LMData);

	/** Write the table prologue **/
	prt_htmlfm_SaveStyle(context, &oldstyle);


	prt_htmlfm_OutputPrintf(context,"<table width=\"100%\" height=\"%fpx\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\"", table->Height * PRT_HTMLFM_YPIXEL);

	char borderbuf[256];
	snprintf(borderbuf, sizeof(borderbuf), " style=\"border-top: %fpx solid #%6.6X; border-right: %fpx solid #%6.6X; border-bottom: %fpx solid #%6.6X; border-left: %fpx solid #%6.6X;\">",
	    lm_data->TopBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->TopBorder.Color[0],
	    lm_data->RightBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->RightBorder.Color[0],
	    lm_data->BottomBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->BottomBorder.Color[0],
	    lm_data->LeftBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->LeftBorder.Color[0]);
	prt_htmlfm_Output(context, borderbuf, -1);

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
		if(cur_row == 1 && lm_data->HeaderRow) {
		    prt_htmlfm_Output(context, "<tr height=10>", -1);
		} else {
		    prt_htmlfm_Output(context, "<tr>", 4);
		}
		while(cell)
		    {
		    if (cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
			{
			cur_col++;
			prt_htmlfm_OutputPrintf(context,"<td width=\"%d\" align=\"left\" valign=\"top\" bgcolor=\"#%6.6X\"; style=\"padding:%dpx;" ,
				(int)(cell->Width*PRT_HTMLFM_XPIXEL),
    				cell->BGColor,
				(int)(lm_data->ColSep * PRT_HTMLFM_XPIXEL / 2));

			/* top border */
			if (cell->BorderTop != 0 || row->BorderTop != 0) {
			    if (cell->BorderTop != 0) {
				prt_htmlfm_OutputPrintf(context, " border-top: %fpx solid;", cell->BorderTop * PRT_HTMLFM_XPIXEL);
			    } else {
				prt_htmlfm_OutputPrintf(context, " border-top: %fpx solid;", row->BorderTop * PRT_HTMLFM_XPIXEL);
			    }
			} else if(cur_row != 1) {
			    prt_htmlfm_OutputPrintf(context, " border-top: %fpx solid #%6.6X;",
	    			lm_data->InnerBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->InnerBorder.Color[0]);
			}
			/* bottom border */
			if (cell->BorderBottom != 0 || row->BorderBottom != 0) {
			    if (cell->BorderBottom != 0) {
				prt_htmlfm_OutputPrintf(context, " border-bottom: %fpx solid;", cell->BorderBottom * PRT_HTMLFM_XPIXEL);
			    } else {
				prt_htmlfm_OutputPrintf(context," border-bottom: %fpx solid;", row->BorderBottom * PRT_HTMLFM_XPIXEL);
			    }
			} 
			/* left border */
			if (cell->BorderLeft != 0) {
			    prt_htmlfm_OutputPrintf(context, " border-left: %fpx solid;", cell->BorderLeft * PRT_HTMLFM_XPIXEL);
			} else if(cur_col != 1) {
			    prt_htmlfm_OutputPrintf(context, " border-left: %fpx solid #%6.6X;", 
				lm_data->InnerBorder.Width[0] * PRT_HTMLFM_XPIXEL, lm_data->InnerBorder.Color[0]);
			}
			/* right border */
			if (cell->BorderRight != 0) {
			    prt_htmlfm_OutputPrintf(context, " border-right: %fpx solid;", cell->BorderRight * PRT_HTMLFM_XPIXEL);
			}
			
			prt_htmlfm_Output(context, "\">", 2);
			prt_htmlfm_InitStyle(context, &(cell->TextStyle));
			for(subobj=cell->ContentHead;subobj;subobj=subobj->Next)
			    {
			    prt_htmlfm_Generate_r(context, subobj);
			    }
			prt_htmlfm_EndStyle(context);
			prt_htmlfm_Output(context,"</td>",5);
			}
		    cell=cell->Next;
		    }
		prt_htmlfm_Output(context, "</tr>\n", 6);
		}
	    else
		{
		/** Row containing arbitrary stuff.  Emit entire row **/
		prt_htmlfm_OutputPrintf(context, "<tr><td width=\"%d\" align=\"left\" valign=\"top\" colspan=\"%d\" bgcolor=\"#%6.6X\">", 
			(int)(row->Width*PRT_HTMLFM_XPIXEL),
			lm_data->nColumns,
			row->BGColor);
		prt_htmlfm_InitStyle(context, cell?(&(cell->TextStyle)):(&(row->TextStyle)));
		for(subobj=row->ContentHead;subobj;subobj=subobj->Next)
		    {
		    prt_htmlfm_Generate_r(context, subobj);
		    }
		prt_htmlfm_EndStyle(context);
		prt_htmlfm_Output(context,"</td></tr>\n",11);
		}
	    }

	/** Output the section epilogue **/
	prt_htmlfm_Output(context,"</table>\n", 9);
	prt_htmlfm_ResetStyle(context, &oldstyle);

    return 0;
    }


