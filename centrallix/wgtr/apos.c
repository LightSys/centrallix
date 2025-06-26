/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	Auto-Positioning					*/
/* Author:	Nathaniel Colson					*/
/* Creation:	August 9, 2005						*/
/* Description:	Applies layout logic to the widgets of an application.	*/
/* See centrallix-sysdoc/Auto-Positioning.md for more information. 	*/
/************************************************************************/

/*** Author: Israel Fuller
 *** Date: June, 2025
 *** 
 *** I wasn't the one to write most of this (although I did write a ton of
 *** comments), but after doing my best to understand it, I hope that you will
 *** find the compiled information below helpful.
 ***
 *** Execution of this file usually begins when wgtrVerify() in wgtr.c calls
 *** aposAutoPositionWidgetTree(). To auto position the tree, the code first
 *** it draws four lines on the four edges of every visible widget (with some
 *** exceptions, @see aposAddLinesToGrid()). These lines divide the page into
 *** into horizontal sections (rows) and vertical sections (columns) which span
 *** the page. @see aposAddSectionsToGrid() for more detail.
 *** 
 *** The program guesses that some of these sections are "spacers", which are
 *** small amounts of space intended to provide visual room between widgets.
 *** When resizing, these do not flex at all. However, many elements are able
 *** to flex. @see aposSetFlexibilities() for more information about flexing.
 ***
 *** Next, the program uses aposSetLimits() to honor minimum and maximum sizes
 *** of widgets, and finally calls aposAutoPositionContainers() to position
 *** the widgets on the screen. Lastly, it calls aposProcessWindows() to handle
 *** floating window widgets, which are typically ignored by most of the rest
 *** of the code.
 ***
 *** Note: Due to this approach, this means that all sections and widgets start
 ***	   and end at a line. The way these lines are set up ensures that start
 ***	   lines are always on the top or left, and end lines are always on the
 ***	   bottom or right. @see aposAddLinesForChildren()
 ***
 *** Notes: I wrote some information about various structs below that's good to
 ***	    know. Some of this is covered elsewhere in the documentation.
 *** 
 *** AposGrid: A data structure to store sections and lines.
 ***
 *** AposLine: An AposLine spans the entire page.
 ***
 *** AposSection: After lines are created, sections are added in between the
 *** 	lines (aka. in between the nodes). Every node begins and ends on the
 *** 	edge of a section, although it may span multiple sections.
 *** 
 *** pWgtrNode: A pointer to a widget node instance. You can think of this
 *** 	like a DOM node, but remember that it's common for them to expand
 *** 	into multiple DOM nodes. Also, these can have children, just like
 *** 	a DOM node, which is why a single widget node pointer is really
 *** 	more of a tree of them.
 *** 
 *** XArray: This array also stores its size (nAlloc) and the number of items
 *** 	stored (nItems), so you don't have to pass that info separately.
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apos.h"
#include "cxlib/xarray.h"
#include "cxlib/datatypes.h"

/*** Allocate space for a grid, section, and line using the custom allocation
 *** system. Note that register is similar to creating a new heap-allocated
 *** variable, then binding it to a name.
 *** 
 *** @returns 0, success.
 ***/
int
aposInit()
{
    nmRegister(sizeof(AposGrid), "AposGrid");
    nmRegister(sizeof(AposSection), "AposSection");
    nmRegister(sizeof(AposLine), "AposLine");
    
    return 0;
}

/*** Dumps the grid content of a widget node and its floating children. This
 *** function is most likely intended for debugging.
 *** 
 *** @param tree   The widget tree from which to extract the layout grid.
 *** @param indent The number of 4-space indentations to indent the output.
 *** 	Note: Included for the sake of recursion; just pass 0.
 *** @returns 0, success.
 ***/
int
aposDumpGrid(pWgtrNode tree, int indent)
{
int i, childCnt, sectionCnt;
pAposSection section;
pWgtrNode child;

    printf("%*.*s*** %s ***\n", indent*4, indent*4, "", tree->Name);
    if (tree->LayoutGrid)
	{
	    /** Dump the grid rows. **/
	    sectionCnt = xaCount(&AGRID(tree->LayoutGrid)->Rows);
	    for(i=0;i<sectionCnt;i++)
		{
		    section = (pAposSection)xaGetItem(&AGRID(tree->LayoutGrid)->Rows, i);
		    printf("%*.*s        y=%d h=%d", indent*4, indent*4, "", section->StartLine->Loc, section->Width);
		    printf("\n");
		}
	
	    /** Dump the grid columns. **/
	    sectionCnt = xaCount(&AGRID(tree->LayoutGrid)->Cols);
	    for(i=0;i<sectionCnt;i++)
		{
		    section = (pAposSection)xaGetItem(&AGRID(tree->LayoutGrid)->Cols, i);
		    printf("%*.*s        x=%d w=%d", indent*4, indent*4, "", section->StartLine->Loc, section->Width);
		    printf("\n");
		}
	}
	
    /** Search for and dump children that are floating windows. **/
    childCnt = xaCount(&tree->Children);
    for(i=0;i<childCnt;i++)
	{
	child = (pWgtrNode)(xaGetItem(&tree->Children, i));
	if (!(child->Flags & WGTR_F_FLOATING))
	    aposDumpGrid(child, indent+1);
	}
    printf("%*.*s*** END %s ***\n", indent*4, indent*4, "", tree->Name);

return 0;
}

/*** Automatically positions widgets in a widget tree.
 ***
 *** Note: You can think of this as the main entry point for the file.
 ***
 *** @param tree The tree being autopositioned.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposAutoPositionWidgetTree(pWgtrNode tree)
{    
XArray PatchedWidgets;
int i=0, count=0;

    xaInit(&(PatchedWidgets),16);

    /** Handle children with unspecified heights. **/
    aposPrepareTree(tree, &PatchedWidgets);

    /** Recursively build the layout grids, including lines and sections, for this tree. **/
    if (aposBuildGrid(tree) < 0)
	{
	    mssError(0, "APOS", "aposAutoPositionWidgetTree: Couldn't build layout grid for '%s'", tree->Name);
	    return -1;
	}

    /** Set flexibilities on containers. **/
    if (aposSetFlexibilities(tree) < 0)
	{
	    return -1;
	}

    /*aposDumpGrid(tree, 0);*/

    /** Detect and honor minimum/maximum space requirements. **/
    if (aposSetLimits(tree) < 0)
	{
	    return -1;
	}
    
    /*aposDumpGrid(tree, 0);*/

    /**Iteration 2**/
    if(aposAutoPositionContainers(tree) < 0)
        {
	    mssError(0, "APOS", "aposAutoPositionWidgetTree: Couldn't auto-position contents of '%s'", tree->Name);
	    return -1;
	}
   
    /*aposDumpGrid(tree, 0);*/
    
    /** Release grid memory **/
    aposFreeGrids(tree);

    /**makes a final pass through the tree and processes html windows**/
    aposProcessWindows(tree, tree);
    
    /** Unpatches the heights specified by aposPrepareTree(). **/
    count=xaCount(&PatchedWidgets);
    for(i=0; i<count; ++i)
	{
	((pWgtrNode)xaGetItem(&PatchedWidgets, i))->height = -1;
	((pWgtrNode)xaGetItem(&PatchedWidgets, i))->pre_height = -1;
	}
    
    /** Free the PatchedWidgets XArray.**/
    xaDeInit(&PatchedWidgets);
    
    return 0;
}

/*** Recursively sets flexibility values for containers and their children.
 *** 
 *** @param Parent The parent node who's flexibilities are being set.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposSetFlexibilities(pWgtrNode Parent)
{
pAposGrid theGrid = AGRID(Parent->LayoutGrid);
pAposSection Sect;
pWgtrNode Child;
int i=0, childCount=xaCount(&(Parent->Children));
int sectCount;

    /** Check recursion. **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Recursively set the flexibilities of all children. **/
    for(i=0; i<childCount; ++i)
        {
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    if (aposSetFlexibilities(Child) < 0)
		{
		    return -1;
		}
	}

    /** Reset flexibility values in the grid. **/
    if (theGrid && childCount > 0 && !(Parent->Flags & WGTR_F_NONVISUAL))
	{
	    sectCount = xaCount(&(theGrid->Rows));
	    for(i=0;i<sectCount;i++)
		{
		    Sect = (pAposSection)xaGetItem(&theGrid->Rows, i);
		    aposSetSectionFlex(Sect, APOS_ROW);
		}
	    sectCount = xaCount(&(theGrid->Cols));
	    for(i=0;i<sectCount;i++)
		{
		    Sect = (pAposSection)xaGetItem(&theGrid->Cols, i);
		    aposSetSectionFlex(Sect, APOS_COL);
		}
	}

    /** Set the flexibility of the given container, if it is visual. **/
    if(!(Parent->Flags & WGTR_F_NONVISUAL))
	if(aposSetContainerFlex(Parent) < 0)
	    {
		mssError(0, "APOS", "aposPrepareTree: Couldn't set %s's flexibility", Parent->Name);
		return -1;
	    }

    return 0;
}

