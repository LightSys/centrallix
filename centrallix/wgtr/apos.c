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
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: apos.c,v 1.14 2008/03/04 01:21:11 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/wgtr/apos.c,v $

    $Log: apos.c,v $
    Revision 1.14  2008/03/04 01:21:11  gbeeley
    - (bugfix) various fixes to the auto-resizing (apos) code, related to
      having two nonvisual containers nested directly inside each other
      (e.g., an osrc and then a form).
    - (bugfix) removed height-extending logic in ProcessWindows which is
      incorrect.  However, various issues with min_height et al will need
      to be addressed at some point.
    - (bugfix) changes to hbox/vbox (autolayout) fix problems wrapping a
      set of widgets around to a new row (hbox) or a new column (vbox).

    Revision 1.13  2007/09/18 17:38:49  gbeeley
    - (feature) stubbing out multiscroll widget.
    - (change) adding capability to auto-position module to allow a container
      to not resize its contents (horiz and/or vert) if the contents can be
      scrolled.

    Revision 1.12  2007/07/31 18:08:47  gbeeley
    - (bugfix) when centering a childwindow (or other floating widget), do not
      make the upper left corner go offscreen if the screen is too small.

    Revision 1.11  2007/04/03 15:50:04  gbeeley
    - (feature) adding capability to pass a widget to a component as a
      parameter (by reference).
    - (bugfix) changed the layout logic slightly in the apos module to better
      handle ratios of flexibility and size when resizing.

    Revision 1.10  2007/03/22 16:29:28  gbeeley
    - (feature) Autolayout widget, better known as hbox and vbox.  Now you
      don't have to manually compute all those X's and Y's!  Only hbox and
      vbox supported right now; other layouts are planned (any takers?)
    - (bugfix) cond_add_children with condition=false on conditional rendering
      now compensates for x/y container offset of nonrendered widget.
    - (change) allow drv->New code in wgtr to have access to the properties
      for the given widget, by moving the ->New call later in the parse-open-
      widget process.

    Revision 1.9  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.8  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.7  2006/10/16 18:34:34  gbeeley
    - (feature) ported all widgets to use widget-tree (wgtr) alone to resolve
      references on client side.  removed all named globals for widgets on
      client.  This is in preparation for component widget (static and dynamic)
      features.
    - (bugfix) changed many snprintf(%s) and strncpy(), and some sprintf(%.<n>s)
      to use strtcpy().  Also converted memccpy() to strtcpy().  A few,
      especially strncpy(), could have caused crashes before.
    - (change) eliminated need for 'parentobj' and 'parentname' parameters to
      Render functions.
    - (change) wgtr port allowed for cleanup of some code, especially the
      ScriptInit calls.
    - (feature) ported scrollbar widget to Mozilla.
    - (bugfix) fixed a couple of memory leaks in allocated data in widget
      drivers.
    - (change) modified deployment of widget tree to client to be more
      declarative (the build_wgtr function).
    - (bugfix) removed wgtdrv_templatefile.c from the build.  It is a template,
      not an actual module.

    Revision 1.6  2006/10/04 17:20:50  gbeeley
    - (feature) allow application to adjust to user agent's configured text
      font size.  Especially the Mozilla versions in CentOS have terrible
      line spacing problems.
    - (feature) to allow the above, added minimum widget height management to
      the auto-layout module (apos)
    - (change) allow floating windows to grow in size if more room is needed
      inside the window.
    - (change) for auto-layout, go with the minimum flexibility in any row or
      column rather than the average.  Not sure of all of the impact of
      doing this.

    Revision 1.5  2006/04/07 06:48:34  gbeeley
    - (feature) if a floating object (window) is centered in the original
      layout, re-center it when resizing things.

    Revision 1.4  2005/10/18 22:47:12  gbeeley
    - (change) use r_width/r_height instead of the minimums if possible

    Revision 1.3  2005/10/09 07:51:29  gbeeley
    - (change) popup menus are floating objects like windows.
    - (change) allow geometry values to be unset
    - (change) add Parent property to WgtrNode

    Revision 1.2  2005/10/01 00:23:46  gbeeley
    - (change) renamed 'htmlwindow' to 'childwindow' to remove the terminology
      dependence on the dhtml/http app delivery mechanism

    Revision 1.1  2005/08/10 16:26:49  ncolson
    Initial commit of the auto-positioning module.
    This code is run during the verification of the widget tree in wgtr.c. It's
    purpose is to make one final pass through the tree to assess the dimensions,
    types, and structure of the widgets within, and adjust the size and position of
    each of them in order to scale the layout of the application up or down as is
    necessary to fit a desired browser window size. This process is guided by the
    flexibility property of each widget, which specifies how much space a widget
    can absorb or give up, as well as the size, individual properties, and
    orientation of each widget relative to all the other widgets.

 **END-CVSDATA***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apos.h"
