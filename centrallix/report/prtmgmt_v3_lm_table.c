#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
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
/* Copyright (C) 2001-2003 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt.c,prtmgmt.h                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	March 1, 2003                                           */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/*									*/
/*		prtmgmt_v3_lm_table.c:  This module contains the logic	*/
/*		for formatting tabular data - tables, headers, rows,	*/
/*		and cells.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_table.c,v 1.4 2003/03/07 06:16:12 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_table.c,v $

    $Log: prtmgmt_v3_lm_table.c,v $
    Revision 1.4  2003/03/07 06:16:12  gbeeley
    Added border-drawing functionality, and converted the multi-column
    layout manager to use that for column separators.  Added border
    capability to textareas.  Reworked the deinit/init kludge in the
    Reflow logic.

    Revision 1.3  2003/03/06 02:52:35  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.2  2003/03/03 23:45:22  gbeeley
    Added support for multi-column formatting where columns are not equal
    in width.  Specifying width/height as negative when adding one object
    to another causes that object to fill its container in the respective
    dimension(s).  Fixed a bug in the Justification logic.

    Revision 1.1  2003/03/02 04:17:35  gbeeley
    Adding skeleton tabular layout manager, for table/row/cell formatted
    data.


 **END-CVSDATA***********************************************************/


#define PRT_TABLM_MAXCOLS		256	/* maximum columns in a table */

#define PRT_TABLM_F_ISHEADER		1	/* row is a header that repeats */
#define PRT_TABLM_F_ISFOOTER		2	/* row is a repeating footer */

#define PRT_TABLM_DEFAULT_FLAGS		(0)
#define PRT_TABLM_DEFAULT_COLSEP	1.0	/* column separation */


/*** table specific data ***/
typedef struct _PTB
    {
    double		ColWidths[PRT_TABLM_MAXCOLS];
    double		ColX[PRT_TABLM_MAXCOLS];
    double		ColSep;
    int			nColumns;	/* number of columns in table */
    int			CurColID;	/* next cell inserted is this col. */
    pPrtObjStream	HeaderRow;	/* row that is the table header */
    pPrtObjStream	FooterRow;	/* table footer row */
    int			Flags;
    PrtBorder		TopBorder;
    PrtBorder		BottomBorder;
    PrtBorder		LeftBorder;
    PrtBorder		RightBorder;
    PrtBorder		Shadow;		/* only valid on table as a whole */
    }
    PrtTabLMData, *pPrtTabLMData;


/*** prt_tablm_Break() - this is called when we actually are going to do
 *** a break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_tablm_Break(pPrtObjStream this, pPrtObjStream *new_this)
    {
    return -1;
    }


/*** prt_tablm_ChildBreakReq() - this is called when a child object requests
 *** a break.
 ***/
int
prt_tablm_ChildBreakReq(pPrtObjStream this, pPrtObjStream child, pPrtObjStream *new_this)
    {
    return this->LayoutMgr->Break(this, new_this);
    }


/*** prt_tablm_ChildResizeReq() - this is called when a child object
 *** within this one is about to be resized.  This method gives this
 *** layout manager a chance to prevent the resize operation (return -1).  
 *** If the OK is given (return 0), a ChildResized method call will occur
 *** shortly thereafter (once the resize of the child has completed).
 ***/
int
prt_tablm_ChildResizeReq(pPrtObjStream this, pPrtObjStream child, double req_width, double req_height)
    {
    return -1;
    }


/*** prt_tablm_ChildResized() - this function is called when a child
 *** object has actually been resized.
 ***/
int
prt_tablm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {
    return -1;
    }


/*** prt_tablm_Resize() - request to resize.
 ***/
int
prt_tablm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
    return -1;
    }


/*** prt_tablm_AddObject() - used to add a new object to the table.
 ***/
int
prt_tablm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {
    return -1;
    }


/*** prt_tablm_InitTable() - initializes a table container, which can contain
 *** table row objects but nothing else.
 ***/
