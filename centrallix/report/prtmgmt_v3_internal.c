#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
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
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_internal.c,v 1.23 2009/07/01 18:50:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_internal.c,v $

    $Log: prtmgmt_v3_internal.c,v $
    Revision 1.23  2009/07/01 18:50:09  gbeeley
    - (bugfix) need to include limits.h to get INT_MAX on some systems.

    Revision 1.22  2009/06/26 16:18:59  gbeeley
    - (change) GetCharacterMetric now returns both height and width
    - (performance) change from bubble sort to merge sort for page generation
      and output sequencing (this made a BIG difference)
    - (bugfix) attempted fix of text output overlapping problems, but there
      are still trouble points here.

    Revision 1.21  2007/04/08 03:52:01  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.20  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.19  2007/02/17 04:34:51  gbeeley
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

    Revision 1.18  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.17  2003/07/09 18:10:02  gbeeley
    Further fixes and enhancements to prtmgmt layer, particularly regarding
    visual layout of graphical borders around objects; border/shadow
    thickness is now automatically computed into the total margin between
    exterior edges and interior edges of an object.

    Revision 1.16  2003/04/21 21:00:44  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

    Revision 1.15  2003/03/18 04:06:25  gbeeley
    Added basic image (picture/bitmap) support; only PNG images supported
    at present.  Moved image and border (rectangles) functionality into a
    new file prtmgmt_v3_graphics.c.  Graphics are only monochrome at the
    present and work only under PCL (not plain text!!!).  PNG support is
    via libpng, so libpng was added to configure/autoconf.

    Revision 1.14  2003/03/15 04:46:00  gbeeley
    Added borders to tables.  Not fully tested yet.  Added a new component
    of the "PrtBorder" object: "Pad", which is the padding 'outside' of
    the border.  The reporting objdriver is going to have to really
    simplify the margins/borders stuff on tables because there are so many
    params that can be set - it can be confusing and hard to get right.

    Revision 1.13  2003/03/12 20:51:36  gbeeley
    Tables now working, but borders on tables not implemented yet.
    Completed the prt_internal_Duplicate routine and reworked the
    API interface to InitContainer on the layout managers.  Not all
    features/combinations on tables have been tested.  Footers on
    tables not working but (repeating) headers are.  Added a new
    prt obj stream field called "ContentSize" which provides the
    allocated memory size of the "Content" field.

    Revision 1.12  2003/03/07 06:16:12  gbeeley
    Added border-drawing functionality, and converted the multi-column
    layout manager to use that for column separators.  Added border
    capability to textareas.  Reworked the deinit/init kludge in the
    Reflow logic.

    Revision 1.11  2003/03/06 02:52:34  gbeeley
    Added basic rectangular-area support (example - border lines for tables
    and separator lines for multicolumn areas).  Works on both PCL and
    textonly.  Palette-based coloring of rectangles (via PCL) not seeming
    to work consistently on my system, however.  Warning: using large
    dimensions for the 'rectangle' command in test_prt may consume much
    printer ink!!  Now it's time to go watch the thunderstorms....

    Revision 1.10  2003/03/01 07:24:02  gbeeley
    Ok.  Balanced columns now working pretty well.  Algorithm is currently
    somewhat O(N^2) however, and is thus a bit expensive, but still not
    bad.  Some algorithmic improvements still possible with both word-
    wrapping and column balancing, but this is 'good enough' for the time
    being, I think ;)

    Revision 1.9  2003/02/27 22:02:20  gbeeley
    Some improvements in the balanced multi-column output.  A lot of fixes
    in the multi-column output and in the text layout manager.  Added a
    facility to "schedule" reflows rather than having them take place
    immediately.

    Revision 1.8  2003/02/27 05:21:19  gbeeley
    Added multi-column layout manager functionality to support multi-column
    sections (this is newspaper-style multicolumn formatting).  Tested in
    test_prt "columns" command with various numbers of columns.  Balanced
    mode not yet working.

    Revision 1.7  2003/02/25 03:57:50  gbeeley
    Added incremental reflow capability and test in test_prt.  Added stub
    multi-column layout manager.  Reflow is horribly inefficient, but not
    worried about that at this point.

    Revision 1.6  2003/02/19 22:53:54  gbeeley
    Page break now somewhat operational, both with hard breaks (form feeds)
    and with soft breaks (page wrapping).  Some bugs in how my printer (870c)
    places the text on pages after a soft break (but the PCL seems to look
    correct), and in how word wrapping is done just after a page break has
    occurred.  Use "printfile" command in test_prt to test this.

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

    Revision 1.4  2002/10/18 22:01:38  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.3  2002/10/17 20:23:18  gbeeley
    Got printing v3 subsystem open/close session working (basically)...

    Revision 1.2  2002/04/25 04:30:14  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


/*** prt_internal_AllocObj - allocate a new object of a given type.
 ***/
