/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	wgtr/wgtdrv_menu.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/

#include <string.h>
#include <strings.h>

#include "cxlib/datatypes.h"
#include "cxlib/range.h"
#include "cxlib/xarray.h"
#include "wgtr.h"


/*** Estimated heights of the insets htdrv_menu.c draws around a menu's rows.
 *** Each tracks specific markup there, so revisit these if it changes.
 ***/
#define MN_BORDER_H	2	/** #mnNmain 1px border, top plus bottom **/
#define MN_OUTER_GAP_H	2	/** cellspacing="1" wrapper table, top plus bottom **/
#define MN_ROW_GAP_H	2	/** cellspacing="2" row table, one gap **/
#define MN_SEP_H	4	/** the line a widget/menusep draws **/
#define MN_TRAILING_H	1	/** the trailing position tracking row **/


/*** wgtmenu_internal_RowHeight() - estimate the height of the row that
 *** htdrv_menu.c will draw for one child of a menu.  Every row holds a
 *** position tracking image of row_h, so none of them come out shorter.
 ***
 *** @param child The child widget to measure.
 *** @param row_h Height of one menu row.
 *** @returns The row height in px, or -1 if the child draws no row.
 ***/
int
wgtmenu_internal_RowHeight(pWgtrNode child, int row_h)
    {
	if (!strcmp(child->Type, "widget/menu")
	    || !strcmp(child->Type, "widget/menuitem")
	    || !strcmp(child->Type, "widget/menutitle"))
	    return row_h;
	if (!strcmp(child->Type, "widget/menusep"))
	    return max(row_h, MN_SEP_H);

    return -1;
    }


/*** wgtmenu_internal_EstimateRows() - sum and count the rows htdrv_menu.c will
 *** draw for a vertical menu.  Mirrors htmenuRender(), which looks through a
 *** control structure (e.g. widget/repeat) exactly one level, and gives every
 *** row it draws one call to htmenu_internal_AddDot().
 ***
 *** @param menu    The menu whose children are to be measured.
 *** @param row_h   Height of one menu row.
 *** @param row_cnt Incremented once per row found.
 *** @returns The summed height of the rows, in px.
 ***/
int
wgtmenu_internal_EstimateRows(pWgtrNode menu, int row_h, int* row_cnt)
    {
    pWgtrNode child;
    pWgtrNode sub;
    int i, j, cnt, subcnt;
    int this_h;
    int height = 0;

	cnt = xaCount(&(menu->Children));
	for (i = 0; i < cnt; i++)
	    {
	    child = xaGetItem(&(menu->Children), i);
	    if (child->Flags & WGTR_F_CONTROL)
		{
		/** Look through the control structure, but no deeper. **/
		subcnt = xaCount(&(child->Children));
		for (j = 0; j < subcnt; j++)
		    {
		    sub = xaGetItem(&(child->Children), j);
		    if ((this_h = wgtmenu_internal_RowHeight(sub, row_h)) < 0)
			continue;
		    height += this_h;
		    (*row_cnt)++;
		    }
		continue;
		}
	    if ((this_h = wgtmenu_internal_RowHeight(child, row_h)) < 0)
		continue;
	    height += this_h;
	    (*row_cnt)++;
	    }

    return height;
    }


/*** wgtmenuVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtmenuVerify(pWgtrVerifySession s)
    {
    pWgtrNode menu = s->CurrWidget;
    char* str;
    int i;
    int min_height = s->ClientInfo->ParagraphHeight + 4;
    int para_h = s->ClientInfo->ParagraphHeight;
    int row_h, row_cnt, est_height;

	if (menu->min_height < min_height) menu->min_height = min_height;

	if (menu->Parent && !strcmp(menu->Parent->Type, "widget/menu"))
	    {
	    menu->Flags |= WGTR_F_FLOATING;
	    if (wgtrGetPropertyType(menu, "popup") < 0)
		{
		str = "yes";
		wgtrAddProperty(menu, "popup", DATA_T_STRING, POD(&str), 0);
		}
	    if (wgtrGetPropertyType(menu, "direction") < 0)
		{
		str = "vertical";
		wgtrAddProperty(menu, "direction", DATA_T_STRING, POD(&str), 0);
		}
	    }
	if (wgtrGetPropertyValue(menu, "popup", DATA_T_STRING, POD(&str)) == 0)
	    {
	    if (!strcasecmp(str,"yes") || !strcasecmp(str,"true") || !strcasecmp(str,"on"))
		menu->Flags |= WGTR_F_FLOATING;
	    }
	else if (wgtrGetPropertyValue(menu, "popup", DATA_T_INTEGER, POD(&i)) == 0 && i)
	    menu->Flags |= WGTR_F_FLOATING;

	/** Only a menu draws rows; the other types keep the one-line estimate. **/
	est_height = min_height;
	if (!strcmp(menu->Type, "widget/menu"))
	    {
	    /*** Estimate one row's height.  A taller icon or checkbox overflows
	     *** it, since image sizes are unknown here.
	     ***/
	    if (wgtrGetPropertyValue(menu, "row_height", DATA_T_INTEGER, POD(&row_h)) != 0)
		row_h = 0;
	    row_h = max(row_h, para_h);

	    /** A menu is never shorter than one line of text plus its insets. **/
	    min_height = para_h + MN_ROW_GAP_H * 2 + MN_OUTER_GAP_H + MN_BORDER_H;
	    menu->min_height = max(menu->min_height, min_height);

	    /*** A horizontal menu fits every item in the one row.  A vertical
	     *** one gets a row per item, plus a trailing tracking row.
	     ***/
	    if (wgtrGetPropertyValue(menu, "direction", DATA_T_STRING, POD(&str)) == 0
		    && !strcmp(str, "vertical"))
		{
		row_cnt = 1;	/** the trailing tracking row **/
		est_height = MN_TRAILING_H
		    + wgtmenu_internal_EstimateRows(menu, row_h, &row_cnt)
		    + MN_ROW_GAP_H * (row_cnt + 1)
		    + MN_OUTER_GAP_H + MN_BORDER_H;
		}
	    else
		est_height = row_h + MN_ROW_GAP_H * 2 + MN_OUTER_GAP_H + MN_BORDER_H;
	    }

	/** A menu with no height sizes itself in the browser; estimate it here. **/
	if (menu->height < 0)
	    {
	    menu->Flags |= WGTR_F_AUTOHEIGHT;
	    menu->height = menu->pre_height = max(est_height, min_height);
	    }

    return 0;
    }


/*** wgtmenuNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtmenuNew(pWgtrNode node)
    {

	if(node->fl_width < 0) node->fl_width = 25;
	if(node->fl_height < 0) node->fl_height = 1;
	
	/*** Declare a 1px inset for the border that htdrv_menu.c draws around
	 *** the menu itself.  The other types render no box of their own.
	 ***/
	if (!strcmp(node->Type, "widget/menu"))
	    wgtrSetInsets(node, 1, 1, 1, 1);

    return 0;
    }


int
wgtmenuInitialize()
    {
    char* name = "Menu Widget Driver";

	wgtrRegisterDriver(name, wgtmenuVerify, wgtmenuNew);
	wgtrAddType(name, "menu");
	wgtrAddType(name, "menuitem");
	wgtrAddType(name, "menusep");
	wgtrAddType(name, "menutitle");

	return 0;
    }