int
prt_tablm_InitTable(pPrtObjStream this, va_list va)
    {
    pPrtTabLMData lm_inf;
    int i;
    double totalwidth;
    char* attrname;

	/** Allocate our lm-specific data **/
	lm_inf = (pPrtTabLMData)nmMalloc(sizeof(PrtTabLMData));
	if (!lm_inf) return -ENOMEM;
	this->LMData = (void*)lm_inf;
	memset(lm_inf, 0, sizeof(PrtTabLMData));

	/** Set up the defaults **/
	lm_inf->nColumns = 1;
	lm_inf->CurColID = 1;
	lm_inf->HeaderRow = NULL;
	lm_inf->FooterRow = NULL;
	lm_inf->Flags = PRT_TABLM_DEFAULT_FLAGS;
	lm_inf->ColSep = PRT_TABLM_DEFAULT_COLSEP;
	lm_inf->ColWidths[0] = -1.0;

	/** Get params from the caller **/
	while(va && (attrname = va_arg(va, char*)) != NULL)
	    {
	    if (!strcmp(attrname, "numcols"))
		{
		/** set number of columns; default = 1 **/
		lm_inf->nColumns = va_arg(va, int);
		if (lm_inf->nColumns > PRT_TABLM_MAXCOLS || lm_inf->nColumns < 1)
		    {
		    nmFree(lm_inf, sizeof(PrtTabLMData));
		    this->LMData = NULL;
		    mssError(1,"TABLM","Invalid number of columns (numcols=%d) for a table; max is %d", lm_inf->nColumns, PRT_TABLM_MAXCOLS);
		    return -EINVAL;
		    }
		}
	    else if (!strcmp(attrname, "colwidths"))
		{
		/** Set widths of columns; default = 1 column and full width **/
		for(i=0;i<lm_inf->nColumns;i++)
		    {
		    lm_inf->ColWidths[i] = va_arg(va, double);
		    lm_inf->ColWidths[i] = prtUnitX(this->Session, lm_inf->ColWidths[i]);
		    }
		}
	    else if (!strcmp(attrname, "colsep"))
		{
		/** Set separation between columns; default = 1.0 internal unit (0.1 inch) **/
		lm_inf->ColSep = va_arg(va, double);
		lm_inf->ColSep = prtUnitX(this->Session, lm_inf->ColSep);
		if (lm_inf->ColSep < 0.0)
		    {
		    nmFree(lm_inf, sizeof(PrtTabLMData));
		    this->LMData = NULL;
		    mssError(1,"TABLM","Column separation amount (colsep) must not be negative");
		    return -EINVAL;
		    }
		}
	    }

	/** Widths not yet set? **/
	if (lm_inf->ColWidths[0] < 0)
	    {
	    for(i=0;i<lm_inf->nColumns;i++)
		{
		lm_inf->ColWidths[i] = (this->Width - this->MarginLeft - this->MarginRight - (lm_inf->nColumns-1)*lm_inf->ColSep)/lm_inf->nColumns;
		}
	    }

	/** Check column width constraints **/
	totalwidth = lm_inf->ColSep*(lm_inf->nColumns-1);
	for(i=0;i<lm_inf->nColumns;i++)
	    {
	    if (lm_inf->ColWidths[i] <= 0.0 || lm_inf->ColWidths[i] > (this->Width - this->MarginLeft - this->MarginRight + 0.0001))
		{
		mssError(1,"TABLM","Invalid width for column #%d",i+1);
		nmFree(lm_inf, sizeof(PrtTabLMData));
		this->LMData = NULL;
		return -EINVAL;
		}
	    totalwidth += lm_inf->ColWidths[i];
	    }

	/** Check total of column widths. **/
	if (totalwidth > (this->Width - this->MarginLeft - this->MarginRight + 0.0001))
	    {
	    mssError(1,"TABLM","Total of column widths and separations exceeds available table width");
	    nmFree(lm_inf, sizeof(PrtTabLMData));
	    this->LMData = NULL;
	    return -EINVAL;
	    }

	/** Set column X positions **/
	totalwidth = 0.0;
	for(i=0;i<lm_inf->nColumns;i++)
	    {
	    lm_inf->ColX[i] = totalwidth;
	    totalwidth += lm_inf->ColWidths[i];
	    totalwidth += lm_inf->ColSep;
	    }

    return 0;
    }


/*** prt_tablm_InitRow() - initialize a table row object, which can be contained
 *** by a table, and can contain either cell objects or other types of objects
 *** such as an area, or even another table.
 ***/
int
prt_tablm_InitRow(pPrtObjStream row, va_list va)
    {
    return 0;
    }


/*** prt_tablm_InitCell() - initialize a table cell object, which can be contained
 *** only by table row objects and which can contain most anything.
 ***/
int
prt_tablm_InitCell(pPrtObjStream cell, va_list va)
    {
    return 0;
    }


/*** prt_tablm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.
 ***/
int
prt_tablm_InitContainer(pPrtObjStream this, va_list va)
    {

	/** Init which kind of container? **/
	switch (this->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_TABLE:
		return prt_tablm_InitTable(this, va);

	    case PRT_OBJ_T_TABLEROW:
		return prt_tablm_InitRow(this, va);

	    case PRT_OBJ_T_TABLECELL:
		return prt_tablm_InitCell(this, va);

	    default:
		mssError(1,"TABLM","Bark!  Object of type '%s' is not handled by this layout manager!", 
			this->ObjType->TypeName);
		return -EINVAL;
	    }

    return 0;
    }


/*** prt_tablm_DeinitContainer() - de-initialize a container using this
 *** layout manager.
 ***/
int
prt_tablm_DeinitContainer(pPrtObjStream this)
    {
    return 0;
    }


/*** prt_tablm_SetValue() - sets parameters unique to the tabular layout
 *** manager.  Used via prtSetValue().  Uses a varargs approach so that
 *** multiple values can be passed in, depending on the context.
 ***/
int
prt_tablm_SetValue(pPrtObjStream this, char* attrname, va_list va)
    {
    return -1;
    }


/*** prt_tablm_Finalize() - puts the finishing touches on a table just before
 *** the page is printed.  This mainly means adding the nice graphics for the
 *** table's borders and shadow now that the geometry of the table is stable.
 ***/
int
prt_tablm_Finalize(pPrtObjStream this)
    {
    return 0;
    }


/*** prt_tablm_Initialize() - initialize the textflow layout manager and
 *** register with the print management subsystem.
 ***/
int
prt_tablm_Initialize()
    {
    pPrtLayoutMgr lm;

	/** Allocate a layout manager structure **/
	lm = prtAllocLayoutMgr();
	if (!lm) return -1;

	/** Setup the structure **/
	lm->AddObject = prt_tablm_AddObject;
	lm->ChildResizeReq = prt_tablm_ChildResizeReq;
	lm->ChildResized = prt_tablm_ChildResized;
	lm->InitContainer = prt_tablm_InitContainer;
	lm->DeinitContainer = prt_tablm_DeinitContainer;
	lm->ChildBreakReq = prt_tablm_ChildBreakReq;
	lm->Break = prt_tablm_Break;
	lm->Resize = prt_tablm_Resize;
	lm->SetValue = prt_tablm_SetValue;
	lm->Reflow = NULL;
	lm->Finalize = prt_tablm_Finalize;
	strcpy(lm->Name, "tabular");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