pPrtObjStream
prt_internal_AllocObj(char* type)
    {
    pPrtObjStream pobj;
    pPrtObjType ot;
    int i;

	/** Allocate the structure memory **/
	pobj = (pPrtObjStream)nmMalloc(sizeof(PrtObjStream));
	if (!pobj) return NULL;
	memset(pobj, 0, sizeof(PrtObjStream));
	SETMAGIC(pobj, MGK_PRTOBJSTRM);

	/** Lookup the object type **/
	for(i=0;i<PRTMGMT.TypeList.nItems;i++)
	    {
	    ot = (pPrtObjType)(PRTMGMT.TypeList.Items[i]);
	    ASSERTMAGIC(ot, MGK_PRTOBJTYPE);
	    if (!strcmp(ot->TypeName, type))
	        {
		pobj->ObjType = ot;
		pobj->LayoutMgr = ot->PrefLayoutMgr;
		break;
		}
	    }
	if (!pobj->ObjType)
	    {
	    mssError(1,"PRT","Bark!  Unknown print stream object type '%s'", type);
	    nmFree(pobj, sizeof(PrtObjStream));
	    return NULL;
	    }
	pobj->BGColor = 0x00FFFFFF; /* white */
	pobj->FGColor = 0x00000000; /* black */
	pobj->TextStyle.Color = 0x00000000; /* black */

    return pobj;
    }


/*** prt_internal_AllocObjByID - allocate a new object of a given type.
 ***/
pPrtObjStream
prt_internal_AllocObjByID(int type_id)
    {
    pPrtObjStream pobj;
    pPrtObjType ot;
    int i;

	/** Allocate the structure memory **/
	pobj = (pPrtObjStream)nmMalloc(sizeof(PrtObjStream));
	if (!pobj) return NULL;
	memset(pobj, 0, sizeof(PrtObjStream));
	SETMAGIC(pobj, MGK_PRTOBJSTRM);

	/** Lookup the object type **/
	for(i=0;i<PRTMGMT.TypeList.nItems;i++)
	    {
	    ot = (pPrtObjType)(PRTMGMT.TypeList.Items[i]);
	    ASSERTMAGIC(ot, MGK_PRTOBJTYPE);
	    if (ot->TypeID == type_id)
	        {
		pobj->ObjType = ot;
		pobj->LayoutMgr = ot->PrefLayoutMgr;
		break;
		}
	    }
	if (!pobj->ObjType)
	    {
	    mssError(1,"PRT","Bark!  Unknown print stream object type '%d'", type_id);
	    nmFree(pobj, sizeof(PrtObjStream));
	    return NULL;
	    }
	pobj->BGColor = 0x00FFFFFF; /* white */
	pobj->FGColor = 0x00000000; /* black */

    return pobj;
    }


/*** prt_internal_Add - adds a child object to the content stream of
 *** a given parent object.
 ***/
int
prt_internal_Add(pPrtObjStream parent, pPrtObjStream new_child)
    {

	/** Call the Add method on the layout manager for the parent **/
	/* GRB - always use parent->LayoutMgr->AddObject instead of this routine */
	/*if (parent->LayoutMgr)
	    return parent->LayoutMgr->AddObject(parent, new_child);*/

	/** No layout manager? Add it manually. **/
	if (parent->ContentTail)
	    {
	    /** Container has content already **/
	    new_child->Prev = parent->ContentTail;
	    parent->ContentTail->Next = new_child;
	    parent->ContentTail = new_child;
	    new_child->Parent = parent;
	    }
	else
	    {
	    /** Container has no content at all. **/
	    parent->ContentHead = new_child;
	    parent->ContentTail = new_child;
	    new_child->Parent = parent;
	    }
	new_child->Session = parent->Session;

    return 0;
    }


/*** prt_internal_Insert() - adds a child object after a sibling.
 ***/
int
prt_internal_Insert(pPrtObjStream sibling, pPrtObjStream new_obj)
    {

	new_obj->Next = sibling->Next;
	new_obj->Prev = sibling;
	if (new_obj->Next)
	    new_obj->Next->Prev = new_obj;
	else
	    sibling->Parent->ContentTail = new_obj;
	sibling->Next = new_obj;
	new_obj->Parent = sibling->Parent;
	new_obj->Session = sibling->Session;

    return 0;
    }


/*** prt_internal_CopyAttrs - duplicate the formatting attributes of one 
 *** object to another object.
 ***/
int
prt_internal_CopyAttrs(pPrtObjStream src, pPrtObjStream dst)
    {

	/** Copy just the formatting attributes, nothing else **/
	dst->FGColor = src->FGColor;
	dst->BGColor = src->BGColor;
	memcpy(&(dst->TextStyle), &(src->TextStyle), sizeof(PrtTextStyle));
	dst->LineHeight = src->LineHeight;
	dst->ConfigLineHeight = src->ConfigLineHeight;

    return 0;
    }


/*** prt_internal_FontToLineHeight() - determine the appropriate line height
 *** given the font and size.
 ***/
