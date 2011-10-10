#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "centrallix.h"
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "prtmgmt_v3/prtmgmt_v3_lm_text.h"
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
/*		prtmgmt_v3_lmtext.c:  This module implements the text-	*/
/*		flow layout manager.  The textflow layout manager	*/
/*		provides justification, word wrapping, image embedding,	*/
/*		and so forth.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_text.c,v 1.24 2008/03/29 02:26:16 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_lm_text.c,v $

    $Log: prtmgmt_v3_lm_text.c,v $
    Revision 1.24  2008/03/29 02:26:16  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.23  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.22  2007/02/17 04:34:51  gbeeley
    - (bugfix) test_obj should open destination objects with O_TRUNC
    - (bugfix) prtmgmt should remember 'configured' line height, so it can
      auto-adjust height only if the line height is not explicitly set.
    - (change) report writer should assume some default margin settings on
      tables/table cells, so that tables aren't by default ugly :)
    - (bugfix) various floating point comparison fixes
    - (feature) allow top/bottom/left/right border options on the entire table
      itself in a report.
    - (feature) allow setting of text line height with "lineheight" attribute
    - (change) allow table to auto-scale columns should the total of column
      widths and separations exceed the available inner width of the table.
    - (feature) full justification of text.

    Revision 1.21  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.20  2003/07/09 18:10:02  gbeeley
    Further fixes and enhancements to prtmgmt layer, particularly regarding
    visual layout of graphical borders around objects; border/shadow
    thickness is now automatically computed into the total margin between
    exterior edges and interior edges of an object.

    Revision 1.19  2003/04/21 21:00:46  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

    Revision 1.18  2003/03/12 20:51:38  gbeeley
    Tables now working, but borders on tables not implemented yet.
    Completed the prt_internal_Duplicate routine and reworked the
    API interface to InitContainer on the layout managers.  Not all
    features/combinations on tables have been tested.  Footers on
    tables not working but (repeating) headers are.  Added a new
    prt obj stream field called "ContentSize" which provides the
    allocated memory size of the "Content" field.

    Revision 1.17  2003/03/07 06:16:12  gbeeley
    Added border-drawing functionality, and converted the multi-column
    layout manager to use that for column separators.  Added border
    capability to textareas.  Reworked the deinit/init kludge in the
    Reflow logic.

    Revision 1.16  2003/03/06 02:52:35  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.15  2003/03/03 23:45:22  gbeeley
    Added support for multi-column formatting where columns are not equal
    in width.  Specifying width/height as negative when adding one object
    to another causes that object to fill its container in the respective
    dimension(s).  Fixed a bug in the Justification logic.

    Revision 1.14  2003/03/01 07:24:02  gbeeley
    Ok.  Balanced columns now working pretty well.  Algorithm is currently
    somewhat O(N^2) however, and is thus a bit expensive, but still not
    bad.  Some algorithmic improvements still possible with both word-
    wrapping and column balancing, but this is 'good enough' for the time
    being, I think ;)

    Revision 1.13  2003/02/28 16:36:48  gbeeley
    Fixed most problems with balanced mode multi-column sections.  Still
    a couple of them remain and require some restructuring, so doing a
    commit first to be able to rollback in the event of trouble ;)

    Revision 1.12  2003/02/27 22:02:22  gbeeley
    Some improvements in the balanced multi-column output.  A lot of fixes
    in the multi-column output and in the text layout manager.  Added a
    facility to "schedule" reflows rather than having them take place
    immediately.

    Revision 1.11  2003/02/27 05:21:19  gbeeley
    Added multi-column layout manager functionality to support multi-column
    sections (this is newspaper-style multicolumn formatting).  Tested in
    test_prt "columns" command with various numbers of columns.  Balanced
    mode not yet working.

    Revision 1.10  2003/02/25 03:57:50  gbeeley
    Added incremental reflow capability and test in test_prt.  Added stub
    multi-column layout manager.  Reflow is horribly inefficient, but not
    worried about that at this point.

    Revision 1.9  2003/02/20 03:25:22  gbeeley
    Fixed a problem which inserted a blank line at the beginning of pages
    when soft page breaks would occur.

    Revision 1.8  2003/02/20 03:05:19  gbeeley
    Mostly fixed a wordwrap bug affecting situations where multiple
    prtWriteString calls were made, and the wrap point ended up being in the
    content of the first of the two calls.  This takes care of the common
    case.

    Revision 1.7  2003/02/19 22:53:54  gbeeley
    Page break now somewhat operational, both with hard breaks (form feeds)
    and with soft breaks (page wrapping).  Some bugs in how my printer (870c)
    places the text on pages after a soft break (but the PCL seems to look
    correct), and in how word wrapping is done just after a page break has
    occurred.  Use "printfile" command in test_prt to test this.

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


pPrtObjStream prt_textlm_SplitString(pPrtObjStream stringobj, int splitpt, int splitlen, double new_width);

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

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Does object not allow break operations? **/
	if (!(this->Flags & PRT_OBJ_F_ALLOWBREAK)) return -1;

	/** Object already has a continuing point? **/
	if (this->LinkNext)
	    {
	    *new_this = this->LinkNext;

	    /** Update the handle so that later adds go to the correct place. **/
	    prtUpdateHandleByPtr(this, *new_this);
	    }
	else
	    {
	    /** Duplicate the object... without the content. **/
	    new_object = prt_internal_AllocObjByID(this->ObjType->TypeID);
	    /*prt_internal_CopyAttrs(this, new_object);
	    prt_internal_CopyGeom(this, new_object);
	    new_object->Height = this->ConfigHeight;
	    new_object->Width = this->ConfigWidth;
	    new_object->Session = this->Session;
	    new_object->Flags = this->Flags;*/
	    new_object = prt_internal_Duplicate(this,0);

	    /** Init the new object. **/
	    /*new_object->LayoutMgr->InitContainer(new_object, this->LMData, NULL);*/

	    /** Update the handle so that later adds go to the correct place. **/
	    prtUpdateHandleByPtr(this, new_object);

	    /** Request break from parent, which may eject the page...
	     ** (which is why we copied our data from 'this' ahead of time) 
	     **/
	    if (this->Parent->LayoutMgr->ChildBreakReq(this->Parent, this, &new_container) < 0)
		{
		/** Oops - put the handle back and get rid of new_object **/
		prtUpdateHandleByPtr(new_object, this);
		new_object->LayoutMgr->DeinitContainer(new_object);
		prt_internal_FreeObj(new_object);

		return -1;
		}

	    /** Add the new object to the new parent container, and set the linkages **/
	    new_container->LayoutMgr->AddObject(new_container, new_object);
	    *new_this = new_object;

	    /** Was page ejected?  If LinkPrev on our container is set, then the page
	     ** is still valid.
	     **/
	    if (new_container->LinkPrev || (new_container->Parent && new_container->Parent->LinkPrev))
		{
		this->LinkNext = new_object;
		new_object->LinkPrev = this;
		}
	    }

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

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

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
	for(scan=starting_point; scan; scan=scan->Next)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND) && scan->Y + scan->Height > *bottom) 
	        *bottom = scan->Y + scan->Height;
	    if (scan->Y < *top) 
	        *top = scan->Y;
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }

	/** Now scan backwards **/
	for(scan=starting_point; scan; scan=scan->Prev)
	    {
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND) && scan->Y + scan->Height > *bottom) 
	        *bottom = scan->Y + scan->Height;
	    if (scan->Y < *top) 
	        *top = scan->Y;
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
	for(scan=starting_point; scan; scan=scan->Next)
	    {
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND)) 
		{
		scan->Y += y_offset;
		}
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }

	/** Now scan backwards **/
	for(scan=starting_point->Prev; scan; scan=scan->Prev)
	    {
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    if (!(scan->Flags & PRT_OBJ_F_FLOWAROUND)) 
		{
		scan->Y += y_offset;
		}
	    }

    return 0;
    }


