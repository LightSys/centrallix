#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_col.h"
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
/* Date:	February 20, 2003                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/*									*/
/*		prtmgmt_v3_lm_col.c:  This module contains the logic	*/
/*		for multi-column formatting.				*/
/************************************************************************/



int prt_collm_CreateCols(pPrtObjStream this);


/*** prt_collm_Break() - this is called when we actually are going to do
 *** a break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_collm_Break(pPrtObjStream this, pPrtObjStream *new_this)
    {
    pPrtObjStream new_object,new_container;
    pPrtColLMData lm_inf;
    pPrtObjStream parent;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Shouldn't be called on the main section object but rather on a 
	 ** child of that (a column object)!  Make sure first... 
	 **/
	if (this->ObjType->TypeID != PRT_OBJ_T_SECTCOL) return -1;
	parent = this->Parent;
	lm_inf = (pPrtColLMData)(parent->LMData);

	/** Just move on to the next column? **/
	if (this->Next)
	    {
	    *new_this = this->Next;
	    lm_inf->CurrentCol = this->Next;
	    return 0;
	    }

	/** Does object not allow break operations? **/
	if (!(parent->Flags & PRT_OBJ_F_ALLOWBREAK)) return -1;

	/** Object already has a continuing point? **/
	if (parent->LinkNext)
	    {
	    *new_this = parent->LinkNext->ContentHead;

	    /** Update the handle so that later adds go to the correct place. **/
	    prtUpdateHandleByPtr(this->Parent, (*new_this)->Parent);
	    }
	else
	    {
	    /** Duplicate the object... without the content... but with the
	     ** column child objects.
	     **/
	    /*new_object = prt_internal_AllocObjByID(parent->ObjType->TypeID);
	    prt_internal_CopyAttrs(parent, new_object);
	    prt_internal_CopyGeom(parent, new_object);
	    new_object->Height = parent->ConfigHeight;
	    new_object->Width = parent->ConfigWidth;
	    new_object->Session = parent->Session;
	    new_object->Flags = parent->Flags;*/

	    /** Allocate layout manager specific info **/
	    /*new_lm_inf = (pPrtColLMData)nmMalloc(sizeof(PrtColLMData));
	    if (!new_lm_inf) return -ENOMEM;
	    memcpy(new_lm_inf, lm_inf, sizeof(PrtColLMData));
	    new_object->LMData = new_lm_inf;
	    prt_collm_CreateCols(new_object);*/
	    new_object = prt_internal_Duplicate(parent,0);

	    /** Update the handle so that later adds go to the correct place. **/
	    prtUpdateHandleByPtr(parent, new_object);

	    /** Request break from parent, which may eject the page...
	     ** (which is why we copied our data from 'this' ahead of time) 
	     **/
	    if (parent->Parent->LayoutMgr->ChildBreakReq(parent->Parent, parent, &new_container) < 0)
		{
		/** Oops - put the handle back and get rid of new_object **/
		prtUpdateHandleByPtr(new_object, parent);
		prt_internal_FreeObj(new_object);
		return -1;
		}

	    /** Add the new object to the new parent container, and set the linkages **/
	    new_container->LayoutMgr->AddObject(new_container, new_object);
	    *new_this = new_object->ContentHead;

	    /** Was page ejected?  If LinkPrev on our container is set, then the page
	     ** is still valid.
	     **/
	    if (new_container->LinkPrev || (new_container->Parent && new_container->Parent->LinkPrev))
		{
		parent->LinkNext = new_object;
		new_object->LinkPrev = parent;
		}
	    }

    return 0;
    }


