#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
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
/* Date:	December 12, 2001                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/*									*/
/*		prtmgmt_v3_lmtext.c:  This module implements the text-	*/
/*		flow layout manager.  The textflow layout manager	*/
/*		provides justification, word wrapping, image embedding,	*/
/*		and so forth.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_text.c,v 1.6 2002/10/22 04:12:56 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_text.c,v $

    $Log: prtmgmt_v3_lm_text.c,v $
    Revision 1.6  2002/10/22 04:12:56  gbeeley
    Added justification (left/center/right) support.  Full justification
    does not yet work.  Also, attempted a screen-based color text output
    mechanism which needs to be refined but unfortunately will not work
    on some/most/any pcl inkjets (tested: 870C) but may eventually work
    on lasers (tested: hp4550).  I will probably force the use of a
    postscript output driver if the user wants better color support; no
    real need to spend more time on it in the pcl output driver.  Reverted
    to palette-based color text support.

    Revision 1.5  2002/10/21 22:55:11  gbeeley
    Added font/size test in test_prt to test the alignment of different fonts
    and sizes on one line or on separate lines.  Fixed lots of bugs in the
    font baseline alignment logic.  Added prt_internal_Dump() to debug the
    document's structure.  Fixed a YSort bug where it was not sorting the
    YPrev/YNext pointers but the Prev/Next ones instead, and had a loop
    condition problem causing infinite looping as well.  Fixed some problems
    when adding an empty obj to a stream of objects and then modifying
    attributes which would change the object's geometry.

    There are still some glitches in the line spacing when different font
    sizes are used, however.

    Revision 1.4  2002/10/21 20:22:12  gbeeley
    Text foreground color attribute now basically operational.  Range of
    colors is limited however.  Tested on PCL output driver, on hp870c
    and hp4550 printers.  Also tested on an hp3si (black&white) to make
    sure the color pcl commands didn't garble things up there.  Use the
    "colors" test_prt command to test color output (and "output" to
    "/dev/lp0" if desired).

    Revision 1.3  2002/10/18 22:01:39  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.2  2002/04/25 04:30:14  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** prt_textlm_Break() - performs a break operation on this area, which 
 *** will pass the break operation on up to the parent container and so
 *** forth before creating an empty duplicate of this container in the new
 *** page.
 ***/
int
prt_textlm_Break(pPrtObjStream this, pPrtObjStream *new_this)
    {
    pPrtObjStream new_container = NULL;
    pPrtObjStream new_object = NULL;

	/** Does object not allow break operations? **/
	if (!(this->Flags & PRT_OBJ_F_ALLOWBREAK)) return -1;

	/** Object already has a continuing point? **/
	if (this->LinkNext)
	    {
	    *new_this = this->LinkNext;
	    }
	else
	    {
	    /** Request break from parent first. **/
	    if (this->Parent->LayoutMgr->ChildBreakReq(this->Parent, this, &new_container) < 0)
		{
		return -1;
		}

	    /** Duplicate the object... without the content. **/
	    new_object = prt_internal_AllocObjByID(this->ObjType->TypeID);
	    prt_internal_CopyAttrs(this, new_object);
	    prt_internal_CopyGeom(this, new_object);
	    new_object->Session = this->Session;
	    new_object->LayoutMgr->InitContainer(new_object);

	    /** Add the new object to the new parent container, and set the linkages **/
	    new_container->LayoutMgr->AddObject(new_container, new_object);
	    *new_this = new_object;
	    this->LinkNext = new_object;
	    new_object->LinkPrev = this;
	    }

	/** Update the handle so that later adds go to the correct place. **/
	prtUpdateHandleByPtr(this, *new_this);

    return 0;
    }


/*** prt_textlm_Resize() - request a resize on the current container object,
 *** which will require getting permission to do so from the parent and then
 *** notifying the parent once the resize has completed.
 ***/
int
prt_textlm_Resize(pPrtObjStream this, double new_width, double new_height)
    {
    double ow, oh;

	/** Can container be resized? **/
	if (this->Flags & PRT_OBJ_F_FIXEDSIZE) 
	    {
	    return -1;
	    }

	/** Get permission from this object's container. **/
	if (this->Parent->LayoutMgr->ChildResizeReq(this->Parent, this, new_width, new_height) < 0)
	    {
	    return -1;
	    }

	/** Do the resize. **/
	ow = this->Width;
	oh = this->Height;
	this->Width = new_width;
	this->Height = new_height;

	/** Notify parent container **/
	this->Parent->LayoutMgr->ChildResized(this->Parent, this, ow, oh);

    return 0;
    }