double
prt_internal_FontToLineHeight(pPrtObjStream obj)
    {
    double correction_factor;

	if (obj->TextStyle.FontID == PRT_FONT_T_MONOSPACE)
	    correction_factor = 1.0;
	else
	    correction_factor = 1.116;

    return correction_factor * prt_internal_GetFontHeight(obj);
    }


/*** prt_internal_CopyGeom - duplicate the geometry of one object to another
 *** object.
 ***/
int
prt_internal_CopyGeom(pPrtObjStream src, pPrtObjStream dst)
    {

	/** Copy just the formatting attributes, nothing else **/
	dst->Height = src->Height;
	dst->Width = src->Width;
	dst->ConfigHeight = src->ConfigHeight;
	dst->ConfigWidth = src->ConfigWidth;
	dst->X = src->X;
	dst->Y = src->Y;
	dst->MarginLeft = src->MarginLeft;
	dst->MarginRight = src->MarginRight;
	dst->MarginTop = src->MarginTop;
	dst->MarginBottom = src->MarginBottom;
	dst->BorderLeft = src->BorderLeft;
	dst->BorderRight = src->BorderRight;
	dst->BorderTop = src->BorderTop;
	dst->BorderBottom = src->BorderBottom;

    return 0;
    }


/*** prt_internal_GetFontHeight - Query the formatter to determine the
 *** base font height, given an objstream structure containing the 
 *** font and font size data.
 ***/
double
prt_internal_GetFontHeight(pPrtObjStream obj)
    {
    double w, h;
    PRTSESSION(obj)->Formatter->GetCharacterMetric(PRTSESSION(obj)->FormatterData, " ", &(obj->TextStyle), &w, &h);
    /*return obj->TextStyle.FontSize/12.0;*/
    return h;
    }


/*** prt_internal_GetFontBaseline - Figure out the distance from the top
 *** of the printed font to its baseline.  Ask the formatter about this.
 ***/
double
prt_internal_GetFontBaseline(pPrtObjStream obj)
    {
    return PRTSESSION(obj)->Formatter->GetCharacterBaseline(PRTSESSION(obj)->FormatterData, &(obj->TextStyle));
    }


/*** prt_internal_GetStringWidth - obtain, via char metrics, the physical
 *** width of the given string of text.
 ***/
double
prt_internal_GetStringWidth(pPrtObjStream obj, char* str, int n)
    {
    double w = 0.0, h;
    char oldend;
    int l;
    
	/** Add it up for each char in the string **/
	l = strlen(str);
	if (l > n)
	    {
	    oldend = str[n];
	    str[n] = '\0';
	    }
	PRTSESSION(obj)->Formatter->GetCharacterMetric(PRTSESSION(obj)->FormatterData, str, &(obj->TextStyle), &w, &h);
	if (l > n)
	    {
	    str[n] = oldend;
	    }

    return w;
    }


/*** prt_internal_FreeObj - release the memory used by an object.
 ***/
int
prt_internal_FreeObj(pPrtObjStream obj)
    {

	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	nmFree(obj, sizeof(PrtObjStream));

    return 0;
    }


/*** prt_internal_YCompare() - compares two objects to see which one comes
 *** first in the Y order.  Returns 0 if they are the same, -1 if the first
 *** object comes first, and 1 if the second object comes first (and thus
 *** the first one is 'greater').
 ***/
int
prt_internal_YCompare(pPrtObjStream first, pPrtObjStream second)
    {
    if (first->PageY > second->PageY || (first->PageY == second->PageY && first->PageX > second->PageX))
	return 1;
    else if (first->PageY == second->PageY && first->PageX == second->PageX)
	return 0;
    else
	return -1;
    }


/*** prt_internal_YMergeSetup_r() - determines the absolute page coordinates of
 *** each object on the page, as well as setting up the initial sequential
 *** ynext/yprev linkages to facilitate the sorting procedure.  Sets the
 *** first_obj to the first object in the sequential (unsorted) chain.
 ***/
int
prt_internal_YMergeSetup_r(pPrtObjStream obj)
    {
    pPrtObjStream objptr;
    int cnt = 1;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Set this object's absolute y/x positions **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_PAGE)
	    {
	    obj->PageX = 0.0;
	    obj->PageY = 0.0;
	    }
	else
	    {
	    if (obj->Flags & PRT_OBJ_F_MARGINRELEASE)
		{
		obj->PageX = obj->Parent->PageX + obj->X;
		obj->PageY = obj->Parent->PageY + obj->Y;
		}
	    else
		{
		obj->PageX = obj->Parent->PageX + obj->Parent->MarginLeft + obj->Parent->BorderLeft + obj->X;
		obj->PageY = obj->Parent->PageY + obj->Parent->MarginTop + obj->Parent->BorderTop + obj->Y;
		}
	    }

	for(objptr=obj->ContentHead;objptr;objptr=objptr->Next)
	    {
	    /** Do the subtree **/
	    cnt += prt_internal_YMergeSetup_r(objptr);
	    }

    return cnt;
    }


/*** prt_internal_YMergeCopy_r() - copy the entire tree of object pointers into
 *** an array, to be sorted.
 ***/
