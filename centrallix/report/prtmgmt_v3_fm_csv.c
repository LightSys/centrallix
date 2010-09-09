#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/datatypes.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_table.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_text.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2009 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_fm_csv.c                                     */
/* Author:	Greg Beeley                                             */
/* Date:	October 28, 2009                                        */
/*									*/
/* Description:	This module is the CSV report formatter, which takes any*/
/*		data in tabular form and presents it as a CSV file.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_csv.c,v 1.2 2010/09/09 00:51:07 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_fm_csv.c,v $

    $Log: prtmgmt_v3_fm_csv.c,v $
    Revision 1.2  2010/09/09 00:51:07  gbeeley
    - (bugfix) output money datatypes in a csv file in a way that will
      consistently load right into OO Calc and Excel.
    - (bugfix) handle space suppresion (when undoing word wrapping) correctly

    Revision 1.1  2010/01/10 07:20:18  gbeeley
    - (feature) adding CSV file output from report writer.  Simply outputs
      only tabular data (report/table data) into a CSV file format.
    - (change) API addition to prtmgmt -- report writer can specify data type
      of a piece of printed data; used as "hints" by the CSV file output to
      output a cell as a quoted string vs. an integer or currency value


 **END-CVSDATA***********************************************************/


/*** GLOBAL DATA FOR THIS MODULE ***/
typedef struct _PSC
    {
    }
    PRT_CSVFM_t;

PRT_CSVFM_t PRT_CSVFM;


/*** formatter internal structure.  Typedef incomplete def'n is in the
 *** header file.  This completes it. 
 ***/
typedef struct _PSFI
    {
    pPrtSession		Session;	/* print session */
    XString		LastHeader;	/* last CSV header written to .csv file, used to avoid dup headers */
    XString		CurrentLine;	/* current line in CSV file that is being built. */
    int			CellStringCnt;	/* count of strings added to the cell */
    }
    PrtCSVfmInf, *pPrtCSVfmInf;


/*** prt_csvfm_Output() - outputs a string of text into the HTML
 *** document.
 ***/
int
prt_csvfm_Output(pPrtCSVfmInf context, char* str, int len)
    {

	/** Check length **/
	if (len < 0) len = strlen(str);

    return context->Session->WriteFn(context->Session->WriteArg, str, len, 0, FD_U_PACKET);
    }


/*** prt_csvfm_OutputPrintf() - outputs a string of text into the
 *** HTML document, using "printf" semantics.
 ***/
int
prt_csvfm_OutputPrintf(pPrtCSVfmInf context, char* fmt, ...)
    {
    va_list va;
    int rval;

	va_start(va, fmt);
	rval = xsGenPrintf_va(context->Session->WriteFn, context->Session->WriteArg, NULL, NULL, fmt, va);
	va_end(va);

    return rval;
    }


/*** prt_csvfm_Probe() - this function is called when a new printmanagement
 *** session is opened and this driver is being asked whether or not it can
 *** print the given content type.
 ***/
void*
prt_csvfm_Probe(pPrtSession s, char* output_type)
    {
    pPrtCSVfmInf context;

	/** Is it html? **/
	if (strcasecmp(output_type,"text/csv") != 0) return NULL;

	/** Allocate our context inf structure **/
	context = (pPrtCSVfmInf)nmMalloc(sizeof(PrtCSVfmInf));
	if (!context) return NULL;
	memset(context, 0, sizeof(PrtCSVfmInf));
	context->Session = s;
	xsInit(&context->LastHeader);
	xsInit(&context->CurrentLine);

    return (void*)context;
    }


/*** prt_csvfm_GetNearestFontSize - return the nearest font size that this
 *** driver supports.  We're creating a CSV file, so we don't care about font
 *** sizes.
 ***/
int
prt_csvfm_GetNearestFontSize(void* context_v, int req_size)
    {
    /*pPrtCSVfmInf context = (pPrtCSVfmInf)context_v;*/

    return req_size;
    }


/*** prt_csvfm_GetCharacterMetric - return the sizing information for a given
 *** character, in standard units.
 ***/
void
prt_csvfm_GetCharacterMetric(void* context_v, char* str, pPrtTextStyle style, double* width, double* height)
    {
    /*pPrtCSVfmInf context = (pPrtCSVfmInf)context_v;*/
    
	/** Based on font, style, and size... **/
	if (style->FontID == PRT_FONT_T_MONOSPACE)
	    {
	    *width = strlen(str)*style->FontSize/12.0;
	    *height = style->FontSize/12.0;
	    return;
	    }

	*width = strlen(str)*style->FontSize/12.0;
	*height = style->FontSize/12.0;

	/** HACK: make layout mgr think everything fits, no wrapping/etc. **/
	*width /= 100;

    return;
    }