/*** prt_textlm_ChildBreakReq() - a child object is requesting a page break
 *** operation.  Right now, we ain't gonna support such thangs in this lm.
 ***/
int
prt_textlm_ChildBreakReq(pPrtObjStream this, pPrtObjStream child, pPrtObjStream *new_this)
    {
    return -1;
    }


/*** prt_textlm_SameLine() - determines whether two objects are on the
 *** same line of text by comparing Y and height with each other.  Returns
 *** 1 if same line, 0 if not.
 ***/
int
prt_textlm_SameLine(pPrtObjStream a, pPrtObjStream b)
    {
    if (a->Y >= b->Y && a->Y < b->Y + b->Height) return 1;
    if (b->Y >= a->Y && b->Y < a->Y + a->Height) return 1;
    return 0;
    }


/*** prt_textlm_LineGeom() - figure out how 'low' the lowest-descending
 *** object on the line of text is.  This is for determining how to space
 *** lines of text.  Also figure out how 'high' the highest-ascending object
 *** in the line is.
 ***/
int
prt_textlm_LineGeom(pPrtObjStream starting_point, double* bottom, double* top)
    {
    pPrtObjStream scan;

	*bottom = 0.0;
	*top = 9999999.0;

	/** Scan forwards first **/
	for(scan=starting_point; scan && !(scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)); scan=scan->Next)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND) && scan->Y + scan->Height > *bottom) 
	        *bottom = scan->Y + scan->Height;
	    if (scan->Y < *top) 
	        *top = scan->Y;
	    }

	/** Now scan backwards **/
	for(scan=starting_point; scan; scan=scan->Prev)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND) && scan->Y + scan->Height > *bottom) 
	        *bottom = scan->Y + scan->Height;
	    if (scan->Y < *top) 
	        *top = scan->Y;
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }

    return 0;
    }


/*** prt_textlm_UpdateLineY() - update the Y position of an entire line
 *** by offsetting it by a given amount.
 ***/
int
prt_textlm_UpdateLineY(pPrtObjStream starting_point, double y_offset)
    {
    pPrtObjStream scan;

	/** Scan forwards first **/
	for(scan=starting_point; scan && !(scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)); scan=scan->Next)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND)) 
		{
		scan->Y += y_offset;
		}
	    }

	/** Now scan backwards **/
	for(scan=starting_point->Prev; scan; scan=scan->Prev)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND)) 
		{
		scan->Y += y_offset;
		}
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }

    return 0;
    }


/*** prt_textlm_JustifyLine() - applies justification to a given line,
 *** keeping anything pre-positioned with xset, but changing
 *** the positioning of just about everything else.
 ***/
int
prt_textlm_JustifyLine(pPrtObjStream starting_point, int jtype)
    {
    pPrtObjStream scan, start, end;
    double slack_space, total_width, width_so_far;
    int n_items;

	/** Locate the beginning and end of the line **/
	for(scan=starting_point; scan; scan=scan->Prev)
	    {
	    start = scan;
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE | PRT_OBJ_F_XSET)) break;
	    }
	end = scan;
	for(scan=starting_point; scan; scan=scan->Next)
	    {
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE | PRT_OBJ_F_XSET)) break;
	    end = scan;
	    }

	/** Compute the amount of "slack space" in the region.  We need to count
	 ** everything because we may be re-justifying the line or whatever...
	 **/
	slack_space=0.0;
	n_items=0;
	total_width = end->Parent->Width - end->Parent->MarginLeft - end->Parent->MarginRight;
	for(scan=start; scan && scan->Next && scan != end; scan=scan->Next)
	    {
	    slack_space += (scan->Next->X - (scan->X + scan->Width));
	    n_items++;
	    }

	/** add in the space in the margins? **/
	if (!(start->Flags & PRT_OBJ_F_XSET))
	    slack_space += (start->X);
	if (!(end->Flags & PRT_OBJ_F_XSET))
	    slack_space += (total_width - (end->X + end->Width));

	/** Ok, apply the justification. **/
	width_so_far = 0.0;
	for(scan=start; scan; scan=scan->Next)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_XSET)) 
		{
		switch(jtype)
		    {
		    case PRT_JUST_T_LEFT:
			scan->X = width_so_far;
			break;
		    case PRT_JUST_T_RIGHT:
			scan->X = slack_space + width_so_far;
			break;
		    case PRT_JUST_T_CENTER:
			scan->X = (slack_space/2.0) + width_so_far;
			break;
		    case PRT_JUST_T_FULL:
			/** not yet implemented; we need to chop the 
			 ** pieces into components and get rid of the
			 ** spaces (sort of).  yuck. 
			 **/
			scan->X = width_so_far;
			break;
		    }
		width_so_far += scan->Width;
		}
	    else
		{
		width_so_far = scan->Width + scan->X;
		}
	    if (scan == end) break;
	    }

    return 0;
    }