#include "cxlib/xarray.h"
#include "cxlib/datatypes.h"

int
aposInit()
{
    nmRegister(sizeof(AposGrid), "AposGrid");
    nmRegister(sizeof(AposSection), "AposSection");
    nmRegister(sizeof(AposLine), "AposLine");
    
    return 0;
}

int
aposDumpGrid(pWgtrNode tree, int indent)
{
int i, childCnt, sectionCnt;
pAposSection section;
pWgtrNode child;

    printf("%*.*s*** %s ***\n", indent*4, indent*4, "", tree->Name);
    if (tree->LayoutGrid)
	{
	    sectionCnt = xaCount(&AGRID(tree->LayoutGrid)->Rows);
	    for(i=0;i<sectionCnt;i++)
		{
		    section = (pAposSection)xaGetItem(&AGRID(tree->LayoutGrid)->Rows, i);
		    printf("%*.*s        y=%d h=%d", indent*4, indent*4, "", section->StartLine->Loc, section->Width);
		    printf("\n");
		}
	    sectionCnt = xaCount(&AGRID(tree->LayoutGrid)->Cols);
	    for(i=0;i<sectionCnt;i++)
		{
		    section = (pAposSection)xaGetItem(&AGRID(tree->LayoutGrid)->Cols, i);
		    printf("%*.*s        x=%d w=%d", indent*4, indent*4, "", section->StartLine->Loc, section->Width);
		    printf("\n");
		}
	}
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

int
aposAutoPositionWidgetTree(pWgtrNode tree)
{    
XArray PatchedWidgets;
int i=0, count=0;

    xaInit(&(PatchedWidgets),16);

    /** Preliminary adjustments before BuildGrid is done **/
    aposPrepareTree(tree, &PatchedWidgets);

    /** Build the layout grids **/
    if (aposBuildGrid(tree) < 0)
	{
	    mssError(0, "APOS", "aposAutoPositionWidgetTree: Couldn't build layout grid for '%s'", tree->Name);
	    return -1;
	}

    /** Set flexibilities on containers **/
    if (aposSetFlexibilities(tree) < 0)
	{
	    return -1;
	}

    /*aposDumpGrid(tree, 0);*/

    /** Honor minimum/maximum space requirements **/
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
    
    /**unpatches all of the heights that were specified in aposPrepareTree**/
    count=xaCount(&PatchedWidgets);
    for(i=0; i<count; ++i)
	{
	((pWgtrNode)xaGetItem(&PatchedWidgets, i))->height = -1;
	((pWgtrNode)xaGetItem(&PatchedWidgets, i))->pre_height = -1;
	}
    
    xaDeInit(&PatchedWidgets);
    
    return 0;
}


int
aposSetFlexibilities(pWgtrNode Parent)
{
pAposGrid theGrid = AGRID(Parent->LayoutGrid);
pAposSection Sect;
pWgtrNode Child;
int i=0, childCount=xaCount(&(Parent->Children));
int sectCount;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}


    for(i=0; i<childCount; ++i)
        {
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    if (aposSetFlexibilities(Child) < 0)
		{
		    return -1;
		}
	}

    /** Reset flexibility values in the grid **/
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

    /**set the flexibility of the given container, if it is visual**/
    if(!(Parent->Flags & WGTR_F_NONVISUAL))
	if(aposSetContainerFlex(Parent) < 0)
	    {
		mssError(0, "APOS", "aposPrepareTree: Couldn't set %s's flexibility", Parent->Name);
		return -1;
	    }

    return 0;
}


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

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Figure what is needed for children **/
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

    /** If space add/sub for children, use that, otherwise go with
     ** space add/sub for this widget itself
     **/
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
		    for(i=0;i<sectionCount;i++)
			{
			    s = (pAposSection)(xaGetItem(&(AGRID(Parent->LayoutGrid)->Rows), i));
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
			    if (s->DesiredWidth >= 0)
				{
				    *delta_w += (s->DesiredWidth - s->Width);
				    /*printf("Changing width of column section at %d from %d to %d\n", s->StartLine->Loc, s->Width, s->DesiredWidth);*/
				    s->Width = s->DesiredWidth;
				}
			}
		}
	}

    /** Make space for this widget bigger **/
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