/*** prt_csvfm_GetCharacterBaseline - return the distance from the upper
 *** left corner of the character cell to the left baseline point of the 
 *** character cell, in standard units.
 ***/
double
prt_csvfm_GetCharacterBaseline(void* context_v, pPrtTextStyle style)
    {
    /*pPrtCSVfmInf context = (pPrtCSVfmInf)context_v;*/
    return 0.75*style->FontSize/12.0;
    }


/*** prt_csvfm_Close() - end a printing session and destroy the context
 *** structure.
 ***/
int
prt_csvfm_Close(void* context_v)
    {
    pPrtCSVfmInf context = (pPrtCSVfmInf)context_v;

	/** Free memory used **/
	xsDeInit(&context->LastHeader);
	xsDeInit(&context->CurrentLine);
	nmFree(context, sizeof(PrtCSVfmInf));

    return 0;
    }



/*** prt_csvfm_GenerateTableCell_r() - recursive helper for the below function,
 *** this searches through things to find string data to "include"...
 ***/
int
prt_csvfm_GenerateTableCell_r(pPrtCSVfmInf context, pPrtObjStream obj, char esc_char)
    {
    int pos, len;
    char* esc_char_ptr;
    pPrtObjStream subobj;

	/** If a string, just add it to the line **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    pos = context->CurrentLine.Length;
	    len = strlen((char*)(obj->Content));
	    if (len)
		{
		/*if (context->CellStringCnt) xsConcatenate(&context->CurrentLine, " ", 1);*/
		xsConcatenate(&context->CurrentLine, (char*)obj->Content, len);
		context->CellStringCnt++;
		if (esc_char)
		    {
		    while(1)
			{
			/** If we encounter the char to be "escaped", double it, CSV / SQL style **/
			esc_char_ptr = strchr(context->CurrentLine.String + pos, esc_char);
			if (!esc_char_ptr) break;
			pos = esc_char_ptr - context->CurrentLine.String;
			xsInsertAfter(&context->CurrentLine, &esc_char, 1, pos);
			pos += 2;
			}
		    }
		}
	    if (obj->Flags & PRT_TEXTLM_F_RMSPACE) xsConcatenate(&context->CurrentLine, " ", 1);
	    return 0;
	    }

	/** Otherwise, recurse through the print object tree structure **/
	for(subobj=obj->ContentHead; subobj; subobj=subobj->Next)
	    prt_csvfm_GenerateTableCell_r(context, subobj, esc_char);

    return 0;
    }


/*** prt_csvfm_GenerateTableCell() - generate a table cell, or one datum
 *** in the csv file
 ***/
int
prt_csvfm_GenerateTableCell(pPrtCSVfmInf context, pPrtObjStream cell_obj)
    {
    pPrtObjStream subobj;
    int datatype;
    char esc_char;
    int value_start;
    MoneyType m;
    char* ptr;

	/** First, determine data type of the cell, if available. **/
	datatype = 0;
	if (cell_obj->DataType) datatype = cell_obj->DataType;
	if (!datatype)
	    {
	    for(subobj=cell_obj->ContentHead; subobj; subobj=subobj->Next)
		{
		if (subobj->DataType)
		    {
		    datatype = subobj->DataType;
		    break;
		    }
		}
	    }
	if (!datatype) datatype = DATA_T_STRING;

	/** Add quotes and set escape char if needed **/
	if (datatype == DATA_T_STRING)
	    {
	    xsConcatenate(&context->CurrentLine, "\"", 1);
	    esc_char = '"';
	    }
	else
	    {
	    esc_char = '\0';
	    }
	value_start = context->CurrentLine.Length;

	/** Generate the content **/
	context->CellStringCnt = 0;
	prt_csvfm_GenerateTableCell_r(context, cell_obj, esc_char);

	/** Add closing quote if needed **/
	if (datatype == DATA_T_STRING)
	    {
	    xsConcatenate(&context->CurrentLine, "\"", 1);
	    }

	/** If date or money type, re-interpret value and output in a "vanilla" format **/
	if (datatype == DATA_T_MONEY)
	    {
	    /** Remove any commas **/ /** now done inside objDataToMoney **/
	    /*while (xsReplace(&context->CurrentLine, ",", 1, value_start, "", 0) >= 0) ;*/

	    if (objDataToMoney(DATA_T_STRING, context->CurrentLine.String + value_start, &m) == 0)
		{
		ptr = objFormatMoneyTmp(&m, "0.00");
		xsSubst(&context->CurrentLine, value_start, -1, ptr, -1);
		}
	    }

    return 0;
    }


/*** prt_csvfm_GenerateTableRow() - starts generation of a table row
 ***/
