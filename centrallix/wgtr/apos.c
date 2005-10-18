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

    $Id: apos.c,v 1.4 2005/10/18 22:47:12 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/wgtr/apos.c,v $

    $Log: apos.c,v $
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
aposAutoPositionWidgetTree(pWgtrNode tree)
{    
XArray PatchedWidgets;
int i=0, count=0;

    xaInit(&(PatchedWidgets),16);
    
    /**Iteration 1**/
    aposPrepareTree(tree, &PatchedWidgets);
    
    /**Iteration 2**/
    if(aposAutoPositionContainers(tree) < 0)
        {
	    mssError(0, "APOS", "aposAutoPositionWidgetTree: Couldn't auto-position contents of '%s'", tree->Name);
	    return -1;
	}
    
    /**makes a final pass through the tree and processes html windows**/
    aposProcessWindows(tree, tree);
    
    /**unpatches all of the heights that were specified in aposPrepareTree**/
    count=xaCount(&PatchedWidgets);
    for(i=0; i<count; ++i)
	((pWgtrNode)xaGetItem(&PatchedWidgets, i))->height = -1;
    
    xaDeInit(&PatchedWidgets);
    
    return 0;
}

int
aposPrepareTree(pWgtrNode Parent, pXArray PatchedWidgets)
{
pWgtrNode Child;
int i=0, childCount=xaCount(&(Parent->Children));

    for(i=0; i<childCount; ++i)
        {
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    /**if a visual child has an unspecified height, patch it, unless it is a scrollpane**/
	    if((Child->height < 0) && !(Child->Flags & WGTR_F_NONVISUAL) && 
	        strcmp(Parent->Type, "widget/scrollpane"))
	        aposPatchNegativeHeight(Child, PatchedWidgets);
	    
	    /** If child is a container but not a window, recursively prepare it as well **/
	    if((Child->Flags & WGTR_F_CONTAINER) && !(Child->Flags & WGTR_F_FLOATING))
		aposPrepareTree(Child, PatchedWidgets);
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
aposPatchNegativeHeight(pWgtrNode Widget, pXArray PatchedWidgets)
{
ObjData val;

    /** set unspecified height of widget to an educated guess**/
    if(!strcmp(Widget->Type, "widget/editbox"))
	{
	    Widget->height = 16;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/textbutton"))
	{
	    wgtrGetPropertyValue(Widget, "text", DATA_T_STRING, &val);
	    Widget->height = 13.0 * (0.5 + (float)(strlen(val.String) * 7) / (float)(Widget->width));
	    if(Widget->height < 20) Widget->height = 20;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/treeview"))
	{
	    Widget->height = 100;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/html"))
	{
	    Widget->height = Widget->width;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/dropdown"))
	{
	    if(xaCount(&(Widget->Children)) < 4) 
	         Widget->height = 40+16*xaCount(&(Widget->Children));
	    else Widget->height = 104;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/table"))
	{
	    Widget->height = 100;
	    xaAddItem(PatchedWidgets, Widget);
	}
    else if(!strcmp(Widget->Type, "widget/menu"))
	{
	    Widget->height = 20;
	    xaAddItem(PatchedWidgets, Widget);
	}

    return 0;
}

int
aposSetContainerFlex(pWgtrNode W)
{
AposGrid theGrid;
pAposSection Sect;
int i=0, sectCount=0, TotalWidth=0, ProductSum=0;

    /**initiallize the XArrays in the grid**/
    aposInitiallizeGrid(&(theGrid));
    
    /**Add the lines to the grid**/
    if(aposAddLinesToGrid(W, &(theGrid.HLines), &(theGrid.VLines)) < 0)
        {
	    mssError(0, "APOS", "aposSetContainerFlex: Couldn't add lines to %s's pre-grid", W->Name);
	    return -1;
	}
	
    /**Add the sections to the grid**/
    if(aposAddSectionsToGrid(&(theGrid), (W->height-W->r_height), (W->width-W->r_width)) < 0)
        {
	    mssError(0, "APOS", "aposSetContainerFlex: Couldn't add rows and columns to %s's pre-grid", W->Name);
	    return -1;
	}
    
    /**calculate average row flexibility, weighted by height **/
    sectCount = xaCount(&(theGrid.Rows));
    for(i=0; i<sectCount; ++i)
        {
	    Sect = (pAposSection)xaGetItem(&(theGrid.Rows), i);
	    TotalWidth += Sect->Width;
	    ProductSum += Sect->Flex * Sect->Width;
        }
    if(TotalWidth) W->fl_height = APOS_FUDGEFACTOR + (float)ProductSum / (float)TotalWidth;
    
    TotalWidth = ProductSum = 0;
    
    /**calculate average column flexibility, weighted by width **/
    sectCount = xaCount(&(theGrid.Cols));
    for(i=0; i<sectCount; ++i)
        {
	    Sect = (pAposSection)xaGetItem(&(theGrid.Cols), i);
	    TotalWidth += Sect->Width;
	    ProductSum += Sect->Flex * Sect->Width;
        }
    if(TotalWidth) W->fl_width = APOS_FUDGEFACTOR + (float)ProductSum / (float)TotalWidth;
    
    /**deallocate memory and deinit XArrays in the grid**/
    aposFree(&(theGrid));
    
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
aposAutoPositionContainers(pWgtrNode Parent)
{
AposGrid theGrid;
pWgtrNode Child, GrandChild;
int i=0, j=0, grandchildCount=0, childCount = xaCount(&(Parent->Children));

    /**initiallize the XArrays in the grid**/
    aposInitiallizeGrid(&(theGrid));
    
    /**Add the lines to the grid**/
    if(aposAddLinesToGrid(Parent, &(theGrid.HLines), &(theGrid.VLines)) < 0)
        {
	    mssError(0, "APOS", "aposAutoPositionContainers: Couldn't add lines to %s's grid", Parent->Name);
	    return -1;
	}

    /**Add the sections to the grid**/
    if(aposAddSectionsToGrid(&(theGrid), (Parent->height-Parent->r_height), (Parent->width-Parent->r_width)) < 0)
        {
	    mssError(0, "APOS", "aposAutoPositionContainers: Couldn't add rows and columns to %s's grid", Parent->Name);
	    return -1;
	}
    
    /**Adjust the spaces between lines to fit the grid to the container**/
    aposSpaceOutLines(&(theGrid.HLines), &(theGrid.Rows), (Parent->height-Parent->r_height));	//rows
    aposSpaceOutLines(&(theGrid.VLines), &(theGrid.Cols), (Parent->width-Parent->r_width));	 //columns
    
    /**modify the widgets' x,y,w, and h values to snap to their adjusted lines**/
    aposSnapWidgetsToGrid(&(theGrid.HLines), APOS_ROW);	//rows
    aposSnapWidgetsToGrid(&(theGrid.VLines), APOS_COL);	//columns
    
    /**recursive call to auto-position subsequent visual containers, except windows**/
    for(i=0; i<childCount; ++i)
	{
	    Child = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    
	    /**looks under nonvisual containers for more visual containers**/
	    if( (Child->Flags & WGTR_F_CONTAINER) && 
	        (Child->Flags & WGTR_F_NONVISUAL))
	        {
		    grandchildCount = xaCount(&(Child->Children));
		    for(j=0; j<grandchildCount; ++j)
		        {
			    GrandChild = (pWgtrNode)xaGetItem(&(Child->Children), j);
			    if((GrandChild->Flags & WGTR_F_CONTAINER) &&
			        !(GrandChild->Flags & WGTR_F_NONVISUAL) &&
				!(GrandChild->Flags & WGTR_F_FLOATING))
				if(aposAutoPositionContainers(GrandChild) < 0)
				    {
					mssError(0, "APOS", "aposAutoPositionContainers: Couldn't auto-position contents of '%s'", GrandChild->Name);
					return -1;
				    }
			}
		}
	    /**auto-positions subsequent visual container**/
	    else if( (Child->Flags & WGTR_F_CONTAINER) &&
	        !(Child->Flags & WGTR_F_NONVISUAL) &&
		!(Child->Flags & WGTR_F_FLOATING))
		if(aposAutoPositionContainers(Child) < 0)
		    {
			mssError(0, "APOS", "aposAutoPositionContainers: Couldn't auto-position contents of '%s'", Child->Name);
			return -1;
		    }
	}

    /**deallocate memory and deinit XArrays in the grid**/
    aposFree(&(theGrid));
    
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
int i=0, count=0, isWin=0, isSP=0;
pAposLine CurrLine, PrevLine;
pXArray FirstCross, LastCross;

    aposSetOffsetBools(Parent, &isSP, &isWin, NULL, NULL, NULL);

    /**Add the 2 horizontal border lines, unless parent is a scrollpane**/
    if(strcmp(Parent->Type, "widget/scrollpane"))
        {
	    if(aposCreateLine(NULL, HLines, 0, 0, 1) < 0)
	        goto CreateLineError;
	    if(aposCreateLine(NULL, HLines, (Parent->r_height-isWin*24), 0, 1) < 0)
	        goto CreateLineError;
        }
    /**Add the 2 vertical border lines**/
    if(aposCreateLine(NULL, VLines, 0, 0, 1) < 0)
        goto CreateLineError;
    if(aposCreateLine(NULL, VLines, (Parent->r_width-isSP*18), 0, 1) < 0)
        goto CreateLineError;

    aposAddLinesForChildren(Parent, HLines, VLines);

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
pWgtrNode C;

    /**loop through the children and create 4 lines for each child's 4 edges**/
    for(i=0; i<childCount; ++i)
        {
	    C = (pWgtrNode)xaGetItem(&(Parent->Children), i);
	    aposSetOffsetBools(C, NULL, NULL, &isTopTab, &isSideTab, &tabWidth);
	    
	    /** If C is a nonvisual container, add lines for
	    *** the grandchildren. Otherwise, if C is visual
	    *** and not a window, just add 4 lines for it **/
	    if((C->Flags & WGTR_F_NONVISUAL) && (C->Flags & WGTR_F_CONTAINER))
		aposAddLinesForChildren(C, HLines, VLines);
	    else if(!(C->Flags & WGTR_F_NONVISUAL) && !(C->Flags & WGTR_F_FLOATING))
	        {
		    /**add horizontal lines, unless parent is a scrollpane**/
		    if(strcmp(Parent->Type, "widget/scrollpane"))
			{
			    if(aposCreateLine(C, HLines, (C->y), APOS_SWIDGETS, 0) < 0)
			        goto CreateLineError;
			    if(aposCreateLine(C, HLines, (C->y + C->height + isTopTab*24), APOS_EWIDGETS, 0) < 0)
			        goto CreateLineError;
			}
		    
		    /**add vertical lines**/
		    if(aposCreateLine(C, VLines, (C->x), APOS_SWIDGETS, 0) < 0)
			goto CreateLineError;
		    if(aposCreateLine(C, VLines, (C->x + C->width + isSideTab*tabWidth), APOS_EWIDGETS, 0) < 0)
			goto CreateLineError;
	        }
	}
    
    return 0;
    
    CreateLineError:
    mssError(0, "APOS", "aposAddLinesForChildren: Couldn't create a new line");
    return -1;
}

int
aposCreateLine(pWgtrNode Widget, pXArray Lines, int Loc, int type, int isBorder)
{
pAposLine NewLine, ExistingLine = aposExistingLine(Lines, Loc);
    
    /**if there is already a line, just upgrade it**/
    if(ExistingLine != NULL)	
        {
	    if(type == APOS_SWIDGETS)
	        xaAddItem(&(ExistingLine->SWidgets), Widget);
	    else if(type == APOS_EWIDGETS) 
	        xaAddItem(&(ExistingLine->EWidgets), Widget);

	    return 0;
	}
    
    /**otherwise, create and add the new line**/    
    if((NewLine = (pAposLine)nmMalloc(sizeof(AposLine))) < 0)
        {
	    mssError(1, "APOS", "aposCreateLine: Couldn't allocate memory for new grid line");
	    return -1;
	}
    
    /**initiallize new line**/
    memset(NewLine, 0, sizeof(AposLine));
    xaInit(&(NewLine->SWidgets),16);
    xaInit(&(NewLine->EWidgets),16);
    xaInit(&(NewLine->CWidgets),16);
    NewLine->Loc = Loc;
    NewLine->isBorder = isBorder;
    if(type == APOS_SWIDGETS) xaAddItem(&(NewLine->SWidgets), Widget);
    else if(type == APOS_EWIDGETS) xaAddItem(&(NewLine->EWidgets), Widget);
    
    /**add new line, sorted by location**/
    xaAddItemSortedInt32(Lines, NewLine, 0);

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
aposCreateSection(pXArray Sections, pAposLine StartL, pAposLine EndL, int Diff, int type)
{
pAposSection NewSect;
int sCount = xaCount(&(StartL->SWidgets));
int cCount = xaCount(&(StartL->CWidgets));
    
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
    
    /** Set flex to 0 if the section is a spacer or contains non-flexible children, 
    *** otherwise set it to the average of the children. If none of those apply
    *** it must be a wide, widgetless gap, assign a default flexibility **/
    if((NewSect->isSpacer) || (aposNonFlexChildren(StartL, type)))
        NewSect->Flex = 0;
    else if(sCount || cCount) 
        NewSect->Flex = aposAverageChildFlex(StartL, type);
    else if (Diff < 0) NewSect->Flex = APOS_CGAPFLEX;
    else NewSect->Flex = APOS_EGAPFLEX;
    
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
aposSpaceOutLines(pXArray Lines, pXArray Sections, int Diff)
{
pAposLine CurrLine, PrevLine;
pAposSection PrevSect, CurrSect;
int TotalFlex=0, TotalFlexibleSpace=0, Adj=0, i=0, Extra=0, count=xaCount(Sections);
float FlexWeight=0, SizeWeight=0;

    /**if there are no sections, don't bother going on**/
    if(!count) return 0;
    
    /**Sum the flexibilities of the sections**/
    for(i=0; i<count; ++i)
	{
	    CurrSect = ((pAposSection)xaGetItem(Sections, i));
	    TotalFlex += CurrSect->Flex;
	    if(CurrSect->Flex)
	        TotalFlexibleSpace += CurrSect->Width;
	}
    
    /** if there is no flexibility for expansion or contraction return 0**/
    if(TotalFlex == 0) return 0;
    
    /** sets each line's location equal to the previous line's location
    *** plus the adjusted width of the preceding section **/
    count = xaCount(Lines);
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
		    Adj = APOS_FUDGEFACTOR + (float)(Diff) * ((float)(FlexWeight+SizeWeight)/2.0);
		    CurrLine->Loc = PrevLine->Loc + PrevSect->Width + Adj;
		    PrevSect->Width += Adj;
		}
	    /**for contracting lines**/
	    else if(Diff < 0)
		{
		    Adj = (float)(Diff) * ((float)(FlexWeight+SizeWeight)/2.0) - APOS_FUDGEFACTOR;
		    
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
	aposSpaceOutLines(Lines, Sections, Extra);
	
    return 0;
}

int
aposSnapWidgetsToGrid(pXArray Lines, int flag)
{
int i=0, j=0, count=0, lineCount = xaCount(Lines);
int isTopTab=0, isSideTab=0, tabWidth=0;
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
			    if((CurrLine->Loc - Widget->y - isTopTab*24) > APOS_MINWIDTH)
				Widget->height = CurrLine->Loc - Widget->y - isTopTab*24;
			    else 
				/*Widget->height = APOS_MINWIDTH;*/
				Widget->height = Widget->r_height;
			}
		    else if(flag==APOS_COL  &&  Widget->fl_width)
		        {
			    if((CurrLine->Loc - Widget->x - isSideTab*tabWidth) > APOS_MINWIDTH)
				Widget->width = CurrLine->Loc - Widget->x - isSideTab*tabWidth;
			    else
				/*Widget->width = APOS_MINWIDTH;*/
				Widget->width = Widget->r_width;
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

    aposSetOffsetBools(Parent, &isSP, &isWin, NULL, NULL, NULL);

    /**loop through children and process any windows**/
    for(i=0; i<childCount; ++i)
	{
	    Child = (pWgtrNode)(xaGetItem(&(Parent->Children), i));
	    if(Child->Flags & WGTR_F_FLOATING)
		{
		    /**if it's outside the top left corner pull the whole window in**/
		    if(Child->x < 0) Child->x = 0;
		    if(Child->y < 0) Child->y = 0;
		    
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
		    
		    /**if it's outside the bottom right corner, pull the whole window in**/
		    if((Child->x + Child->width) > (VisualRef->width - isSP*18))
			Child->x = (VisualRef->width - isSP*18) - Child->width;
		    if((Child->y + Child->height) > (VisualRef->height - isWin*24))
			Child->y = (VisualRef->height - isWin*24) - Child->height;
		    
		    /**if the window changed width or height, process it like a widget tree**/
		    if(changed) aposAutoPositionWidgetTree(Child);
		}
	    
	    /**recursive call on visual containers; new visual reference is passed**/
	    if((Child->Flags & WGTR_F_CONTAINER) && !(Child->Flags & WGTR_F_NONVISUAL))
		aposProcessWindows(Child, Child);
	    
	    /**recursive call on nonvisual containers; old visual reference is maintained**/
	    if((Child->Flags & WGTR_F_CONTAINER) && (Child->Flags & WGTR_F_NONVISUAL))
		aposProcessWindows(VisualRef, Child);
	
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