/*** Adjusts space to acomodate children, somehow? I think?
 ***
 *** @param Parent  The widget node parent who's limits are being calculated.
 *** @param delta_w The change in width required to accomodate children.
 *** @param delta_h The change in height required to accomodate children.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposSetLimits_r(pWgtrNode Parent, int* delta_w, int* delta_h)
{
int childCount;
int child_delta_w, child_delta_h;
int total_child_delta_w, total_child_delta_h;
int i;
int sectionCount;
pAposSection s;
pWgtrNode Child;

    /** Check recursion. **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Calculate the total required space for children. **/
    childCount = xaCount(&(Parent->Children));
    total_child_delta_w = total_child_delta_h = 0;
    for(i=0;i<childCount;i++)
	{
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    if (aposSetLimits_r(Child, &child_delta_w, &child_delta_h) < 0)
		return -1;
	    total_child_delta_w += child_delta_w;
	    total_child_delta_h += child_delta_h;
	}

    /*** If space was added or subtracted for children, use that. Otherwise,
     *** calculate and use the space added or subtracted by the parent widget
     *** itself.
     ***
     *** It seems like this makes containers copy the total values of their
     *** children, but noncontainers calculate their own values.
     ***/
    if (total_child_delta_w || total_child_delta_h)
	{
	    *delta_w = total_child_delta_w;
	    *delta_h = total_child_delta_h;
	}
    else
	{
	    *delta_w = 0;
	    *delta_h = 0;
	    if (Parent->LayoutGrid)
		{
		    sectionCount = xaCount(&(AGRID(Parent->LayoutGrid)->Rows));
		    for(i=0;i<sectionCount;i++)	// Loop through each row.
			{
			    s = (pAposSection)(xaGetItem(&(AGRID(Parent->LayoutGrid)->Rows), i));
			    /*** If it has a desired width, increase the height
			     *** enough to give it that width.
			     ***/
			    if (s->DesiredWidth >= 0)
				{
				    *delta_h += (s->DesiredWidth - s->Width);
				    /*printf("Changing height of row section at %d from %d to %d\n", s->StartLine->Loc, s->Width, s->DesiredWidth);*/
				    s->Width = s->DesiredWidth;
				}
			}
		    sectionCount = xaCount(&(AGRID(Parent->LayoutGrid)->Cols));
		    for(i=0;i<sectionCount;i++)
			{
			    s = (pAposSection)(xaGetItem(&(AGRID(Parent->LayoutGrid)->Cols), i));
			    /*** If it has a desired width, increase the width
			     *** enough to give it that width.
			     ***/
			    if (s->DesiredWidth >= 0)
				{
				    *delta_w += (s->DesiredWidth - s->Width);
				    /*printf("Changing width of column section at %d from %d to %d\n", s->StartLine->Loc, s->Width, s->DesiredWidth);*/
				    s->Width = s->DesiredWidth;
				}
			}
		}
	}

    /** If there is extra space, expand this widget to fill that space. **/
    if (*delta_w)
	{
	    if (Parent->StartVLine && ALINE(Parent->StartVLine)->SSection)
		{
		    ALINE(Parent->StartVLine)->SSection->Width += *delta_w;
		}

	    Parent->pre_width += *delta_w;
	    /*printf("Adjusting width of %s by %d\n", Parent->Name, *delta_w);*/
	}
    if (*delta_h)
	{
	    if (Parent->StartHLine && ALINE(Parent->StartHLine)->SSection)
		{
		    ALINE(Parent->StartHLine)->SSection->Width += *delta_h;
		}

	    Parent->pre_height += *delta_h;
	    /*printf("Adjusting height of %s by %d\n", Parent->Name, *delta_h);*/
	}

    return 0;
}

/*** Adjusts space to acomodate children, somehow? I think?
 ***
 *** @param Parent  The widget node parent who's limits are being calculated.
 *** @returns 0, success.
 ***/
int
aposSetLimits(pWgtrNode Parent)
{
int delta_w, delta_h;
int rval;

    delta_w = 0;
    delta_h = 0;

    rval = aposSetLimits_r(Parent, &delta_w, &delta_h);

    return 0;
}

/*** Patch children of the given Parent node with unspecified heights. Searches
 *** recursively within containers. Patched children are logged in the given
 *** PatchedWidgets array.
 *** 
 *** @param Parent         The parent node who's childen should be patched.
 *** @param PatchedWidgets The widget children which have been patched.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposPrepareTree(pWgtrNode Parent, pXArray PatchedWidgets)
{
pWgtrNode Child;
int i=0, childCount=xaCount(&(Parent->Children));

    /** Check recursion. **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Loop through each child. **/
    for(i=0; i<childCount; ++i)
        {
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    /*** If a visual child has an unspecified height, patch it, unless it is in a scrollpane
	     *** Remember here that strcmp() returns 0 (false) if the strings are equal.
	     ***/
	    if((Child->height < 0) && !(Child->Flags & WGTR_F_NONVISUAL) && 
	        strcmp(Parent->Type, "widget/scrollpane"))
	        aposPatchNegativeHeight(Child, PatchedWidgets);
	    
	    /** If child is a container, but not a floating window, recursively prepare it as well. **/
	    if((Child->Flags & WGTR_F_CONTAINER) && !(Child->Flags & WGTR_F_FLOATING))
		if (aposPrepareTree(Child, PatchedWidgets) < 0)
		    return -1;
	}
    
    return 0;
}

/*** Try to guess the height of a widget with an unspecified height.
 *** 
 *** @param Widget         The widget child who's height is unspecified.
 *** @param PatchedWidgets The array to add the widget to after patching it.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposPatchNegativeHeight(pWgtrNode Widget, pXArray PatchedWidgets)
{
ObjData val;

    /** Try to guess the height based on the type of widget. **/
    if(!strcmp(Widget->Type, "widget/editbox"))
	{
	    Widget->height = 16;
	}
    else if(!strcmp(Widget->Type, "widget/textbutton"))
	{
	    wgtrGetPropertyValue(Widget, "text", DATA_T_STRING, &val);
	    Widget->height = 13.0 * (0.5 + (float)(strlen(val.String) * 7) / (float)(Widget->width));
	    if(Widget->height < 20) Widget->height = 20;
	}
    else if(!strcmp(Widget->Type, "widget/treeview"))
	{
	    Widget->height = 100;
	}
    else if(!strcmp(Widget->Type, "widget/html"))
	{
	    Widget->height = Widget->width;
	}
    else if(!strcmp(Widget->Type, "widget/dropdown"))
	{
	    /*if(xaCount(&(Widget->Children)) < 4) 
	         Widget->height = 40+16*xaCount(&(Widget->Children));
	    else Widget->height = 104;*/
	    Widget->height = Widget->min_height;
	}
    else if(!strcmp(Widget->Type, "widget/table"))
	{
	    Widget->height = 100;
	}
    else if(!strcmp(Widget->Type, "widget/menu"))
	{
	    Widget->height = 20;
	}
    else
	{
	return 0;
	}

    /** Add the widget to the provided array. **/
    xaAddItem(PatchedWidgets, Widget);

    /** Overwrite the "prepositioning" height because it's most likely also invalid. **/
    Widget->pre_height = Widget->height;

    return 0;
}

/*** Calculates and sets the flexibility for a container by taking weighted
 *** averages in each direction.
 *** 
 *** @param W The container to be set.
 *** @returns 0, success.
 ***/