int
prt_csvfm_GenerateTableRow(pPrtCSVfmInf context, pPrtObjStream row_obj)
    {
    pPrtObjStream subobj;
    int first_one = 1;

	/** Clear the row buffer **/
	xsCopy(&context->CurrentLine, "", 0);

	/** Search for cells **/
	for(subobj=row_obj->ContentHead; subobj; subobj=subobj->Next)
	    {
	    if (subobj->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		{
		if (!first_one)
		    xsConcatenate(&context->CurrentLine, ",", 1);
		else
		    first_one = 0;
		prt_csvfm_GenerateTableCell(context, subobj);
		}
	    }

	/** No cells found - just a content row?  Just output it verbatim if so. **/
	if (first_one)
	    {
	    context->CellStringCnt = 0;
	    prt_csvfm_GenerateTableCell_r(context, row_obj, '\0');
	    }

	/** Newline on the end **/
	xsConcatenate(&context->CurrentLine, "\n", 1);

	/** Output the line, suppressing blank lines (with just a \n on the end) **/
	if (context->CurrentLine.Length > 1)
	    {
	    /** Header?  If same as prior header, skip it, otherwise save it. **/
	    if ((((pPrtTabLMData)(row_obj->LMData))->Flags & PRT_TABLM_F_ISHEADER))
		{
		if (!strcmp(context->CurrentLine.String, context->LastHeader.String))
		    return 0;
		else
		    xsCopy(&context->LastHeader, context->CurrentLine.String, -1);
		}

	    /** Output it **/
	    prt_csvfm_Output(context, context->CurrentLine.String, -1);
	    }

    return 0;
    }


/*** prt_csvfm_GenerateTable() - starts generation of a table, which is
 *** what the CSV file really is...
 ***/
int
prt_csvfm_GenerateTable(pPrtCSVfmInf context, pPrtObjStream table_obj)
    {
    pPrtObjStream subobj;

	/** Search for rows... **/
	for(subobj=table_obj->ContentHead; subobj; subobj=subobj->Next)
	    {
	    if (subobj->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		{
		prt_csvfm_GenerateTableRow(context, subobj);
		}
	    }

    return 0;
    }


/*** prt_csvfm_Generate() - generate the csv file for the "page". All we
 *** do is hunt down tables to generate.
 ***/
int
prt_csvfm_Generate(void* context_v, pPrtObjStream page_obj)
    {
    pPrtCSVfmInf context = (pPrtCSVfmInf)context_v;
    pPrtObjStream subobj;

	/** Hunt for tables... **/
	for(subobj=page_obj->ContentHead; subobj; subobj=subobj->Next)
	    {
	    switch(subobj->ObjType->TypeID)
		{
		case PRT_OBJ_T_AREA:
		case PRT_OBJ_T_SECTION:
		case PRT_OBJ_T_SECTCOL:
		    /** recurse, looking for tables **/
		    prt_csvfm_Generate(context_v, subobj);
		    break;

		case PRT_OBJ_T_TABLE:
		    prt_csvfm_GenerateTable(context, subobj);
		    break;
		}
	    }

    return 0;
    }


int
prt_csvfm_GetType(void* ctx, char* objname, char* attrname, void* val_v)
    {
    POD(val_v)->String = "text/csv";
    return 0;
    }


/*** prt_csvfm_Initialize() - init this module and register with the main
 *** print management system.
 ***/
int
prt_csvfm_Initialize()
    {
    pPrtFormatter fmtdrv;
    pSysInfoData si;

	/** Init our globals **/
	memset(&PRT_CSVFM, 0, sizeof(PRT_CSVFM));

	/** Allocate the formatter structure, and init it **/
	fmtdrv = prtAllocFormatter();
	if (!fmtdrv) return -1;
	strcpy(fmtdrv->Name, "csv");
	fmtdrv->Probe = prt_csvfm_Probe;
	fmtdrv->Generate = prt_csvfm_Generate;
	fmtdrv->GetNearestFontSize = prt_csvfm_GetNearestFontSize;
	fmtdrv->GetCharacterMetric = prt_csvfm_GetCharacterMetric;
	fmtdrv->GetCharacterBaseline = prt_csvfm_GetCharacterBaseline;
	fmtdrv->Close = prt_csvfm_Close;

	/** Register with the main prtmgmt system **/
	prtRegisterFormatter(fmtdrv);

	/** Register with the cx.sysinfo /prtmgmt/output_types dir **/
	si = sysAllocData("/prtmgmt/output_types/csv", NULL, NULL, NULL, NULL, prt_csvfm_GetType, NULL, 0);
	sysAddAttrib(si, "type", DATA_T_STRING);
	sysRegister(si, NULL);

    return 0;
    }


