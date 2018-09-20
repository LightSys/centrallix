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
#include "prtmgmt_v3/prtmgmt_v3_lm_table.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"

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




/*** prt_tablm_Break() - this is called when we actually are going to do
 *** a break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_tablm_Break(pPrtObjStream this, pPrtObjStream *new_this)
    {
    pPrtObjStream new_parent, cur_parent, new_obj, search_obj;
    pPrtTabLMData lm_inf = (pPrtTabLMData)(this->LMData);
    pPrtTabLMData new_lm_inf;
    pPrtObjStream new_cells[PRT_TABLM_MAXCOLS];
    int n_cells = 0;
    int err,i;

	PRT_DEBUG("prt_tablm_Break() %s %8.8x\n", this->ObjType->TypeName, this);
	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Is this object allowed to break? **/
	if (!(this->Flags & PRT_OBJ_F_ALLOWBREAK)) return -1;

	/** Object already has a continuing point? **/
	if (this->LinkNext)
	    {
	    *new_this = this->LinkNext;
	    prtUpdateHandleByPtr(this, this->LinkNext);
	    return 0;
	    }

	/** If this is a cell, it only breaks with its row. **/
	switch(this->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_TABLECELL:
		/** For cells, we need to prepare to replicate all cells in the row, 
		 ** so that our new cell is in the right position in the new row
		 ** on the following page.
		 **/
		search_obj = this;
		while (search_obj->Prev) search_obj = search_obj->Prev;
		n_cells = 0;
		while (search_obj)
		    {
		    if (n_cells >= PRT_TABLM_MAXCOLS)
			break;
		    new_cells[n_cells] = prt_internal_Duplicate(search_obj, 0);
		    if (search_obj == this)
			new_obj = new_cells[n_cells];
		    search_obj->LinkNext = new_cells[n_cells];
		    new_cells[n_cells]->LinkPrev = search_obj;
		    n_cells++;
		    search_obj = search_obj->Next;
		    }
		cur_parent = this->Parent;
		prtUpdateHandleByPtr(this, new_obj);
		if (cur_parent->LayoutMgr->ChildBreakReq(cur_parent, this, &new_parent) < 0)
		    {
		    /** Row won't break, so neither will this cell. **/
		    prtUpdateHandleByPtr(new_obj, this);
		    for(i=0;i<n_cells;i++)
			{
			new_cells[i]->LinkPrev->LinkNext = NULL;
			new_obj->LayoutMgr->DeinitContainer(new_cells[i]);
			prt_internal_FreeObj(new_cells[i]);
			}
		    return -1;
		    }

		/** Add our duplicated (continuation) objects to the new page **/
		err = 0;
		for(i=0;i<n_cells;i++)
		    {
		    if (new_parent->LayoutMgr->AddObject(new_parent, new_cells[i]) < 0)
			{
			/** Argh.  We're in a bad state if we leave new_obj
			 ** dangling.  Lesser of evils is to force-add it.
			 **/
			prt_internal_Add(new_parent, new_cells[i]);
			err = 1;
			}
		    }
		if (err)
		    {
		    *new_this = new_obj;
		    return -1;
		    }
		break;

	    case PRT_OBJ_T_TABLEROW:
		new_obj = prt_internal_Duplicate(this,0);
		cur_parent = this->Parent;
		prtUpdateHandleByPtr(this, new_obj);
		if (cur_parent->LayoutMgr->ChildBreakReq(cur_parent, this, &new_parent) < 0)
		    {
		    /** Table won't break, so neither will this row. **/
		    prtUpdateHandleByPtr(new_obj, this);
		    new_obj->LayoutMgr->DeinitContainer(new_obj);
		    prt_internal_FreeObj(new_obj);
		    return -1;
		    }

		/** Add our duplicated (continuation) object to the new page **/
		if (new_parent->LayoutMgr->AddObject(new_parent, new_obj) < 0)
		    {
		    /** Argh.  We're in a bad state if we leave new_obj
		     ** dangling.  Lesser of evils is to force-add it.
		     **/
		    prt_internal_Add(new_parent, new_obj);
		    *new_this = new_obj;
		    return -1;
		    }
		break;

	    case PRT_OBJ_T_TABLE:
		/** Table.  We need to break it, but also include the header if there is one. **/
		new_obj = prt_internal_Duplicate(this,0);
		new_lm_inf = (pPrtTabLMData)(new_obj->LMData);
		cur_parent = this->Parent;
		prtUpdateHandleByPtr(this, new_obj);
		if (lm_inf->HeaderRow)
		    {
		    new_lm_inf->HeaderRow = prt_internal_Duplicate(lm_inf->HeaderRow, 1);
		    prt_internal_Add(new_obj, new_lm_inf->HeaderRow);
		    }
		if (cur_parent->LayoutMgr->ChildBreakReq(cur_parent, this, &new_parent) < 0)
		    {
		    /** Row won't break, so neither will the cell. **/
		    prtUpdateHandleByPtr(new_obj, this);
		    prt_internal_FreeTree(new_obj);
		    return -1;
		    }

		/** Add our duplicated (continuation) object to the new page **/
		if (new_parent->LayoutMgr->AddObject(new_parent, new_obj) < 0)
		    {
		    /** Argh.  We're in a bad state if we leave new_obj
		     ** dangling.  Lesser of evils is to force-add it.
		     **/
		    prt_internal_Add(new_parent, new_obj);
		    *new_this = new_obj;
		    return -1;
		    }
		break;

	    default:
		/** What kind of object is this? **/
		return -1;
	    }

	*new_this = new_obj;

	/** Link to previous/next containers if the page was not ejected yet **/
	if (new_parent->LinkPrev || (new_parent->Parent && new_parent->Parent->LinkPrev))
	    {
	    this->LinkNext = new_obj;
	    new_obj->LinkPrev = this;
	    }

    return 0;
    }


