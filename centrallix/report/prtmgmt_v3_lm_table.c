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

    $Id: prtmgmt_v3_lm_table.c,v 1.1 2003/03/02 04:17:35 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_table.c,v $

    $Log: prtmgmt_v3_lm_table.c,v $
    Revision 1.1  2003/03/02 04:17:35  gbeeley
    Adding skeleton tabular layout manager, for table/row/cell formatted
    data.


 **END-CVSDATA***********************************************************/


/*** table specific data ***/
typedef struct _PMC
    {
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


/*** prt_tablm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.
 ***/
int
prt_tablm_InitContainer(pPrtObjStream this, va_list va)
    {
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
	strcpy(lm->Name, "tabular");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


