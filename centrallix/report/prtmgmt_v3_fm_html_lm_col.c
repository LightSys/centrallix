#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "magic.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_col.h"
#include "prtmgmt_v3/prtmgmt_v3_fm_html.h"
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
/* Module:	prtmgmt_v3_fm_html_lm_col.c                             */
/* Author:	Greg Beeley                                             */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This module is the part of the html formatter which is	*/
/*		intelligent about PRT_OBJ_T_SECTION objects, which are	*/
/*		multicolumn layout sections.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_html_lm_col.c,v 1.1 2003/04/21 21:01:56 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_fm_html_lm_col.c,v $

    $Log: prtmgmt_v3_fm_html_lm_col.c,v $
    Revision 1.1  2003/04/21 21:01:56  gbeeley
    Adding LM specific components to the HTML formatter

 **END-CVSDATA***********************************************************/


/*** prt_htmlfm_GenerateMultiCol() - generate a multicolumn section
 ***/
int
prt_htmlfm_GenerateMultiCol(pPrtHTMLfmInf context, pPrtObjStream section)
    {
    pPrtObjStream column, subobj;
    PrtTextStyle oldstyle;
    double end_y = 0.0;

	/** Write the section prologue **/
	prt_htmlfm_SaveStyle(context, &oldstyle);
	prt_htmlfm_Output(context,"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr>\n", -1);

	/** Loop through the column objects **/
	for(column = section->ContentHead; column; column = column->Next)
	    {
	    if (column->ObjType->TypeID != PRT_OBJ_T_SECTCOL) continue;
	    if (end_y > 0.0 && end_y != column->Y)
		{
		prt_htmlfm_OutputPrintf(context, "<td width=\"%d\">&nbsp;</td>", (int)(column->Y - end_y + 0.001));
		}
	    prt_htmlfm_OutputPrintf(context, "<td valign=\"top\" align=\"left\" width=\"%d\">", (int)(column->Width*PRT_HTMLFM_XPIXEL + 0.001));
	    prt_htmlfm_InitStyle(context, &(column->TextStyle));
	    subobj = column->ContentHead;
	    while(subobj)
		{
		prt_htmlfm_Generate_r(context, subobj);
		subobj = subobj->Next;
		}
	    prt_htmlfm_EndStyle(context);
	    prt_htmlfm_Output(context, "</td>",5);
	    end_y = column->Y + column->Width;
	    }

	/** Output the section epilogue **/
	prt_htmlfm_Output(context,"</tr></table>\n", -1);
	prt_htmlfm_ResetStyle(context, &oldstyle);

    return 0;
    }

