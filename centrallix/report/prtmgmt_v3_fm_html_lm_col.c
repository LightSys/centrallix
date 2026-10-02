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
#include "prtmgmt_v3/prtmgmt_v3_lm_col.h"
#include "prtmgmt_v3/prtmgmt_v3_fm_html.h"
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
/* Module:	prtmgmt_v3_fm_html_lm_col.c                             */
/* Author:	Greg Beeley                                             */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This module is the part of the html formatter which is	*/
/*		intelligent about PRT_OBJ_T_SECTION objects, which are	*/
/*		multicolumn layout sections.				*/
/************************************************************************/



/*** prt_htmlfm_GenerateMultiCol() - generate a multicolumn section
 ***/
int
prt_htmlfm_GenerateMultiCol(pPrtHTMLfmInf context, pPrtObjStream section)
    {
    pPrtObjStream column, subobj;
    PrtHTMLfmSavedStyle oldstyle;
    double end_y = 0.0;

	/** Write the section prologue **/
	if (UNLIKELY(prt_htmlfm_SaveStyle(context, &oldstyle) < 0))
	    {
	    mssError(0, "PRT", "Failed to save style.");
	    goto err;
	    }
	if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context,
	    "<table role=\"presentation\" cellpadding=\"0\"><tr>\n"
	) < 0))
	    {
	    mssError(0, "PRT", "Failed to write section opening tags.");
	    goto err;
	    }

	/** Loop through the column objects **/
	for(column = section->ContentHead; column; column = column->Next)
	    {
	    if (column->ObjType->TypeID != PRT_OBJ_T_SECTCOL) continue;
	    if (end_y > 0.0 && end_y != column->Y)
		{
		if (UNLIKELY(prt_htmlfm_OutputPrintf(context,
			"<td width=\"%d\">&nbsp;</td>",
			(int)(column->Y - end_y + 0.001)
		) < 0))
		    {
		    mssError(0, "PRT", "Failed to write column gap.");
		    goto err;
		    }
		}
	    if (UNLIKELY(prt_htmlfm_OutputPrintf(context,
		"<td width=\"%d\">",
		(int)(column->Width*PRT_HTMLFM_XPIXEL + 0.001)
	    ) < 0))
		{
		mssError(0, "PRT", "Failed to write column opening tag.");
		goto err;
		}
	    if (UNLIKELY(prt_htmlfm_InitStyle(context, &(column->TextStyle)) < 0)) goto err;
	    subobj = column->ContentHead;
	    while(subobj)
		{
		if (UNLIKELY(prt_htmlfm_Generate_r(context, subobj) < 0)) goto err;
		subobj = subobj->Next;
		}
	    if (UNLIKELY(prt_htmlfm_EndStyle(context) < 0)) goto err;
	    if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</td>") < 0))
		{
		mssError(0, "PRT", "Failed to write column closing tag.");
		goto err;
		}
	    end_y = column->Y + column->Width;
	    }

	/** Output the section epilogue **/
	if (UNLIKELY(prt_htmlfm_OutputStrLiteral(context, "</tr></table>\n") < 0))
	    {
	    mssError(0, "PRT", "Failed to write section closing tags.");
	    goto err;
	    }
	if (UNLIKELY(prt_htmlfm_ResetStyle(context, &oldstyle) < 0))
	    {
	    mssError(0, "PRT", "Failed to reset style.");
	    goto err;
	    }

	return 0;

    err:
	mssError(0, "PRT", "Failed to generate multicolumn section.");
	return -1;
    }