/*** prt_collm_ChildBreakReq() - this is called when a child object requests
 *** a break, which means we either spill to the next column in a multi column
 *** situation, or request a break from our parent, which often means moving
 *** to a new page.
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
    pPrtColLMData lm_inf;
    pPrtObjStream parent;
    double new_h;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Shouldn't be called on the main section object but rather on a 
	 ** child of that (a column object)!  Make sure first... 
	 **/
	if (this->ObjType->TypeID != PRT_OBJ_T_SECTCOL) return -1;
	parent = this->Parent;
	lm_inf = (pPrtColLMData)(parent->LMData);

	/** If this is just a width resize, allow it if it fits, otherwise
	 ** outright deny the resize altogether.
	 **/
	if (req_width != child->Width && req_height == child->Height)
	    {
	    if (child->X + req_width - PRT_FP_FUDGE <= prtInnerWidth(this)) 
		return 0;
	    else
		return -1;
	    }

	/** Question is whether to resize or to force a break.  This depends
	 ** on whether 'balanced' is enabled or not, on whether the child
	 ** object allows soft breaks, and on which column is being written
	 ** into.
	 **/
	if (!(lm_inf->Flags & PRT_COLLM_F_BALANCED) || !(child->Flags & PRT_OBJ_F_ALLOWSOFTBREAK))
	    {
	    /** Nonbalanced section, or a balanced section but the child can't
	     ** do soft column/page breaks, so we try to make it fit anyhow.
	     ** First check to see if it fits without needing any resizing.  
	     **/
	    if (child->Y + req_height - PRT_FP_FUDGE <= prtInnerHeight(this)) return 0;
	    
	    /** Next, try resizing the container. **/
	    new_h = child->Y + req_height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom;
	    if (!(parent->Flags & PRT_OBJ_F_FIXEDSIZE) && 
		    this->LayoutMgr->Resize(this,this->Width,new_h) >= 0)
		return 0;
	    }
	else  /* Balanced section. */
	    {
	    /** Like before, we allow if it fits without a resize.  This allows the
	     ** app to construct a section that only starts balancing when it needs
	     ** to expand vertically.
	     **/
	    if (child->Y + req_height - PRT_FP_FUDGE <= prtInnerHeight(this)) return 0;

	    /** If not in last column and can resize, and resize would not be more than
	     ** one LineHeight larger than last column, then go ahead.
	     **/
	    new_h = child->Y + req_height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom;
	    if (!(parent->Flags & PRT_OBJ_F_FIXEDSIZE) && this != parent->ContentTail &&
		    new_h - PRT_FP_FUDGE <= parent->ContentTail->Height &&
		    this->LayoutMgr->Resize(this,this->Width,new_h) >= 0)
		return 0;

	    /** If not in the last column, simply force a break to make things start
	     ** spreading out between the columns. 
	     **/
	    if (this != parent->ContentTail) return -1;

	    /** In last column.  Resize if we can, then a reflow will happen. **/
	    if (!(parent->Flags & PRT_OBJ_F_FIXEDSIZE) && 
		    this->LayoutMgr->Resize(this,this->Width,new_h) >= 0)
		return 0;
	    }

    return -1;
    }


/*** prt_collm_ChildResized() - this function is called when a child
 *** object has actually been resized.  This is where we actually force a
 *** reflow of the child object if we're operating in balanced mode and
 *** columns other than the last one changed height.
 ***/
int
prt_collm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {
    pPrtColLMData lm_inf;
    pPrtObjStream parent;
    pPrtObjStream reflow_obj;

	/** Shouldn't be called on the main section object but rather on a 
	 ** child of that (a column object)!  Make sure first... 
	 **/
	if (this->ObjType->TypeID != PRT_OBJ_T_SECTCOL) return -1;
	parent = this->Parent;
	lm_inf = (pPrtColLMData)(parent->LMData);

	/** If balanced mode and last column affected, do a reflow.  Start
	 ** the reflow from the *first* column.  Affect all child objects of
	 ** the first column.
	 **/
	if ((lm_inf->Flags & PRT_COLLM_F_BALANCED) && this == parent->ContentTail)
	    {
	    reflow_obj = parent->ContentHead->ContentHead;
	    while(reflow_obj)
		{
		prt_internal_ScheduleEvent(PRTSESSION(this), reflow_obj, PRT_EVENT_T_REFLOW, NULL);
		reflow_obj = reflow_obj->Next;
		}
	    }
	
    return 0;
    }