int
prt_internal_YMergeCopy_r(pPrtObjStream obj, pPrtObjStream* arr)
    {
    int cnt = 0;
    pPrtObjStream objptr;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Copy all sublists **/
	for(objptr=obj->ContentHead;objptr;objptr=objptr->Next)
	    {
	    arr[cnt++] = objptr;
	    if (objptr->ContentHead)
		cnt += prt_internal_YMergeCopy_r(objptr, arr+cnt);
	    }

    return cnt;
    }


/*** prt_internal_YMergeSort_r() - actually do the merge sort on the arrays
 *** of print objects
 ***/
int
prt_internal_YMergeSort_r(pPrtObjStream * arr1, pPrtObjStream * arr2, int cnt, int okleaf)
    {
    int i,j,found;
    int m,dst;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Do a selection sort if cnt <= 32, IF we are starting with the right buffer **/
	if (cnt <= 32 && okleaf)
	    {
	    for(i=0;i<cnt;i++)
		{
		found=0;
		for(j=1;j<cnt;j++)
		    {
		    if (!arr1[found])
			found = j;
		    else if (arr1[j] && prt_internal_YCompare(arr1[j], arr1[found]) < 0)
			found = j;
		    }
		arr2[i] = arr1[found];
		arr1[found] = NULL;
		}
	    }
	else
	    {
	    /** merge sort 0...m-1 and m...cnt-1 **/
	    /** sort the halves **/
	    m = cnt / 2;
	    if (prt_internal_YMergeSort_r(arr2, arr1, m, !okleaf) < 0) return -1;
	    if (prt_internal_YMergeSort_r(arr2+m, arr1+m, cnt-m, !okleaf) < 0) return -1;

	    /** Merge them **/
	    dst = 0;
	    i = 0;
	    j = m;
	    while(i<m || j<cnt)
		{
		if (i==m)
		    arr2[dst++] = arr1[j++];
		else if (j==cnt)
		    arr2[dst++] = arr1[i++];
		else if (prt_internal_YCompare(arr1[i], arr1[j]) <= 0)
		    arr2[dst++] = arr1[i++];
		else
		    arr2[dst++] = arr1[j++];
		}
	    }

    return 0;
    }


/*** prt_internal_YPrintArray() - show the contents of a y sort array ***/
int
prt_internal_PrintArray(pPrtObjStream * arr, int cnt)
    {
    int i;

	for(i=0;i<cnt;i++)
	    {
	    printf("%2.2d %8.8x: (%.2lf,%.2lf)\n", arr[i]->ObjType->TypeID, arr[i], arr[i]->PageX, arr[i]->PageY);
	    }

    return 0;
    }


/*** prt_internal_YMergeSort() - do a merge sort on the entire tree of objects,
 *** so we can output them in visually sorted order.
 ***/
pPrtObjStream
prt_internal_YMergeSort(pPrtObjStream page)
    {
    pPrtObjStream first;
    int cnt, i;
    pPrtObjStream * arr1;
    pPrtObjStream * arr2;

	/** Setup pageX, pageY, and count the number of objects **/
	cnt = prt_internal_YMergeSetup_r(page);
	if (cnt <= 0) return NULL;
	if (cnt >= INT_MAX / sizeof(pPrtObjStream)) return NULL;

	/** Copy the objects into the array **/
	arr1 = (pPrtObjStream *)nmSysMalloc(cnt * sizeof(pPrtObjStream));
	arr2 = (pPrtObjStream *)nmSysMalloc(cnt * sizeof(pPrtObjStream));
	arr1[0] = page;
	prt_internal_YMergeCopy_r(page, arr1+1);

	/** Now do the mergesort. **/
	prt_internal_YMergeSort_r(arr1, arr2, cnt, 1);

	/** Set up the links **/
	for(i=0;i<cnt-1;i++)
	    {
	    arr2[i]->YNext = arr2[i+1];
	    arr2[i]->YNext->YPrev = arr2[i];
	    }
	arr2[cnt-1]->YNext = NULL;
	arr2[0]->YPrev = NULL;
	first = arr2[0];
	nmSysFree(arr1);
	nmSysFree(arr2);

    return first;
    }


/*** prt_internal_YSetup_r() - determines the absolute page coordinates of
 *** each object on the page, as well as setting up the initial sequential
 *** ynext/yprev linkages to facilitate the sorting procedure.  Sets the
 *** first_obj to the first object in the sequential (unsorted) chain.
 ***/