int
aposPrepareTree(pWgtrNode Parent, pXArray PatchedWidgets)
{
pWgtrNode Child;
int i=0, childCount=xaCount(&(Parent->Children));

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    for(i=0; i<childCount; ++i)
        {
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    /**if a visual child has an unspecified height, patch it, unless it is a scrollpane**/
	    if((Child->height < 0) && !(Child->Flags & WGTR_F_NONVISUAL) && 
	        strcmp(Parent->Type, "widget/scrollpane"))
	        aposPatchNegativeHeight(Child, PatchedWidgets);
	    
	    /** If child is a container but not a window, recursively prepare it as well **/
	    if((Child->Flags & WGTR_F_CONTAINER) && !(Child->Flags & WGTR_F_FLOATING))
		if (aposPrepareTree(Child, PatchedWidgets) < 0)
		    return -1;
	}
    
    return 0;
}

int
aposPatchNegativeHeight(pWgtrNode Widget, pXArray PatchedWidgets)
{
ObjData val;

    /** set unspecified height of widget to an educated guess**/
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

    xaAddItem(PatchedWidgets, Widget);
    Widget->pre_height = Widget->height;

    return 0;
}

int
aposSetContainerFlex(pWgtrNode W)
{
pAposGrid theGrid = AGRID(W->LayoutGrid);
pAposSection Sect;
int i=0, sectCount=0, TotalWidth=0, ProductSum=0;

    if (!theGrid) return 0;

    /**calculate average row flexibility, weighted by height **/
    sectCount = xaCount(&(theGrid->Rows));
    for(i=0; i<sectCount; ++i)
        {
	    Sect = (pAposSection)xaGetItem(&(theGrid->Rows), i);
	    TotalWidth += Sect->Width;
	    ProductSum += Sect->Flex * Sect->Width;
        }
    if(TotalWidth) W->fl_height = APOS_FUDGEFACTOR + (float)ProductSum / (float)TotalWidth;
    
    TotalWidth = ProductSum = 0;
    
    /**calculate average column flexibility, weighted by width **/
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

int
aposSetOffsetBools(pWgtrNode W, int *isSP, int *isWin, int *isTopTab, int *isSideTab, int *tabWidth)
{
ObjData val;

    /**set isSP to compensate for scrollpane scrollbars**/
    if(isSP) *isSP = (!strcmp(W->Type, "widget/scrollpane"));
    
    /**set isWin to compensate windows' titlebars, if any**/
    if(isWin && !strcmp(W->Type, "widget/childwindow"))
        {
	    if(wgtrGetPropertyValue(W, "titlebar", DATA_T_STRING, &val) < 0)
	        *isWin = 1;	//if property not found, assume it has a titlebar
	    else *isWin = !strcmp(val.String, "yes");
	}
    
    /**isTopTab and isSideTab are used to compensate for tabs**/
    if(isTopTab && !strcmp(W->Type, "widget/tab"))
        {
	    /**set isTopTab and isSideTab**/
	    if(wgtrGetPropertyValue(W, "tab_location", DATA_T_STRING, &val) < 0)
	        *isTopTab = 1;	//if property not found, assume top tab**/
	    else 
		{
		    *isTopTab = (!strcmp(val.String, "top") || !strcmp(val.String, "bottom"));
		    *isSideTab = (!strcmp(val.String, "left") || (!strcmp(val.String, "right")));
		}
		    
	    /**set tabWidth**/
	    if(wgtrGetPropertyValue(W, "tab_width", DATA_T_INTEGER, &val) < 0)
		*tabWidth = 80;
	    else *tabWidth = val.Integer;
	}
	
    return 0;
}


int
aposBuildGrid(pWgtrNode Parent)
{
int childCount, i;
pWgtrNode Child;
pAposGrid theGrid = NULL;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /** Allocate a grid **/
    if (Parent->Flags & WGTR_F_CONTAINER)
	{
	    if (!(Parent->Flags & WGTR_F_NONVISUAL) || !Parent->Parent)
		{
		    theGrid = Parent->LayoutGrid = (pAposGrid)nmMalloc(sizeof(AposGrid));
		    if (!Parent->LayoutGrid) goto error;

		    /**initiallize the XArrays in the grid**/
		    aposInitiallizeGrid(theGrid);

		    /**Add the lines to the grid**/
		    if(aposAddLinesToGrid(Parent, &(theGrid->HLines), &(theGrid->VLines)) < 0)
			{
			    mssError(0, "APOS", "aposBuildGrid: Couldn't add lines to %s's grid",
				    Parent->Name);
			    return -1;
			}

		    /**Add the sections to the grid**/
		    if(aposAddSectionsToGrid(theGrid, 
			    (Parent->height-Parent->pre_height), 
			    (Parent->width-Parent->pre_width)) < 0)
			{
			    mssError(0, "APOS", "aposBuildGrid: Couldn't add rows and columns to %s's grid",
				    Parent->Name);
			    return -1;
			}
		}
    
	    /** Do it for all children of this widget **/
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


int
aposFreeGrids(pWgtrNode tree)
{
int childCount, i;
pWgtrNode Child;

    /**deallocate memory and deinit XArrays in the grid**/
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


int
aposInitiallizeGrid(pAposGrid theGrid)
{
    xaInit(&(theGrid->Rows),16);
    xaInit(&(theGrid->Cols),16);
    xaInit(&(theGrid->HLines),16);
    xaInit(&(theGrid->VLines),16);

    return 0;
}

int
aposAddLinesToGrid(pWgtrNode Parent, pXArray HLines, pXArray VLines)
{
int i=0, count=0, isWin=0, isSP=0, height_adj=0, width_adj=0;
pAposLine CurrLine, PrevLine;
pXArray FirstCross, LastCross;

    aposSetOffsetBools(Parent, &isSP, &isWin, NULL, NULL, NULL);

    /** Does this widget need more room than it was given? **/
    if (Parent->pre_width < Parent->min_width && Parent->min_width != 0)
	width_adj = Parent->min_width - Parent->pre_width;
    if (Parent->pre_height < Parent->min_height && Parent->min_height != 0)
	height_adj = Parent->min_height - Parent->pre_height;

    /**Add the 2 horizontal border lines, unless parent is a scrollpane**/
    if(strcmp(Parent->Type, "widget/scrollpane"))
        {
	    if(aposCreateLine(NULL, HLines, 0, 0, 1, 0, 0) < 0)
	        goto CreateLineError;
	    if(aposCreateLine(NULL, HLines, (Parent->pre_height-isWin*24), 0, 1, height_adj, 0) < 0)
	        goto CreateLineError;
        }
    /**Add the 2 vertical border lines**/
    if(aposCreateLine(NULL, VLines, 0, 0, 1, 0, 1) < 0)
        goto CreateLineError;
    if(aposCreateLine(NULL, VLines, (Parent->pre_width-isSP*18), 0, 1, width_adj, 1) < 0)
        goto CreateLineError;

    if(aposAddLinesForChildren(Parent, HLines, VLines) < 0)
	goto CreateLineError;

    /**populate horizontal line cross XArrays**/
    count = xaCount(HLines);
    for(i=1; i<count; ++i)	//loop through all horizontal lines, skip the borderline
        {
	    CurrLine = (pAposLine)xaGetItem(HLines, i);
	    PrevLine = (pAposLine)xaGetItem(HLines, (i-1));
	    aposFillInCWidget(&(PrevLine->SWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	    aposFillInCWidget(&(PrevLine->CWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	}

    /**populate vertical line cross XArrays**/
    count = xaCount(VLines);
    for(i=1; i<count; ++i)	//loop through all vertical lines, skip the borderline
        {
	    CurrLine = (pAposLine)xaGetItem(VLines, i);
	    PrevLine = (pAposLine)xaGetItem(VLines, (i-1));
	    aposFillInCWidget(&(PrevLine->SWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	    aposFillInCWidget(&(PrevLine->CWidgets), &(CurrLine->EWidgets), &(CurrLine->CWidgets));
	}
	
    /**sanity check to make sure no widgets cross the border lines**/
    if(xaCount(HLines))	//don't test borderlines unless they exist
	{
	    FirstCross = &(((pAposLine)xaGetItem(HLines, 0))->CWidgets);
	    LastCross  = &(((pAposLine)xaGetItem(HLines, (xaCount(HLines)-1)))->CWidgets);
	    if(xaCount(FirstCross))
		mssError(1, "APOS", "%d widget(s) crossed the top borderline, including %s '%s'", xaCount(FirstCross),
		    ((pWgtrNode)xaGetItem(FirstCross, 0))->Type, ((pWgtrNode)xaGetItem(FirstCross, 0))->Name);
	    if(xaCount(LastCross))
		mssError(1, "APOS", "%d widget(s) crossed the bottom borderline, including %s '%s'", xaCount(LastCross),
		    ((pWgtrNode)xaGetItem(LastCross, 0))->Type, ((pWgtrNode)xaGetItem(LastCross, 0))->Name);
	}
	
    FirstCross = &(((pAposLine)xaGetItem(VLines, 0))->CWidgets);
    LastCross  = &(((pAposLine)xaGetItem(VLines, (xaCount(VLines)-1)))->CWidgets);
    if(xaCount(FirstCross))
	mssError(1, "APOS", "%d widget(s) crossed the left borderline, including %s '%s'", xaCount(FirstCross), 
	    ((pWgtrNode)xaGetItem(FirstCross, 0))->Type, ((pWgtrNode)xaGetItem(FirstCross, 0))->Name);
    if(xaCount(LastCross))
	mssError(1, "APOS", "%d widget(s) crossed the right borderline, including %s '%s'", xaCount(LastCross), 
	    ((pWgtrNode)xaGetItem(LastCross, 0))->Type, ((pWgtrNode)xaGetItem(LastCross, 0))->Name);

    return 0;
    
    CreateLineError:
        mssError(0, "APOS", "aposAddLinesToGrid: Couldn't create a border line");
        return -1;
}

int
aposAddLinesForChildren(pWgtrNode Parent, pXArray HLines, pXArray VLines)
{
int i=0, childCount=xaCount(&(Parent->Children));
int isTopTab=0, isSideTab=0, tabWidth=0;
int height_adj, width_adj;
pWgtrNode C;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	    mssError(1,"APOS","Could not layout application: resource exhaustion occurred");
	    return -1;
	}

    /**loop through the children and create 4 lines for each child's 4 edges**/
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

	    /** If C is a nonvisual container, add lines for
	    *** the grandchildren. Otherwise, if C is visual
	    *** and not a window, just add 4 lines for it **/
	    if((C->Flags & WGTR_F_NONVISUAL) && (C->Flags & WGTR_F_CONTAINER))
		{
		    if (aposAddLinesForChildren(C, HLines, VLines) < 0)
			goto CreateLineError;
		}
	    else if(!(C->Flags & WGTR_F_NONVISUAL) && !(C->Flags & WGTR_F_FLOATING))
	        {
		    /**add horizontal lines, unless parent is a scrollpane**/
		    if(strcmp(Parent->Type, "widget/scrollpane"))
			{
			    if(aposCreateLine(C, HLines, (C->y), APOS_SWIDGETS, 0, 0, 0) < 0)
			        goto CreateLineError;
			    if(aposCreateLine(C, HLines, (C->y + C->height + isTopTab*24), APOS_EWIDGETS, 0, height_adj, 0) < 0)
			        goto CreateLineError;
			}
		    
		    /**add vertical lines**/
		    if(aposCreateLine(C, VLines, (C->x), APOS_SWIDGETS, 0, 0, 1) < 0)
			goto CreateLineError;
		    if(aposCreateLine(C, VLines, (C->x + C->width + isSideTab*tabWidth), APOS_EWIDGETS, 0, width_adj, 1) < 0)
			goto CreateLineError;
	        }
	}
    
    return 0;
    
    CreateLineError:
    mssError(0, "APOS", "aposAddLinesForChildren: Couldn't create a new line");
    return -1;
}

int
aposCreateLine(pWgtrNode Widget, pXArray Lines, int Loc, int type, int isBorder, int adj, int is_vert)
{
pAposLine Line = aposExistingLine(Lines, Loc);
    
    /**if there is already a line, just upgrade it**/
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
    
	    /**otherwise, create and add the new line**/    
	    if((Line = (pAposLine)nmMalloc(sizeof(AposLine))) < 0)
		{
		    mssError(1, "APOS", "aposCreateLine: Couldn't allocate memory for new grid line");
		    return -1;
		}
	    
	    /**initiallize new line**/
	    memset(Line, 0, sizeof(AposLine));
	    xaInit(&(Line->SWidgets),16);
	    xaInit(&(Line->EWidgets),16);
	    xaInit(&(Line->CWidgets),16);
	    Line->Loc = Loc;
	    Line->isBorder = isBorder;
	    Line->Adj = adj;
	    Line->SSection = NULL;
	    Line->ESection = NULL;

	    /**add new line, sorted by location**/
	    xaAddItemSortedInt32(Lines, Line, 0);
	}

    /** Link the line and the widget together **/
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

int
aposFillInCWidget(pXArray PrevList, pXArray EWidgets, pXArray CWidgets)
{
pWgtrNode AddCandidate;
int found=0, i=0, j=0, pCount=xaCount(PrevList), eCount=xaCount(EWidgets);

    /** loop through the SWidgets or CWidgets array of the previous line**/
    for(i=0; i<pCount; ++i)
	{
	    AddCandidate = (pWgtrNode)xaGetItem(PrevList, i);
	    found = 0;			//reset found
	    for(j=0; j<eCount; ++j)	//loop through the EWidgets array of the current line
		if(AddCandidate == (pWgtrNode)xaGetItem(EWidgets, j))
		    {
			found = 1;	//it was found in the EWidgets array, don't add
			break;
		    }
	    /** if a match wasn't found among EWidgets, the widget  
	    *** must continue across the line; add it to CWidgets **/
	    if(!found) xaAddItem(CWidgets, AddCandidate);
	}
    return 0;
}

int
aposAddSectionsToGrid(pAposGrid theGrid, int VDiff, int HDiff)
{
int count=0, i=0;
    
    /**Add rows**/
    count = xaCount(&(theGrid->HLines));
    for(i=1; i<count; ++i)
	if(aposCreateSection(&(theGrid->Rows), ((pAposLine)xaGetItem(&(theGrid->HLines),(i-1))),
	    ((pAposLine)xaGetItem(&(theGrid->HLines),(i))), VDiff, APOS_ROW) < 0)
	    {
		mssError(1, "APOS", "aposCreateSection(): Couldn't create a new row or column");
		return -1;
	    }
    
    /**Add columns**/
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

int
aposSetSectionFlex(pAposSection sect, int type)
{
int sCount = xaCount(&(sect->StartLine->SWidgets));
int cCount = xaCount(&(sect->StartLine->CWidgets));

    /** Set flex to 0 if the section is a spacer or contains non-flexible children, 
    *** otherwise set it to the average of the children. If none of those apply
    *** it must be a wide, widgetless gap, assign a default flexibility **/
    if((sect->isSpacer) || (aposNonFlexChildren(sect->StartLine, type)))
        sect->Flex = 0;
    else if(sCount || cCount) 
        sect->Flex = aposMinimumChildFlex(sect->StartLine, type);
    else
	return -1;

    return 0;
}

int
aposCreateSection(pXArray Sections, pAposLine StartL, pAposLine EndL, int Diff, int type)
{
pAposSection NewSect;
    
    /**Allocate and initiallize a new section**/
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

    /** Need to adjust section width/height? **/
    if (EndL->Adj)
	NewSect->DesiredWidth = NewSect->Width + EndL->Adj;
   
    /** Set section flexibility **/
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

int
aposIsSpacer(pAposLine StartL, pAposLine EndL, int type, int isBorder)
{
pWgtrNode SW, EW;
int i=0, j=0;
int sCount=xaCount(&(EndL->SWidgets)); 
int eCount=xaCount(&(StartL->EWidgets));

    if((EndL->Loc - StartL->Loc) <= APOS_MINSPACE)	//if section is sufficiently narrow
	{
	    /**gap between border and widget**/
	    if(isBorder && (sCount || eCount)) return 1;
	    
	    /** Checks every widget ending on one side of the section against 
	    *** every widget beginning on the other side to see if any of them 
	    *** are directly across from each other **/ 
	    for(i=0; i<sCount; ++i)
	        {
		    SW = (pWgtrNode)(xaGetItem(&(EndL->SWidgets), i));
		    for(j=0; j<eCount; ++j)
		        {
			    EW = (pWgtrNode)(xaGetItem(&(StartL->EWidgets), j));
			    
			    /** if a corner of a widget on one side of the 
			    *** section falls between the two corners of a widget 
			    *** on the other side, return 1 **/
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

int
aposNonFlexChildren(pAposLine L, int type)
{
int i=0;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /** returns 1 if the widgets starting on or crossing the given
    *** line have children that are completely non-flexible **/
    if(type == APOS_ROW)
        {
	    for(i=0; i<sCount; ++i) 
	        if(((pWgtrNode)(xaGetItem(&(L->SWidgets), i)))->fl_height == 0)
		    return 1;
	    for(i=0; i<cCount; ++i)	
	        if(((pWgtrNode)(xaGetItem(&(L->CWidgets), i)))->fl_height == 0)
		    return 1;
        }
    else //type == APOS_COL
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

int
aposAverageChildFlex(pAposLine L, int type)
{
int TotalFlex=0, i=0;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /** Sum the flexibilities of widgets within the section proceeding the line**/
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
    
    /**return average flexibility**/
    return (int)(APOS_FUDGEFACTOR + ((float)TotalFlex)/((float)sCount+(float)cCount));
}

int
aposMinimumChildFlex(pAposLine L, int type)
{
int MinFlex=100, i=0, f;
int sCount = xaCount(&(L->SWidgets));
int cCount = xaCount(&(L->CWidgets));

    /** Find the min flex within the section proceeding the line**/
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
    
    /**return min flexibility**/
    return MinFlex;
}

int
aposSpaceOutLines(pXArray Lines, pXArray Sections, int Diff)
{
pAposLine CurrLine, PrevLine;
pAposSection PrevSect, CurrSect;
int TotalFlex=0, TotalFlexibleSpace=0, Adj=0, i=0, Extra=0, count=xaCount(Sections);
int FlexibleSections=0;
float FlexWeight=0, SizeWeight=0;
float TotalSum=0;

    /**if there are no sections, don't bother going on**/
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
	}
    
    /** if there is no flexibility for expansion or contraction return 0**/
    if(TotalFlex == 0) return Diff;
    
    /** sets each line's location equal to the previous line's location
    *** plus the adjusted width of the preceding section **/
    count = xaCount(Lines);
    for(i=1; i<count; ++i)
	{
	    PrevSect = (pAposSection)xaGetItem(Sections, (i-1));
	    FlexWeight = (float)(PrevSect->Flex) / (float)(TotalFlex);
	    SizeWeight = 0;
	    if(FlexWeight > 0)
	        SizeWeight = (float)(PrevSect->Width) / (float)(TotalFlexibleSpace);

	    TotalSum += (FlexWeight * SizeWeight);
	}

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
	    
	    /**for expanding lines**/
	    if(Diff > 0)
	        {
		    /*Adj = APOS_FUDGEFACTOR + (float)(Diff) * ((float)(FlexWeight+SizeWeight)/2.0);*/
		    Adj = APOS_FUDGEFACTOR + (float)(Diff) * ((float)(FlexWeight*SizeWeight)/TotalSum);
		    CurrLine->Loc = PrevLine->Loc + PrevSect->Width + Adj;
		    PrevSect->Width += Adj;
		}
	    /**for contracting lines**/
	    else if(Diff < 0)
		{
		    /*Adj = (float)(Diff) * ((float)(FlexWeight+SizeWeight)/2.0) - APOS_FUDGEFACTOR;*/
		    Adj = (float)(Diff) * ((float)(FlexWeight*SizeWeight)/TotalSum) - APOS_FUDGEFACTOR;
		    
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
		    if(flag == APOS_ROW) Widget->y = CurrLine->Loc;
		    else Widget->x = CurrLine->Loc;
		}
	    
	    /** Adjusts width or height of widgets ending on this line **/
	    count = xaCount(&(CurrLine->EWidgets));
	    for(j=0; j<count; ++j)
	        {
	            Widget = (pWgtrNode)xaGetItem(&(CurrLine->EWidgets), j);
		    aposSetOffsetBools(Widget, NULL, NULL, &isTopTab, &isSideTab, &tabWidth);
		    if(flag==APOS_ROW  &&  Widget->fl_height)
			{
			    newsize = CurrLine->Loc - Widget->y - isTopTab*24;
			    if (newsize < APOS_MINWIDTH && Widget->pre_height >= APOS_MINWIDTH)
				Widget->height = APOS_MINWIDTH;
			    else if(newsize >= APOS_MINWIDTH || newsize >= Widget->pre_height)
				Widget->height = newsize;
			    else 
				/*Widget->height = APOS_MINWIDTH;*/
				Widget->height = Widget->pre_height;
			}
		    else if(flag==APOS_COL  &&  Widget->fl_width)
		        {
			    newsize = CurrLine->Loc - Widget->x - isSideTab*tabWidth;
			    if (newsize < APOS_MINWIDTH && Widget->pre_width >= APOS_MINWIDTH)
				Widget->width = APOS_MINWIDTH;
			    else if (newsize >= APOS_MINWIDTH || newsize >= Widget->pre_width)
				Widget->width = newsize;
			    else
				/*Widget->width = APOS_MINWIDTH;*/
				Widget->width = Widget->pre_width;
			}
		}
	}
	
    return 0;
}

int
aposProcessWindows(pWgtrNode VisualRef, pWgtrNode Parent)
{
int i=0, changed=0, isWin=0, isSP=0;
int childCount=xaCount(&(Parent->Children));
pWgtrNode Child;

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

		    /** auto-detect centered floating objects **/
		    if (abs(Child->pre_x - (VisualRef->pre_width - (Child->pre_x + Child->pre_width))) < 10)
			{
			    Child->x = (VisualRef->width - Child->width)/2;
			    if (Child->x < 0) Child->x = 0;
			    changed = 1;
			}
		    if (abs(Child->pre_y - (VisualRef->pre_height - (Child->pre_y + Child->pre_height))) < 10)
			{
			    Child->y = (VisualRef->height - Child->height)/2;
			    if (Child->y < 0) Child->y = 0;
			    changed = 1;
			}

		    /**if it's larger than its container, shrink it and set flag**/
		    if(Child->width > (VisualRef->width - isSP*18))
		        {
			    Child->width = (VisualRef->width - isSP*18);
			    changed = 1;
			}
		    if(Child->height > (VisualRef->height - isWin*24))
			{
			    Child->height = (VisualRef->height - isWin*24);
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
		    if((Child->x + Child->width) > (VisualRef->width - isSP*18))
			Child->x = (VisualRef->width - isSP*18) - Child->width;
		    if((Child->y + Child->height) > (VisualRef->height - isWin*24))
			Child->y = (VisualRef->height - isWin*24) - Child->height;
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