/*** prt_collm_Resize() - request to resize a columnar layout.
 ***/
int
prt_collm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
    int rval;
    double oh = 0, ow = 0;
    pPrtObjStream col_obj;
    double npw, nph;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Being called on a column rather than the section as a whole? If so,
	 ** reflect the call to the parent section object instead.
	 **/
	if (this->ObjType->TypeID == PRT_OBJ_T_SECTCOL)
	    {
	    rval = 0;
	    ow = this->Width;
	    npw = new_width - this->Width + this->Parent->Width;
	    nph = new_height + this->Y + this->Parent->MarginTop + this->Parent->MarginBottom;
	    if (nph < this->Parent->Height) nph = this->Parent->Height;
	    if (npw != this->Parent->Width || nph != this->Parent->Height)
		{
		rval = this->Parent->LayoutMgr->Resize(this->Parent, npw, nph);
		}

	    /** If width changed, we need to apply that here. **/
	    if (rval >= 0 && new_width != ow)
		{
		this->Width = new_width;
		for(col_obj=this->Next; col_obj; col_obj=col_obj->Next)
		    {
		    col_obj->X += (new_width - ow);
		    }
		}
            // TODO Initialize oh and ow before use
	    if (rval >= 0 && new_height != oh)
		{
		this->Height = new_height;
		}
	    return rval;
	    }

	/** Do a resize request from our parent container. **/
	if (this->Parent->LayoutMgr->ChildResizeReq(this->Parent, this, new_width, new_height) < 0)
	    return -1;

	/** Parent okayed the resize.  Go for it, and affect all column objects
	 ** within this section.
	 **/
	ow = this->Width;
	oh = this->Height;
	this->Width = new_width;
	this->Height = new_height;

	/** Notify parent that resize finished. **/
	this->Parent->LayoutMgr->ChildResized(this->Parent, this, ow, oh);

    return 0;
    }


/*** prt_collm_CreateCols() - create the column objects within the section
 *** object, based on the LMData set up.
 ***/
int
prt_collm_CreateCols(pPrtObjStream this)
    {
    pPrtColLMData lm_inf = (pPrtColLMData)(this->LMData);
    pPrtObjStream col_obj;
    int i;
    double totalwidth;

	/** Create the required number of column objects **/
	totalwidth = 0.0;
	for(i=0;i<lm_inf->nColumns;i++)
	    {
	    col_obj = prt_internal_AllocObjByID(PRT_OBJ_T_SECTCOL);
	    prt_internal_CopyAttrs(this, col_obj);
	    col_obj->X = totalwidth;
	    col_obj->Y = 0.0;
	    col_obj->Width = lm_inf->ColWidths[i];
	    col_obj->Session = this->Session;
	    col_obj->Height = prtInnerHeight(this);
	    col_obj->LMData = NULL;
	    col_obj->ObjID = i;
	    col_obj->Flags |= PRT_OBJ_F_PERMANENT;
	    totalwidth += (lm_inf->ColSep + lm_inf->ColWidths[i]);
	    prt_internal_Add(this, col_obj);
	    if (i > 0)
		{
		/** emulate link prev/next **/
		col_obj->LinkPrev = col_obj->Prev;
		col_obj->Prev->LinkNext = col_obj;
		}
	    }
	lm_inf->CurrentCol = this->ContentHead;

    return 0;
    }


/*** prt_collm_AddObject() - used to add a new object to the mcol section.  If the
 *** object is too big, it will end up being clipped by the formatting stage
 *** later on.  The hard work for the mcol module is mostly done in the ChildBreakReq
 *** and ChildResizeReq methods.
 ***/