/*** prt_textlm_ChildResizeReq() - this is called when a child object
 *** within this one is about to be resized.  This method gives this
 *** layout manager a chance to prevent the resize operation (return -1).  
 *** If the OK is given (return 0), a ChildResized method call will occur
 *** shortly thereafter (once the resize of the child has completed).
 ***/
int
prt_textlm_ChildResizeReq(pPrtObjStream this, pPrtObjStream child, double req_width, double req_height, pPrtObjStream *new_container)
    {
    /** For now, do not allow any resizing. **/
    return -1;
    }


/*** prt_textlm_ChildResized() - this function is called when a child
 *** object has actually been resized.  This will probably result in a 
 *** reflow being done for most or all of the container, which may cause
 *** a ResizeReq to be placed to the container of this container.
 ***/
int
prt_textlm_ChildResized(pPrtObjStream this, pPrtObjStream child, double old_width, double old_height)
    {
    return -1;
    }


/*** prt_textlm_AddObject() - used to add a new object to this one, using
 *** textflow layout semantics.  This can, in some cases, result in a
 *** resize request / resized event being placed to the layout manager
 *** that contains this object.
 ***
 *** Textflow child objects take on one of four forms, controlled by the
 *** presence/absence of two flags.  The PRT_OBJ_F_FLOWAROUND flag 
 *** determines whether the object helps set line height or if subsequent
 *** lines will "flow around" this (perhaps unusually tall) object.  Note
 *** that a forced newline (PRT_OBJ_F_NEWLINE) will skip below all objects
 *** in a line, whether flowaround-enabled or not.  The PRT_OBJ_F_NOLINESEQ
 *** flag causes the layout manager to ignore the exact position in the 
 *** text stream and then honor the given X coordinate instead of the exact
 *** sequence; otherwise the X coordinate is determined by the sequence of
 *** the object.
 ***
 *** Currently, FLOWAROUND and NOLINESEQ are not fully implemented.
 ***/