int
prt_internal_YSetup_r(pPrtObjStream obj, pPrtObjStream* first_obj, pPrtObjStream* last_obj)
    {
    pPrtObjStream subtree_first_obj;
    pPrtObjStream subtree_last_obj;
    pPrtObjStream objptr;
    int total = 0;
    int rval;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Set first obj to the obj itself for now. **/
	*first_obj = obj;
	*last_obj = obj;

	/** Set this object's absolute y/x positions **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_PAGE)
	    {
	    obj->PageX = 0.0;
	    obj->PageY = 0.0;
	    }
	else
	    {
	    if (obj->Flags & PRT_OBJ_F_MARGINRELEASE)
		{
		obj->PageX = obj->Parent->PageX + obj->X;
		obj->PageY = obj->Parent->PageY + obj->Y;
		}
	    else
		{
		obj->PageX = obj->Parent->PageX + obj->Parent->MarginLeft + obj->Parent->BorderLeft + obj->X;
		obj->PageY = obj->Parent->PageY + obj->Parent->MarginTop + obj->Parent->BorderTop + obj->Y;
		}
	    }

	/** For each child obj, run setup on it and add the chain. **/
	for(objptr=obj->ContentHead;objptr;objptr=objptr->Next)
	    {
	    total++;

	    /** Do the subtree **/
	    rval = prt_internal_YSetup_r(objptr, &subtree_first_obj, &subtree_last_obj);
	    if (rval < 0) return rval;
	    total += rval;

	    /** Add the subtree to the list. **/
	    (*last_obj)->YNext = subtree_first_obj;
	    (*last_obj)->YNext->YPrev = *last_obj;
	    *last_obj = subtree_last_obj;
	    }
	(*last_obj)->YNext = NULL;
	(*first_obj)->YPrev = NULL;

    return total;
    }


/*** prt_internal_YSort() - sorts all objects on a page by their absolute
 *** Y location on the page, so that the page formatters can output the 
 *** page data in correct sequence.  Returns the first object in the page,
 *** in Y sequence (then X sequence within the same Y row).
 ***
 *** This sort could use some optimization.  Because of the large numbers of
 *** mostly-sorted subtrees (from textflow layout), a merge sort might be in
 *** order here.  But, something simple for now will do.
 ***/
pPrtObjStream
prt_internal_YSort(pPrtObjStream obj)
    {
    pPrtObjStream first,last,tmp1,tmp2;
    pPrtObjStream *sortptr;
    int did_swap;
    int cnt;

	/** Walk the tree, determining absolute coordinates as well as 
	 ** setting up the initial linear sortable sequence of objects.
	 **/
	cnt = prt_internal_YSetup_r(obj, &first, &last) + 1;
	if (cnt < 0) return NULL;

	/** Basic stuff here - do a bubble sort on the objects.  We point to
	 ** the *pointer* with sortptr in order to keep 'first' pointing to
	 ** the top of the list.
	 **/
	do  {
	    did_swap = 0;
	    for(sortptr= &first;*sortptr && (*sortptr)->YNext;sortptr=&((*sortptr)->YNext))
		{
		if (prt_internal_YCompare(*sortptr, (*sortptr)->YNext) > 0)
		    {
		    /** Do the swap.  Tricky, but doable :) **/
		    did_swap = 1;
		    tmp1 = (*sortptr);
		    tmp2 = (*sortptr)->YNext;

		    /** forward pointers **/
		    (*sortptr) = tmp2;
		    tmp1->YNext = tmp2->YNext;
		    tmp2->YNext = tmp1;

		    /** Backwards pointers **/
		    tmp2->YPrev = tmp1->YPrev;
		    tmp1->YPrev = tmp2;
		    if (tmp1->YNext) tmp1->YNext->YPrev = tmp1;
		    }
		}
	    } while(did_swap);

    return first;
    }


/*** prt_internal_AddYSorted() - adds an object to the YPrev/YNext chain in its
 *** proper place in sorted order.  'obj' is the object to start the 'Y' search
 *** at, and 'newobj' is the object to add to the chain.
 ***/
int
prt_internal_AddYSorted(pPrtObjStream obj, pPrtObjStream newobj)
    {

	/** Search for the place to insert it **/
	while (obj->YNext && prt_internal_YCompare(newobj, obj->YNext) > 0)
	    obj = obj->YNext;

	/** Ok, insert *after* the 'obj' **/
	newobj->YNext = obj->YNext;
	newobj->YPrev = obj;
	newobj->YPrev->YNext = newobj;
	if (newobj->YNext) newobj->YNext->YPrev = newobj;

    return 0;
    }


/*** prt_internal_MakeOrphan() - deletes an object from its parent's list of
 *** child objects.
 ***/
int
prt_internal_MakeOrphan(pPrtObjStream obj)
    {

	/** Make sure there is a parent before unlinking **/
	if (obj->Parent)
	    {
	    if (obj->Prev) obj->Prev->Next = obj->Next;
	    if (obj->Next) obj->Next->Prev = obj->Prev;
	    if (obj->Parent->ContentHead == obj) obj->Parent->ContentHead = obj->Next;
	    if (obj->Parent->ContentTail == obj) obj->Parent->ContentTail = obj->Prev;
	    }

	/** Clear the prev/next pointers locally **/
	obj->Parent = NULL;
	obj->Next = NULL;
	obj->Prev = NULL;

    return 0;
    }


/*** prt_internal_FreeTree() - releases memory and resources used by an entire
 *** subtree.
 ***/