int
aposSetContainerFlex(pWgtrNode W)
{
pAposGrid theGrid = AGRID(W->LayoutGrid);
pAposSection Sect;
int i=0, sectCount=0, TotalWidth=0, ProductSum=0;

    if (!theGrid) return 0;

    /** Calculate average row flexibility, weighted by height. **/
    /** Note: Height is called width because rows are 1 dimentional. **/
    sectCount = xaCount(&(theGrid->Rows));
    for(i=0; i<sectCount; ++i)
        {
	    Sect = (pAposSection)xaGetItem(&(theGrid->Rows), i);
	    TotalWidth += Sect->Width;
	    ProductSum += Sect->Flex * Sect->Width;
        }
    if(TotalWidth) W->fl_height = APOS_FUDGEFACTOR + (float)ProductSum / (float)TotalWidth;
    
    TotalWidth = ProductSum = 0;
    
    /** Calculate average column flexibility, weighted by width. **/
    sectCount = xaCount(&(theGrid->Cols));
    for(i=0; i<sectCount; ++i)
        {
	    Sect = (pAposSection)xaGetItem(&(theGrid->Cols), i);
	    TotalWidth += Sect->Width;
	    ProductSum += Sect->Flex * Sect->Width;
        }
    if(TotalWidth) W->fl_width = APOS_FUDGEFACTOR + (float)ProductSum / (float)TotalWidth;

    return 0;
}

/*** Determines offset booleans for scrollpanes, windows, and tabs. Only
 *** requested data is calculated. If an argument is NULL, the function skips
 *** calculating that value. The contract assumes pWgtrNode W is not NULL.
 *** 
 *** isSP (is scrollpane) is used to compensate for scrollpane scrollbars.
 *** isWin (is window) is used to compensate for the titlebar (if any).
 *** isTopTab and isSideTab are used to compensate for tabs, if they exist.
 *** 
 *** Remember: A variable with a name that starts with "is" (e.g. isSP)
 *** 	     pretty much always represents a boolean.
 *** 
 *** @param W         The widget Node to inspect.
 *** @param isSP      Is scrollpane, used to compensate for scrollpane scrollbars.
 *** @param isWin     Is window, used to compensate for the titlebar (if any).
 *** @param isTopTab  Recieves a boolean that is true if this node is a top tab.
 *** @param isSideTab Recieves a boolean that is true if this node is a side tab.
 *** @param tabWidth  Int pointer to recieve the tab width.
 *** @returns 0, success.
 ***/
int
aposSetOffsetBools(pWgtrNode W, int *isSP, int *isWin, int *isTopTab, int *isSideTab, int *tabWidth)
{
ObjData val;

    /** Set isSP to compensate for scrollpane scrollbars. **/
    if(isSP) *isSP = (!strcmp(W->Type, "widget/scrollpane"));
    
    /** Set isWin to compensate windows' titlebars, if any. **/
    if(isWin && !strcmp(W->Type, "widget/childwindow"))
        {
	    /*** Set isWin (is window) to compensate for a titlebar. If the
	     *** node does not specify if it has a titlebar, assume it does.
	     ***/
	    if(wgtrGetPropertyValue(W, "titlebar", DATA_T_STRING, &val) < 0)
	        *isWin = 1;	// Property not found, assume it has a titlebar.
	    else *isWin = !strcmp(val.String, "yes");
	}
    
    /** isTopTab and isSideTab are used to compensate for tabs. **/
    if(isTopTab && !strcmp(W->Type, "widget/tab"))
        {
	    /*** Set isTopTab and isSideTab. If the node does not specify the
	     *** tab location, assume it has a top tab and leave side-tab unset.
	     **/
	    if(wgtrGetPropertyValue(W, "tab_location", DATA_T_STRING, &val) < 0)
	        *isTopTab = 1;	// Property not found, assume it has a top tab only.
	    else 
		{
		    *isTopTab = (!strcmp(val.String, "top") || !strcmp(val.String, "bottom"));
		    *isSideTab = (!strcmp(val.String, "left") || (!strcmp(val.String, "right"))); // Warning: Unchecked assignment.
		}
		    
	    /** Set the tab width. If none is specified, default to 80. **/
	    if(wgtrGetPropertyValue(W, "tab_width", DATA_T_INTEGER, &val) < 0)
		*tabWidth = 80;
	    else *tabWidth = val.Integer;
	}
	
    return 0;
}

/*** Builds the layout grid for recursively for this container and all of its
 *** children, including the lines and sections required for positioning.
 *** 
 *** @param Parent The parent node who's grid is being built.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposBuildGrid(pWgtrNode Parent)
{
int childCount, i;
pWgtrNode Child;
pAposGrid theGrid = NULL;

    /** Check recursion. **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Allocate a grid. **/
    if (Parent->Flags & WGTR_F_CONTAINER)
	{
	    if (!(Parent->Flags & WGTR_F_NONVISUAL) || !Parent->Parent)
		{
		    /** Allocate and initialize a new pAposGrid. **/
		    theGrid = Parent->LayoutGrid = (pAposGrid)nmMalloc(sizeof(AposGrid));
		    if (!Parent->LayoutGrid) goto error;
		    aposInitiallizeGrid(theGrid);

		    /** Add lines for children to the grid. **/
		    if(aposAddLinesToGrid(Parent, &(theGrid->HLines), &(theGrid->VLines)) < 0)
			{
			    mssError(0, "APOS", "aposBuildGrid: Couldn't add lines to %s's grid",
				    Parent->Name);
			    return -1;
			}

		    /** Add the sections to the grid. **/
		    if(aposAddSectionsToGrid(theGrid, 
			    (Parent->height-Parent->pre_height), 
			    (Parent->width-Parent->pre_width)) < 0)
			{
			    mssError(0, "APOS", "aposBuildGrid: Couldn't add rows and columns to %s's grid",
				    Parent->Name);
			    return -1;
			}
		}
    
	    /** Recursively build this grid for all children of this widget. **/
	    childCount = xaCount(&(Parent->Children));
	    for(i=0; i<childCount; ++i)
		{
		    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
		    
		    /**auto-positions subsequent visual container**/
		    if(!(Child->Flags & WGTR_F_FLOATING))
			if(aposBuildGrid(Child) < 0)
			    {
				/*mssError(0, "APOS", "aposBuildGrid: Couldn't build layout grid for '%s'", Child->Name);*/
				return -1;
			    }
		}
	}

    return 0;

error:
    return -1;
}

/*** Recursively auto-positions containers and their children based on their grids.
 *** 
 *** Note: Assumes that the grild was already built with a call to aposBuildGrid().
 *** 
 *** @param Parent The parent node who's containers are being autopositioned.
 *** @returns 0 if successful, -1 otherwise.
 *** 
 *** @see aposBuildGrid()
 ***/
int
aposAutoPositionContainers(pWgtrNode Parent)
{
pAposGrid theGrid;
pWgtrNode Child;
int i=0, childCount = xaCount(&(Parent->Children));
int rows_extra=0, cols_extra=0;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Grid is pre-built by aposBuildGrid **/
    theGrid = (pAposGrid)(Parent->LayoutGrid);

    /** Autoposition anything with a grid setup **/
    if (theGrid)
	{
	    /**Adjust the spaces between lines to fit the grid to the container**/
	    if (!(Parent->Flags & WGTR_F_VSCROLLABLE))
		rows_extra = aposSpaceOutLines(&(theGrid->HLines), &(theGrid->Rows), (Parent->height - Parent->pre_height));	//rows
	    if (!(Parent->Flags & WGTR_F_HSCROLLABLE))
		cols_extra = aposSpaceOutLines(&(theGrid->VLines), &(theGrid->Cols), (Parent->width - Parent->pre_width));	 //columns
	    
	    /**modify the widgets' x,y,w, and h values to snap to their adjusted lines**/
	    if (!(Parent->Flags & WGTR_F_VSCROLLABLE))
		aposSnapWidgetsToGrid(&(theGrid->HLines), APOS_ROW);	//rows
	    if (!(Parent->Flags & WGTR_F_HSCROLLABLE))
		aposSnapWidgetsToGrid(&(theGrid->VLines), APOS_COL);	//columns

	    /** did not resize? **/
	    /*if (rows_extra < 0)
		Parent->height += -rows_extra;
	    if (cols_extra < 0)
		Parent->width += -cols_extra;*/
	}
    
    /**recursive call to auto-position subsequent visual containers, except windows**/
    for(i=0; i<childCount; ++i)
	{
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    /**looks under nonvisual containers for more visual containers**/
	    if(aposAutoPositionContainers(Child) < 0)
		{
		    mssError(0, "APOS", "aposAutoPositionContainers: Couldn't auto-position contents of '%s'", Child->Name);
		    return -1;
		}
	}

    return 0;
}

/*** Frees memory used by all grids in the widget tree.
 *** 
 *** @param tree The tree containing the grids to be freed.
 *** @returns 0, success.
 ***/