int
prt_collm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {
    pPrtColLMData lm_inf = (pPrtColLMData)(this->LMData);

	/** Section not initialized? **/
	if (this->ObjType->TypeID == PRT_OBJ_T_SECTION && !this->ContentHead)
	    prt_collm_CreateCols(this);

	/** Called on child object? **/
	if (this->ObjType->TypeID == PRT_OBJ_T_SECTCOL)
	    lm_inf = this->Parent->LMData;

	/** Just makin' sure... **/
	if (!lm_inf) return -1;

	/** Need to adjust the height/width if unspecified? **/
	if (new_child_obj->Width < 0)
	    new_child_obj->Width = prtInnerWidth(lm_inf->CurrentCol);
	if (new_child_obj->Height < 0)
	    new_child_obj->Height = prtInnerHeight(lm_inf->CurrentCol);

	/** Just add it to the currently active column object **/
	prt_internal_Add(lm_inf->CurrentCol, new_child_obj);

    return 0;
    }


/*** prt_collm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.  This involves adding a subcontainer for each
 *** column that is configured.
 ***/
int
prt_collm_InitContainer(pPrtObjStream this, pPrtColLMData old_lm_inf, va_list va)
    {
    pPrtColLMData lm_inf;
    char* attrname;
    int i;
    double totalwidth;
    pPrtBorder b;

	/** section objects have a minimum height of one LineHeight. **/
	if (this->Height < this->LineHeight + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom) 
	    this->Height = this->LineHeight + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom;
	this->ConfigHeight = this->Height;

	/** Allocate our layout manager specific info **/
	lm_inf = (pPrtColLMData)nmMalloc(sizeof(PrtColLMData));
	if (!lm_inf) return -ENOMEM;
	memset(lm_inf, 0, sizeof(PrtColLMData));
	this->LMData = lm_inf;

	/** Params provided from another lm_inf? **/
	if (old_lm_inf)
	    {
	    memcpy(lm_inf, old_lm_inf, sizeof(PrtColLMData));
	    prt_collm_CreateCols(this);
	    return 0;
	    }

	/** Set up some defaults **/
	lm_inf->nColumns = PRT_COLLM_DEFAULT_COLS;
	lm_inf->ColWidths[0] = -1.0;
	lm_inf->ColSep = PRT_COLLM_DEFAULT_SEP;
	lm_inf->Flags = PRT_COLLM_DEFAULT_FLAGS;
	
	/** Look for layoutmanager-specific settings passed in. **/
	while(va && (attrname = va_arg(va, char*)) != NULL)
	    {
	    if (!strcmp(attrname, "numcols"))
		{
		/** set number of columns; default = 1 **/
		lm_inf->nColumns = va_arg(va, int);
		if (lm_inf->nColumns > PRT_COLLM_MAXCOLS || lm_inf->nColumns < 1)
		    {
		    nmFree(lm_inf, sizeof(PrtColLMData));
		    this->LMData = NULL;
		    mssError(1,"COLLM","Invalid number of columns (%d) for a section; max is %d", lm_inf->nColumns, PRT_COLLM_MAXCOLS);
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
		    nmFree(lm_inf, sizeof(PrtColLMData));
		    this->LMData = NULL;
		    mssError(1,"COLLM","Column separation amount must not be negative");
		    return -EINVAL;
		    }
		}
	    else if (!strcmp(attrname, "balanced"))
		{
		/** Whether or not the content is balanced between the columns, thus making
		 ** the section shorter but fuller.  This causes incremental reflows of the
		 ** section's content VERY OFTEN.
		 **/
		if (va_arg(va,int) != 0)
		    lm_inf->Flags |= PRT_COLLM_F_BALANCED;
		else
		    lm_inf->Flags &= ~PRT_COLLM_F_BALANCED;
		}
	    else if (!strcmp(attrname, "softflow"))
		{
		/** Whether or not content will automatically flow from one column to the
		 ** next without a break.  This must be enabled if 'balanced' is enabled.
		 **/
		if (va_arg(va,int) != 0)
		    lm_inf->Flags |= PRT_COLLM_F_SOFTFLOW;
		else
		    lm_inf->Flags &= ~PRT_COLLM_F_SOFTFLOW;
		}
	    else if (!strcmp(attrname, "separator"))
		{
		/** Width of separation line - 0.0 to turn it off **/
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->Separator), b, sizeof(PrtBorder));
		}
	    }

	/** Widths not yet set? **/
	if (lm_inf->ColWidths[0] < 0)
	    {
	    for(i=0;i<lm_inf->nColumns;i++)
		{
		lm_inf->ColWidths[i] = (prtInnerWidth(this) - (lm_inf->nColumns-1)*lm_inf->ColSep)/lm_inf->nColumns;
		}
	    }

	/** Integrity checks on parameters **/
	if (!(lm_inf->Flags & PRT_COLLM_F_SOFTFLOW) && (lm_inf->Flags & PRT_COLLM_F_BALANCED))
	    {
	    mssError(1,"COLLM","Section specifies 'balanced' but not 'softflow'");
	    nmFree(lm_inf, sizeof(PrtColLMData));
	    this->LMData = NULL;
	    return -EINVAL;
	    }
	totalwidth = lm_inf->ColSep*(lm_inf->nColumns-1);
	for(i=0;i<lm_inf->nColumns;i++)
	    {
	    /** Check column width constraints **/
	    if (lm_inf->ColWidths[i] <= 0.0 || lm_inf->ColWidths[i] > (prtInnerWidth(this) + PRT_FP_FUDGE))
		{
		mssError(1,"COLLM","Invalid width for column #%d",i+1);
		nmFree(lm_inf, sizeof(PrtColLMData));
		this->LMData = NULL;
		return -EINVAL;
		}
	    totalwidth += lm_inf->ColWidths[i];
	    }
	/** Check total of column widths. **/
	if (totalwidth > (prtInnerWidth(this) + PRT_FP_FUDGE))
	    {
	    mssError(1,"COLLM","Total of column widths and separations exceeds available section width");
	    nmFree(lm_inf, sizeof(PrtColLMData));
	    this->LMData = NULL;
	    return -EINVAL;
	    }

    return 0;
    }


