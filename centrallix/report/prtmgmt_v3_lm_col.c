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
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
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
/* Date:	February 20, 2003                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/*									*/
/*		prtmgmt_v3_lm_col.c:  This module contains the logic	*/
/*		for multi-column formatting.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_col.c,v 1.1 2003/02/25 03:57:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_col.c,v $

    $Log: prtmgmt_v3_lm_col.c,v $
    Revision 1.1  2003/02/25 03:57:50  gbeeley
    Added incremental reflow capability and test in test_prt.  Added stub
    multi-column layout manager.  Reflow is horribly inefficient, but not
    worried about that at this point.


 **END-CVSDATA***********************************************************/


/*** multicolumn area specific data ***/
typedef struct _PMC
    {
    }
    PrtColLMData, *pPrtColLMData;


/*** prt_collm_Break() - this is called when we actually are going to do
 *** a break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_collm_Break(pPrtObjStream this, pPrtObjStream *new_container)
    {
    pPrtObjStream next_page;
    int page_handle_id;
    pPrtHandle h;

    return 0;
    }


/*** prt_collm_ChildBreakReq() - this is called when a child object
 *** actually requests a break, thus starting a chain of events that will
 *** create a new page, if it hasn't already been created.  This routine
 *** doesn't actually duplicate the child objects; that is done only once
 *** the respective objects need extension to the next page.
 ***/
int
prt_collm_ChildBreakReq(pPrtObjStream this, pPrtObjStream child, pPrtObjStream *new_this)
    {
    return this->LayoutMgr->Break(this, new_this);
    }


/*** prt_collm_ChildResizeReq() - this is called when a child object
 *** within this one is about to be resized.  This method gives this
 *** layout manager a chance to prevent the resize operation (return -1).  
 *** If the OK is given (return 0), a ChildResized method call will occur
 *** shortly thereafter (once the resize of the child has completed).
 ***/
int
prt_collm_ChildResizeReq(pPrtObjStream this, pPrtObjStream child, double req_width, double req_height)
    {
    return -1;
    }


/*** prt_collm_ChildResized() - this function is called when a child
 *** object has actually been resized.
 ***/
int
prt_collm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {
    return 0;
    }


/*** prt_collm_Resize() - request to resize a columnar layout.  This only works if the
 *** page is empty.
 ***/
int
prt_collm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
    return -1;
    }


/*** prt_collm_AddObject() - used to add a new object to the mcol section.  If the
 *** object is too big, it will end up being clipped by the formatting stage
 *** later on.
 ***/
int
prt_collm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {

	/** Just add it... **/
	prt_internal_Add(this, new_child_obj);

    return 0;
    }


/*** prt_collm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.  Nothing for now.
 ***/
int
prt_collm_InitContainer(pPrtObjStream this, va_list va)
    {
    pPrtObjStream page_obj;
    pPrtColLMData lm_inf;

	/** Allocate our layout manager specific info **/
	lm_inf = (pPrtColLMData)nmMalloc(sizeof(PrtColLMData));
	if (!lm_inf) return -ENOMEM;
	memset(lm_inf, 0, sizeof(PrtColLMData));
	this->LMData = lm_inf;

    return 0;
    }


/*** prt_collm_DeinitContainer() - de-initialize a container using this
 *** layout manager.  In this case, it does basically nothing.
 ***/
int
prt_collm_DeinitContainer(pPrtObjStream this)
    {

	/** Release the multicol-specific data **/
	if (this->LMData) nmFree(this->LMData, sizeof(PrtColLMData));
	this->LMData = NULL;

    return 0;
    }


/*** prt_collm_SetValue() - sets parameters unique to the columnar layout
 *** manager.  Used via prtSetValue().  Uses a varargs approach so that
 *** multiple values can be passed in, depending on the context.
 ***/
int
prt_collm_SetValue(pPrtObjStream this, char* attrname, va_list va)
    {
    return -1;
    }


/*** prt_collm_Initialize() - initialize the textflow layout manager and
 *** register with the print management subsystem.
 ***/
int
prt_collm_Initialize()
    {
    pPrtLayoutMgr lm;

	/** Allocate a layout manager structure **/
	lm = prtAllocLayoutMgr();
	if (!lm) return -1;

	/** Setup the structure **/
	lm->AddObject = prt_collm_AddObject;
	lm->ChildResizeReq = prt_collm_ChildResizeReq;
	lm->ChildResized = prt_collm_ChildResized;
	lm->InitContainer = prt_collm_InitContainer;
	lm->DeinitContainer = prt_collm_DeinitContainer;
	lm->ChildBreakReq = prt_collm_ChildBreakReq;
	lm->Break = prt_collm_Break;
	lm->Resize = prt_collm_Resize;
	lm->SetValue = prt_collm_SetValue;
	lm->Reflow = NULL;
	strcpy(lm->Name, "columnar");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