int
prt_textlm_AddObject(pPrtObjStream this, pPrtObjStream new_child_obj)
    {
    double top,bottom,w,maxw,lastw,ckw;
    /*double oldheight;*/
    int n,sl,last_sep;
    pPrtObjStream objptr;
    pPrtObjStream split_obj = NULL;
    /*unsigned char* spaceptr;*/
    pPrtObjStream new_parent;
    double x,y;

	/** Loop, adding one piece of the string at a time if it is wrappable **/
	new_child_obj->Justification = this->Justification;
	objptr = new_child_obj;
	while(1)
	    {
	    /** Get geometries for current line. **/
	    prt_textlm_LineGeom(this->ContentTail, &bottom, &top);

	    /** Determine X and Y for the new object. **/
	    if (objptr->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE))
		{
		/** Justify previous line? **/
		if (this->ContentTail)
		    {
		    prt_textlm_JustifyLine(this->ContentTail, this->ContentTail->Justification);
		    }
		objptr->X = 0.0;
		objptr->Y = bottom;
		}
	    else
		{
		/** Where will this go, by default?  X is pretty easy... **/
		x = this->ContentTail->X + this->ContentTail->Width;

		/** for the Y location, we have to look at the baseline for text. **/
		y = this->ContentTail->Y + this->ContentTail->YBase - objptr->YBase;
		if (y < top && !(objptr->Flags & PRT_OBJ_F_YSET))
		    {
		    prt_textlm_UpdateLineY(this->ContentTail, top - y);
		    y = top;
		    }

		/*if (objptr->Height > bottom - top)
		    y = top;
		else
		    y = bottom - objptr->Height;*/

		/** Set X position **/
		if (!(objptr->Flags & PRT_OBJ_F_XSET) || (objptr->X < x) || (objptr->X >= this->Width - this->MarginLeft - this->MarginRight))
		    {
		    objptr->X = x;
		    }

		/** Set Y position **/
		if (!(objptr->Flags & PRT_OBJ_F_YSET) || (objptr->Y < y))
		    {
		    objptr->Y = y;
		    }
		}

	    /** Need to break this into two parts to wrap it? **/
	    if (objptr->X + objptr->Width > this->Width - this->MarginLeft - this->MarginRight)
	        {
		if (objptr->ObjType->TypeID == PRT_OBJ_T_STRING)
		    {
		    /** String.  First, try to find a suitable break point. **/
		    w = 0.0;
		    maxw = this->Width - this->MarginLeft - this->MarginRight - objptr->X;
		    sl = strlen(objptr->Content);
		    last_sep = 0;
		    lastw = 0.0;
		    for(n=0;n<sl;n++)
			{
			/** Note a separation point (space, hyphen)? **/
			if (objptr->Content[n] == ' ' || (n > 0 && objptr->Content[n-1] == '-' && objptr->Content[n] >= 'A'))
			    {
			    last_sep = n;
			    lastw = w;
			    }

			/** Is that all that will fit? **/
			ckw = prt_internal_GetStringWidth(objptr, objptr->Content+n, 1);
			if (w + ckw > maxw)
			    {
			    /** Did we find a break point?  last_sep != 0 if so. **/
			    if (last_sep != 0)
				{
				split_obj = prt_internal_AllocObj("string");
				split_obj->Session = objptr->Session;
				split_obj->Justification = this->Justification;
				prt_internal_CopyAttrs(objptr, split_obj);
				if (objptr->Content[last_sep] == ' ')
				    split_obj->Content = nmSysStrdup(objptr->Content+last_sep+1);
				else
				    split_obj->Content = nmSysStrdup(objptr->Content+last_sep);
				objptr->Content[last_sep] = '\0';
				objptr->Width = lastw;
				split_obj->Height = objptr->Height;
				split_obj->YBase = objptr->YBase;
				split_obj->Width = prt_internal_GetStringWidth(split_obj, split_obj->Content, -1);
				split_obj->Flags |= PRT_OBJ_F_SOFTNEWLINE;
				}
			    else
			        {
				/** If the line is less than 50% full, split it, otherwise move 
				 ** it down to the next line.   maxw is *remaining* area on line.
				 ** Don't move to next line if X was manually set.
				 **/
				if (maxw/(this->Width - this->MarginLeft - this->MarginRight) < 0.50 && !(objptr->Flags & PRT_OBJ_F_XSET))
				    {
				    /** Move down to the next line. **/
				    objptr->X = 0.0;
				    objptr->Y = bottom;
				    objptr->Flags |= PRT_OBJ_F_SOFTNEWLINE;
				    }
				else
				    {
				    /** Split it. **/
				    split_obj = prt_internal_AllocObj("string");
				    split_obj->Session = objptr->Session;
				    split_obj->Justification = this->Justification;
				    prt_internal_CopyAttrs(objptr, split_obj);
				    split_obj->Content = nmSysStrdup(objptr->Content+n);
				    objptr->Content[n] = '\0';
				    objptr->Width = w;
				    split_obj->Height = objptr->Height;
				    split_obj->YBase = objptr->YBase;
				    split_obj->Width = prt_internal_GetStringWidth(split_obj, split_obj->Content, -1);
				    split_obj->Flags |= PRT_OBJ_F_SOFTNEWLINE;
				    }
				}
			    break; /* out of for() loop */
			    }
			w += ckw;
			}
		    }
		else
		    {
		    /** Non-string (e.g., image, area, etc.).  Cannot break into two. **/
		    if (objptr->X != 0.0)
		        {
			objptr->X = 0.0;
			objptr->Y = bottom;
			objptr->Flags |= PRT_OBJ_F_SOFTNEWLINE;
			}
		    split_obj = NULL;
		    }
		}

	    /** Special case of unsplit string here **/
	    if (objptr->ObjType->TypeID == PRT_OBJ_T_STRING && !split_obj && 
	        objptr->X == 0.0 && objptr->Width > this->Width - this->MarginLeft - this->MarginRight)
	        {
		/** It's on its own line now, so we can try splitting it again. **/
		continue;
		}

	    /** Ok, done any initial splitting or moving that was needed. **/
	    /** Add the objptr, and then see about adding split_obj if needed. **/
	    /** First, do we need to request more room in the 'area'? **/
	    new_parent = NULL;
	    if (objptr->Y + objptr->Height > this->Height - this->MarginTop - this->MarginBottom)
	        {
		/** Request the additional space **/
		if (this->LayoutMgr->Resize(this, this->Width, objptr->Y + objptr->Height + this->MarginTop + this->MarginBottom) < 0)
		    {
		    /** Resize denied.  If container is empty, we can't continue on, so error out here. **/
		    if (!this->ContentHead || (this->ContentHead->X + this->ContentHead->Width == 0.0 && !this->ContentHead->Next))
			{
			if (split_obj) 
			    {
			    if (split_obj->Content) nmSysFree(split_obj->Content);
			    prt_internal_FreeObj(split_obj);
			    }
			return -1;
			}

		    /** Try a break operation. **/
		    if (this->LayoutMgr->Break(this, &new_parent) < 0)
			{
			/** Break also denied?  Fail if so. **/
			if (split_obj) 
			    {
			    if (split_obj->Content) nmSysFree(split_obj->Content);
			    prt_internal_FreeObj(split_obj);
			    }
			return -1;
			}
		    }

		/** Bumped to a new container? **/
		if (new_parent && this != new_parent)
		    {
		    objptr->X = 0.0;
		    objptr->Y = 0.0;
		    this = new_parent;
		    if (split_obj) split_obj->Flags &= ~PRT_OBJ_F_SOFTNEWLINE;
		    continue;
		    }
		}

	    /** Now add the object to the container **/
	    prt_internal_Add(this, objptr);

	    /** No split_obj?  If not, we're done! **/
	    if (!split_obj) break;

	    /** Repeat the procedure for the split-off part of the object **/
	    objptr = split_obj;
	    split_obj = NULL;
	    }

    return 0;
    }