/*** prt_collm_DeinitContainer() - de-initialize a container using this
 *** layout manager.  In this case, it does basically nothing.  We don't
 *** need to free the column objects since those get handled just fine
 *** all on their own via the standard FreeTree prtmgmt function.
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


/*** prt_collm_Finalize() - this is called when the object is just about to
 *** be printed, and gives the layout manager a chance to do some last 
 *** minute changes once the geometry has stabilized.  If the columns are
 *** configured to have a vertical separating line between them, then we
 *** add that here (a very thin rectangle)
 ***/
int
prt_collm_Finalize(pPrtObjStream this)
    {
    pPrtColLMData lm_inf = (pPrtColLMData)(this->LMData);
    pPrtObjStream col_obj;
    int i;

	/** Work only on the parent section object, not on individual column
	 ** objects, which don't have an LMData setting.
	 **/
	if (!lm_inf) return 0;

	/** Do we need the separator? **/
	if (lm_inf->Separator.nLines > 0)
	    {
	    col_obj = this->ContentHead;
	    for(i=0;i<lm_inf->nColumns-1 && col_obj && col_obj->ObjType->TypeID == PRT_OBJ_T_SECTCOL;i++)
		{
		/** Allocate the rectangle and set it up **/
		prt_internal_MakeBorder(this, col_obj->X + col_obj->Width + lm_inf->ColSep*0.5, 0.0,
			prtInnerHeight(this), PRT_MKBDR_F_LEFT | PRT_MKBDR_F_RIGHT,
			&(lm_inf->Separator), NULL, NULL);
		col_obj = col_obj->Next;
		}
	    }

    return 0;
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
	lm->Finalize = prt_collm_Finalize;
	strcpy(lm->Name, "columnar");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