int
aposFreeGrids(pWgtrNode tree)
{
int childCount, i;
pWgtrNode Child;

    /** Recursively deallocate memory and deinit XArrays in the grid. **/
    childCount = xaCount(&(tree->Children));
    for(i=0;i<childCount;i++)
	{
	    Child = (pWgtrNode)xaGetItem(&(tree->Children), i);
	    aposFreeGrids(Child);
	}
    if (tree->LayoutGrid) 
	{
	    aposFree(AGRID(tree->LayoutGrid));
	    nmFree(AGRID(tree->LayoutGrid), sizeof(AposGrid));
	    tree->LayoutGrid = NULL;
	}

    return 0;
}

/*** Initialize an AposGrid with empty arrays.
 *** 
 *** @param theGrid The grid to be initialized.
 *** @returns 0, success.
 ***/
int
aposInitiallizeGrid(pAposGrid theGrid)
{
    xaInit(&(theGrid->Rows),16);
    xaInit(&(theGrid->Cols),16);
    xaInit(&(theGrid->HLines),16);
    xaInit(&(theGrid->VLines),16);

    return 0;
}

/*** Adds 4 border lines around the edges of the grid. Then recursively adds 4
 *** lines for each visual child. Searches nonvisual containers recursively for
 *** qualifying grandchildren. Floating windows are ignored.
 *** 
 *** Scrollpanes recieve only 2 vertical lines (skipping their horizontal edges),
 *** and if the parent node is a scrollpane, the horizontal border lines are also
 *** skipped.
 *** 
 *** The HLines and VLines arrays will be populated with the generated horizontal
 *** and vertical lines respectively. The SWidgets, CWidgets, and EWidgets arrays
 *** are also properly populated with the widgets that start, cross, and end each
 *** line respectively.
 *** 
 *** @param Parent The parent who's grid is being populated with lines.
 *** @param HLines The array of horizontal lines, populated by this function.
 *** @param VLines The array of vertical lines, populated by this function.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposAddLinesToGrid(pWgtrNode Parent, pXArray HLines, pXArray VLines)
{
int i=0, count=0, isWin=0, isSP=0, height_adj=0, width_adj=0;
pAposLine CurrLine, PrevLine;
pXArray FirstCross, LastCross;

    /** Check if this node a scrollbar or window that needs an offset. **/
    aposSetOffsetBools(Parent, &isSP, &isWin, NULL, NULL, NULL);

    /** Does this widget need more room than it was given? **/
    if (Parent->pre_width < Parent->min_width && Parent->min_width != 0)
	width_adj = Parent->min_width - Parent->pre_width;
    if (Parent->pre_height < Parent->min_height && Parent->min_height != 0)
	height_adj = Parent->min_height - Parent->pre_height;

    /** Add the 2 horizontal border lines, unless parent is a scrollpane. **/
    if(strcmp(Parent->Type, "widget/scrollpane"))
        {
	    int minHeightLoc = 0, maxHeightLoc = Parent->pre_height - isWin * 24;
	    if(aposCreateLine(NULL, HLines, minHeightLoc, APOS_NOT_LINKED, APOS_IS_BORDER, 0, APOS_HORIZONTAL) < 0)
	        goto CreateLineError;
	    if(aposCreateLine(NULL, HLines, maxHeightLoc, APOS_NOT_LINKED, APOS_IS_BORDER, height_adj, APOS_HORIZONTAL) < 0)
	        goto CreateLineError;
        }

    /** Add the 2 vertical border lines. **/
    int minWidthLoc = 0, maxWidthLoc = (Parent->pre_width-isSP*18);
    if(aposCreateLine(NULL, VLines, minWidthLoc, APOS_NOT_LINKED, APOS_IS_BORDER, 0, APOS_VERTICAL) < 0)
        goto CreateLineError;
    if(aposCreateLine(NULL, VLines, maxWidthLoc, APOS_NOT_LINKED, APOS_IS_BORDER, width_adj, APOS_VERTICAL) < 0)
        goto CreateLineError;

    /** Recursively add the nonborder lines for all child nodes. **/
    if(aposAddLinesForChildren(Parent, HLines, VLines) < 0)
	goto CreateLineError;

    /** Record the widgets that cross each horizontal line in its CWidgets XArray. **/
    count = xaCount(HLines);
    for(i=1; i<count; ++i)	// Loop through all horizontal lines, skipping the borderline.
        {
	    CurrLine = (pAposLine)xaGetItem(HLines, i);
	    PrevLine = (pAposLine)xaGetItem(HLines, (i-1));
	    aposFillInCWidget(&(PrevLine->SWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	    aposFillInCWidget(&(PrevLine->CWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	}

    /** Record the widgets that cross each vertical line in its CWidgets XArray. **/
    count = xaCount(VLines);
    for(i=1; i<count; ++i)	// Loop through all vertical lines, skipping the borderline.
        {
	    CurrLine = (pAposLine)xaGetItem(VLines, i);
	    PrevLine = (pAposLine)xaGetItem(VLines, (i-1));
	    aposFillInCWidget(&(PrevLine->SWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	    aposFillInCWidget(&(PrevLine->CWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	}
	
    /** Sanity check to make sure no widgets cross the border lines. **/
    if(xaCount(HLines))	// Only check borderlines if they exist.
	{
	    FirstCross = &(((pAposLine)xaGetItem(HLines, 0))->CWidgets);
	    LastCross  = &(((pAposLine)xaGetItem(HLines, (xaCount(HLines)-1)))->CWidgets);
	    /*if(xaCount(FirstCross))
		mssError(1, "APOS", "%d widget(s) crossed the top borderline, including %s '%s'", xaCount(FirstCross),
		    ((pWgtrNode)xaGetItem(FirstCross, 0))->Type, ((pWgtrNode)xaGetItem(FirstCross, 0))->Name);
	    if(xaCount(LastCross))
		mssError(1, "APOS", "%d widget(s) crossed the bottom borderline, including %s '%s'", xaCount(LastCross),
		    ((pWgtrNode)xaGetItem(LastCross, 0))->Type, ((pWgtrNode)xaGetItem(LastCross, 0))->Name);*/
	}
	
    FirstCross = &(((pAposLine)xaGetItem(VLines, 0))->CWidgets);
    LastCross  = &(((pAposLine)xaGetItem(VLines, (xaCount(VLines)-1)))->CWidgets);
    /*if(xaCount(FirstCross))
	mssError(1, "APOS", "%d widget(s) crossed the left borderline, including %s '%s'", xaCount(FirstCross), 
	    ((pWgtrNode)xaGetItem(FirstCross, 0))->Type, ((pWgtrNode)xaGetItem(FirstCross, 0))->Name);
    if(xaCount(LastCross))
	mssError(1, "APOS", "%d widget(s) crossed the right borderline, including %s '%s'", xaCount(LastCross), 
	    ((pWgtrNode)xaGetItem(LastCross, 0))->Type, ((pWgtrNode)xaGetItem(LastCross, 0))->Name);*/

    return 0;
    
    CreateLineError:
        mssError(0, "APOS", "aposAddLinesToGrid: Couldn't create a border line");
        return -1;
}

/*** Adds 4 lines for the edges of each visual child. Searches nonvisual
 *** containers recursively for qualifying grandchildren. Floating windows
 *** are ignored. Scrollpanes recieve only 2 vertical lines (skipping their
 *** horizontal edges).
 *** 
 *** @param Parent The parent who's children are being given lines.
 *** @param HLines The array to which horizontal lines should be added.
 *** @param VLines The array to which vertical lines should be added.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposAddLinesForChildren(pWgtrNode Parent, pXArray HLines, pXArray VLines)
{
int i=0, childCount=xaCount(&(Parent->Children));
int isTopTab=0, isSideTab=0, tabWidth=0;
int height_adj, width_adj;
pWgtrNode C;

    /** Check recursion. **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Loop through the children and create 4 lines for each child's 4 edges. **/
    for(i=0; i<childCount; ++i)
        {
	    C = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    aposSetOffsetBools(C, NULL, NULL, &isTopTab, &isSideTab, &tabWidth);
	    
	    /** Does this widget need more room than it was given? **/
	    height_adj = width_adj = 0;
	    if (C->pre_width < C->min_width && C->min_width != 0)
		width_adj = C->min_width - C->pre_width;
	    if (C->pre_height < C->min_height && C->min_height != 0)
		height_adj = C->min_height - C->pre_height;

	    /** If the child (C) is a nonvisual container, recursively add lines for any grandchildren. **/
	    if((C->Flags & WGTR_F_NONVISUAL) && (C->Flags & WGTR_F_CONTAINER))
		{
		    if (aposAddLinesForChildren(C, HLines, VLines) < 0)
			goto CreateLineError;
		}
	    /** Otherwise, if child (C) is visual and not a floating window, add 4 lines for it. **/
	    else if(!(C->Flags & WGTR_F_NONVISUAL) && !(C->Flags & WGTR_F_FLOATING))
	        {
		    /** Add horizontal lines, unless parent is a scrollpane. **/
		    if(strcmp(Parent->Type, "widget/scrollpane"))
			{
			    /*** From this code, we see that the start line is
			     *** always the minY, and the end of the line is
			     *** always the maxY. Thus, the top line is the
			     *** start line and the bottom line is the end line
			     *** because Y increases as we decend the page.
			     ***/
			    int minY = (C->y), maxY = (C->y + C->height + isTopTab*24);
			    if(aposCreateLine(C, HLines, minY, APOS_SWIDGETS, APOS_NOT_BORDER, 0, APOS_HORIZONTAL) < 0)
			        goto CreateLineError;
			    if(aposCreateLine(C, HLines, maxY, APOS_EWIDGETS, APOS_NOT_BORDER, height_adj, APOS_HORIZONTAL) < 0)
			        goto CreateLineError;
			}
		    
		    /** Add vertical lines. **/
		    /*** From this code, we see that the start line is always
		     *** the minX, and the end of the line is always the maxX.
		     *** Thus, the left line is the start line and the right
		     *** line is the end line because X increases as we move
		     *** right along the page.
		     ***/
		    int minX = (C->x), maxX = (C->x + C->width + isSideTab*tabWidth);
		    if(aposCreateLine(C, VLines, minX, APOS_SWIDGETS, APOS_NOT_BORDER, 0, APOS_VERTICAL) < 0)
			goto CreateLineError;
		    if(aposCreateLine(C, VLines, maxX, APOS_EWIDGETS, APOS_NOT_BORDER, width_adj, APOS_VERTICAL) < 0)
			goto CreateLineError;
	        }
	}
    
    return 0;
    
    CreateLineError:
    mssError(0, "APOS", "aposAddLinesForChildren: Couldn't create a new line");
    return -1;
}

/*** Creates a new line in the grid or updates an existing line if it exists
 *** in the same location. Remember that lines record the widgets that start
 *** on them (SWidgets), end on them (EWidgets), and cross them (CWidgets).
 *** 
 *** Note: This function all lines in the given array are oriented in the same
 *** 	 direction as the new line. At the time of this writing (June 2025),
 *** 	 all known calling functions upheld by maintaining an HLines and a
 *** 	 VLines array to store horizontal and vertical lines separately.
 *** 
 *** @param Widget   The widget which determined the location of this line,
 *** 		   which we add to the SWidgets or EWidgets array.
 *** @param Lines    The array that stores the lines.
 *** @param Loc      The location of the line. Only a single coordinate in
 *** 		   one dimension is needed since lines span the entire grid.
 *** @param type     The type, indicating whether the associated widget starts
 *** 		   or ends on this line.
 *** @param isBorder A boolean that is true if this is a grid border line.
 *** @param adj      An adjustment added to or subtracted from the line to
 *** 		   satisfy min or max constraints (respectively).
 *** @param is_vert  A boolean that is true if this line is vertical.
 *** 		   See APOS_VERTICAL and APOS_HORIZONTAL.
 *** 
 *** @returns 0, success.
 ***/
int
aposCreateLine(pWgtrNode Widget, pXArray Lines, int Loc, int type, int isBorder, int adj, int is_vert)
{
pAposLine Line = aposExistingLine(Lines, Loc);
    
    /** If there is already a line, we upgrade it instead of creating a new one. **/
    if(Line != NULL)	
        {

	    /** Change the line position Adjustment value if needed **/
	    if (Line->Adj != 0 && adj > Line->Adj)
		Line->Adj = adj;
	    else if (Line->Adj == 0 && adj)
		Line->Adj = adj;

	}
    else
	{
    
	    /** There's not already a line, so we allocate a new one. **/    
	    if((Line = (pAposLine)nmMalloc(sizeof(AposLine))) < 0)
		{
		    mssError(1, "APOS", "aposCreateLine: Couldn't allocate memory for new grid line");
		    return -1;
		}
	    
	    /** Initiallize the new line. **/
	    memset(Line, 0, sizeof(AposLine));
	    xaInit(&(Line->SWidgets),16);
	    xaInit(&(Line->EWidgets),16);
	    xaInit(&(Line->CWidgets),16);
	    Line->Loc = Loc;
	    Line->isBorder = isBorder;
	    Line->Adj = adj;
	    Line->SSection = NULL;
	    Line->ESection = NULL;

	    /** Add the new line, to the list of lines, sorted by location. **/
	    xaAddItemSortedInt32(Lines, Line, 0);
	}

    /** Link the line and the widget together. **/
    if (type == APOS_SWIDGETS)
	{
	    xaAddItem(&(Line->SWidgets), Widget);

	    if (is_vert)
		Widget->StartVLine = Line;
	    else
		Widget->StartHLine = Line;
	}
    else if (type == APOS_EWIDGETS) 
	{
	    xaAddItem(&(Line->EWidgets), Widget);

	    if (is_vert)
		Widget->EndVLine = Line;
	    else
		Widget->EndHLine = Line;
	}
    
    return 0;
}

/*** Gets a line from the array at the location, or returns NULL if none exists.
 ***
 *** Note: This function all lines in the given array are oriented in the same
 *** 	 direction. This is not tested, although at the time of this writing
 *** 	 (June, 2025), all calling functions upheld this contract.
 *** 
 *** @param Lines The array of lines to search.
 *** @param Loc	The location to check for a line.
 *** @returns A pointer to the line, if it exists, and NULL otherwise.
 ***/
pAposLine
aposExistingLine(pXArray Lines, int Loc)
{
/** loops through all the lines in the given array, returning NULL if no line with
*** the given location can be found, or a pointer to the line if it is found. **/
int i, count = xaCount(Lines);

    for(i=0; i<count; ++i)
	if(((pAposLine)(xaGetItem(Lines, i)))->Loc == Loc)
	    return (pAposLine)xaGetItem(Lines, i);
    
    return NULL;
}

/*** Detects if a widget in PrevList (usually the widgets that started in or
 *** crossed the pevious line) ends on this line (aka. appears in EWidgets).
 *** If it does not end on this line, we know it crosses this line, so we add
 *** the widget to CWidgets.
 *** 
 *** @param PrevList The list of previous widgets being checked.
 *** @param EWidgets The list of widgets ending on the line in question.
 *** @param CWidgets The list to which widgets that cross should be added.
 *** @returns 0, success.
 ***/
int
aposFillInCWidget(pXArray PrevList, pXArray EWidgets, pXArray CWidgets)
{
pWgtrNode AddCandidate;
int found=0, i=0, j=0, pCount=xaCount(PrevList), eCount=xaCount(EWidgets);

    /*** Loop through the array from the previous line.
     *** Note: Could be that line's SWidgets OR CWidgets.
     **/
    for(i=0; i<pCount; ++i)
	{
	    AddCandidate = (pWgtrNode)xaGetItem(PrevList, i);

	    /*** Search through the list of widgets ending on this line,
	     *** looking for the current widget.
	     ***/
	    found = 0;
	    for(j=0; j<eCount; ++j)
		if(AddCandidate == (pWgtrNode)xaGetItem(EWidgets, j))
		    {
			/*** The widget was found in the EWidgets array.
			 *** Thus, it ends on this line and does not cross it.
			 ***/
			found = 1;
			break;
		    }

	    /*** If a match was NOT found among EWidgets, the widget must
	     *** continue across the line, so add it to CWidgets.
	     ***/
	    if(!found) xaAddItem(CWidgets, AddCandidate);
	}
    return 0;
}

/*** Adds row and column sections to the grid based on the lines.
 *** 
 *** @param theGrid The grid to which sections shoudl be added.
 *** @param VDiff   I had a hard time figuring out what this means.
 *** @param HDiff   I had a hard time figuring out what this means.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposAddSectionsToGrid(pAposGrid theGrid, int VDiff, int HDiff)
{
int count=0, i=0;
    
    /** Add rows sections between horizontal lines. **/
    count = xaCount(&(theGrid->HLines));
    for(i=1; i<count; ++i)
	if(aposCreateSection(&(theGrid->Rows), ((pAposLine)xaGetItem(&(theGrid->HLines),(i-1))),
	    ((pAposLine)xaGetItem(&(theGrid->HLines),(i))), VDiff, APOS_ROW) < 0)
	    {
		mssError(1, "APOS", "aposCreateSection(): Couldn't create a new row or column");
		return -1;
	    }
    
    /** Add column sections between vertical lines. **/
    count = xaCount(&(theGrid->VLines));
    for(i=1; i<count; ++i)
	if(aposCreateSection(&(theGrid->Cols), ((pAposLine)xaGetItem(&(theGrid->VLines),(i-1))),  
	    ((pAposLine)xaGetItem(&(theGrid->VLines),(i))), HDiff, APOS_COL) < 0)
	    {
		mssError(1, "APOS", "aposCreateSection(): Couldn't create a new row or column");
		return -1;
	    }

    return 0;
}

/*** Calculate and set the flexibility value for a section. Spacers have 0 flex
 *** and containers use the flex of their least flexible children.
 *** 
 *** @param sect The section being set.
 *** @param type The type of section (either APOS_ROW or APOS_COL).
 *** @returns 0 if successful or -1 if a default value should be used instead
 ***/
int
aposSetSectionFlex(pAposSection sect, int type)
{
/*** Note:
 *** sCount + cCount includes all widgets intersecting this section because a
 *** widget cannot begin inside a section. It always starts or eds at the edge
 *** of a section.
 ***/
int sCount = xaCount(&(sect->StartLine->SWidgets));
int cCount = xaCount(&(sect->StartLine->CWidgets));

    /** Set flex to 0 if the section is a spacer or contains non-flexible
    *** children, otherwise set it to the minimum of the children. If none
    *** of those apply it must be a wide, widgetless gap. In this case,
    *** return -1 to prompt the caller to determine a default flexibility.
    ***/
    if((sect->isSpacer) || (aposNonFlexChildren(sect->StartLine, type)))
        sect->Flex = 0;
    else if(sCount || cCount) 
        sect->Flex = aposMinimumChildFlex(sect->StartLine, type);
    else
	return -1;

    return 0;
}

/*** Creates a new row or column section between two lines in the grid.
 *** 
 *** @param Sections The array of sections, to which this section will be added.
 *** @param StartL   The line which starts this section (typically the top/left line).
 *** @param EndL     The line which ends this section (typically the bottom/right line).
 *** @param Diff     I had a hard time figuring out what this means.
 *** @param type     Whether the section is a row (APOS_ROW) or a column (APAS_COL).
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposCreateSection(pXArray Sections, pAposLine StartL, pAposLine EndL, int Diff, int type)
{
pAposSection NewSect;
    
    /** Allocate and initiallize a new section. **/
    if((NewSect = (pAposSection)(nmMalloc(sizeof(AposSection)))) < 0)
	{
	    mssError(1, "APOS", "nmMalloc(): Couldn't allocate memory for new row or column");
	    return -1;
	}
    memset(NewSect, 0, sizeof(AposSection));
    NewSect->StartLine = StartL;
    NewSect->EndLine = EndL;
    NewSect->Width = (EndL->Loc - StartL->Loc);
    NewSect->isBorder = (StartL->isBorder || EndL->isBorder);
    NewSect->isSpacer = aposIsSpacer(StartL, EndL, type, NewSect->isBorder);
    NewSect->DesiredWidth = -1;
    StartL->SSection = NewSect;
    EndL->ESection = NewSect;

    /** Apply the adjustment from the end line, if needed. **/
    if (EndL->Adj)
	NewSect->DesiredWidth = NewSect->Width + EndL->Adj;
   
    /** Set section flexibility. **/
    if (aposSetSectionFlex(NewSect, type) < 0)
	{
	if (Diff < 0)
	    NewSect->Flex = APOS_CGAPFLEX;
	else
	    NewSect->Flex = APOS_EGAPFLEX;
	}

    xaAddItem(Sections, NewSect);
    
    return 0;
}

/*** Determines if a section between two lines is a spacer.
 ***
 *** If a section is a spacer, the assumption is that the designer probably put
 *** that space there to provide visual breathing room in their design. Thus, we
 *** should avoid resizing it as this may interfere with their design.
 *** 
 *** @param StartL   The line starting the section. (I think this is always the left/top.)
 *** @param EndL     The line starting the section. (I think this is always the right/bottom.)
 *** @param type     Whether the section is a row (APOS_ROW) or a column (APAS_COL).
 *** @param isBorder Whether the section is on the border of the page.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposIsSpacer(pAposLine StartL, pAposLine EndL, int type, int isBorder)
{
pWgtrNode SW, EW;
int i=0, j=0;
/** @brief The number of widgets starting at the end of this section.**/
int sCount=xaCount(&(EndL->SWidgets)); 
/** @brief The number of widgets ending at the start of this section.**/
int eCount=xaCount(&(StartL->EWidgets));

    if((EndL->Loc - StartL->Loc) <= APOS_MINSPACE)	// If section is sufficiently narrow.
	{
	    /** Gaps between the border and any widget(s) are spacers. **/
	    if(isBorder && (sCount || eCount)) return 1;
	    
	    /** Checks every widget ending on one side to see if a widget
	    *** starts directly across from it on the other side.
	    ***/
	    for(i=0; i<sCount; ++i)
	        {
		    SW = (pWgtrNode)(xaGetItem(&(EndL->SWidgets), i));
		    for(j=0; j<eCount; ++j)
		        {
			    EW = (pWgtrNode)(xaGetItem(&(StartL->EWidgets), j));
			    
			    /** If a corner of the widget on one side falls
			    *** between the two corners of a widget on the
			    *** other side, this is a spacer.
			    ***/
			    if((type == APOS_ROW) && (((EW->x >= SW->x) && (EW->x < (SW->x + SW->width))) || 
				(((EW->x + EW->width) > SW->x) && ((EW->x + EW->width) <= (SW->x + SW->width)))))
				return 1;
			    
			    else if((type == APOS_COL) && (((EW->y >= SW->y) && (EW->y < (SW->y + SW->height))) || 
				(((EW->y + EW->height) > SW->y) && ((EW->y + EW->height) <= (SW->y + SW->height)))))
				return 1;
			}
		}
	}
	
    return 0;
}

/*** Checks for any widgets starting on or crossing a line that are non-flexible
 *** in the relevant dimention.
 ***
 *** @param L    The line along which to check.
 *** @param type Specifies the relevant dimetion using APOS_ROW or APOS_COL.
 *** @returns    1 if any child widget is non-flexible in the relevant dimension,
 ***             0 otherwise.
 ***/
int
aposNonFlexChildren(pAposLine L, int type)
{
int i=0;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /*** Return 1 if the widgets starting on or crossing the given line have
     *** children that are completely non-flexible.
     ***/
    if(type == APOS_ROW)
        {
	    for(i=0; i<sCount; ++i) 
	        if(((pWgtrNode)(xaGetItem(&(L->SWidgets), i)))->fl_height == 0)
		    return 1;
	    for(i=0; i<cCount; ++i)	
	        if(((pWgtrNode)(xaGetItem(&(L->CWidgets), i)))->fl_height == 0)
		    return 1;
        }
    else // type == APOS_COL
        {
	    for(i=0; i<sCount; ++i)
	        if(((pWgtrNode)(xaGetItem(&(L->SWidgets), i)))->fl_width == 0)
		    return 1;
	    for(i=0; i<cCount; ++i)
	        if(((pWgtrNode)(xaGetItem(&(L->CWidgets), i)))->fl_width == 0)
		    return 1;
	}
    
    return 0;
}

/*** Calculates the average flexibility of widgets on a line.
 ***
 *** @param L    The line along which to check.
 *** @param type Specifies the relevant dimetion using APOS_ROW or APOS_COL.
 *** @returns    The average flexibility of children on the line.
 ***/
int
aposAverageChildFlex(pAposLine L, int type)
{
int TotalFlex=0, i=0;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /** Sum the flexibilities. **/
    if(type == APOS_ROW)
        {
	    for(i=0; i<sCount; ++i)
	        TotalFlex += ((pWgtrNode)xaGetItem(&(L->SWidgets), i))->fl_height;
	    for(i=0; i<cCount; ++i)
	        TotalFlex += ((pWgtrNode)xaGetItem(&(L->CWidgets), i))->fl_height;
        }
    else //type == APOS_COL
        {
	    for(i=0; i<sCount; ++i)
	        TotalFlex += ((pWgtrNode)xaGetItem(&(L->SWidgets), i))->fl_width;
	    for(i=0; i<cCount; ++i)
	        TotalFlex += ((pWgtrNode)xaGetItem(&(L->CWidgets), i))->fl_width;
	}
    
    /** Return the average flexibility with an aditional fudge factor. **/
    return (int)(APOS_FUDGEFACTOR + ((float)TotalFlex)/((float)sCount+(float)cCount));
}

/*** Calculates the minimum flexibility of widgets on a line.
 ***
 *** @param L    The line along which to check.
 *** @param type Specifies the relevant dimetion using APOS_ROW or APOS_COL.
 *** @returns    The minimum flexibility of children on the line.
 ***/
int
aposMinimumChildFlex(pAposLine L, int type)
{
int MinFlex=100, i=0, f;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /** Find the min flexibility. **/
    if(type == APOS_ROW)
        {
	    for(i=0; i<sCount; ++i)
		{
		f = ((pWgtrNode)xaGetItem(&(L->SWidgets), i))->fl_height;
		if (MinFlex > f) MinFlex = f;
		}
	    for(i=0; i<cCount; ++i)
		{
		f = ((pWgtrNode)xaGetItem(&(L->CWidgets), i))->fl_height;
		if (MinFlex > f) MinFlex = f;
		}
        }
    else //type == APOS_COL
        {
	    for(i=0; i<sCount; ++i)
		{
		f = ((pWgtrNode)xaGetItem(&(L->SWidgets), i))->fl_width;
		if (MinFlex > f) MinFlex = f;
		}
	    for(i=0; i<cCount; ++i)
		{
		f = ((pWgtrNode)xaGetItem(&(L->CWidgets), i))->fl_width;
		if (MinFlex > f) MinFlex = f;
		}
	}
    
    /** Return the minimum flexibility. **/
    return MinFlex;
}

/*** Distributes extra or missing space among grid lines based on section flexibility.
 *** 
 *** @param Lines The array of lines in the relevant direction on this grid.
 *** @param Secctions The array of sections in the relevant direction on this grid.
 *** @param Diff The space differencce from how the elements are currently spaced.
 *** @returns The remaining space difference after spacing out elements as much as possible.
 ***/
int
aposSpaceOutLines(pXArray Lines, pXArray Sections, int Diff)
{
pAposLine CurrLine, PrevLine;
pAposSection PrevSect, CurrSect;
int TotalFlex=0, TotalFlexibleSpace=0, Adj=0, i=0, Extra=0, count=xaCount(Sections);
int FlexibleSections=0;
float FlexWeight=0, SizeWeight=0;
float TotalSum=0;

    /** If there are no sections, we have nothing to space out. **/
    if(!count) return Diff;
    
    /**Sum the flexibilities of the sections**/
    for(i=0; i<count; ++i)
	{
	    CurrSect = ((pAposSection)xaGetItem(Sections, i));
	    TotalFlex += CurrSect->Flex;
	    if(CurrSect->Flex)
		{
		    FlexibleSections++;
		    TotalFlexibleSpace += CurrSect->Width;
		}
	    else CurrSect->AdjWeight = 0.0f;
	}
    
    /*** If there is no flexibility (no expansion or contraction), we can't
     *** space anything out. Return the original difference so this can be
     *** spaced out elsewhere.
     ***/
    if(TotalFlex == 0) return Diff;
    
    /** sets each line's location equal to the previous line's location
    *** plus the adjusted width of the preceding section **/
    count = xaCount(Lines);
    for(i=1; i<count; ++i)
	{
	    PrevSect = (pAposSection)xaGetItem(Sections, (i-1));
	    FlexWeight = (float)(PrevSect->Flex) / (float)(TotalFlex);
	    SizeWeight = (FlexWeight > 0) ? (float)(PrevSect->Width) / (float)(TotalFlexibleSpace) : 0;

	    TotalSum += (FlexWeight * SizeWeight);
	}

    /** The initial borders do not adjust.**/
    pAposLine leftBorder = (pAposLine)xaGetItem(Lines, 0);
    leftBorder->LocAdjWeight = leftBorder->MyAdjWeight = 0.0f;

    for(i=1; i<count; ++i)	//starts at i=1 to skip the first borderline
        {
	    CurrLine = (pAposLine)xaGetItem(Lines, i);
	    PrevLine = (pAposLine)xaGetItem(Lines, (i-1));
	    PrevSect = (pAposSection)xaGetItem(Sections, (i-1));
	    FlexWeight = (float)(PrevSect->Flex) / (float)(TotalFlex);
	    SizeWeight = 0;
	    
	    /** unless there's at least some flexibility, don't factor in size **/
	    if(FlexWeight > 0)
	        SizeWeight = (float)(PrevSect->Width) / (float)(TotalFlexibleSpace);

	    /*** Calculate the adjustment weight, and also save it so we can
	     *** replicate some of the following logic in the CSS we will
	     *** eventually send to the client.
	     ***/
	    float AdjWeight = PrevSect->AdjWeight = (float)(FlexWeight*SizeWeight)/TotalSum;
	    
	    /**for expanding lines**/
	    if(Diff > 0)
	        {
		    /** Calculate adjustment using the adjustment weight. **/
		    Adj = (float)(Diff) * AdjWeight + APOS_FUDGEFACTOR;
		    
		    /** Store the line adjustment weight for responsive CSS later.**/
		    CurrLine->LocAdjWeight = PrevLine->LocAdjWeight + AdjWeight;
		    CurrLine->MyAdjWeight = AdjWeight;

		    /** Apply the calculated adjustment.**/
		    CurrLine->Loc = PrevLine->Loc + PrevSect->Width + Adj;
		    PrevSect->Width += Adj;
		}
	    /**for contracting lines**/
	    else if(Diff < 0)
		{
		    /** Calculate adjustment using the adjustment weight. **/
		    Adj = (float)(Diff) * AdjWeight - APOS_FUDGEFACTOR;
		    
		    /** if the section width will be unacceptably 
		    *** narrow or negative after the adjustment **/
		    if((Adj + PrevSect->Width) <= APOS_MINSPACE)
			{
			    /** if its width was unacceptable to begin with, pass the
			    *** entire adjustment on to the next recursive call **/
			    if(PrevSect->Width <= APOS_MINSPACE)
			        {
				    Extra += Adj;	
				    CurrLine->Loc = PrevLine->Loc + PrevSect->Width;
				}
			    /** if its width was acceptable before, pass only
			    *** the difference on to the next recursive call **/
			    else
			        {
				    Extra += (PrevSect->Width + Adj) - APOS_MINSPACE;
				    CurrLine->Loc = PrevLine->Loc + APOS_MINSPACE;
				    PrevSect->Width = APOS_MINSPACE;
				}
			    /** Lock section in place for the next recursive call **/
			    PrevSect->isSpacer = 1;
			    PrevSect->Flex = 0;
			}
		    else
			{
			    CurrLine->Loc = PrevLine->Loc + PrevSect->Width + Adj;
			    PrevSect->Width += Adj;
			}
		}
	}

    /** Spread any homeless extra space out over the remaining flexible sections**/
    if(Extra < 0)
	return aposSpaceOutLines(Lines, Sections, Extra);
	
    return Extra;
}

/*** Adjusts widget positions and sizes to snap them to grid lines. This
 *** function should be called after updating grid lines to ensure that
 *** widgets properly reflect the changes.
 *** 
 *** @param Lines The lines being updated.
 *** @param flag Either APOS_ROW or APOS_COL.
 *** @returns 0, success.
 ***/
int
aposSnapWidgetsToGrid(pXArray Lines, int flag)
{
int i=0, j=0, count=0, lineCount = xaCount(Lines);
int isTopTab=0, isSideTab=0, tabWidth=0;
int newsize;
pAposLine CurrLine;
pWgtrNode Widget;

    for(i=0; i<lineCount; ++i)	//loop through all the lines in V/HLines
        {
	    CurrLine = (pAposLine)xaGetItem(Lines, i);
	    
	    /** Adjusts x or y of the widgets starting on this line**/
	    count = xaCount(&(CurrLine->SWidgets));
	    for(j=0; j<count; ++j)
	        {
	            Widget = (pWgtrNode)xaGetItem(&(CurrLine->SWidgets), j);
		    if(flag == APOS_ROW) {
			Widget->y = CurrLine->Loc;
			Widget->yAdjWeight = CurrLine->LocAdjWeight;
		    }
		    else {
			Widget->x = CurrLine->Loc;
			Widget->xAdjWeight = CurrLine->LocAdjWeight;
		    }
		}
	    
	    /** Adjusts width or height of widgets ending on this line **/
	    count = xaCount(&(CurrLine->EWidgets));
	    for(j=0; j<count; ++j)
	        {
	            Widget = (pWgtrNode)xaGetItem(&(CurrLine->EWidgets), j);
		    aposSetOffsetBools(Widget, NULL, NULL, &isTopTab, &isSideTab, &tabWidth);
		    if(flag==APOS_ROW  &&  Widget->fl_height)
			{
			    /** Calculate the new size, taking APOS_MINWIDTH into account.**/
			    newsize = CurrLine->Loc - Widget->y - isTopTab*24;
			    if (newsize < APOS_MINWIDTH && Widget->pre_height >= APOS_MINWIDTH)
				Widget->height = APOS_MINWIDTH;
			    else if(newsize >= APOS_MINWIDTH || newsize >= Widget->pre_height)
				Widget->height = newsize;
			    else 
				/*Widget->height = APOS_MINWIDTH;*/
				Widget->height = Widget->pre_height;
			
			    /*** The widget copies the adjustment weight of the
			     *** line, ignoring APOS_MINWIDTH. This might lead
			     *** to problems down the road, but I plan to fix
			     *** them if and when I encounter them.
			     ***/
			    Widget->hAdjWeight = CurrLine->MyAdjWeight;
			}
		    else if(flag==APOS_COL  &&  Widget->fl_width)
		        {
			    /** Calculate the new size, taking APOS_MINWIDTH into account.**/
			    newsize = CurrLine->Loc - Widget->x - isSideTab*tabWidth;
			    if (newsize < APOS_MINWIDTH && Widget->pre_width >= APOS_MINWIDTH)
				Widget->width = APOS_MINWIDTH;
			    else if (newsize >= APOS_MINWIDTH || newsize >= Widget->pre_width)
				Widget->width = newsize;
			    else
				/*Widget->width = APOS_MINWIDTH;*/
				Widget->width = Widget->pre_width;

			    /*** The widget copies the adjustment weight of the
			     *** line, ignoring APOS_MINWIDTH. This might lead
			     *** to problems down the road, but I plan to fix
			     *** them if and when I encounter them.
			     ***/
			    Widget->hAdjWeight = CurrLine->MyAdjWeight;
			}
		}
	}
	
    return 0;
}

/***
 *** Processes floating windows and recursively positions visual and
 *** nonvisual containers.
 ***
 *** @param VisualRef The last visual container up the inheritance tree.
 *** @param Parent The widget being scanned for windows.
 *** @returns 0 if successful, -1 otherwise.
 ***/
int
aposProcessWindows(pWgtrNode VisualRef, pWgtrNode Parent)
{
int i=0, changed=0, isWin=0, isSP=0;
int childCount=xaCount(&(Parent->Children));
pWgtrNode Child;
int rw, rh, rpw, rph;
char* val;
int ival;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    aposSetOffsetBools(Parent, &isSP, &isWin, NULL, NULL, NULL);

    /**loop through children and process any windows**/
    for(i=0; i<childCount; ++i)
	{
	    Child = (pWgtrNode)(xaGetItem(&(Parent->Children), i));
	    if(Child->Flags & WGTR_F_FLOATING)
		{

		    /** Top level or local reference for container? **/
		    if ((wgtrGetPropertyValue(Child, "toplevel", DATA_T_STRING, POD(&val)) == 0 && (!strcasecmp(val, "yes") || !strcasecmp(val, "true") || !strcasecmp(val, "1") || !strcmp(val, "on"))) || (wgtrGetPropertyValue(Child, "toplevel", DATA_T_INTEGER, POD(&ival)) == 0 && ival == 1))
			{
			rpw = rw = Parent->Root->ClientInfo->AppMaxWidth;
			rph = rh = Parent->Root->ClientInfo->AppMaxHeight;
			}
		    else
			{
			rw = VisualRef->width;
			rh = VisualRef->height;
			rpw = VisualRef->pre_width;
			rph = VisualRef->pre_height;
			}

		    /** auto-detect centered floating objects **/
		    if (abs(Child->pre_x - (rpw - (Child->pre_x + Child->pre_width))) < 10)
			{
			    Child->x = (rw - Child->width)/2;
			    if (Child->x < 0) Child->x = 0;
			    changed = 1;
			}
		    if (abs(Child->pre_y - (rph - (Child->pre_y + Child->pre_height))) < 10)
			{
			    Child->y = (rh - Child->height)/2;
			    if (Child->y < 0) Child->y = 0;
			    changed = 1;
			}

		    /**if it's larger than its container, shrink it and set flag**/
		    if(Child->width > (rw - isSP*18))
		        {
			    Child->width = (rw - isSP*18);
			    changed = 1;
			}
		    if(Child->height > (rh - isWin*24))
			{
			    Child->height = (rh - isWin*24);
			    changed = 1;
			}
		    
		    /**if the window changed width or height, process it like a widget tree**/
		    //if(changed) 
		    aposAutoPositionWidgetTree(Child);
		    /*Child->width = Child->pre_width;
		    Child->height = Child->pre_height;*/

		    /**if it's outside the top left corner pull the whole window in**/
		    if(Child->x < 0) Child->x = 0;
		    if(Child->y < 0) Child->y = 0;
		    
		    /**if it's outside the bottom right corner, pull the whole window in**/
		    if((Child->x + Child->width) > (rw - isSP*18))
			Child->x = (rw - isSP*18) - Child->width;
		    if((Child->y + Child->height) > (rh - isWin*24))
			Child->y = (rh - isWin*24) - Child->height;
		}
	    
	    /**recursive call on visual containers; new visual reference is passed**/
	    if((Child->Flags & WGTR_F_CONTAINER) && !(Child->Flags & WGTR_F_NONVISUAL))
		{
		    if (aposProcessWindows(Child, Child) < 0)
			return -1;
		}
	    
	    /**recursive call on nonvisual containers; old visual reference is maintained**/
	    if((Child->Flags & WGTR_F_CONTAINER) && (Child->Flags & WGTR_F_NONVISUAL))
		{
		    if (aposProcessWindows(VisualRef, Child) < 0)
			return -1;
		}
	
	}
	
    return 0;
}

/*** Frees all memory used by a grid, including its lines and sections.
 *** 
 *** @param theGrid The grid being freed.
 ***/
int
aposFree(pAposGrid theGrid)
{
int count=0, i=0;
pAposLine Line;

    /** free rows **/
    count = xaCount(&(theGrid->Rows));
    for(i=0; i<count; ++i) 
	nmFree((pAposSection)(xaGetItem(&(theGrid->Rows), i)), sizeof(AposSection));

    /** free columns **/
    count = xaCount(&(theGrid->Cols));
    for(i=0; i<count; ++i)
	nmFree((pAposSection)(xaGetItem(&(theGrid->Cols), i)), sizeof(AposSection));
    
    /** free horizontal lines **/
    count = xaCount(&(theGrid->HLines));
    for(i=0; i<count; ++i)
	{
	    Line = (pAposLine)(xaGetItem(&(theGrid->HLines), i));
	    xaDeInit(&(Line->SWidgets));
	    xaDeInit(&(Line->EWidgets));
	    xaDeInit(&(Line->CWidgets));
	    nmFree((pAposLine)(xaGetItem(&(theGrid->HLines), i)), sizeof(AposLine));
	}
    
    /** free vertical lines **/
    count = xaCount(&(theGrid->VLines));
    for(i=0; i<count; ++i)
	{
	    Line = (pAposLine)(xaGetItem(&(theGrid->VLines), i));
	    xaDeInit(&(Line->SWidgets));
	    xaDeInit(&(Line->EWidgets));
	    xaDeInit(&(Line->CWidgets));
	    nmFree((pAposLine)(xaGetItem(&(theGrid->VLines), i)), sizeof(AposLine));
	}
    
    /** DeInit XArrays **/
    xaDeInit(&(theGrid->Rows));
    xaDeInit(&(theGrid->Cols));
    xaDeInit(&(theGrid->HLines));
    xaDeInit(&(theGrid->VLines));
    
    return 0;
}
