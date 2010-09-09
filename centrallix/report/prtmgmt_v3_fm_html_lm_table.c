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

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_html_lm_table.c,v 1.4 2010/09/09 00:46:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_fm_html_lm_table.c,v $

    $Log: prtmgmt_v3_fm_html_lm_table.c,v $
    Revision 1.4  2010/09/09 00:46:13  gbeeley
    - (bugfix) HTML table output - handle condition where row is entirely
      empty (row->ContentHead == NULL)

    Revision 1.3  2007/04/08 03:52:01  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.2  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1  2003/04/21 21:01:56  gbeeley
    Adding LM specific components to the HTML formatter

 **END-CVSDATA***********************************************************/


/*** prt_htmlfm_GenerateTable() - output a tabular data section,
 *** including its rows and cells, into html
 ***/
int
prt_htmlfm_GenerateTable(pPrtHTMLfmInf context, pPrtObjStream table)
    {
    pPrtObjStream row, cell, subobj;
    PrtTextStyle oldstyle;
    pPrtTabLMData lm_data = (pPrtTabLMData)(table->LMData);

	/** Write the table prologue **/
	prt_htmlfm_SaveStyle(context, &oldstyle);
	prt_htmlfm_Output(context,"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n", -1);

	/** Loop through the subobjects, generating the rows **/
	for(row = table->ContentHead; row; row=row->Next)
	    {
	    if (row->ObjType->TypeID != PRT_OBJ_T_TABLEROW) continue;

	    /** Got a row.  Does it contain cells or otherwise? **/
	    cell = row->ContentHead;
	    if (cell && cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		{
		/** Got a cell.  Emit list of cells in the row **/
		prt_htmlfm_Output(context, "<tr>", 4);
		while(cell)
		    {
		    if (cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
			{
			prt_htmlfm_OutputPrintf(context,"<td width=\"%d\" align=\"left\" valign=\"top\" bgcolor=\"#%6.6X\">",
				(int)(cell->Width*PRT_HTMLFM_XPIXEL),
				cell->BGColor);
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