/*** prt_tablm_ChildBreakReq() - this is called when a child object requests
 *** a break.
 ***/
int
prt_tablm_ChildBreakReq(pPrtObjStream this, pPrtObjStream child, pPrtObjStream *new_this)
    {
	PRT_DEBUG("prt_tablm_ChildBreakReq() %s %8.8x %s %8.8x\n", this->ObjType->TypeName, this, child->ObjType->TypeName, child);
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
    pPrtObjStream table_obj, new_parent, old_parent = NULL;
    double new_height;

	PRT_DEBUG("prt_tablm_ChildResizeReq() %s %8.8x %s %8.8x\n", this->ObjType->TypeName, this, child->ObjType->TypeName, child);
	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Allow width resizes on a limited basis **/
	if ((child->ObjType->TypeID == PRT_OBJ_T_TABLE || child->ObjType->TypeID == PRT_OBJ_T_TABLEROW ||
		 child->ObjType->TypeID == PRT_OBJ_T_TABLECELL || 
		 child->X + req_width - PRT_FP_FUDGE > prtInnerWidth(this)) &&
		req_width != child->Width)
	    {
	    /** Wouldn't fit, or is a table/row/cell object and can't resize its width. **/
	    return -1;
	    }

	/** Height resize? **/
	if (req_height != child->Height)
	    {
	    /** Fits? **/
	    new_height = child->Y + req_height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom;
	    if (new_height <= this->Height)
		return 0;

	    /** Doesn't fit.  Try to resize ourselves. **/
	    if (this->LayoutMgr->Resize(this, this->Width, new_height) >= 0)
		{
		/** Resize succeeded. **/
		return 0;
		}

	    /** Ok, couldn't resize.  If we are in the header row, or in the first
	     ** data row, try moving the table to the next page altogether.
	     **/
	    if (this->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		{
		if (!this->Prev || (((pPrtTabLMData)(this->Prev->LMData))->Flags & PRT_TABLM_F_ISHEADER))
		    {
		    table_obj = this->Parent;
		    new_parent = old_parent;
		    old_parent = table_obj->Parent;
		    prt_internal_MakeOrphan(table_obj);

		    /** We use Break rather than ChildBreakReq here because we are asking the
		     ** parent to break, instead of requesting *permission* for ourselves to
		     ** break.  There's a difference :)
		     **/
		    if (old_parent->LayoutMgr->Break(old_parent, &new_parent) >= 0)
			{
			/** break succeeded, now re-add table to new parent. **/
			if (new_parent->LayoutMgr->AddObject(new_parent, table_obj) >= 0)
			    {
			    /** Object added - try to resize once again. **/
			    return this->LayoutMgr->Resize(this, this->Width, new_height);
			    }
			else
			    {
			    prt_internal_Add(new_parent, table_obj);
			    return -1;
			    }
			}

		    /** break failed, re-add to old container.  Bad news though.  If
		     ** the new_parent got set to NULL, we no longer have a old_parent
		     ** or a new_parent.  Exit without doing anything.
		     **/
		    prt_internal_Add(new_parent, table_obj);
		    /*if (new_parent == old_parent)
			old_parent->LayoutMgr->AddObject(old_parent, table_obj);*/
		    return -1;
		    }
		}

	    /** If we are inside a row, and row cannot break (or row is empty), 
	     ** break table and move entire row to next page.
	     **/
	    if (this->ObjType->TypeID == PRT_OBJ_T_TABLEROW && (!(this->Flags & PRT_OBJ_F_ALLOWBREAK) || this->Height == 0))
		{
		table_obj = this->Parent;
		prt_internal_MakeOrphan(this);
		new_parent = table_obj;
		if (table_obj->LayoutMgr->Break(table_obj, &new_parent) >= 0)
		    {
		    /** Break worked.  Re-add row to new table and attempt to resize. **/
		    if (new_parent->LayoutMgr->AddObject(new_parent, this) >= 0)
			{
			return this->LayoutMgr->Resize(this, this->Width, new_height);
			}
		    else
			{
			prt_internal_Add(new_parent, this);
			return -1;
			}
		    }

		/** break failed, re-add to old container.  Bad news though.  If
		 ** the new_parent got set to NULL, we no longer have a table_obj
		 ** or a new_parent.  Exit without doing anything.
		 **/
		prt_internal_Add(new_parent, this);
		/*if (new_parent == table_obj)
		    table_obj->LayoutMgr->AddObject(table_obj, this);*/
		return -1;
		}

	    /** Failed **/
	    return -1;
	    }

    return 0;
    }


/*** prt_tablm_ChildResized() - this function is called when a child
 *** object has actually been resized.
 ***/
int
prt_tablm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {

	/** If child was a row object, and not the last one, we need to request a reflow
	 ** here because the other rows need to be repositioned and one or more may no
	 ** longer fit on the page.  Bummer.
	 **/
	if (this->ObjType->TypeID == PRT_OBJ_T_TABLE && child != this->ContentTail)
	    {
	    prt_internal_ScheduleEvent(PRTSESSION(this), this, PRT_EVENT_T_REFLOW, NULL);
	    }

    return 0;
    }


/*** prt_tablm_Resize() - request to resize.
 ***/
int
prt_tablm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
    double ow, oh;
    pPrtObjStream peers;

	PRT_DEBUG("prt_tablm_Resize() %s %8.8x %.2lf %.2lf\n", this->ObjType->TypeName, this, new_width, new_height);
	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Can we resize? **/
	if (this->Flags & PRT_OBJ_F_FIXEDSIZE) return -1;

	/** Get permission from parent container **/
	if (this->Parent->LayoutMgr->ChildResizeReq(this->Parent, this, new_width, new_height) < 0)
	    {
	    /** Failed for some reason, probably because it wouldn't fit **/
	    return -1;
	    }

	/** Do the resize **/
	ow = this->Width;
	oh = this->Height;
	this->Width = new_width;
	this->Height = new_height;

	/** Notify parent **/
	this->Parent->LayoutMgr->ChildResized(this->Parent, this, ow, oh);

	/** Resize the rest of our peer cells, if this was a cell. **/
	if (this->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
	    {
	    peers = this->Parent->ContentHead;
	    while(peers)
		{
		if (peers != this) 
		    {
		    peers->Height = new_height;
		    }
		peers = peers->Next;
		}
	    }

    return 0;
    }


/*** prt_tablm_SetWidths() - sets up the widths of the columns within a table
 *** in general or within a particular row, based on the row's geometry and
 *** on the provided information.
 ***/
int
prt_tablm_SetWidths(pPrtObjStream this)
    {
    pPrtTabLMData lm_inf = (pPrtTabLMData)(this->LMData);
    double totalwidth;
    int i;

	/** Widths not yet set? **/
	if (lm_inf->ColWidths[0] < 0)
	    {
	    for(i=0;i<lm_inf->nColumns;i++)
		{
		lm_inf->ColWidths[i] = (prtInnerWidth(this) - (lm_inf->nColumns-1)*lm_inf->ColSep)/lm_inf->nColumns;
		}
	    }

	/** Check column width constraints **/
	totalwidth = lm_inf->ColSep*(lm_inf->nColumns-1);
	for(i=0;i<lm_inf->nColumns;i++)
	    {
	    if (lm_inf->ColWidths[i] <= 0.0 || lm_inf->ColWidths[i] > (prtInnerWidth(this) + PRT_FP_FUDGE))
		{
		mssError(1,"TABLM","Invalid width for column #%d",i+1);
		return -EINVAL;
		}
	    totalwidth += lm_inf->ColWidths[i];
	    }

	/** Check total of column widths.  If too wide, we always scale it back
	 ** to fit.  If too narrow, we only scale it to fill if AUTOWIDTH is
	 ** enabled.
	 **/
	if (totalwidth > (prtInnerWidth(this) + PRT_FP_FUDGE) || (lm_inf->Flags & PRT_TABLM_F_AUTOWIDTH))
	    {
	    for(i=0;i<lm_inf->nColumns;i++)
		{
		lm_inf->ColWidths[i] *= (prtInnerWidth(this)/totalwidth);
		}
	    lm_inf->ColSep *= (prtInnerWidth(this)/totalwidth);
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


/*** prt_tablm_AddObject() - used to add a new object to the table, row, or
 *** cell.  We need to mainly make sure that the added object is valid in its
 *** container, as well as make any final settings.  The object will have
 *** been initialized (via InitContainer), if appropriate, *before* this 
 *** routine is called.
 ***/
int
prt_tablm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {
    pPrtTabLMData lm_inf = (pPrtTabLMData)(new_child_obj->LMData);
    pPrtTabLMData parent_lm_inf = (pPrtTabLMData)(this->LMData);
    pPrtObjStream new_parent;
    int i,rval;

	/** Make sure the types are OK **/
	if (this->ObjType->TypeID == PRT_OBJ_T_TABLE && new_child_obj->ObjType->TypeID != PRT_OBJ_T_TABLEROW)
	    {
	    mssError(1,"TABLM","Tables may only contain table-row objects");
	    return -1;
	    }
	if (new_child_obj->ObjType->TypeID == PRT_OBJ_T_TABLEROW && this->ObjType->TypeID != PRT_OBJ_T_TABLE)
	    {
	    mssError(1,"TABLM","Table-row objects may only be contained within a table");
	    return -1;
	    }
	if (new_child_obj->ObjType->TypeID == PRT_OBJ_T_TABLECELL && this->ObjType->TypeID != PRT_OBJ_T_TABLEROW)
	    {
	    mssError(1,"TABLM","Table-cell objects may only be contained within a table-row");
	    return -1;
	    }

	/** Init the column widths? **/
	if (!this->ContentHead && this->ObjType->TypeID == PRT_OBJ_T_TABLE)
	    {
	    rval = prt_tablm_SetWidths(this);
	    if (rval < 0) return rval;
	    }

	/** Determine geometries and propagate config data. **/
	if (this->ObjType->TypeID == PRT_OBJ_T_TABLE)
	    {
	    /** parent is a table; child is a row object. **/
	    new_child_obj->X = 0.0;
	    new_child_obj->Y = (this->ContentTail)?(this->ContentTail->Y + this->ContentTail->Height):0.0;
	    new_child_obj->Width = prtInnerWidth(this);
	    if (new_child_obj->Height < 0) new_child_obj->Height = 0.0;
	    if (lm_inf->ColWidths[0] < 0.0)
		{
		lm_inf->nColumns = parent_lm_inf->nColumns;
		for(i=0;i<lm_inf->nColumns;i++)
		    {
		    lm_inf->ColWidths[i] = parent_lm_inf->ColWidths[i];
		    lm_inf->ColX[i] = parent_lm_inf->ColX[i];
		    }
		lm_inf->ColSep = parent_lm_inf->ColSep;
		}
	    if (this->ContentTail)
		new_child_obj->ObjID = this->ContentTail->ObjID+1;
	    else
		new_child_obj->ObjID = 0;

	    /** Check for a header/footer **/
	    if (lm_inf->Flags & PRT_TABLM_F_ISHEADER)
		parent_lm_inf->HeaderRow = new_child_obj;
	    else if (lm_inf->Flags & PRT_TABLM_F_ISFOOTER)
		parent_lm_inf->FooterRow = new_child_obj;
	    }
	else if (this->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
	    {
	    /** parent is a row object; child may be a cell, or may be something else. **/
	    if (new_child_obj->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		{
		/** Position the cell correctly within the row... **/
		if (this->ContentHead && this->ContentHead->ObjType->TypeID != PRT_OBJ_T_TABLECELL)
		    {
		    mssError(1,"TABLM","Cannot mix cell and non-cell objects in the same table-row");
		    return -1;
		    }
		if (parent_lm_inf->CurColID >= parent_lm_inf->nColumns)
		    {
		    mssError(1,"TABLM","Too many cells in table-row.  Maximum is %d.", parent_lm_inf->nColumns);
		    return -1;
		    }
		new_child_obj->X = parent_lm_inf->ColX[parent_lm_inf->CurColID];
		new_child_obj->Width = parent_lm_inf->ColWidths[parent_lm_inf->CurColID];
		for(i=1;i<lm_inf->ColSpan && i+parent_lm_inf->CurColID < parent_lm_inf->nColumns;i++)
		    {
		    new_child_obj->Width += parent_lm_inf->ColSep;
		    new_child_obj->Width += parent_lm_inf->ColWidths[i+parent_lm_inf->CurColID];
		    }
		if (new_child_obj->Height < 0)
		    new_child_obj->Height = prtInnerHeight(this);
		new_child_obj->Y = 0.0;
		new_child_obj->ObjID = parent_lm_inf->CurColID;
		parent_lm_inf->CurColID += lm_inf->ColSpan;
		if (parent_lm_inf->CurColID > parent_lm_inf->nColumns)
		    parent_lm_inf->CurColID = parent_lm_inf->nColumns;
		}
	    else
		{
		/** Position the object within the row... **/
		if (this->ContentHead && this->ContentHead->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
		    {
		    mssError(1,"TABLM","Cannot mix cell and non-cell objects in the same table-row");
		    return -1;
		    }
		if (new_child_obj->Width < 0) 
		    new_child_obj->Width = prtInnerWidth(this) - new_child_obj->X;
		if (new_child_obj->Height < 0) 
		    new_child_obj->Height = prtInnerHeight(this) - new_child_obj->Y;
		}
	    }
	else if (this->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
	    {
	    /** Parent is table cell object.  Pretty much anything goes here. **/
	    if (new_child_obj->Width < 0) 
		new_child_obj->Width = prtInnerWidth(this) - new_child_obj->X;
	    if (new_child_obj->Height < 0) 
		new_child_obj->Height = prtInnerHeight(this) - new_child_obj->Y;
	    }

	/** Ok, geometry now set.  Now see if the thing will actually fit. **/
	if ((new_child_obj->Width + new_child_obj->X - PRT_FP_FUDGE) > prtInnerWidth(this))
	    {
	    /** Too wide.  Not much we can do about this in a table. **/
	    mssError(1,"TABLM","Object too wide; extends off of right edge of table/row/cell.");
	    return -1;
	    }
	if ((new_child_obj->Height + new_child_obj->Y - PRT_FP_FUDGE) > prtInnerHeight(this))
	    {
	    /** Too tall.  Attempt to resize to meet the occasion. **/
	    new_parent = NULL;
	    if (this->LayoutMgr->Resize(this, this->Width, new_child_obj->Y + new_child_obj->Height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom) < 0)
		{
		/** Resize denied.  Try a break if we can. **/
		if (!(this->Flags & PRT_OBJ_F_ALLOWSOFTBREAK) || this->LayoutMgr->Break(this, &new_parent))
		    {
		    /** Break also denied???  Oops... **/
		    return -1;
		    }
		}

	    /** Bumped to new parent container? **/
	    if (new_parent && this != new_parent)
		{
		/** Point to new container **/
		this = new_parent;

		/** Re-check height sizing; new parent may need to be resized. **/
		if (new_child_obj->Height + new_child_obj->Y - PRT_FP_FUDGE > prtInnerHeight(this))
		    {
		    if (this->LayoutMgr->Resize(this, this->Width, new_child_obj->Y + new_child_obj->Height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom) < 0)
			{
			/** Could not fit even in new container? **/
			return -1;
			}
		    }
		}
	    }

	/** Ok, it should fit nicely now.  Go ahead and add it **/
	prt_internal_Add(this, new_child_obj);

    return 0;
    }


/*** prt_tablm_InitTable() - initializes a table container, which can contain
 *** table row objects but nothing else.
 ***/
int
prt_tablm_InitTable(pPrtObjStream this, pPrtTabLMData old_lm_data, va_list va)
    {
    pPrtTabLMData lm_inf = (pPrtTabLMData)(this->LMData);
    int i;
    char* attrname;
    pPrtBorder b;
    double* widths;

	/** Info already provided? **/
	if (old_lm_data)
	    {
	    memcpy(lm_inf, old_lm_data, sizeof(PrtTabLMData));
	    lm_inf->CurColID = 0;
	    return 0;
	    }

	/** Set up the defaults **/
	lm_inf->nColumns = 1;
	lm_inf->CurColID = 0;
	lm_inf->HeaderRow = NULL;
	lm_inf->FooterRow = NULL;
	lm_inf->Flags = PRT_TABLM_DEFAULT_FLAGS;
	lm_inf->ColSep = PRT_TABLM_DEFAULT_COLSEP;
	lm_inf->ColWidths[0] = -1.0;
	lm_inf->ShadowWidth = 0.0;

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
		widths= va_arg(va, double*);
		for(i=0;i<lm_inf->nColumns;i++)
		    {
		    lm_inf->ColWidths[i] = prtUnitX(this->Session, widths[i]);
		    }
		}
	    else if (!strcmp(attrname, "autowidth"))
		{
		if (va_arg(va, int) != 0)
		    lm_inf->Flags |= PRT_TABLM_F_AUTOWIDTH;
		else
		    lm_inf->Flags &= ~PRT_TABLM_F_AUTOWIDTH;
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
	    else if (!strcmp(attrname, "border") || !strcmp(attrname,"outerborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b)
		    {
		    memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		    if (!strcmp(attrname,"border"))
			{
			memcpy(&(lm_inf->InnerBorder), b, sizeof(PrtBorder));
			}
		    }
		}
	    else if (!strcmp(attrname, "shadow"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) 
		    {
		    memcpy(&(lm_inf->Shadow), b, sizeof(PrtBorder));
		    lm_inf->ShadowWidth = 0.0;
		    for(i=0;i<b->nLines;i++)
			{
			if (i > 0) lm_inf->ShadowWidth += b->Sep;
			lm_inf->ShadowWidth += (b->Width[i]);
			}
		    }
		}
	    else if (!strcmp(attrname, "topborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->TopBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "bottomborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->BottomBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "leftborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->LeftBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "rightborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->RightBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "innerborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->InnerBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->InnerBorder), 0, sizeof(PrtBorder));
		}
	    }

	/** Compute border widths for object **/
	this->BorderTop = lm_inf->TopBorder.TotalWidth;
	this->BorderLeft = lm_inf->LeftBorder.TotalWidth;
	this->BorderBottom = lm_inf->BottomBorder.TotalWidth + lm_inf->Shadow.TotalWidth;
	this->BorderRight = lm_inf->RightBorder.TotalWidth + lm_inf->Shadow.TotalWidth;

    return 0;
    }


/*** prt_tablm_InitRow() - initialize a table row object, which can be contained
 *** by a table, and can contain either cell objects or other types of objects
 *** such as an area, or even another table.
 ***/
int
prt_tablm_InitRow(pPrtObjStream row, pPrtTabLMData old_lm_data, va_list va)
    {
    pPrtTabLMData lm_inf = (pPrtTabLMData)(row->LMData);
    char* attrname;
    int i;
    pPrtBorder b;

	/** Info already provided? **/
	if (old_lm_data)
	    {
	    memcpy(lm_inf, old_lm_data, sizeof(PrtTabLMData));
	    lm_inf->CurColID = 0;
	    return 0;
	    }

	/** Set up the defaults **/
	lm_inf->nColumns = -1;
	lm_inf->CurColID = 0;
	lm_inf->HeaderRow = NULL;
	lm_inf->FooterRow = NULL;
	lm_inf->Flags = PRT_TABLM_DEFAULT_FLAGS;
	lm_inf->ColSep = PRT_TABLM_DEFAULT_COLSEP;
	lm_inf->ColWidths[0] = -1.0;

	/** Get params from the caller **/
	while(va && (attrname = va_arg(va, char*)) != NULL)
	    {
	    if (!strcmp(attrname, "header"))
		{
		if (va_arg(va, int) != 0)
		    lm_inf->Flags |= PRT_TABLM_F_ISHEADER;
		else
		    lm_inf->Flags &= ~PRT_TABLM_F_ISHEADER;
		}
	    else if (!strcmp(attrname, "footer"))
		{
		if (va_arg(va, int) != 0)
		    lm_inf->Flags |= PRT_TABLM_F_ISFOOTER;
		else
		    lm_inf->Flags &= ~PRT_TABLM_F_ISFOOTER;
		}
	    else if (!strcmp(attrname, "numcols"))
		{
		/** set number of columns; default = get data from parent table object **/
		lm_inf->nColumns = va_arg(va, int);
		if (lm_inf->nColumns > PRT_TABLM_MAXCOLS || lm_inf->nColumns < 1)
		    {
		    nmFree(lm_inf, sizeof(PrtTabLMData));
		    row->LMData = NULL;
		    mssError(1,"TABLM","Invalid number of columns (numcols=%d) for a table row; max is %d", lm_inf->nColumns, PRT_TABLM_MAXCOLS);
		    return -EINVAL;
		    }
		}
	    else if (!strcmp(attrname, "colwidths"))
		{
		/** Set widths of columns; default = 1 column and full width **/
		for(i=0;i<lm_inf->nColumns;i++)
		    {
		    lm_inf->ColWidths[i] = va_arg(va, double);
		    lm_inf->ColWidths[i] = prtUnitX(row->Session, lm_inf->ColWidths[i]);
		    }
		}
	    else if (!strcmp(attrname, "colsep"))
		{
		/** Set separation between columns; default = 1.0 internal unit (0.1 inch) **/
		lm_inf->ColSep = va_arg(va, double);
		lm_inf->ColSep = prtUnitX(row->Session, lm_inf->ColSep);
		if (lm_inf->ColSep < 0.0)
		    {
		    nmFree(lm_inf, sizeof(PrtTabLMData));
		    row->LMData = NULL;
		    mssError(1,"TABLM","Column separation amount (colsep) must not be negative");
		    return -EINVAL;
		    }
		}
	    else if (!strcmp(attrname, "border") || !strcmp(attrname, "outerborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b)
		    {
		    memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		    if (!strcmp(attrname,"border"))
			{
			memcpy(&(lm_inf->InnerBorder), b, sizeof(PrtBorder));
			}
		    }
		}
	    else if (!strcmp(attrname, "topborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->TopBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "bottomborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->BottomBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "leftborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->LeftBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "rightborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->RightBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "innerborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->InnerBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->InnerBorder), 0, sizeof(PrtBorder));
		}
	    }

	/** Compute border widths for object **/
	row->BorderTop = lm_inf->TopBorder.TotalWidth;
	row->BorderLeft = lm_inf->LeftBorder.TotalWidth;
	row->BorderBottom = lm_inf->BottomBorder.TotalWidth;
	row->BorderRight = lm_inf->RightBorder.TotalWidth;

    return 0;
    }


/*** prt_tablm_InitCell() - initialize a table cell object, which can be contained
 *** only by table row objects and which can contain most anything.
 ***/
int
prt_tablm_InitCell(pPrtObjStream cell, pPrtTabLMData old_lm_data, va_list va)
    {
    pPrtTabLMData lm_inf = (pPrtTabLMData)(cell->LMData);
    char* attrname;
    pPrtBorder b;
    int n;

	/** Info already provided? **/
	if (old_lm_data)
	    {
	    memcpy(lm_inf, old_lm_data, sizeof(PrtTabLMData));
	    return 0;
	    }

	/** Set up the defaults **/
	lm_inf->Flags = PRT_TABLM_DEFAULT_FLAGS;
	lm_inf->ColSep = PRT_TABLM_DEFAULT_COLSEP;
	lm_inf->ColSpan = 1;

	/** Get params from the caller **/
	while(va && (attrname = va_arg(va, char*)) != NULL)
	    {
	    if (!strcmp(attrname, "border"))
		{
		b = va_arg(va, pPrtBorder);
		if (b)
		    {
		    memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		    memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		    }
		}
	    else if (!strcmp(attrname, "topborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->TopBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->TopBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "bottomborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->BottomBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->BottomBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "leftborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->LeftBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->LeftBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "rightborder"))
		{
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->RightBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->RightBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "innerborder"))
		{
		lm_inf->Flags |= PRT_TABLM_F_INNEROUTER;
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->InnerBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->InnerBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "outerborder"))
		{
		lm_inf->Flags |= PRT_TABLM_F_INNEROUTER;
		b = va_arg(va, pPrtBorder);
		if (b) memcpy(&(lm_inf->OuterBorder), b, sizeof(PrtBorder));
		else memset(&(lm_inf->OuterBorder), 0, sizeof(PrtBorder));
		}
	    else if (!strcmp(attrname, "colspan"))
		{
		n = va_arg(va, int);
		if (n >= 1)
		    lm_inf->ColSpan = n;
		}
	    }

	/** Compute border widths for object **/
	cell->BorderTop = lm_inf->TopBorder.TotalWidth;
	cell->BorderLeft = lm_inf->LeftBorder.TotalWidth;
	cell->BorderBottom = lm_inf->BottomBorder.TotalWidth;
	cell->BorderRight = lm_inf->RightBorder.TotalWidth;

    return 0;
    }


/*** prt_tablm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.
 ***/
int
prt_tablm_InitContainer(pPrtObjStream this, void* lmdata, va_list va)
    {
    pPrtTabLMData lm_inf;
    pPrtTabLMData old_lm_inf = (pPrtTabLMData)lmdata;

	/** Allocate our lm-specific data **/
	lm_inf = (pPrtTabLMData)nmMalloc(sizeof(PrtTabLMData));
	if (!lm_inf) return -ENOMEM;
	this->LMData = (void*)lm_inf;
	memset(lm_inf, 0, sizeof(PrtTabLMData));

	/** Init which kind of container? **/
	switch (this->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_TABLE:
		return prt_tablm_InitTable(this, old_lm_inf, va);

	    case PRT_OBJ_T_TABLEROW:
		return prt_tablm_InitRow(this, old_lm_inf, va);

	    case PRT_OBJ_T_TABLECELL:
		return prt_tablm_InitCell(this, old_lm_inf, va);

	    default:
		mssError(1,"TABLM","Bark!  Object of type '%s' is not handled by this layout manager!", 
			this->ObjType->TypeName);
		nmFree(lm_inf, sizeof(PrtTabLMData));
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

	if (this->LMData) nmFree(this->LMData, sizeof(PrtTabLMData));
	this->LMData = NULL;

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
    pPrtTabLMData lm_inf = (pPrtTabLMData)(this->LMData);
    pPrtObjStream row, cell;
    pPrtTabLMData row_inf, cell_inf, adjrow_inf, adjcell_inf;
    pPrtBorder rowtop, rowbottom, rowleft, rowright;
    pPrtBorder celltop, cellbottom, cellleft, cellright;
    int collapse_row_border, collapse_cell_border;
    int is_first, is_last, is_first_row, is_last_row;
    int i;

	/** Only operate on table as a whole, because in many many cases
	 ** we will be combining borders together from cells, rows, table
	 ** edges, and adjacent cells/rows/etc. 
	 **/
	if (this->ObjType->TypeID != PRT_OBJ_T_TABLE) return 0;

	/** Draw the table's shadow **/
	if (lm_inf->Shadow.nLines > 0)
	    {
	    prt_internal_MakeBorder(this, lm_inf->ShadowWidth, this->Height,
		    this->Width - lm_inf->ShadowWidth, 
		    PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->Shadow), NULL, &(lm_inf->Shadow));
	    prt_internal_MakeBorder(this, this->Width, lm_inf->ShadowWidth,
		    this->Height - lm_inf->ShadowWidth, 
		    PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->Shadow), NULL, &(lm_inf->Shadow));
	    }

	/** Draw main table borders **/
	if (lm_inf->TopBorder.nLines > 0)
	    prt_internal_MakeBorder(this, 0.0, 0.0, this->Width - lm_inf->ShadowWidth,
		    PRT_MKBDR_F_TOP | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->TopBorder), &(lm_inf->LeftBorder), &(lm_inf->RightBorder));
	if (lm_inf->BottomBorder.nLines > 0)
	    prt_internal_MakeBorder(this, 0.0, this->Height - lm_inf->ShadowWidth*PRT_XY_CORRECTION_FACTOR, 
		    this->Width - lm_inf->ShadowWidth,
		    PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->BottomBorder), &(lm_inf->LeftBorder), &(lm_inf->RightBorder));
	if (lm_inf->LeftBorder.nLines > 0)
	    prt_internal_MakeBorder(this, 0.0, 0.0, this->Height - lm_inf->ShadowWidth*PRT_XY_CORRECTION_FACTOR,
		    PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->LeftBorder), &(lm_inf->TopBorder), &(lm_inf->BottomBorder));
	if (lm_inf->RightBorder.nLines > 0)
	    prt_internal_MakeBorder(this, this->Width - lm_inf->ShadowWidth, 0.0,
		    this->Height - lm_inf->ShadowWidth*PRT_XY_CORRECTION_FACTOR,
		    PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_MARGINRELEASE,
		    &(lm_inf->RightBorder), &(lm_inf->TopBorder), &(lm_inf->BottomBorder));

	/** Inner table borders (between columns) **/
	if (lm_inf->InnerBorder.nLines > 0)
	    {
	    for(i=0;i<lm_inf->nColumns-1;i++)
		{
		prt_internal_MakeBorder(this, this->MarginLeft + this->BorderLeft + lm_inf->ColX[i+1] - 0.5*lm_inf->ColSep, 0.0,
			this->Height - lm_inf->ShadowWidth*PRT_XY_CORRECTION_FACTOR,
			PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
			&(lm_inf->InnerBorder), &(lm_inf->TopBorder), &(lm_inf->BottomBorder));
		}
	    }

	/** Now draw borders for individual rows and cells **/
	row = this->ContentHead;
	while(row)
	    {
	    if (row->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		{
		/** Okay, a genuine row object.  Do borders for it.  We need to 
		 ** be careful to not emit a border if a border in that area was
		 ** already printed (such as left row border and a left table border
		 ** at the same time when the table's left margin was 0.0)
		 **/
		row_inf = (pPrtTabLMData)(row->LMData);
		collapse_row_border = 0;
		is_first_row = (row->Prev == NULL || row->Prev->ObjType->TypeID != PRT_OBJ_T_TABLEROW);
		is_last_row = (row->Next == NULL || row->Next->ObjType->TypeID != PRT_OBJ_T_TABLEROW);

		/** Figure out what other borders we overlap with **/
		if (!row->Prev)
		    {
		    if (lm_inf->TopBorder.nLines > 0 && this->MarginTop == 0.0)
			rowtop = &(lm_inf->TopBorder);
		    else
			rowtop = NULL;
		    }
		else if (row->Prev->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		    {
		    adjrow_inf = (pPrtTabLMData)(row->Prev->LMData);
		    if (adjrow_inf->BottomBorder.nLines >= row_inf->TopBorder.nLines)
			rowtop = &(adjrow_inf->BottomBorder);
		    else
			rowtop = NULL;
		    }
		else
		    {
		    rowtop = NULL;
		    }
		if (lm_inf->LeftBorder.nLines > 0 && this->MarginLeft == 0.0)
		    rowleft = &(lm_inf->LeftBorder);
		else
		    rowleft = NULL;
		if (lm_inf->RightBorder.nLines > 0 && this->MarginRight == 0.0)
		    rowright = &(lm_inf->RightBorder);
		else
		    rowright = NULL;
		if (!row->Next || row->Next->ObjType->TypeID == PRT_OBJ_T_RECT)
		    {
		    if (lm_inf->BottomBorder.nLines > 0 && this->MarginBottom <= lm_inf->ShadowWidth*PRT_XY_CORRECTION_FACTOR)
			rowbottom = &(lm_inf->BottomBorder);
		    else
			rowbottom = NULL;
		    }
		else if (row->Next->ObjType->TypeID == PRT_OBJ_T_TABLEROW)
		    {
		    adjrow_inf = (pPrtTabLMData)(row->Next->LMData);
		    if (adjrow_inf->TopBorder.nLines > row_inf->BottomBorder.nLines)
			rowbottom = &(adjrow_inf->TopBorder);
		    else
			rowbottom = NULL;
		    if (memcmp(&(adjrow_inf->TopBorder), &(row_inf->BottomBorder), sizeof (PrtBorder)) == 0)
			collapse_row_border = 1;
		    }
		else
		    {
		    rowbottom = NULL;
		    }

		/** Now that we know what other borders we're conflicting with, go
		 ** ahead and output the borders that we can...
		 **/
		if (!rowtop)
		    {
		    prt_internal_MakeBorder(row, 0.0, 0.0, row->Width,
			    PRT_MKBDR_F_TOP | PRT_MKBDR_F_MARGINRELEASE,
			    &(row_inf->TopBorder), rowleft?rowleft:&(row_inf->LeftBorder), 
			    rowright?rowright:&(row_inf->RightBorder));
		    }
		if (!rowbottom)
		    {
		    if (!collapse_row_border)
			prt_internal_MakeBorder(row, 0.0, row->Height, row->Width,
				PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_MARGINRELEASE,
				&(row_inf->BottomBorder), rowleft?rowleft:&(row_inf->LeftBorder), 
				rowright?rowright:&(row_inf->RightBorder));
		    else
			prt_internal_MakeBorder(row, 0.0, row->Height, row->Width,
				PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_TOP | PRT_MKBDR_F_MARGINRELEASE,
				&(row_inf->BottomBorder), rowleft?rowleft:&(row_inf->LeftBorder), 
				rowright?rowright:&(row_inf->RightBorder));
		    }
		if (!rowleft)
		    {
		    prt_internal_MakeBorder(row, 0.0, 0.0, row->Height,
			    PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
			    &(row_inf->LeftBorder), rowtop?rowtop:&(row_inf->TopBorder), 
			    rowbottom?rowbottom:&(row_inf->BottomBorder));
		    }
		if (!rowright)
		    {
		    prt_internal_MakeBorder(row, row->Width, 0.0, row->Height,
			    PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_MARGINRELEASE,
			    &(row_inf->RightBorder), rowtop?rowtop:&(row_inf->TopBorder), 
			    rowbottom?rowbottom:&(row_inf->BottomBorder));
		    }

		/** Inner borders between columns in the row? **/
		if (row_inf->InnerBorder.nLines > 0)
		    {
		    for(i=0;i<row_inf->nColumns-1;i++)
			{
			prt_internal_MakeBorder(row, row->BorderLeft + row->MarginLeft + row_inf->ColX[i+1] - 0.5*row_inf->ColSep, 0.0,
				row->Height,
				PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
				&(row_inf->InnerBorder), rowtop?rowtop:&(row_inf->TopBorder), 
				rowbottom?rowbottom:&(row_inf->BottomBorder));
			}
		    }

		/** Now do borders for the cells in the row **/
		cell = row->ContentHead;
		while(cell)
		    {
		    if (cell->ObjType->TypeID == PRT_OBJ_T_TABLECELL)
			{
			cell_inf = (pPrtTabLMData)(cell->LMData);
			is_first = (cell->Prev == NULL);
			is_last = (cell->Next == NULL || cell->Next->ObjType->TypeID == PRT_OBJ_T_RECT);
			collapse_cell_border = 0;

			/** Do inner/outer border settings if cell has that mode **/
			if (cell_inf->Flags & PRT_TABLM_F_INNEROUTER)
			    {
			    if (is_first_row)
				memcpy(&(cell_inf->TopBorder), &(cell_inf->OuterBorder), sizeof(PrtBorder));
			    else
				memcpy(&(cell_inf->TopBorder), &(cell_inf->InnerBorder), sizeof(PrtBorder));
			    if (is_last_row)
				memcpy(&(cell_inf->BottomBorder), &(cell_inf->OuterBorder), sizeof(PrtBorder));
			    else
				memcpy(&(cell_inf->BottomBorder), &(cell_inf->InnerBorder), sizeof(PrtBorder));
			    if (is_first)
				memcpy(&(cell_inf->LeftBorder), &(cell_inf->OuterBorder), sizeof(PrtBorder));
			    else
				memcpy(&(cell_inf->LeftBorder), &(cell_inf->InnerBorder), sizeof(PrtBorder));
			    if (is_last)
				memcpy(&(cell_inf->RightBorder), &(cell_inf->OuterBorder), sizeof(PrtBorder));
			    else
				memcpy(&(cell_inf->RightBorder), &(cell_inf->InnerBorder), sizeof(PrtBorder));
			    }

			/** Again, we first need to do elimination of borders already written **/
			if (row->MarginTop == 0.0 && rowtop)
			    celltop = rowtop;
			else if (row->MarginTop == 0.0 && row_inf->TopBorder.nLines > 0)
			    celltop = &(row_inf->TopBorder);
			else
			    celltop = NULL;
			if (row->MarginBottom == 0.0 && rowbottom)
			    cellbottom = rowbottom;
			else if (row->MarginBottom == 0.0 && row_inf->BottomBorder.nLines > 0)
			    cellbottom = &(row_inf->BottomBorder);
			else
			    cellbottom = NULL;
			if (is_first && row->MarginLeft == 0.0 && rowleft)
			    cellleft = rowleft;
			else if (is_first && row->MarginLeft == 0.0 && row_inf->LeftBorder.nLines > 0)
			    cellleft = &(row_inf->LeftBorder);
			else if (!is_first && row_inf->ColSep == 0.0)
			    {
			    adjcell_inf = (pPrtTabLMData)(cell->Prev->LMData);
			    if (adjcell_inf->RightBorder.nLines >= cell_inf->LeftBorder.nLines)
				cellleft = &(adjcell_inf->RightBorder);
			    else 
				cellleft = NULL;
			    }
			else
			    cellleft = NULL;
			if (is_last && row->MarginRight == 0.0 && rowright)
			    cellright = rowright;
			else if (is_last && row->MarginRight == 0.0 && row_inf->RightBorder.nLines > 0)
			    cellright = &(row_inf->RightBorder);
			else if (!is_last && row_inf->ColSep == 0.0)
			    {
			    adjcell_inf = (pPrtTabLMData)(cell->Next->LMData);
			    if (adjcell_inf->LeftBorder.nLines > cell_inf->RightBorder.nLines)
				cellright = &(adjcell_inf->LeftBorder);
			    else
				cellright = NULL;
			    if (adjcell_inf->LeftBorder.nLines == cell_inf->RightBorder.nLines)
				{
				collapse_cell_border = 1;
				cellright = NULL;
				}
			    }
			else
			    cellright = NULL;

			/** Ok, determined border interference.  Now draw the thing. **/
			if (!celltop)
			    {
			    prt_internal_MakeBorder(cell, 0.0, 0.0, cell->Width,
				    PRT_MKBDR_F_TOP | PRT_MKBDR_F_MARGINRELEASE,
				    &(cell_inf->TopBorder), cellleft?cellleft:&(cell_inf->LeftBorder), 
				    cellright?cellright:&(cell_inf->RightBorder));
			    }
			if (!cellbottom)
			    {
			    prt_internal_MakeBorder(cell, 0.0, cell->Height, cell->Width,
				    PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_MARGINRELEASE,
				    &(cell_inf->BottomBorder), cellleft?cellleft:&(cell_inf->LeftBorder), 
				    cellright?cellright:&(cell_inf->RightBorder));
			    }
			if (!cellleft)
			    {
			    prt_internal_MakeBorder(cell, 0.0, 0.0, cell->Height,
				    PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
				    &(cell_inf->LeftBorder), celltop?celltop:&(cell_inf->TopBorder), 
				    cellbottom?cellbottom:&(cell_inf->BottomBorder));
			    }
			if (!cellright)
			    {
			    if (!collapse_cell_border)
				prt_internal_MakeBorder(cell, cell->Width, 0.0, cell->Height,
					PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_MARGINRELEASE,
					&(cell_inf->RightBorder), celltop?celltop:&(cell_inf->TopBorder), 
					cellbottom?cellbottom:&(cell_inf->BottomBorder));
			    else
				prt_internal_MakeBorder(cell, cell->Width, 0.0, cell->Height,
					PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE,
					&(cell_inf->RightBorder), celltop?celltop:&(cell_inf->TopBorder), 
					cellbottom?cellbottom:&(cell_inf->BottomBorder));
			    }
			}
		    cell = cell->Next;
		    }
		}
	    row = row->Next;
	    }

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