/*** prt_textlm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.  For this manager, it typically means adding
 *** an empty-string element to the container to give a starting point for
 *** font settings, line height, and so forth.
 ***/
int
prt_textlm_InitContainer(pPrtObjStream this)
    {
    pPrtObjStream first_obj;

	/** Allocate the initial object **/
	first_obj = prt_internal_AllocObj("string");
	prt_internal_CopyAttrs(this, first_obj);
	first_obj->Content = nmSysStrdup("");
	first_obj->X = 0.0;
	first_obj->Y = 0.0;
	first_obj->Width = 0.0;
	first_obj->Session = this->Session;
	first_obj->Height = this->LineHeight;
	first_obj->YBase = prt_internal_GetFontBaseline(first_obj);

	/** Add the initial object **/
	prt_internal_Add(this, first_obj);

    return 0;
    }


/*** prt_textlm_DeinitContainer() - de-initialize a container using this
 *** layout manager.  In this case, it does basically nothing.
 ***/
int
prt_textlm_DeinitContainer(pPrtObjStream this)
    {
    return 0;
    }


/*** prt_textlm_Initialize() - initialize the textflow layout manager and
 *** register with the print management subsystem.
 ***/
int
prt_textlm_Initialize()
    {
    pPrtLayoutMgr lm;

	/** Allocate a layout manager structure **/
	lm = prtAllocLayoutMgr();
	if (!lm) return -1;

	/** Setup the structure **/
	lm->AddObject = prt_textlm_AddObject;
	lm->ChildResizeReq = prt_textlm_ChildResizeReq;
	lm->ChildResized = prt_textlm_ChildResized;
	lm->InitContainer = prt_textlm_InitContainer;
	lm->DeinitContainer = prt_textlm_DeinitContainer;
	lm->ChildBreakReq = prt_textlm_ChildBreakReq;
	lm->Break = prt_textlm_Break;
	lm->Resize = prt_textlm_Resize;
	strcpy(lm->Name, "textflow");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