/*** prt_textlm_Chopify() - splits a string object up into components,
 *** so that full justification can be done by adjusting component
 *** positioning.  Returns the last component in the created chain.
 ***/
pPrtObjStream
prt_textlm_Chopify(pPrtObjStream to_be_chopped)
    {
    pPrtObjStream end_of_chop = to_be_chopped;
    pPrtObjStream new_chunk;
    double space_width = prt_internal_GetStringWidth(to_be_chopped, " ", 1);
    char* spaceptr;

	/** Trim them off the initial string one word at a time **/
	while((spaceptr = strrchr((char*)to_be_chopped->Content, ' ')) != NULL)
	    {
	    new_chunk = prt_textlm_SplitString(to_be_chopped, spaceptr - (char*)to_be_chopped->Content, 0, -1);
	    prt_internal_Insert(to_be_chopped, new_chunk);
	    if (end_of_chop == to_be_chopped) end_of_chop = new_chunk;
	    new_chunk->Y = to_be_chopped->Y;
	    new_chunk->X = to_be_chopped->X + to_be_chopped->Width + space_width;
	    }

    return end_of_chop;
    }


/*** prt_textlm_JustifyLine() - applies justification to a given line,
 *** keeping anything pre-positioned with xset, but changing
 *** the positioning of just about everything else.
 ***/
