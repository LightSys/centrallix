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

/**Line Structure**/
typedef struct
    {
    int		Loc;		//coordinate of line
    int		isBorder;	//set for grid borderlines
    XArray	SWidgets;	//widgets that start on line
    XArray	EWidgets;	//widgets that end on line
    XArray	CWidgets;	//widgets that cross the line
    } AposLine, *pAposLine;
    
/**Section Structure (used for both rows and columns)**/
typedef struct
    {
    pAposLine	StartLine;	//left line for columns and top line for rows
    pAposLine	EndLine;	//right line for columns and bottom line for rows
    int		Flex;		//section's flexibility
    int		Width;		//Width of columns or height of rows
    int		isSpacer;	//set for narrow spaces between widgets
    int		isBorder;	//set for grid border sections
    } AposSection, *pAposSection;

/**Grid Structure**/
typedef struct
    {
    XArray	Rows;		//array of sections for storing rows
    XArray	Cols;		//array of sections for storing columns
    XArray	VLines;		//array of vertical lines, sorted by Location
    XArray	HLines;		//array of horizontal lines, sorted by Location
    } AposGrid, *pAposGrid;

/**Function Definitions**/
int aposAutoPositionWidgetTree(pWgtrNode);	/**top-level function, called from wgtr module**/
int aposAutoPositionContainers (pWgtrNode);	/**Auto-positions all widgets inside a container**/
int aposInit();					/**Registers datastructures used in auto-positioning**/
int aposInitiallizeGrid (pAposGrid);		/**Initiallizes the XArrays in the grid object**/
int aposFree(pAposGrid);				/**Frees dynamically allocated memory**/
int aposSetOffsetBools(pWgtrNode, int*, int*, int*, int*, int*); /**sets bools used to offset widgets**/

/**Tree Preparation**/
int aposPrepareTree(pWgtrNode, pXArray);	/**Prepares widget tree for auto-positioning**/
int aposPatchNegativeHeight(pWgtrNode, pXArray);/**Temporarily sets unspecified heights**/
int aposSetContainerFlex(pWgtrNode);		/**Determines a container's flexibility**/

/**Line Creation**/
int aposAddLinesToGrid(pWgtrNode, pXArray, pXArray);	/**Adds all of the necessary lines to the grid**/
int aposAddLinesForChildren(pWgtrNode, pXArray, pXArray); /**Adds four lines for every child **/
int aposCreateLine(pWgtrNode, pXArray, int, int, int);  /**Creates a line and adds it to the grid**/
pAposLine aposExistingLine(pXArray, int);		/**Checks for a line with a certain location**/
int aposFillInCWidget(pXArray, pXArray, pXArray);	/**Fills in the CWidget array of a line**/

/**Section Creation**/
int aposAddSectionsToGrid(pAposGrid, int, int);	 /**Adds all of the necessary sections to the grid**/
int aposCreateSection(pXArray, pAposLine, pAposLine, int, int); /**Creates a section object**/
int aposIsSpacer(pAposLine, pAposLine, int, int);/**Determines if a section is a space between widgets**/
int aposNonFlexChildren(pAposLine, int);	 /**Checks section after line for non-flexible widgets**/
int aposAverageChildFlex(pAposLine, int);	 /**Returns average flexibility of widgets**/

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