int
prt_internal_FreeTree(pPrtObjStream obj)
    {
    pPrtObjStream subtree,del;
    int handle_id = -1;
    pPrtHandle h;

	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** First, scan the content list for subtrees to free up **/
	subtree=obj->ContentHead;
	while(subtree)
	    {
	    ASSERTMAGIC(subtree, MGK_PRTOBJSTRM);
	    del = subtree;
	    subtree=subtree->Next;
	    prt_internal_FreeTree(del);
	    }

	/** Now, free any content, if need be **/
	if (obj->Content) nmSysFree(obj->Content);
	obj->Content = NULL;

	/** Disconnect from linknext/linkprev **/
	if (obj->LinkNext) 
	    {
	    ASSERTMAGIC(obj->LinkNext, MGK_PRTOBJSTRM);
	    obj->LinkNext->LinkPrev = NULL;
	    obj->LinkNext = NULL;
	    }

	/** Disconnect from parent **/
	prt_internal_MakeOrphan(obj);

	/** De-init the container via the layout manager **/
	if (obj->LayoutMgr) obj->LayoutMgr->DeinitContainer(obj);

	/** Free the memory used by the object itself **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTableByPtr, (void*)&obj);
	if (h) handle_id = h->HandleID;
	if (handle_id >= 0) prtFreeHandle(handle_id);
	nmFree(obj,sizeof(PrtObjStream));

    return 0;
    }


/*** prt_internal_Finalize_r() - gives the layout managers a chance to 
 *** make last minute, non-geometric changes to the containers they
 *** manage before the container is actually printed.
 ***/
int
prt_internal_Finalize_r(pPrtObjStream this)
    {
    pPrtObjStream content;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"PRT","Could not generate page: resource exhaustion occurred");
	    return -1;
	    }

	/** Finalize children first.  That way, since a finalize may modify
	 ** the content of an object, we don't have trouble looking at the 
	 ** content while it is being modified. 
	 **/
	content = this->ContentHead;
	while(content)
	    {
	    if (prt_internal_Finalize_r(content) < 0)
		return -1;
	    content = content->Next;
	    }

	/** Then do this container **/
	if (this->LayoutMgr && this->LayoutMgr->Finalize) this->LayoutMgr->Finalize(this);

    return 0;
    }


/*** prt_internal_GeneratePage() - starts the formatting process for a given
 *** page and sends it to the output device.
 ***/
int 
prt_internal_GeneratePage(pPrtSession s, pPrtObjStream page)
    {
    pPrtObjStream first;

	ASSERTMAGIC(s, MGK_PRTOBJSSN);
	ASSERTMAGIC(page, MGK_PRTOBJSTRM);

	/** First, give the layout managers a chance to 'finalize' **/
	if (prt_internal_Finalize_r(page) < 0)
	    return -1;

	/** Next, y-sort the page **/
	/*first = prt_internal_YSort(page);*/
	first = prt_internal_YMergeSort(page);
	if (!first)
	    return -1;

	/** Now, send it to the formatter **/
	s->Formatter->Generate(s->FormatterData, page);

    return 0;
    }


/*** prt_internal_GetPage() - determines the page which contains the given
 *** object.
 ***/
pPrtObjStream
prt_internal_GetPage(pPrtObjStream obj)
    {

	/** Follow the Parent links until we get a page or until NULL. **/
	while (obj && obj->ObjType->TypeID != PRT_OBJ_T_PAGE)
	    {
	    obj = obj->Parent;
	    }

    return obj;
    }


/*** prt_internal_CreateEmptyObj() - creates an empty object but does not 
 *** add it to the container.  We make sure the content has room to be 
 *** expanded to a single space (for resolving reflow / unwordwrap stuff).
 ***/
pPrtObjStream
prt_internal_CreateEmptyObj(pPrtObjStream container)
    {
    pPrtObjStream obj,prev_obj;

	if (container->ObjType->TypeID != PRT_OBJ_T_AREA)
	    return NULL;

	obj = prt_internal_AllocObjByID(PRT_OBJ_T_STRING);
	obj->Session = container->Session;
	obj->Content = nmSysMalloc(2);
	obj->ContentSize = 2;
	obj->Content[0] = '\0';
	obj->Width = 0.0;
	obj->ConfigLineHeight = -1.0;
	prev_obj = (container->ContentTail)?(container->ContentTail):container;
	prt_internal_CopyAttrs(prev_obj, obj);
	obj->Height = prt_internal_GetFontHeight(prev_obj);
	obj->YBase = prt_internal_GetFontBaseline(prev_obj);

    return obj;
    }


/*** prt_internal_AddEmptyObj() - adds an empty text string object to an area
 *** so that the font and so forth attributes can be changed.  For non-AREA
 *** containers, this routine simply returns a reference to the most appropriate
 *** object to set font/etc properties on.
 ***/