int
prt_textlm_JustifyLine(pPrtObjStream starting_point, int jtype)
    {
    pPrtObjStream scan, start, end, endchop;
    double slack_space, total_width, width_so_far;
    int n_items, items_so_far, n_fj_items;;

	/** Locate the beginning and end of the line **/
	for(scan=starting_point; scan; scan=scan->Prev)
	    {
	    start = scan;
	    if (scan->Flags & PRT_OBJ_F_XSET) break;
	    if (scan->Prev && scan->Prev->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }
	end = scan;
	for(scan=starting_point; scan; scan=scan->Next)
	    {
	    end = scan;
	    if (scan->Next && scan->Next->Flags & PRT_OBJ_F_XSET) break;
	    if (scan->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) break;
	    }

	/** Use left justification on a newline'd line **/
	if (jtype == PRT_JUST_T_FULL && !(end->Flags & PRT_OBJ_F_SOFTNEWLINE))
	    jtype = PRT_JUST_T_LEFT;

	/** Does line end in a space?  Trim it if so **/
	if (end->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    while (*(end->Content) && end->Content[strlen((char*)end->Content)-1] == ' ')
		{
		end->Content[strlen((char*)end->Content)-1] = '\0';
		end->Width = prt_internal_GetStringWidth(end, (char*)end->Content, -1);
		end->Flags |= PRT_TEXTLM_F_RMSPACE;
		}
	    }

	/** Compute the amount of "slack space" in the region.  We need to count
	 ** everything because we may be re-justifying the line or whatever...
	 **/
	slack_space=0.0;
	n_items=0;
	n_fj_items = 0;
	total_width = prtInnerWidth(end->Parent);
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

	/** Chop into components? **/
	if (jtype == PRT_JUST_T_FULL)
	    {
	    for(scan=start; scan; scan=scan->Next)
		{
		if (scan->ObjType->TypeID == PRT_OBJ_T_STRING)
		    {
		    endchop = prt_textlm_Chopify(scan);
		    if (scan == end) end = endchop;
		    scan = endchop;
		    }
		if (scan == end) break;
		}
	    for(scan=start; scan && scan->Next && scan != end; scan=scan->Next)
		{
		if (scan->Width > PRT_FP_FUDGE)
		    n_fj_items++;
		}
	    }

	/** Ok, apply the justification. **/
	width_so_far = 0.0;
	items_so_far = 0;
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
			scan->X = width_so_far + (slack_space/n_fj_items)*items_so_far;
			if (scan->Width > PRT_FP_FUDGE)
			    {
			    items_so_far++;
			    }
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


/*** prt_textlm_BytesForChar() - determine the number of bytes the given
 *** character uses.  In UTF-8 mode, this is variable.  Otherwise, it is
 *** just 1 byte (we do not support UTF-16 or UCS-2).
 ***/
int
prt_textlm_BytesForChar(unsigned char* str)
    {
    int n;

	if (str[0] > 0x7F && CxGlobals.CharacterMode == CharModeUTF8)
	    {
	    n = 1;
	    while ((str[n] & 0xC0) == 0x80)
		n++;
	    }
	else
	    {
	    n = 1;
	    }

    return n;
    }


/*** prt_textlm_FindWrapPoint() - finds a suitable location within a given
 *** string object to break it in order to do word wrapping.  Basically,
 *** a hyphen or a space makes a wrap point.  This function will find the
 *** optimum wrap point without causing the line to exceed the given line
 *** width.  Returns -1 if no wrap point could be found, or 0 if a suitable
 *** point was found.  In either event indicates what the break point would
 *** be (if -1 is returned, there is no space/hyphen available).  If the whole
 *** thing fits, 1 is returned.
 ***/
int
prt_textlm_FindWrapPoint(pPrtObjStream stringobj, double maxwidth, int* brkpoint, double* brkwidth)
    {
    int sl,n,last_sep,sw;
    double lastw, w, ckw;
    int n_bytes;

	/** If empty, we can't wrap it!! **/
	if (stringobj->Content[0] == '\0') 
	    {
	    *brkpoint = 0;
	    *brkwidth = 0.0;
	    return -1;
	    }

	/** Start looking at beginning of string, computing width as we go **/
	w = 0.0;
	sl = strlen((char*)stringobj->Content);
	last_sep = -1;
	lastw = 0.0;
	n=0;
	while (n < sl)
	    {
	    /** Note a separation point (space, hyphen)? **/
	    if (stringobj->Content[n] == ' ' || (n > 1 && stringobj->Content[n-1] == '-' && stringobj->Content[n] >= 'A' && stringobj->Content[n-2] >= 'A'))
		{
		last_sep = n;
		lastw = w;
		}

	    /** Catch hyphens where the objects are already split **/
	    if (n == 0 && stringobj->Content[0] >= 'A' && stringobj->Prev && stringobj->Prev->ObjType->TypeID == PRT_OBJ_T_STRING)
		{
		sw = strlen((char*)stringobj->Prev->Content);
		if (sw > 1 && stringobj->Prev->Content[sw-1] == '-' && stringobj->Prev->Content[sw-2] >= 'A')
		    {
		    last_sep = n;
		    lastw = w;
		    }
		}

	    /** Is that all that will fit? **/
	    n_bytes = prt_textlm_BytesForChar(stringobj->Content+n);
	    ckw = prt_internal_GetStringWidth(stringobj, (char*)stringobj->Content+n, n_bytes);
	    if (w + ckw > maxwidth)
		{
		if (last_sep == -1)
		    {
		    *brkpoint = n;
		    *brkwidth = w;
		    return -1;
		    }
		else
		    {
		    *brkpoint = last_sep;
		    *brkwidth = lastw;
		    return 0;
		    }
		}
	    else
		{
		w += ckw;
		}

	    n += n_bytes;
	    }

	/** It all fits.  Return most reasonable split pt anyhow **/
	*brkpoint = last_sep;
	*brkwidth = lastw;

    return 1;
    }



/*** prt_textlm_SplitString() - splits a string into two string objects,
 *** creating a new string object which is returned.  The content of the
 *** original string object is modified to reflect the split.  Used mainly
 *** in word wrapping.  'splitpt' is the start point of where the split
 *** occurs, and 'splitlen' is the number of characters to actually omit
 *** from both strings (such as a space character).  'new_width' is a new
 *** width for the first string, set to < 0 to force this routine to re-
 *** compute that width.
 ***/
pPrtObjStream
prt_textlm_SplitString(pPrtObjStream stringobj, int splitpt, int splitlen, double new_width)
    {
    pPrtObjStream split_obj;
    int n;

	/** Create a new object for the second half of the string **/
	split_obj = prt_internal_AllocObj("string");
	split_obj->Session = stringobj->Session;
	split_obj->Justification = stringobj->Justification;
	prt_internal_CopyAttrs(stringobj, split_obj);

	/** Copy the second part of the content; leave room to add a space if needed 
	 ** later during a reflow 
	 **/
	n = strlen((char*)stringobj->Content+splitpt+splitlen);
	split_obj->Content = nmSysMalloc(n+2);
	split_obj->ContentSize = n+2;
	strcpy((char*)split_obj->Content, (char*)stringobj->Content+splitpt+splitlen);

	/** Truncate the first string to give the first part. **/
	stringobj->Content[splitpt] = '\0';
	if (new_width >= 0)
	    stringobj->Width = new_width;
	else
	    stringobj->Width = prt_internal_GetStringWidth(stringobj, (char*)stringobj->Content, -1);

	/** Transfer newlines or rmspace flags to the second half **/
	if (stringobj->Flags & PRT_TEXTLM_F_RMSPACE && n > 0)
	    {
	    split_obj->Flags |= PRT_TEXTLM_F_RMSPACE;
	    stringobj->Flags &= ~PRT_TEXTLM_F_RMSPACE;
	    }
	if (stringobj->Flags & PRT_OBJ_F_NEWLINE)
	    {
	    split_obj->Flags |= PRT_OBJ_F_NEWLINE;
	    stringobj->Flags &= ~PRT_OBJ_F_NEWLINE;
	    }
	if (stringobj->Flags & PRT_OBJ_F_SOFTNEWLINE)
	    {
	    split_obj->Flags |= PRT_OBJ_F_SOFTNEWLINE;
	    stringobj->Flags &= ~PRT_OBJ_F_SOFTNEWLINE;
	    }

	/** Set up the second half of the string's width etc **/
	if (splitlen == 1) stringobj->Flags |= PRT_TEXTLM_F_RMSPACE;
	split_obj->Height = stringobj->Height;
	split_obj->YBase = stringobj->YBase;
	split_obj->Width = prt_internal_GetStringWidth(split_obj, (char*)split_obj->Content, -1);

    return split_obj;
    }


/*** prt_textlm_WordWrap() - this does the word wrapping logic by finding
 *** a suitable point, given the current object and those already
 *** in the area's content, and then splitting the appropriate object
 *** up and returning the new split-off chain of objects.
 ***
 *** This function *may* modify the existing content of the area if
 *** it has to search backwards for the wrap point far enough.
 ***
 *** NOTE: the ->Prev pointer on the first returned list item will point to
 *** the last one in the list for convenience.  Be sure to appropriately
 *** account for that as needed.
 ***/
pPrtObjStream
prt_textlm_WordWrap(pPrtObjStream area, pPrtObjStream* curobj)
    {
    double maxw;
    pPrtObjStream search;
    pPrtObjStream splitlist=NULL;
    pPrtObjStream splitobj;
    pPrtObjStream oldcurobj;
    int rval,n;
    int sep,worstcasesep=-1;
    double sepw,worstcasesepw;

	/** First, temporarily add the curobj to the area's content **/
	oldcurobj = *curobj;
	prt_internal_Add(area, *curobj);

	/** Search for the appropriate point **/
	search = area->ContentTail;
	while(1)
	    {
	    /** Can we wrap in the selected object? **/
	    maxw = prtInnerWidth(area) - search->X;
	    rval = prt_textlm_FindWrapPoint(search, maxw, &sep, &sepw);

	    /** Could wrap? **/
	    if (rval == 0 || (rval == 1 && sep >= 0)) break;

	    /** If tail object, record worst case wrap point. **/
	    if (search == area->ContentTail)
		{
		worstcasesep = sep;
		worstcasesepw = sepw;
		}

	    /** Try previous objects. **/
	    if (search->Prev && !(search->Prev->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)) &&
		    search->Prev->ObjType->TypeID == PRT_OBJ_T_STRING && !(search->Flags & PRT_OBJ_F_XSET) &&
		    search->Y == search->Prev->Y)
		{
		search = search->Prev;
		}
	    else
		{
		search = area->ContentTail;
		sep = worstcasesep;
		sepw = worstcasesepw;
		break;
		}
	    }

	/** Ok, found a wrap point.  Split the thing?  At the end of this
	 ** stuff, the 'search' pointer should point to the object beginning
	 ** the new line.
	 **/
	n = strlen((char*)search->Content);
	if (sep == 0 && search->Content[0] != ' ')
	    {
	    /** No good split point; split must occur with this object. **/
	    assert(search->Prev != NULL);
	    search->Prev->Flags |= (PRT_OBJ_F_SOFTNEWLINE);
	    splitobj = NULL;
	    }
	else if (sep == 0)
	    {
	    /** object begins w/ a space - no split, just remove the space
	     ** and get on with it.  Soft newline and space removal indicators
	     ** belong to previous object, for sanity's sake.
	     **/
	    memmove(search->Content, search->Content+1, n);
	    search->Width -= prt_internal_GetStringWidth(search, " ", 1);
	    assert(search->Prev != NULL);
	    search->Prev->Flags |= (PRT_TEXTLM_F_RMSPACE | PRT_OBJ_F_SOFTNEWLINE);
	    splitobj = NULL;
	    }
	else if (sep == n-1 && search->Content[sep] == ' ')
	    {
	    /** removing a single space at the end.  Again, no split, just
	     ** remove the space and get on with things.
	     **/
	    search->Content[n-1] = '\0';
	    search->Width -= prt_internal_GetStringWidth(search, " ", 1);
	    search->Flags |= (PRT_TEXTLM_F_RMSPACE | PRT_OBJ_F_SOFTNEWLINE);
	    search = search->Next;
	    splitobj = NULL;
	    }
	else
	    {
	    /** Need to split the object. **/
	    splitobj = prt_textlm_SplitString(search, sep, (search->Content[sep]==' ')?1:0, sepw);
	    splitobj->Justification = area->Justification;
	    search->Flags |= PRT_OBJ_F_SOFTNEWLINE;
	    splitobj->Prev = search;
	    splitobj->Next = search->Next;
	    search->Next = splitobj;
	    if (splitobj->Next) splitobj->Next->Prev = splitobj;
	    search = splitobj;
	    }

	/** Grab the list of split-off objects starting at 'search'.  We
	 ** may need to go one further back to grab curobj if it is still
	 ** there.
	 **/
	if (search)
	    {
	    splitlist = search;
	    }
	else
	    {
	    splitlist = *curobj;
	    }
	if (splitlist->Prev == *curobj) splitlist=splitlist->Prev;
	area->ContentTail = splitlist->Prev;
	splitlist->Prev->Next = NULL;
	splitlist->Prev = NULL;
	splitlist->Parent = NULL;

	/** Get ourselves a 'curobj' again. **/
	*curobj = splitlist;
	splitlist = splitlist->Next;
	(*curobj)->Next = NULL;
	(*curobj)->Prev = NULL;

	/** Set up the tail pointer on the splitlist **/
	if (splitlist)
	    {
	    if (!(splitlist->Next)) 
		splitlist->Prev = splitlist;
	    else
		splitlist->Prev = oldcurobj;
	    }

    return splitlist;
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


/*** prt_textlm_UndoWrap() - undoes the effects of word wrapping on two
 *** given objects by restoring the space to one of them as needed.
 ***/
int
prt_textlm_UndoWrap(pPrtObjStream obj)
    {
    int n;

	/** We can just use the extra byte in the string since we made sure
	 ** anything with an RMSPACE flag has extra space allocated, either
	 ** inherently or because we forced it.
	 **/
	if (obj->Flags & PRT_TEXTLM_F_RMSPACE)
	    {
	    n = strlen((char*)obj->Content);
	    obj->Content[n] = ' ';
	    obj->Content[n+1] = '\0';
	    obj->Flags &= ~PRT_TEXTLM_F_RMSPACE;
	    obj->Width += prt_internal_GetStringWidth(obj, " ", 1);
	    }

	/** Remove newline indication **/
	obj->Flags &= ~PRT_OBJ_F_SOFTNEWLINE;

    return 0;
    }


/*** prt_textlm_GetSplitObj() - get a 'split object' from the stack of objects
 *** being stored temporarily.
 ***/
pPrtObjStream
prt_textlm_GetSplitObj(pPrtObjStream* split_obj_list)
    {
    pPrtObjStream tmpobj;

	tmpobj = *split_obj_list;
	if (tmpobj) 
	    {
	    *split_obj_list = tmpobj->Next;
	    tmpobj->Next = NULL;
	    tmpobj->Prev = NULL;
	    }

    return tmpobj;
    }

/*** prt_textlm_Setup() - adds an empty object to a freshly created area
 *** container to be the 'start' of the content.
 ***/
int
prt_textlm_Setup(pPrtObjStream this)
    {
    pPrtObjStream first_obj;

	/** Allocate the initial object **/
	first_obj = prt_internal_AllocObj("string");
	prt_internal_CopyAttrs(this, first_obj);
	first_obj->Content = nmSysMalloc(2);
	first_obj->ContentSize = 2;
	first_obj->Content[0] = '\0';
	first_obj->X = 0.0;
	first_obj->Y = 0.0;
	first_obj->Width = 0.0;
	first_obj->Session = this->Session;
	first_obj->Height = this->LineHeight;
	first_obj->YBase = prt_internal_GetFontBaseline(first_obj);
	first_obj->Flags |= PRT_OBJ_F_PERMANENT;

	/** Add the initial object **/
	prt_internal_Add(this, first_obj);

    return 0;
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
    double top,bottom;
    pPrtObjStream objptr;
    pPrtObjStream split_obj = NULL;
    pPrtObjStream split_obj_list = NULL;
    pPrtObjStream new_parent;
    double x,y;
    int handle_id;

	/** Need to adjust the height/width if unspecified? **/
	if (new_child_obj->Width < 0)
	    new_child_obj->Width = prtInnerWidth(this);
	if (new_child_obj->Height < 0)
	    new_child_obj->Height = prtInnerHeight(this);

	/** Space removed from object previously (e.g., linewrap)? **/
	prt_textlm_UndoWrap(new_child_obj);

	/** Loop, adding one piece of the string at a time if it is wrappable **/
	new_child_obj->Justification = this->Justification;
	objptr = new_child_obj;
	while(objptr)
	    {
	    /** Run any pending events.  We need to save the handle, because the
	     ** actual object being written into may change as a result of
	     ** a reflow occurring, and so 'this' might be different :)
	     **/
	    handle_id = prtLookupHandle(this);
	    prt_internal_DispatchEvents(PRTSESSION(this));
	    this = prtHandlePtr(handle_id);

	    /** Get geometries for current line. **/
	    prt_textlm_LineGeom(this->ContentTail, &bottom, &top);

	    /** Determine X and Y for the new object.  Skip to next line if newline
	     ** is indicated.
	     **/
	    if ((this->ContentTail && this->ContentTail->Flags & (PRT_OBJ_F_NEWLINE | PRT_OBJ_F_SOFTNEWLINE)))
		{
		/** Justify previous line? **/
		prt_textlm_JustifyLine(this->ContentTail, this->ContentTail->Justification);
		objptr->X = 0.0;
		if (this->ContentTail->LineHeight > bottom - top)
		    objptr->Y = top + this->ContentTail->LineHeight;
		else
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

		/** Set X position **/
		if (!(objptr->Flags & PRT_OBJ_F_XSET) || (objptr->X < x) || (objptr->X >= prtInnerWidth(this)))
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
	    if (objptr->X + objptr->Width - PRT_FP_FUDGE > prtInnerWidth(this))
	        {
		if (objptr->ObjType->TypeID == PRT_OBJ_T_STRING)
		    {
		    split_obj = prt_textlm_WordWrap(this,&objptr);

		    /** If no split obj, then we made it fit without a split **/
		    if (split_obj)
			{
			/** Prev refers to the tail in this case **/
			split_obj->Prev->Next = split_obj_list;
			split_obj->Prev = NULL;
			split_obj_list = split_obj;
			}
		    if (this->ContentTail->Flags & PRT_OBJ_F_SOFTNEWLINE)
			{
			objptr->X = 0.0;
			if (this->ContentTail->LineHeight > bottom - top)
			    objptr->Y = top + this->ContentTail->LineHeight;
			else
			    objptr->Y = bottom;
			}
		    }
		else
		    {
		    /** Non-string (e.g., image, area, etc.).  Cannot break into two. **/
		    if (objptr->X != 0.0)
		        {
			objptr->X = 0.0;
			if (this->ContentTail->LineHeight > bottom - top)
			    objptr->Y = top + this->ContentTail->LineHeight;
			else
			    objptr->Y = bottom;
			this->ContentTail->Flags |= PRT_OBJ_F_SOFTNEWLINE;
			}
		    split_obj = NULL;
		    }
		}

	    /** Special case of unsplit string here **/
	    if (objptr->ObjType->TypeID == PRT_OBJ_T_STRING && !split_obj && 
	        objptr->X == 0.0 && objptr->Width - PRT_FP_FUDGE > prtInnerWidth(this))
	        {
		/** It's on its own line now, so we can try splitting it again. **/
		continue;
		}

	    /** Ok, done any initial splitting or moving that was needed. **/
	    /** Add the objptr, and then see about adding split_obj if needed. **/
	    /** First, do we need to request more room in the 'area'? **/
	    new_parent = NULL;
	    if (objptr->Y + objptr->Height - PRT_FP_FUDGE > prtInnerHeight(this))
	        {
		/** Request the additional space, if allowed **/
		if (this->LayoutMgr->Resize(this, this->Width, objptr->Y + objptr->Height + this->MarginTop + this->MarginBottom + this->BorderTop + this->BorderBottom) < 0)
		    {
		    /** Resize denied.  If container is empty, we can't continue on, so error out here. **/
		    if (!this->ContentHead || (this->ContentHead->X + this->ContentHead->Width == 0.0 && !this->ContentHead->Next))
			{
			while ((split_obj = prt_textlm_GetSplitObj(&split_obj_list)) != NULL) 
			    {
			    if (split_obj->Content) nmSysFree(split_obj->Content);
			    prt_internal_FreeObj(split_obj);
			    }
			return -1;
			}

		    /** Try a break operation. **/
		    if (!(this->Flags & PRT_OBJ_F_ALLOWSOFTBREAK) || this->LayoutMgr->Break(this, &new_parent) < 0)
			{
			/** Break also denied?  Fail if so. **/
			while ((split_obj = prt_textlm_GetSplitObj(&split_obj_list)) != NULL) 
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
		    this = new_parent;
		    objptr->X = 0.0;
		    objptr->Y = 0.0;
		    prt_textlm_UndoWrap(this->ContentTail);
		    prt_textlm_UndoWrap(objptr);
		    continue;
		    }
		}

	    /** Now add the object to the container **/
	    prt_internal_Add(this, objptr);

	    /** Repeat the procedure for the split-off part of the object **/
	    objptr = prt_textlm_GetSplitObj(&split_obj_list);
	    }

    return 0;
    }


/*** prt_textlm_InitContainer() - initialize a newly created container that
 *** uses this layout manager.  For this manager, it typically means adding
 *** an empty-string element to the container to give a starting point for
 *** font settings, line height, and so forth.
 ***/
int
prt_textlm_InitContainer(pPrtObjStream this, pPrtTextLMData old_lm_inf, va_list va)
    {
    pPrtTextLMData lm_inf;
    char* attrname;
    pPrtBorder b;

	/** Add our private data structure **/
	lm_inf = (pPrtTextLMData)nmMalloc(sizeof(PrtTextLMData));
	if (!lm_inf) return -1;
	this->LMData = lm_inf;
	memset(lm_inf, 0, sizeof(PrtTextLMData));

	/** Params from the user? **/
	if (!old_lm_inf)
	    {
	    while(va && (attrname = va_arg(va, char*)) != NULL)
		{
		if (!strcmp(attrname,"border"))
		    {
		    b = va_arg(va, pPrtBorder);
		    if (b) 
			{
			memcpy(&(lm_inf->AreaBorder), b, sizeof(PrtBorder));
			this->BorderTop = b->TotalWidth;
			this->BorderBottom = b->TotalWidth;
			this->BorderLeft = b->TotalWidth;
			this->BorderRight = b->TotalWidth;
			}
		    }
		}
	    }
	else
	    {
	    memcpy(lm_inf, old_lm_inf, sizeof(PrtTextLMData));
	    }

	prt_textlm_Setup(this);

    return 0;
    }


/*** prt_textlm_DeinitContainer() - de-initialize a container using this
 *** layout manager.  In this case, it does basically nothing.
 ***/
int
prt_textlm_DeinitContainer(pPrtObjStream this)
    {
    pPrtTextLMData lm_inf = (pPrtTextLMData)(this->LMData);

	if (lm_inf)
	    {
	    nmFree(lm_inf, sizeof(PrtTextLMData));
	    lm_inf = NULL;
	    }

    return 0;
    }


/*** prt_textlm_Finalize() - this routine is run once the page is ready
 *** to be generated and gives us a chance to make any final modifications
 *** to the appearance of the container, but not its overall geometry.  If
 *** the caller asked for a border, we do that here.
 ***/
int
prt_textlm_Finalize(pPrtObjStream this)
    {
    pPrtTextLMData lm_inf = (pPrtTextLMData)(this->LMData);

	/** Finish off justification? **/
	if (this->ContentTail) prt_textlm_JustifyLine(this->ContentTail, this->ContentTail->Justification);

	/** Border? **/
	if (lm_inf->AreaBorder.nLines > 0)
	    {
	    prt_internal_MakeBorder(this, 0.0, 0.0, this->Width, PRT_MKBDR_F_TOP | PRT_MKBDR_F_MARGINRELEASE, 
		    &(lm_inf->AreaBorder), &(lm_inf->AreaBorder), &(lm_inf->AreaBorder));
	    prt_internal_MakeBorder(this, 0.0, 0.0, this->Height, PRT_MKBDR_F_LEFT | PRT_MKBDR_F_MARGINRELEASE, 
		    &(lm_inf->AreaBorder), &(lm_inf->AreaBorder), &(lm_inf->AreaBorder));
	    prt_internal_MakeBorder(this, 0.0, this->Height, this->Width, PRT_MKBDR_F_BOTTOM | PRT_MKBDR_F_MARGINRELEASE, 
		    &(lm_inf->AreaBorder), &(lm_inf->AreaBorder), &(lm_inf->AreaBorder));
	    prt_internal_MakeBorder(this, this->Width, 0.0, this->Height, PRT_MKBDR_F_RIGHT | PRT_MKBDR_F_MARGINRELEASE, 
		    &(lm_inf->AreaBorder), &(lm_inf->AreaBorder), &(lm_inf->AreaBorder));
	    }

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
	lm->SetValue = NULL;
	lm->Reflow = NULL;
	lm->Finalize = prt_textlm_Finalize;
	strcpy(lm->Name, "textflow");

	/** Register the layout manager **/
	prtRegisterLayoutMgr(lm);

    return 0;
    }


