# Auto-Positioning Module Specifications
Author: Nathaniel Colson

Date: August 9, 2005

License: Copyright (C) 2001 LightSys Technology Services.  See LICENSE.txt.

## Synopsis
This document specifies the way in which the auto-positioning module resizes and repositions widgets in the widgets tree so that they best fit any given window size.

- [Auto-Positioning Module Specifications](#auto-positioning-module-specifications)
  - [Synopsis](#synopsis)
  - [Overview](#overview)
  - [Preparing the Tree](#preparing-the-tree)
  - [Setting Up the Grids](#setting-up-the-grids)
  - [Expanding/Contracting the Grid](#expandingcontracting-the-grid)
  - [HTML Windows](#html-windows)
  - [Scrollpanes](#scrollpanes)
  - [Tabs](#tabs)
  - [Nonvisual Containers](#nonvisual-containers)
  - [Modifications to Components Exterior to the APOS Module](#modifications-to-components-exterior-to-the-apos-module)
    - [Changes in the wgt**New and wgt**Verify Functions](#changes-in-the-wgtnew-and-wgtverify-functions)
    - [Miscellaneous Changes](#miscellaneous-changes)
  - [Existing Difficulties and Proposed Fixes](#existing-difficulties-and-proposed-fixes)
  - [Future Functionality](#future-functionality)
  - [Note to Application Authors](#note-to-application-authors)
  - [Function Call Hierarchy](#function-call-hierarchy)
  - [Function Specifications](#function-specifications)
    - [int aposInit()](#int-aposinit)
      - [Description:](#description)
      - [Method:](#method)
    - [int aposSetOffsetBools(pWgtrNode, int*, int*, int*, int*, int*)](#int-apossetoffsetboolspwgtrnode-int-int-int-int-int)
      - [Description:](#description-1)
      - [Inputs:](#inputs)
      - [Results:](#results)
      - [Method:](#method-1)
    - [int aposAutoPositionWidgetTree(pWgtrNode)](#int-aposautopositionwidgettreepwgtrnode)
      - [Description:](#description-2)
      - [Inputs:](#inputs-1)
      - [Method:](#method-2)
    - [int aposPrepareTree(pWgtrNode)](#int-apospreparetreepwgtrnode)
      - [Description:](#description-3)
      - [Inputs:](#inputs-2)
      - [Results:](#results-1)
      - [Method:](#method-3)
    - [int aposPatchNegativeHeights(pWgtrNode, pXArray)](#int-apospatchnegativeheightspwgtrnode-pxarray)
      - [Description:](#description-4)
      - [Inputs:](#inputs-3)
      - [Method:](#method-4)
    - [int aposSetContainerFlex (pWgtrNode)](#int-apossetcontainerflex-pwgtrnode)
      - [Description:](#description-5)
      - [Inputs:](#inputs-4)
      - [Method:](#method-5)
    - [int aposAutoPositionContainers (pWgtrNode)](#int-aposautopositioncontainers-pwgtrnode)
      - [Description:](#description-6)
      - [Inputs:](#inputs-5)
      - [Method:](#method-6)
    - [int aposInitiallizeGrid (pAposGrid)](#int-aposinitiallizegrid-paposgrid)
      - [Description:](#description-7)
      - [Inputs:](#inputs-6)
      - [Results:](#results-2)
      - [Method:](#method-7)
    - [int aposAddLinesToGrid(pWgtrNode, pAposGrid)](#int-aposaddlinestogridpwgtrnode-paposgrid)
      - [Description:](#description-8)
      - [Inputs:](#inputs-7)
      - [Results:](#results-3)
      - [Method:](#method-8)
    - [int aposAddLinesForChildren(pAposGrid, pWgtrNode)](#int-aposaddlinesforchildrenpaposgrid-pwgtrnode)
      - [Description:](#description-9)
      - [Inputs:](#inputs-8)
      - [Results:](#results-4)
      - [Method:](#method-9)
    - [int aposCreateLine(pWgtrNode, pXArray, int, int, int)](#int-aposcreatelinepwgtrnode-pxarray-int-int-int)
      - [Description:](#description-10)
      - [Inputs:](#inputs-9)
      - [Results:](#results-5)
      - [Method:](#method-10)
    - [pAposLine aposExistingLine(pXArray, int)](#paposline-aposexistinglinepxarray-int)
      - [Description:](#description-11)
      - [Inputs:](#inputs-10)
      - [Results:](#results-6)
      - [Method:](#method-11)
    - [int aposFillInCWidget(pXArray, pXArray, pXArray)](#int-aposfillincwidgetpxarray-pxarray-pxarray)
      - [Description:](#description-12)
      - [Inputs:](#inputs-11)
      - [Results:](#results-7)
      - [Method:](#method-12)
    - [int aposAddSectionsToGrid(pAposGrid, int, int)](#int-aposaddsectionstogridpaposgrid-int-int)
      - [Description:](#description-13)
      - [Inputs:](#inputs-12)
      - [Results:](#results-8)
      - [Method:](#method-13)
    - [int aposCreateSection(pXArray, pAposLine, pAposLine, int, int)](#int-aposcreatesectionpxarray-paposline-paposline-int-int)
      - [Description:](#description-14)
      - [Inputs:](#inputs-13)
      - [Method:](#method-14)
    - [int aposIsSpacer(pAposLine, pAposLine, int, int)](#int-aposisspacerpaposline-paposline-int-int)
      - [Description:](#description-15)
      - [Inputs:](#inputs-14)
      - [Results:](#results-9)
      - [Method:](#method-15)
    - [int aposAverageChildFlex(pAposLine, int)](#int-aposaveragechildflexpaposline-int)
      - [Description:](#description-16)
      - [Inputs:](#inputs-15)
      - [Results:](#results-10)
      - [Method:](#method-16)
    - [int aposSpaceOutLines(pXArray, pXArray, int)](#int-aposspaceoutlinespxarray-pxarray-int)
      - [Description:](#description-17)
      - [Inputs:](#inputs-16)
      - [Results:](#results-11)
      - [Method:](#method-17)
    - [int aposSnapWidgetsToGrid(pXArray, int)](#int-apossnapwidgetstogridpxarray-int)
      - [Description:](#description-18)
      - [Inputs:](#inputs-17)
      - [Results:](#results-12)
      - [Method:](#method-18)
    - [int aposProcessWindows(pWgtrNode, pWgtrNode)](#int-aposprocesswindowspwgtrnode-pwgtrnode)
      - [Description:](#description-19)
      - [Inputs:](#inputs-18)
      - [Results:](#results-13)
      - [Method:](#method-19)
    - [int aposFree(pAposGrid)](#int-aposfreepaposgrid)
      - [Description:](#description-20)
      - [Inputs:](#inputs-19)
      - [Results:](#results-14)
      - [Method:](#method-20)

## Overview
The auto-positioning module is run during the verification of the widget tree in the widget tree module. It makes one final pass through the tree to assess the dimensions, types, and structure of the widgets within, and adjusts the size and position of each of them to scale the layout of the application up or down as is necessary to fit the desired application window. This process is guided by the flexibility property of each widget, either default or author defined, which specifies how much space a widget can absorb or give up. A container widget's flexibility is greatly dependent on the flexibility of the widgets it contains.

The auto-positioning process is carried out in two iterations. The first climbs through the tree from the bottom up and prepares it to be auto-positioned, which involves handling widgets with unspecified heights and setting the flexibility property of each container.

Once the tree is prepared, the second iteration climbs through the tree, this time from the top down, and sets up a "grid" within each container it encounters. The lines of this grid outline the edges of all the children widgets in their design positions, as well as the borders of the container itself. Each row or column of the grid is then intelligently expanded or contracted based on the flexibility of the widgets that fall within it, until the grid fits the desired size of its container. Once the dimensions of the adjusted grid are defined it is used to determine the new sizes and positions of the widgets associated with it. This establishes the dimensions of the next level of containers, and the same process can take place within them, setting up grids and resizing their widgets, continuing recursively down the widget tree until all the widgets are adjusted. 

## Preparing the Tree
The first step of preparing the tree is handling widgets like treeviews, dropdowns, and autoheight buttons that allow their heights to go unspecified. These widgets will get a height eventually, it's just not calculated until they are being rendered. In the meantime the auto-positioning module must have a height of some kind, so a temporary one is assigned based on the widget's type and properties. These "patched" widgets are then stored in the PatchedWidgets XArray, and at the end of the auto-positioning process they are iterated through and reverted back to their unspecified height of -1. If any additional widgets are created that allow their height to go unspecified, an additional case will need to be added to aposPatchNegativeHeight, which assigns a reasonable height to that widget. If this isn't done the error "a widget crossed the border line" will show up.

The second step of preparing the tree is calculating the flexibility of the containers in the tree, which is done by setting up a preliminary grid in the container to analyze the flexibility and orientation of the widgets it contains. The fl_width and fl_height assigned to the container are found by averaging the flexibility of all the columns and all the rows, weighted by column/row size.

## Setting Up the Grids
The grids are represented by a grid object, which contains sections and line objects. The section object is used to represent both rows and columns, and likewise the line object is used to represent both horizontal and vertical lines. Each section is defined by the two line objects that it points to, and every line save the border lines is pointed to by two sections, either two rows or two columns. One grid is constructed for each container in the widget tree, with four empty XArrays called HLines, VLines, Rows, and Cols.

HLines and VLines are first filled by looping through all the children in the container and adding four lines for each child's four edges. The line objects store a location integer and three XArrays of widget pointers for the widgets associated with that line - one for widgets that start on the line, one for widgets that end on it, and one for widgets that cross it. The first two are filled while the lines are being created, widget by widget, and the last is filled afterwards when the location of each line is known with respect to all the other lines. Before a line is created in a given location, that location is checked to ensure that a line does not already exist there. If there is, the new line is aborted and the widgets that were to be associated with it are simply added to the existing line.

Finally each XArray of lines is traversed to fill the section arrays, Rows and Cols. One section is created for each pair of adjacent lines. In addition to the two line pointers, the section object stores a flexibility integer, a width integer, and two flags - isBorder and isSpacer. isSpacer is set when a row or column represents a sufficiently narrow space between two widgets (or between a widget and the border), indicating that it is not to be changed. The flexibility property of a section is set to the average flexibility of the children that fall within that section. However, if the section is a spacer, or if it contains completely nonflexible children, it is set to zero. And if the section contains no widgets and is too wide to be a spacer, it is set to a default flexibility.

## Expanding/Contracting the Grid
To expand/contract the grid, each section is allotted a fraction of the total change in space. The total change is the difference between the requested dimensions of the grid's container (r_x, r_y) and the actual dimension (x, y). The requested dimension is the dimension of the container as it is specified in the application definition, and the actual dimension is what the container was given as a child when its parent container was being auto-positioned. The fraction of this total change that is allotted to each section is proportional to the average of the ratio of the section's flexibility to the total flexibility of all the sections, and the ratio of the section's width to the total of the widths of all the flexible sections (non-flexible sections won't absorb/give up any space, so they're left out of this total). The section's right or bottom line is then displaced by that amount to widen the section. Once all the sections have been widened, a pass is made through the lines to "snap" each widget to the line that it starts or ends on, making use of the arrays of widgets stored in each line. Two things will prevent a widget from resizing. One is if resizing would leave it smaller than APOS_MINWIDTH, the lowest acceptable height or width for a widget, the other is if it is nonflexible but was asked to resize anyway. One of these is usually to blame if widgets are overlapping. Once the widgets have been resized and repositioned, all the dynamically allocated memory in the grid is deallocated.

## HTML Windows
The html windows are frequently designed to be invisible until a button is clicked or some similar action, and as such they often lay on top of all of their sibling widgets. For this reason they cannot be allowed to affect the resizing and repositioning of their siblings, and are left entirely out of the auto-positioning process until the end. At that time aposProcessWindows traverses the widget tree looking for windows that are exceeding the borders of their container. If such a window is found it is moved and resized until it fits, and then the top level function is run on the window as if it were an entirely separate widget tree. 

Another oddity of the html window is that it's outside height is not the same as its inside height, due to the 24 pixel title bar. To compensate for this a number of functions use an "isWin" integer to toggle a subtraction of 24 from certain vertical dimensions. And just to make things interesting some windows opt to not have a titlebar, so that is also taken into account when setting isWin.

## Scrollpanes
Scrollpanes are another anomaly in that their exterior height is set, but their interior height is unlimited. This feature is handled by simply not auto-positioning the vertical aspect of scrollpanes. This is done by heading off the auto-positioning process in a few strategic places, such that no horizontal lines are ever created, and there is nothing there to auto-position when it tries. Everywhere you see a strcmp of a widget's type and "widget/scrollpane" this is what it is about. This method of handling scrollpanes may need to be tweaked, as they seem to work smoothly for treeviews and html widgets, but not much else; they won't scroll down to see the rest of a table or window that is hanging below the border. In addition, the space taken up by scrollbars on scrollpanes is accounted for in exactly the same way as the titlebar on windows, only the 18 pixel adjustment is made to horizontal dimensions instead of vertical.

## Tabs
Tab widgets have a similar problem as windows and scrollpanes. Because of the tab buttons, the tab widget is not square like all the other widgets, however it is still described with an x, y, width and height. This causes problems because the auto-positioning algorithm assums that x + width is the far right border of a widget. But if the widget happens to be a tab widget with side mounted tabs, x + width is off by tab_width. This offset is handled by analyzing tabs in aposSetOffsetBools, and using the resulting flags to grid on the real border of the widget, and not just the border of its main body. The compensation takes place in both aposAddLinesForChildren and aposSnapWidgetsToGrid. This poses some difficulty to application authors who want to couch widgets in underneath side mounted tabs, because from the apos module's perspective those widgets are "underneath" the tab, and will share the tab's high flexibility.

## Nonvisual Containers
Yet another trouble widget is the nonvisual container, which can contain visual widgets but has no physical dimensions itself for the auto-positioning logic to get a foothold on. This is handled by virtually bypassing the nonvisual containers in the widget tree. When a grid is being set up in a container and a nonvisual container is found among the children, additional lines are added to the grid to account for the nonvisual container's children. Since the lines keep track of what widgets are associated with them, these "grandchildren" are treated exactly the same as the regular children when the grid is resizing. There are a several strategic places where nonvisual containers are sidestepped in this manner.
    
    
## Modifications to Components Exterior to the APOS Module

### Changes in the wgt**New and wgt**Verify Functions
A container property was added to the widget->Flags bitfield, and set in the wgt**Verify function of pages, framesets, osrc's, forms, panes, scrollpanes, tabs, tabpages, and html windows.

A default width and height flexibility is assigned to every visual or container widget in its wgt**New function (x and y flexibilities are not used). The html widget sports a beefed up version which takes the html properties into account. Unfortunately, those properties are not set until after wgthtmlNew runs, so the flexibility initialization is delayed until the wgthtmlVerify function. This extra logic also prompted the inclusion of "cxlib/datatypes.h" in wgtdrv_html.c. 

Code was added to wgtpgVerify that causes an error if the dimensions of a page is not specified by the application author.

The alerter widget was marked as nonvisual in wgtalrtNew.

The actual and requested geometry of the formstatus widget were hardcoded in wgtfsNew to be 20 by 30 pixels, which is the geometry that it displays with regardless of the dimensions it is given.

The same wgttabNew and wgttabVerify function is shared by the tab and tabpage widgets, so some logic was required to distinguish between the two where necessary. Right now no distinction is needed in the wgttabVerify function, as they are both fully flexible visual containers. This is not the case in wgttabNew, where the tab is left alone but the tabpage's dimensions are set equal to that of the tab.

### Miscellaneous Changes
The dimensions of the top level page in each application definition were given a reasonable value. No auto-positioning can be done unless the author's intended geometry for the application is known, without it there is no benchmark to scale up or down from.

The call to aposAutoPositionWidgetTree was inserted at the end of wgtrVerify in wgtr.c.

The call to aposInit was added to wgtrInitialize.

The initial width and height flexibility parameters passed to wgtNewNode from wgtrParseOpenObject were changed from 100 to -1, to facilitate their recognition as unspecified later on.

apos.h was #included in the wgtr module.

## Existing Difficulties and Proposed Fixes
A problem occurs in aposSpaceOutLines when there is more space stored in Extra than the grid can surrender. After enough recursive calls, all of the sections will be rendered non-flexible spacers, and if there is still unhandled extra space it will be left unhandled, because TotalFlex will come up empty and 0 will be returned. At the core of this problem is the difficulty of calculating an exact flexibility for containers when preparing the tree. Because the aposAutoPositionContainers traverses the widget tree from the top down, if too high a flexibility is assigned, the container will go ahead and contract, only to find out later that it's contents can't go as small as it did, which leaves them hanging over the side. This could potentially be fixed by calculating minimum dimensions for each container while preparing the tree, and then taking them into account before resizing the container. This brings up the question of where to store such dimensions between the preparation process and the auto-positioning itself; perhaps the grids created during the preparation of the tree could be left in their containers, and when aposAutoPosition- Containers comes back down the tree it could reuse them for the actual auto-positioning. This would be more efficient and would provide a convenient place to store minimum coordinates.
    
A problem that could be fixed by adding minumum dimensions is the way the main body of a tab widget can shrink to the point that it doesn't have the surface area to accomodate the tabs, leaving them floating off in space; especially top and bottom mounted tabs.

Another problem arises due to the difference between APOS_MINWIDTH and APOS_MINSPACE. The former is the lowest acceptable height or width for a widget, and the latter is the lowest acceptable width for a row or column in a grid. The problem occurs when the row or column a widget is in contracts all the way down to the minimum, and then the widget displays at its slightly higher minimum, causing it to overflow onto other widgets (see Filemanager.app for an example). If ideal flexibilities could be calculated the row or column would never contract that far with a widget inside of it. The two minimums could be set to each other, but it would result in either very narrow widgets or awkwardly big spaces between widgets.

Applications like duelform.app that are composed of windows tend not to resize very well due to the way the apos module handles windows separate from everything else. It may be desirable to distinguish between windows that are part of the standard interface and windows that pop up as the result a button click or other action. This could be done by adding another layer of conditional in all the places where windows have been screened out, that examines the visible property of each window. Then the standard interface windows could be auto-positioned with all the other widgets, and only the pop-up windows that lay behind everything would be screened out.

If the auto-positioning module is thought to be suspect for problems encountered while testing other parts of Centrallix, it can be entirely turned off for debugging purposes by commenting out the call to aposAutoPositionWidgetTree near the end of the wgtrVerify function in wgtr.c.

## Future Functionality
At present the auto-positioning module only resizes an application once, right before the first time it renders. However, it would't be hard to coerce it to run every time the application window changes size, and dynamically resize the application. This would require making some browser specific modifications outside the apos module.

Another possibility that the auto-positioning module opens up is resizable window widgets. Once the windows have been modified to allow the user to request a new size, auto-positioning the contents would simply involve passing a pointer to the window to the toplevel function of the apos module. 

## Note to Application Authors
It's important to pick dimensions for the top level page of an application that accurately represent the application's shape and size, otherwise it may not display correctly.

It should also be noted that applications built with closely arranged non-flexible widgets will not expand much at all (see basicform2.app). To change this you can either assign your own flexibility value to strategic widgets, or make spaces between widgets wider than APOS_MINSPACE so they no longer count as actual spaces between widgets, creating a place for the extra space to go. 

There are several constants defined in apos.h that may be tweaked as necessary if you feel different values will better address a wider variety of applications.

## Function Call Hierarchy
(R) = Recursive

```
aposAutoPositionWidgetTree
    |-aposPrepareTree(R)
    |    |-aposPatchNegativeHeight
    |    |-aposSetContainerFlex
    |
    |-aposAutoPositionContainers(R)
    |    |-aposInitiallizeGrid
    |    |-aposAddLinesToGrid
    |    |    |-aposSetOffsetBools
    |    |    |-aposCreateLine
    |    |    |-aposAddLinesForChildren(R)
    |    |    |    |-aposSetOffsetBools
    |    |    |    |-aposCreateLine
    |    |    |
    |    |    |-aposExistingLine
    |    |    |-aposFillInCWidgets
    |    |
    |    |-aposAddSectionsToGrid
    |    |    |-aposCreateSection
    |    |    |-aposIsSpacer
    |    |    |-aposNonFlexChildren
    |    |    |-aposAverageChildFlex
    |    |
    |    |-aposSpaceOutLines(R)
    |    |-aposSnapWidgetsToGrid
    |    |    |-aposSetOffsetBools
    |    |
    |    |-aposFree
    |
    |-aposProcessWindows(R)
            |-aposSetOffsetBools
```

## Function Specifications

### int aposInit()
#### Description:
Function called from wgtrInitilize() in wgtr.c to register the datatypes used in the auto-positioning module.

#### Method:
nmRegister is called for grids, sections, and lines.

### int aposSetOffsetBools(pWgtrNode, int*, int*, int*, int*, int*)
#### Description:
Sets all of the integers used to toggle offsets that compensate for scrollbars, titlebars, and tabs.

#### Inputs:
A pointer to a widget with features that make offsets necessary, and five integers passed by reference, isSP (scrollpane), isWin (window titlebars), isTopTab (top mounted tab), isSideTab (side mounted tab), tabWidth (width of tabs).

#### Results:
If the given widget is a scrollpane, window with titlebar, or tab, the appropriate integers are given values that will compensate throughout the auto-positioning process for the extra space they take up.

#### Method:
The integers used are declared in other functions and pointers to them passed to aposSetOffsetBools. If it is known at the point the call is being made from that not all five will be used, NULL can be passed in for the unused ones. isSP is set simple when the widget is a scrollpane. isWin is set when the widget is an html window, and either the titlebar property does not exist, or it does exist and is set to "yes". isTopTab is set when the widget is either a top OR bottom mounted tab widget (if someone can think of a less misleading name, they can change it). isSideTab is set when the widget is a left or right mounted tab widget. tabWidth is set to the tab_width property, unless it isn't found in which case it defaults to 80.

### int aposAutoPositionWidgetTree(pWgtrNode)
#### Description: 
Top-level function, called during the verification process in the widget tree module to auto-position the widgets in the tree.

#### Inputs:
A pointer to the head node of the widget tree.

#### Method:
After a few things in the tree are prepared, a call is made to a recursive function that does most of the auto-positioning, and a second function is called to process window widgets.
    
### int aposPrepareTree(pWgtrNode)
#### Description:
Recursively traverses the widget tree from the bottom up and sets the flexibility property of each containers according to the flexibility of its contents. Also patches any widgets with unspecified heights.

#### Inputs: 
A pointer to the container to be prepared, an XArray pointer to the PatchedWidgets array.

#### Results: 
All containers in the widget tree have a reasonable flexibility value based on their contents, and all widgets have a specified height.

#### Method: 
As all the children of the given node are looped through, each one is tested to see if its height is specified; if not it is passed to aposPatchNegativeHeight, with the exception of nonvisuals and scrollpanes. A recursive call is then made to prepare any container children, with the exception windows. After all children in the container have been addressed, the flexibility of the container is set using aposSetContainerFlex.

### int aposPatchNegativeHeights(pWgtrNode, pXArray)
#### Description:
Sets the height of the given widget to an educated guess.

#### Inputs:  
A pointer to the container of the widgets to be patched, and a pointer to the PatchedWidgets array.

#### Method:  
A switch statement determines the type of the given widget, and uses this along with the widget's properties to set the actual (not requested) height to an estimate of what the height will be at render time.

### int aposSetContainerFlex (pWgtrNode)
#### Description:
Sets up the preliminary grid in containers during the preparation of the tree to analyze the contents of the container and produce reasonable flexibility values.

#### Inputs: 
A pointer to the container in question.

#### Method:
The grid is set up in the same manner as in aposAutoPositionContainers. The average flexibility of the rows and of the columns is then calculated, weighted by size. The equation used is: `AverageFlexibility=(Flex1*Width1+Flex2*Width2...) / (Width1+Width2...)`.

### int aposAutoPositionContainers (pWgtrNode)
#### Description:  
Auto-positions the widgets inside a container and all subsequent containers in the widget tree.

#### Inputs:  
A pointer to the parent container.

#### Method:  
A grid object is declared and initiallized. All the necessary lines and sections are added to the grid. The lines are then spaced out, first the horizontal ones and then the vertical, and the widgets are snapped to the new locations of the lines they were associated with. Once the dimensions of all the children have been established, they are looped through and a recursive call is made on any visual container children, save window widgets. This includes visual containers found one level beneath nonvisual containers. Finally a function frees up all of the dynamically allocated memory in the grid object. 

### int aposInitiallizeGrid (pAposGrid)
#### Description:  
Short function to initiallize the XArrays in the grid object.

#### Inputs:  
A pointer to the grid

#### Results:  
The Row, Col, HLines, and VLines are initiallized with length 16.

#### Method:  
xaInit is called on each XArray.

### int aposAddLinesToGrid(pWgtrNode, pAposGrid)
#### Description: 
Populates the *Line XArrays of the grid object.

#### Inputs: 
A pointer to the container the grid is being set up in, and a pointer to the grid.

#### Results: 
The grid contains all of the lines necessary for auto-positioning.

#### Method: 
First isWin and isSp are set using aposSetOffsetBools. Then the four border lines are created using the application author's original geometry for the container. The lines outlining the children are created using aposAddLinesForChildren. The cross arrays for the lines are filled in using aposFillInCrossArray. Finally a check is made to ensure that there are no widgets are crossing over of the boundary lines. 

### int aposAddLinesForChildren(pAposGrid, pWgtrNode)
#### Description:
Adds four lines to the given grid for the four edges of each child found in the given container.

#### Inputs:
A pointer to a grid and a pointer to its container.

#### Results:
All the lines needed to outline the children in a container are added.

#### Method:
The children of the given container are looped through, and for every one that is visual and not a window, four lines are created using aposCreatLine to outline its edges. aposSetOffsetBools sets the tab flags for each child to toggle the compensation for tabs. If a child is a nonvisual container, a recursive call is made to add all of its children to the same grid.
    
### int aposCreateLine(pWgtrNode, pXArray, int, int, int)
#### Description:
Creates a new line and adds it to the grid. If a line already exists at the would-be line's location, the two are merged into one.

#### Inputs: 
A pointer to the widget whose edge the line is for, a pointer to *Lines array in the current grid, an integer location marking the point where the line should be created, a flag indicating whether the given widget is starting or ending on the line being created, and another flag indicating whether or not the line is a border line.

#### Results: 
A new line exists at the given location, or an existing line is updated. 

#### Method: 
First aposExistingLine is used to check for an existing line at the given location. If one is found the given widget is added to the array indicated by flag. If none is found, memory for a new line is allocated. The line object's arrays are initiallized and it's properties filled in, and it is added to the given *Lines array, sorted by location.
    
### pAposLine aposExistingLine(pXArray, int)
#### Description: 
Checks an array of lines for a line with the given location.

#### Inputs: 
A pointer to an XArray of lines, and an integer location.

#### Results: 
Either a pLine or NULL is returned.

#### Method: 
The array of lines is looped through and each line's location checked against the passed location. If a match is found, a pointer to the matching line is returned, if no match can be found NULL is returned.
    
### int aposFillInCWidget(pXArray, pXArray, pXArray)
#### Description: 
Fills in the CWidget array of a single line, marking which widgets cross that line.

#### Inputs: 
A pointer to the array of widgets approaching the current line (either SWidgets or CWidgets of the preceding line), a pointer to the array of widgets ending on the current line, and a pointer to the array of widgets crossing the current line.

#### Results: 
CWidgets contains all the widgets that cross the current line.

#### Method: 
PrevList is looped through, and every item on it is compared to every item in EWidgets. Any found in the former but not in the latter are added to CWidgets. In other words, widgets that start on or cross the preceding line but do not terminate on the current line are marked down as crossing the current line.

### int aposAddSectionsToGrid(pAposGrid, int, int)
#### Description: 
Populates the Rows and Cols arrays of the current grid.

#### Inputs: 
A pointer to the grid, and two integers whose signs indicate whether we are expanding or contracting in the vertical and horizontal directions. 

#### Results: 
The grid contains all of the necessary sections for auto-positioning.

#### Method: 
Loops through all the lines and uses aposCreateSection to create a section (either a row or a column) for each pair of adjacent lines. 

### int aposCreateSection(pXArray, pAposLine, pAposLine, int, int)
#### Description: 
Creates a new row or column section using the two given lines as boundaries. 

#### Inputs: 
A pointer to the array of sections to be added to (either Rows or Cols of the current grid), two pointers to the two lines that define the new section, an integer whose sign indicates whether the new section will be expanded or contracted, and a flag indicating if it is a row or column.

#### Method: 
Memory for a new section is allocated and initiallized. The properties are filled out using the given lines, aposNonFlexChildren, aposIsSpacer, and aposAverageChildFlex. If the section is a spacer or contains non-flexible children, its flexibility is set to 0; otherwise if it has flexible children its flexibility is set to their average. If neither of those apply it must be a wide, widgetless gap, and a default flexibility is assigned. Finally the new section is added to the given section array.
    
### int aposIsSpacer(pAposLine, pAposLine, int, int)
#### Description: 
Determines if a section defined by the given parameters qualifies as a narrow space between widgets.

#### Inputs: 
Two pointers to the section's two defining lines (a starting line and an ending line), a flag indicating if it is a row or column, and a flag indicating if it is a border section.

#### Results: 
1 is returned if it is a narrow space between widgets.

#### Method: 
If the space between the lines is sufficiently narrow, nested for-loops are used to compare every widget ending on the first line to every widget starting on the second line. If the corner of a widget on one side is found to fall between the two corners of a widget on the other side, then the two are indeed across from each other and 1 is returned.

### int aposAverageChildFlex(pAposLine, int)
#### Description: 
Averages the flexibility of widgets beginning on or crossing the given line, because they are the ones that fall within the section proceeding the line.

#### Inputs: 
A pointer to the line preceding the section whose flexibility is being calculated, a flag indicating whether the line is horizontal or vertical.

#### Results: 
The average flexibility of the widgets within the mentioned section is returned.

#### Method: 
The widgets that start on or cross the given line are looped through, and their fl_widths or fl_heights are summed. The average is calculated and returned, using the total width and a fudge factor for proper rounding.

### int aposSpaceOutLines(pXArray, pXArray, int)
#### Description:
Adjusts spaces between lines to expand or contract the current grid to fit it's container.

#### Inputs:
A pointer to the set of lines to be spaced out (either HLines or VLines), a pointer to the corresponding set of sections, and the integer difference between the requested and actual geometry of the container the grid is resizing into.

#### Results:
The spaces between the given lines add up to span the exact length of the container.

#### Method:
The flexibility of the given sections is summed; if it is 0 there is no flexibility for expansion or contraction, and 0 is returned. Otherwise, the given lines are looped through, from left to right or top to bottom, and each line's location is set equal to the previous line's location plus the adjusted width of the preceding section. If Diff is positive, the adjustment is simply a fraction of Diff that is proportional to the section's relative size and flexibility. If Diff is negative the same adjustment is calculated, but it is then tested to ensure that it is not too negative for the section to handle. If it is too negative and the section would be unacceptably narrow after adjustment, then as much of the adjustment as possible is given to the section, and the rest is stored in Extra. That section's flexability is then set to 0 and it is marked as a spacer to avoid further attempts of recursive calls to extract space from it. If the adjustment is not too negative, then the line is adjusted the same as when Diff was positive. After all the lines have been looped through and their locations adjusted, a recursive call is made on Extra in an attempt to spread the homeless extra space out over some of the more flexible sections. 

### int aposSnapWidgetsToGrid(pXArray, int)
#### Description:
Called after the grid has been expanded or contracted out to make corresponding changes to the actual dimensions of the widgets.

#### Inputs:
A pointer to the array of lines being snapped to, and a flag indicating if they are vertical or horizontal.

#### Results:
The widgets' edges coincide once again with the their corresponding lines in the expanded or contracted grid.

#### Method:
Loops through all of the lines in the given array, and addresses the widgets stored in each line's SWidgets and EWidgets arrays; assigning the left/top edge of SWidgets and the right/bottom edge of EWidgets to the current line's location. Non-flexible widgets are not allowed to change width or height, although the logic in aposNonFlexChild should never allow this safety to be utilized. Tab widgets are snapped to points offset from their lines using flags set by aposSetOffsetBools, to countercompensate for the adjustment made in aposAddLinesForChildren.

### int aposProcessWindows(pWgtrNode, pWgtrNode)

#### Description:  
After all of the underlying widgets in a container have been auto-positioned, this function makes a final pass through the children and ensures that any windows are within that container's bounds. If windows are resized in doing so, their contents are subsequently auto-positioned.

#### Inputs:  
A pointer to the the last visual container encountered when going recursively into nonvisual containers, and a pointer to the container whose windows are to be checked.

#### Results:  
All windows in the given container are within that container's bounds, as well as any windows within those windows and any windows within nonvisual containers of the given container.

#### Method:  
isWin and isSp are set to help deal with any titlebars or scrollbars. The children are looped through and the windows examined. If a window is outside it's container's top or left border, it is moved in to a coordinate of (0,0); if it is bigger than its container, its size is decreased till it matches; and if it is hanging outside its container's bottom or right corner, it is moved up and to the left. Finally, if the window changed size, aposAutoPositionWidgetTree is called on the window to auto-position its contents accordingly. If any nonvisual containers are encountered, aposProcessWindows is called on them, but the original visual container is passed in to be used as a visual reference for windows found within the nonvisual container. 

### int aposFree(pAposGrid)
#### Description:  
Short function that frees dynamically allocated memory and deInits XArrays in the grid after it is used. 

#### Inputs:  
A pointer to the grid object.

#### Results:  
All memory associated with the grid is freed and XArray deinitiallized.

#### Method:  
First all the row and column objects are freed with nmFree. Horizontal and vertical lines follow, after their *widget XArrays are deInit'ed. Finally the 4 XArrays in the grid are deInit'ed.