pPrtObjStream
prt_internal_AddEmptyObj(pPrtObjStream container)
    {
    pPrtObjStream obj;

	/** Is this an area? **/
	if (container->ObjType->TypeID == PRT_OBJ_T_AREA)
	    {
	    /** yes - add an empty string object. **/
	    obj = prt_internal_CreateEmptyObj(container);
	    container->LayoutMgr->AddObject(container, obj);
	    }
	else
	    {
	    /** no - point to container or tail of container's content **/
	    if (container->ContentTail)
		obj = container->ContentTail;
	    else
		obj = container;
	    }

    return obj;
    }


/*** prt_internal_Dump() - debugging printout of an entire subtree.
 ***/
int
prt_internal_Dump_r(pPrtObjStream obj, int level)
    {
    pPrtObjStream subobj;

	printf("%*.*s", level*4, level*4, "");
	switch(obj->ObjType->TypeID)
	    {
	    case PRT_OBJ_T_PAGE: printf("PAGE: "); break;
	    case PRT_OBJ_T_AREA: printf("AREA: "); break;
	    case PRT_OBJ_T_STRING: printf("STRG(%s): ", obj->Content); break;
	    case PRT_OBJ_T_SECTION: printf("SECT: "); break;
	    case PRT_OBJ_T_SECTCOL: printf("COLM: "); break;
	    case PRT_OBJ_T_TABLE: printf("TABL: "); break;
	    case PRT_OBJ_T_TABLEROW: printf("TROW: "); break;
	    case PRT_OBJ_T_TABLECELL: printf("TCEL: "); break;
	    case PRT_OBJ_T_RECT: printf("RECT: "); break;
	    case PRT_OBJ_T_IMAGE: printf("IMG:  "); break;
	    }
	printf("x=%.3g y=%.3g w=%.3g h=%.3g px=%.3g py=%.3g bl=%.3g fs=%d y+bl=%.3g flg=%d id=%d\n",
		obj->X, obj->Y, obj->Width, obj->Height,
		obj->PageX, obj->PageY, obj->YBase, obj->TextStyle.FontSize,
		obj->Y + obj->YBase, obj->Flags, obj->ObjID);
	for(subobj=obj->ContentHead;subobj;subobj=subobj->Next)
	    {
	    prt_internal_Dump_r(subobj, level+1);
	    }

    return 0;
    }

int
prt_internal_Dump(pPrtObjStream obj)
    {
    return prt_internal_Dump_r(obj,0);
    }


/*** prt_internal_Duplicate() - duplicate an entire object, including
 *** its content etc if "with_content" is set.
 ***
 *** GRB note - currently with_content is ignored.
 ***/
pPrtObjStream
prt_internal_Duplicate(pPrtObjStream obj, int with_content)
    {
    pPrtObjStream new_obj = NULL;
    pPrtObjStream child_obj;

	/** Duplicate the object itself **/
	new_obj = prt_internal_AllocObjByID(obj->ObjType->TypeID);
	if (!new_obj) return NULL;
	prt_internal_CopyAttrs(obj, new_obj);
	prt_internal_CopyGeom(obj, new_obj);
	new_obj->Justification = obj->Justification;
	if (!with_content)
	    {
	    /** If we are copying with the content, we want the actual height,
	     ** not the configured height.  Otherwise, if no content, we use the
	     ** configured height.
	     **/
	    new_obj->Height = obj->ConfigHeight;
	    new_obj->Width = obj->ConfigWidth;
	    }
	new_obj->Flags = obj->Flags;
	new_obj->Session = obj->Session;
	new_obj->LMData = NULL;

	/** Init the object as a container if needed **/
	if (new_obj->LayoutMgr)
	    new_obj->LayoutMgr->InitContainer(new_obj, obj->LMData, NULL);

	/** If object has content (for strings and images), copy it **/
	new_obj->ContentSize = obj->ContentSize;
	if (obj->Content && obj->ContentSize)
	    {
	    new_obj->Content = nmSysMalloc(obj->ContentSize);
	    memcpy(new_obj->Content, obj->Content, obj->ContentSize);
	    }

	/** Now take care of child objects within the object **/
	if (with_content)
	    {
	    child_obj = obj->ContentHead;
	    while(child_obj && (child_obj->Flags & PRT_OBJ_F_PERMANENT)) 
		{
		child_obj = child_obj->Next;
		}
	    while(child_obj)
		{
		prt_internal_Add(new_obj, prt_internal_Duplicate(child_obj, 1));
		child_obj = child_obj->Next;
		}
	    }

    return new_obj;
    }


/*** prt_internal_AdjustOpenCount() - adjust the nOpens counter
 *** on the parent container(s) up the tree from the given object.
 *** This is used to keep track of the number of "REQCOMPLETE"
 *** open containers within the tree, so we know when a page is
 *** ready to be emitted.
 ***
 *** Entry: obj == an objstrm object that has been added to the tree.
 ***        adjustment == pos/neg to incr/decr nOpens respectively
 *** Exit:  nOpens adjusted in this and all parent/grandparent/etc 
 ***        containers.
 ***/
