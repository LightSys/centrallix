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

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_page.c,v 1.11 2005/03/01 07:12:32 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_page.c,v $

    $Log: prtmgmt_v3_lm_page.c,v $
    Revision 1.11  2005/03/01 07:12:32  gbeeley
    - rudimentary top-to-bottom layout flow for the page layout manager if
      x and/or y aren't supplied on an object.

    Revision 1.10  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.9  2003/07/09 18:10:02  gbeeley
    Further fixes and enhancements to prtmgmt layer, particularly regarding
    visual layout of graphical borders around objects; border/shadow
    thickness is now automatically computed into the total margin between
    exterior edges and interior edges of an object.

    Revision 1.8  2003/04/21 21:00:45  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

    Revision 1.7  2003/03/12 20:51:36  gbeeley
    Tables now working, but borders on tables not implemented yet.
    Completed the prt_internal_Duplicate routine and reworked the
    API interface to InitContainer on the layout managers.  Not all
    features/combinations on tables have been tested.  Footers on
    tables not working but (repeating) headers are.  Added a new
    prt obj stream field called "ContentSize" which provides the
    allocated memory size of the "Content" field.

    Revision 1.6  2003/03/06 02:52:35  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.5  2003/03/03 23:45:22  gbeeley
    Added support for multi-column formatting where columns are not equal
    in width.  Specifying width/height as negative when adding one object
    to another causes that object to fill its container in the respective
    dimension(s).  Fixed a bug in the Justification logic.

    Revision 1.4  2003/03/01 07:24:02  gbeeley
    Ok.  Balanced columns now working pretty well.  Algorithm is currently
    somewhat O(N^2) however, and is thus a bit expensive, but still not
    bad.  Some algorithmic improvements still possible with both word-
    wrapping and column balancing, but this is 'good enough' for the time
    being, I think ;)

    Revision 1.3  2003/02/25 03:57:50  gbeeley
    Added incremental reflow capability and test in test_prt.  Added stub
    multi-column layout manager.  Reflow is horribly inefficient, but not
    worried about that at this point.

    Revision 1.2  2003/02/19 22:53:54  gbeeley
    Page break now somewhat operational, both with hard breaks (form feeds)
    and with soft breaks (page wrapping).  Some bugs in how my printer (870c)
    places the text on pages after a soft break (but the PCL seems to look
    correct), and in how word wrapping is done just after a page break has
    occurred.  Use "printfile" command in test_prt to test this.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** prt_pagelm_Break() - this is called when we actually are going to do
 *** a page break.  Commonly this will be called from the ChildBreakReq
 *** function later on here.
 ***/
int
prt_pagelm_Break(pPrtObjStream this, pPrtObjStream *new_container)
    {
    pPrtObjStream next_page;
    int page_handle_id;
    pPrtHandle h;

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


