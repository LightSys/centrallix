#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"

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
/* Date:	December 12, 2001                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/*									*/
/*		prtmgmt_v3_lmpage.c:  This module implements the page-	*/
/*		break (pagination) layout manager.  When an area	*/
/*		exceeds the page length, it is broken in two and a new	*/
/*		page is started for that area.				*/
/************************************************************************/



/*** prt_pagelm_Break() - this is called when we actually are going to do
 *** a page break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_pagelm_Break(pPrtObjStream this, pPrtObjStream *new_container)
    {
    pPrtObjStream next_page;

	/** Need to create the next page? **/
	if (!this->Next)
	    {
	    /** Can't do a page break? **/
	    if (!(this->Flags & PRT_OBJ_F_ALLOWBREAK)) return -1;

	    /** Build the new page. **/
	    /*next_page = prt_internal_AllocObj("page");
	    if (!next_page) return -1;
	    prt_internal_CopyAttrs(this, next_page);
	    prt_internal_CopyGeom(this, next_page);
	    next_page->Height = this->ConfigHeight;
	    next_page->Width = this->ConfigWidth;
	    next_page->Session = this->Session;
	    next_page->Flags = this->Flags;*/
	    next_page = prt_internal_Duplicate(this, 0);
	    prt_internal_Add(this->Parent, next_page);

	    /** Increment the page number **/
	    next_page->ObjID = this->ObjID + 1;
	    }
	else
	    {
	    next_page = this->Next;
	    }

	/** Can we emit the previous page? **/
	if (this->nOpens == 0)
	    {
	    if (prt_internal_GeneratePage(PRTSESSION(this), this) >= 0)
		{
		/** Bump the handle **/
		prtUpdateHandleByPtr((void*)this, (void*)next_page);

		/** Free the page data **/
		prt_internal_FreeTree(this);
		}
	    }

	/** Set the new container to the next page **/
	*new_container = next_page;

    return 0;
    }


/*** prt_pagelm_ChildBreakReq() - this is called when a child object
 *** actually requests a break, thus starting a chain of events that will
 *** create a new page, if it hasn't already been created.  This routine
 *** doesn't actually duplicate the child objects; that is done only once
 *** the respective objects need extension to the next page.
 ***/
int
prt_pagelm_ChildBreakReq(pPrtObjStream this, pPrtObjStream child, pPrtObjStream *new_this)
    {
    return this->LayoutMgr->Break(this, new_this);
    }


/*** prt_pagelm_ChildResizeReq() - this is called when a child object
 *** within this one is about to be resized.  This method gives this
 *** layout manager a chance to prevent the resize operation (return -1).  
 *** If the OK is given (return 0), a ChildResized method call will occur
 *** shortly thereafter (once the resize of the child has completed).
 ***
 *** GRB 10/23/02 - removed the logic to request a break if the resize
 *** could not be done.  The textlm code already does a break if the resize
 *** fails.
 ***/
int
prt_pagelm_ChildResizeReq(pPrtObjStream this, pPrtObjStream child, double req_width, double req_height)
    {

	/** Is the resize still within the bounds of the page?  Allow if so. **/
	if (child->Y + req_height - PRT_FP_FUDGE <= prtInnerHeight(this)) return 0;

    return -1;
    }


/*** prt_pagelm_ChildResized() - this function is called when a child
 *** object has actually been resized.
 ***/
int
prt_pagelm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {
    return 0;
    }


/*** prt_pagelm_Resize() - request to resize a page.  This only works if the
 *** page is empty.
 ***/
int
prt_pagelm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
	
	/** Page isn't empty? **/
	if (this->ContentHead) return -1;

	/** Set the values **/
	this->Width = new_width;
	this->Height = new_height;

    return 0;
    }


/*** prt_pagelm_AddObject() - used to add a new object to the page.  If the
 *** object is too big, it will end up being clipped by the formatting stage
 *** later on.
 ***/
int
prt_pagelm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {

	/** Need to adjust the height/width if unspecified? **/
	if (new_child_obj->Width < 0)
	    new_child_obj->Width = prtInnerWidth(this);
	if (new_child_obj->Height < 0)
	    new_child_obj->Height = prtInnerHeight(this);

	/** Sequence the objects on the page if Y not specified **/
	if (!(new_child_obj->Flags & PRT_OBJ_F_YSET))
	    {
	    new_child_obj->X = 0;
	    }
	if (!(new_child_obj->Flags & PRT_OBJ_F_YSET))
	    {
	    if (!this->ContentTail)
		new_child_obj->Y = 0;
	    else
		new_child_obj->Y = this->ContentTail->Y + this->ContentTail->Height;
	    }

	/** Just add it... **/
	prt_internal_Add(this, new_child_obj);

    return 0;
    }


/*** prt_pagelm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.  For this LM, nothing is done for a page
 *** object, but a document object is initialized by adding a single page 
 *** to it.
 ***/
int
prt_pagelm_InitContainer(pPrtObjStream this, void* old_lm_inf, va_list va)
    {
    pPrtObjStream page_obj;

	/** If this is a document, add a single page to it **/
	if (this->ObjType->TypeID == PRT_OBJ_T_DOCUMENT)
	    {
	    /** Allocate the initial object **/
	    page_obj = prt_internal_AllocObj("page");
	    prt_internal_CopyAttrs(this, page_obj);
	    page_obj->X = 0.0;
	    page_obj->Y = 0.0;
	    page_obj->Width = PRTSESSION(this)->PageWidth;
	    page_obj->Height = PRTSESSION(this)->PageHeight;
	    page_obj->ConfigWidth = PRTSESSION(this)->PageWidth;
	    page_obj->ConfigHeight = PRTSESSION(this)->PageHeight;
	    page_obj->Session = this->Session;
	    page_obj->Flags |= PRT_OBJ_F_ALLOWBREAK;
	    page_obj->ObjID = 1;

	    /** Add the initial object **/
	    prt_internal_Add(this, page_obj);
	    }

    return 0;
    }


/*** prt_pagelm_DeinitContainer() - de-initialize a container using this
 *** layout manager.  In this case, it does basically nothing.
 ***/
int
prt_pagelm_DeinitContainer(pPrtObjStream this)
    {
    return 0;
    }


/*** prt_pagelm_Initialize() - initialize the textflow layout manager and
 *** register with the print management subsystem.
 ***/
int
prt_pagelm_Initialize()
    {
    pPrtLayoutMgr lm;

	/** Allocate a layout manager structure **/
	lm = prtAllocLayoutMgr();
	if (!lm) return -1;

	/** Setup the structure **/
	lm->AddObject = prt_pagelm_AddObject;
	lm->ChildResizeReq = prt_pagelm_ChildResizeReq;
	lm->ChildResized = prt_pagelm_ChildResized;
	lm->InitContainer = prt_pagelm_InitContainer;
	lm->DeinitContainer = prt_pagelm_DeinitContainer;
	lm->ChildBreakReq = prt_pagelm_ChildBreakReq;
	lm->Break = prt_pagelm_Break;
	lm->Resize = prt_pagelm_Resize;
	lm->SetValue = NULL;
	lm->Reflow = NULL;
	lm->Finalize = NULL;
	strcpy(lm->Name, "paged");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