int
prt_internal_AdjustOpenCount(pPrtObjStream obj, int adjustment)
    {

	/** Trace the object up the tree... **/
	while(obj)
	    {
	    ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	    assert(obj->nOpens >= 0);
	    obj->nOpens += adjustment;
	    assert(obj->nOpens >= 0);
	    obj = obj->Parent;
	    }

    return 0;
    }


/*** prt_internal_Reflow() - does a reflow operation on a given
 *** container.  If the layout manager has a Reflow method, then
 *** we use that exclusively, otherwise we use our default method,
 *** which simply removes all content from the container and then
 *** does AddObject calls to put them all back.
 ***
 *** If there are LinkPrev/LinkNext linkages, then this affects the
 *** current container and all Next ones, but not all Prev(ious)
 *** ones.  To affect previous ones, follow the LinkPrev link(s)
 *** *before* calling this function.
 ***/
int
prt_internal_Reflow(pPrtObjStream container)
    {
    pPrtObjStream contentlist = NULL, contenttail;
    pPrtObjStream childobj;
    pPrtObjStream cur_container;
    pPrtObjStream search;
    pPrtObjStream child_search;
    int handle_id;

	/** If we don't have a handle for the container, grab one. **/
	handle_id = -1;
	cur_container = container;
	while(cur_container)
	    {
	    handle_id = prtLookupHandle(cur_container);
	    if (handle_id != -1) break;
	    cur_container = cur_container->LinkNext;
	    }
	if (handle_id == -1) handle_id = prtAllocHandle(container);

	/** Clear the containers **/
	for(search=container; search; search = search->LinkNext)
	    {
	    /** Look for removable content **/
	    child_search = search->ContentHead;
	    while(child_search && (child_search->Flags & PRT_OBJ_F_PERMANENT)) 
		child_search = child_search->Next;

	    /** Remove the content and save it in our list. **/
	    if (child_search)
		{
		if (!contentlist) 
		    contentlist = child_search;
		else
		    contenttail->Next = child_search;
		contenttail = search->ContentTail;
		if (child_search == search->ContentHead)
		    {
		    search->ContentHead = NULL;
		    search->ContentTail = NULL;
		    }
		else
		    {
		    search->ContentTail = child_search->Prev;
		    search->ContentTail->Next = NULL;
		    }
		}
	    }

	/** Next, loop through the objects and re-add them **/
	if (cur_container && cur_container != container) prtUpdateHandle(handle_id, container);
	while(contentlist)
	    {
	    childobj = contentlist;
	    contentlist = contentlist->Next;
	    childobj->Parent = NULL;
	    childobj->Prev = NULL;
	    childobj->Next = NULL;
	    search = prtHandlePtr(handle_id);
	    search->LayoutMgr->AddObject(search, childobj);
	    }
	if (!cur_container) prtFreeHandle(handle_id);

    return 0;
    }


/*** prt_internal_ScheduleEvent() - schedules an event to occur as soon as
 *** the current API entry point has completed.  This allows things to be
 *** schedule to occur without interfering with current goings-on.
 ***/
int
prt_internal_ScheduleEvent(pPrtSession s, pPrtObjStream target, int type, void* param)
    {
    pPrtEvent e;
    pPrtEvent *se;

	/** Find queue tail.  Don't add duplictes. **/
	se = &(s->PendingEvents);
	while(*se) 
	    {
	    if ((*se)->TargetObject == target && (*se)->EventType == type && (*se)->Parameter == param)
		return -EEXIST;
	    se = &((*se)->Next);
	    }

	/** Allocate a new event **/
	e = (pPrtEvent)nmMalloc(sizeof(PrtEvent));
	if (!e) return -ENOMEM;
	e->TargetObject = target;
	e->EventType = type;
	e->Parameter = param;
	e->Next = NULL;

	/** Add it. **/
	*se = e;

    return 0;
    }


/*** prt_internal_DispatchEvents() - causes all pending events to be 
 *** activated and removed from the event queue.
 ***/
int
prt_internal_DispatchEvents(pPrtSession s)
    {
    pPrtEvent e;

	/** Loop through events, removing and running them. **/
	while(s->PendingEvents)
	    {
	    e = s->PendingEvents;
	    s->PendingEvents = s->PendingEvents->Next;
	    switch(e->EventType)
		{
		case PRT_EVENT_T_REFLOW:
		    prt_internal_Reflow(e->TargetObject);
		    break;

		default:
		    break;
		}
	    nmFree(e,sizeof(PrtEvent));
	    }

    return 0;
    }


/*** prtInnerWidth() - determine the inner width of an object, inside
 *** of its borders and margins.
 ***/
double
prtInnerWidth(pPrtObjStream obj)
    {
    return obj->Width - obj->MarginLeft - obj->MarginRight - obj->BorderLeft - obj->BorderRight;
    }


/*** prtInnerHeight() - determine the inner height of an object, inside of
 *** its borders and margins.
 ***/
double
prtInnerHeight(pPrtObjStream obj)
    {
    return obj->Height - obj->MarginTop - obj->MarginBottom - obj->BorderTop - obj->BorderBottom;
    }

