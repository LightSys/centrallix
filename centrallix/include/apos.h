#ifndef _AUTO_POSITIONING_H
#define _AUTO_POSITIONING_H

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
/* Creation:	July 11, 2005						*/
/* Description:	Applies layout logic to the widgets of an application.	*/
/************************************************************************/

/**CVSDATA***************************************************************

 **END-CVSDATA***********************************************************/

#include "wgtr.h"
#include "cxlib/xarray.h"

//TODO: these structures' names probably need prefixes

typedef struct _APOS_L AposLine, *pAposLine;
typedef struct _APOS_S AposSection, *pAposSection;

/**Line Structure**/
struct _APOS_L
    {
    int		Loc;		//coordinate of line
    int		Adj;		//set != 0 if need to increase/decrease.
    int		isBorder;	//set for grid borderlines
    XArray	SWidgets;	//widgets that start on line
    XArray	EWidgets;	//widgets that end on line
    XArray	CWidgets;	//widgets that cross the line
    pAposSection SSection;	// section starting with this line
    pAposSection ESection;	// section ending with this line
    };
    
/**Section Structure (used for both rows and columns)**/
struct _APOS_S
    {
    pAposLine	StartLine;	//left line for columns and top line for rows
    pAposLine	EndLine;	//right line for columns and bottom line for rows
    int		Flex;		//section's flexibility. It does not follow any "units" (ie, pixels, percent of container, etc)
    int		Width;		//Width of columns or height of rows
    int		DesiredWidth;	//When we need to resize to honor max/mins
    int		isSpacer;	//set for narrow spaces between widgets
    int		isBorder;	//set for grid border sections
    };

/**Grid Structure**/
typedef struct
    {
    XArray	Rows;		//array of sections for storing rows
    XArray	Cols;		//array of sections for storing columns
    XArray	VLines;		//array of vertical lines, sorted by Location
    XArray	HLines;		//array of horizontal lines, sorted by Location
    } AposGrid, *pAposGrid;

#define AGRID(x) ((pAposGrid)(x))
#define ALINE(x) ((pAposLine)(x))

/**Function Definitions**/
int aposAutoPositionWidgetTree(pWgtrNode);	/**top-level function, called from wgtr module**/
int aposAutoPositionContainers (pWgtrNode);	/**Auto-positions all widgets inside a container**/
int aposInit();					/**Registers datastructures used in auto-positioning**/
int aposInitiallizeGrid (pAposGrid);		/**Initiallizes the XArrays in the grid object**/
int aposFree(pAposGrid);				/**Frees dynamically allocated memory**/
int aposFreeGrids(pWgtrNode);				/**Frees dynamically allocated memory**/
int aposSetOffsetBools(pWgtrNode, int*, int*, int*, int*, int*); /**sets bools used to offset widgets**/
int aposBuildGrid(pWgtrNode);			/** builds the layout grids **/
int aposSetLimits(pWgtrNode);			/** enforce min/max sizing **/

/**Tree Preparation**/
int aposPrepareTree(pWgtrNode, pXArray);	/**Prepares widget tree for auto-positioning**/
int aposPatchNegativeHeight(pWgtrNode, pXArray);/**Temporarily sets unspecified heights**/
int aposSetContainerFlex(pWgtrNode);		/**Determines a container's flexibility**/
int aposSetFlexibilities(pWgtrNode);		/**Determines a container's flexibility**/
int aposSetSectionFlex(pAposSection sect, int type);

/**Line Creation**/
int aposAddLinesToGrid(pWgtrNode, pXArray, pXArray);	/**Adds all of the necessary lines to the grid**/
int aposAddLinesForChildren(pWgtrNode, pXArray, pXArray); /**Adds four lines for every child **/
int aposCreateLine(pWgtrNode, pXArray, int, int, int, int, int);  /**Creates a line and adds it to the grid**/
pAposLine aposExistingLine(pXArray, int);		/**Checks for a line with a certain location**/
int aposFillInCWidget(pXArray, pXArray, pXArray);	/**Fills in the CWidget array of a line**/

/**Section Creation**/
int aposAddSectionsToGrid(pAposGrid, int, int);	 /**Adds all of the necessary sections to the grid**/
int aposCreateSection(pXArray, pAposLine, pAposLine, int, int); /**Creates a section object**/
int aposIsSpacer(pAposLine, pAposLine, int, int);/**Determines if a section is a space between widgets**/
int aposNonFlexChildren(pAposLine, int);	 /**Checks section after line for non-flexible widgets**/
int aposAverageChildFlex(pAposLine, int);	 /**Returns average flexibility of widgets**/
int aposMinimumChildFlex(pAposLine, int);	 /**Returns minimum flexibility of widgets**/

/**Resizing and Repositioning**/
int aposSpaceOutLines(pXArray, pXArray, int);	/**Adjusts spaces between lines to expand or contract grid**/
int aposSnapWidgetsToGrid(pXArray, int);	/**Refreshes widget dimensions to match adjusted grid**/
int aposProcessWindows(pWgtrNode, pWgtrNode);	/**Makes a pass through the tree to process windows**/

/** # defines **/
#define APOS_SWIDGETS 	1
#define APOS_EWIDGETS 	2

#define APOS_ROW 	1
#define APOS_COL 	2

#define APOS_FUDGEFACTOR 0.5

/** The greatest width between two widgets that still defines them as "adjacent," 
*** indicating that we don't want to increase the distance between them **/
#define APOS_MINSPACE 20

/**Lowest acceptable width or height for a widget**/
#define APOS_MINWIDTH 30

/**Default flexibilities for widgetless gaps in expanding or contracting applications **/
#define APOS_EGAPFLEX 30
#define APOS_CGAPFLEX 50

#endif
