#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "wgtr.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	wgtr/wgtdrv_autolayout.c				*/
/* Author:	Greg Beeley (GRB)		 			*/
/* Creation:	March 21, 2007						*/
/* Description:								*/
/************************************************************************/



int
wgtal_internal_AddToArray(pWgtrNode node, pWgtrNode widgetarray[], int * wgt_cnt, int maxsize)
    {
    int count;
    int i;
    pWgtrNode child;

	count = xaCount(&(node->Children));
	if((*wgt_cnt)+count > maxsize)
	    {
	    mssError(1, "WGTRAL", "Too many widgets inside '%s'",node->Name);
	    return -1;
	    }

	for(i=0;i<count;i++)
	    {
	    child = (pWgtrNode)(xaGetItem((&node->Children),i));
	    if((child->Flags & WGTR_F_CONTROL) || (child->Flags & WGTR_F_NONVISUAL))
		{
		if (wgtal_internal_AddToArray(child, widgetarray, wgt_cnt, maxsize) < 0)
		    return -1;
		}
	    else
		{
		widgetarray[(*wgt_cnt)++] = child;
		}
	    }

    return 0;
    }


/*** wgtalVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtalVerify(pWgtrVerifySession s)
    {
    pWgtrNode al = s->CurrWidget;
    pWgtrNode child;
    pWgtrNode widgetarray[80];
    pWgtrNode sortarray[80];
    int ord[80];
    int xo, yo;
    int al_type = -1;
    int i, j;
    char* ptr;
    int wgt_cnt;
    int n, n_remain;
    int next_wgt;
    int spacing;
    int cellsize = -1;
    int maxheight = -1;
    int maxwidth = -1;
    int possible_width, possible_height, tw, th, a1, a2;
    int column_width = -1, row_height = -1;
    int column_offset = 0, row_offset = 0;
    char* justify_mode;
    int adj, adj_remain;
    int this_adj;
    char* align;
    int total_flex, total_size, total_product;
    int after;

	/** Ignore this if it is just a spacer **/
	if (strcmp(al->Type, "widget/autolayoutspacer") != 0)
	    {
	    /** Style specified by type name? **/
	    if (!strcmp(al->Type, "widget/hbox"))
		al_type = 0;
	    else if (!strcmp(al->Type, "widget/vbox"))
		al_type = 1;
		
	    /** Get autolayout type (hbox/vbox/etc) if not already given (by Type). **/
	    if (al_type == -1)
		{
		if (wgtrGetPropertyValue(al, "style", DATA_T_STRING, POD(&ptr)) != 0)
		    {
		    mssError(1, "WGTRAL", "Autolayout style must be specified for widget '%s'", al->Name);
		    return -1;
		    }
		if (!strcmp(ptr, "hbox"))
		    al_type = 0;
		else if (!strcmp(ptr, "vbox"))
		    al_type = 1;
		else
		    {
		    mssError(1, "WGTRAL", "Invalid autolayout style '%s' for widget '%s'", ptr, al->Name);
		    return -1;
		    }
		}

	    /** Spacing between widgets **/
	    if (wgtrGetPropertyValue(al, "spacing", DATA_T_INTEGER, POD(&spacing)) != 0)
		spacing = 0;

	    /** Size of each cell **/
	    if (wgtrGetPropertyValue(al, "cellsize", DATA_T_INTEGER, POD(&cellsize)) != 0)
		cellsize = -1;

	    /** Alignment and Justification **/
	    if (wgtrGetPropertyValue(al, "align", DATA_T_STRING, POD(&align)) != 0)
		align = "left";
	    if (wgtrGetPropertyValue(al, "justify", DATA_T_STRING, POD(&justify_mode)) != 0)
		justify_mode = "none";

	    /** Auto-set width and height of this container? **/
	    if (al->width < 0 && al->height >= 0)
		{
		al->width = al->pre_width = wgtrGetMaxWidth(al, al->height);
		}
	    else if (al->width >= 0 && al->height < 0)
		{
		al->height = al->pre_height = wgtrGetMaxHeight(al, al->width);
		}
	    else if (al->width < 0 && al->height < 0)
		{
		possible_width = wgtrGetMaxWidth(al, 2);
		possible_height = wgtrGetMaxHeight(al, 2);
		th = wgtrGetMaxHeight(al, possible_width);
		a1 = possible_width * th;
		tw = wgtrGetMaxWidth(al, possible_height);
		a2 = possible_height * tw;
		if (a1 > a2)
		    {
		    al->width = al->pre_width = possible_width;
		    al->height = al->pre_height = th;
		    }
		else
		    {
		    al->width = al->pre_width = tw;
		    al->height = al->pre_height = possible_height;
		    }
		}

	    /** Width of columns? **/
	    if (al_type == 1 && wgtrGetPropertyValue(al, "column_width", DATA_T_INTEGER, POD(&column_width)) == 0)
		{
		if (column_width > al->width)
		    column_width = al->width;
		}

	    /** Height of rows? **/
	    if (al_type == 0 && wgtrGetPropertyValue(al, "row_height", DATA_T_INTEGER, POD(&row_height)) == 0)
		{
		if (row_height > al->height)
		    row_height = al->height;
		}

	    /** Grab the widgets and put them in a list **/
	    wgt_cnt = 0;
	    if (wgtal_internal_AddToArray(al, widgetarray, &wgt_cnt, sizeof(widgetarray)/sizeof(pWgtrNode)) < 0)
		return -1;

	    /** Sort the list into the order they will be laid out **/
	    for(i=0;i<wgt_cnt;i++)
		{
		child = widgetarray[i];
		if (wgtrGetPropertyValue(child, "autolayout_order", DATA_T_INTEGER, POD(&n)) == 0 && n >= 0)
		    {
		    ord[i] = n;
		    }
		else
		    {
		    if (i)
			ord[i] = ord[i-1];
		    else
			ord[i] = 100;
		    }
		}
	    for(i=0;i<wgt_cnt;i++)
		{
		next_wgt = 0;
		for(j=0;j<wgt_cnt;j++)
		    {
		    if (ord[j] < ord[next_wgt]) next_wgt = j;
		    }
		sortarray[i] = widgetarray[next_wgt];
		ord[next_wgt] = 0x7FFFFFFF;
		}

	    
	    /** Ok, now set the x and y for all the widgets **/
	    xo = yo = 0;
	    for(i=0;i<wgt_cnt;i++)
		{
		child = sortarray[i];
		if (al_type == 0)	/* hbox */
		    {
		    if (child->width < 0 && cellsize >= 0)
			possible_width = cellsize;
		    else
			possible_width = child->width;
		    if (xo + possible_width > al->width)
			{
			if (xo > 0 && row_height > 0 && row_offset + row_height*2 + spacing <= al->height)
			    {
			    row_offset += (row_height + spacing);
			    xo = 0;
			    maxheight = -1;
			    i--;
			    continue;
			    }
			else
			    mssError(1, "WGTRAL", "Warning: overflow of end of hbox '%s'",al->Name);
			}
		    child->x = child->pre_x = xo;
		    child->pre_width = child->width = possible_width;
		    if (child->y < 0)
			child->pre_y = child->y = row_offset;
		    else
			child->pre_y = child->y = child->y + row_offset;
		    if (child->height < 0 && al->height >= 0)
			child->pre_height = child->height = al->height;
		    if (row_height > 0 && child->height > row_height)
			child->pre_height = child->height = row_height;
		    if (child->height > maxheight)
			maxheight = child->height;
		    wgtrReverify(s, child);
		    if (child->width >= 0)
			xo += child->width;
		    xo += spacing;
		    if (maxwidth < xo) maxwidth = xo;
		    }
		else if (al_type == 1)	/* vbox */
		    {
		    if (child->height < 0 && cellsize >= 0)
			possible_height = cellsize;
		    else
			possible_height = child->height;
		    if (yo + possible_height > al->height)
			{
			if (yo > 0 && column_width > 0 && column_offset + column_width*2 + spacing <= al->width)
			    {
			    column_offset += (column_width + spacing); 
			    yo = 0;
			    maxwidth = -1;
			    i--;
			    continue;
			    }
			else
			    mssError(1, "WGTRAL", "Warning: overflow of end of vbox '%s'",al->Name);
			}
		    child->y = child->pre_y = yo;
		    child->pre_height = child->height = possible_height;
		    if (child->x < 0)
			child->pre_x = child->x = column_offset;
		    else
			child->pre_x = child->x = child->x + column_offset;
		    if (child->width < 0 && al->width >= 0)
			child->pre_width = child->width = al->width;
		    if (column_width > 0 && child->width > column_width)
			child->pre_width = child->width = column_width;
		    if (child->width > maxwidth)
			maxwidth = child->width;
		    wgtrReverify(s, child);
		    if (child->height >= 0)
			yo += child->height;
		    yo += spacing;
		    if (maxheight < yo) maxheight = yo;
		    }
		//mssError(1, "WGTRAL","x=%d,r_x=%d,pre_x=%d,y=%d",child->x,child->r_x,child->pre_x,child->y);
		}

	    /** Set container size if needed **/
	    if (al_type == 0)		/* hbox */
		{
		if (al->width < 0 && maxwidth >= 0)
		    al->pre_width = al->width = maxwidth - spacing;
		if (al->height < 0 && maxheight >= 0)
		    al->pre_height = al->height = maxheight + row_offset;
		}
	    else if (al_type == 1)	/* vbox */
		{
		if (al->height < 0 && maxheight >= 0)
		    al->pre_height = al->height = maxheight - spacing;
		if (al->width < 0 && maxwidth >= 0)
		    al->pre_width = al->width = maxwidth + column_offset;
		}

	    /** Full justification of some or all rows/columns? **/
	    if (!strcmp(justify_mode, "all") || !strcmp(justify_mode, "filled"))
		{
		/** Loop through rows/columns **/
		for(j=0;;j++)
		    {
		    /** Step One:  Total up the sizes and flexibilities **/
		    total_flex = total_size = total_product = 0;
		    for(after=n=i=0;i<wgt_cnt;i++)
			{
			child = sortarray[i];
			after++;
			if (al_type == 0 && ((j == 0 && row_height <= 0) || (row_height > 0 && spacing >= 0 && (int)(child->y / (row_height + spacing)) == j)))
			    {
			    /** widget in an hbox **/
			    if (child->fl_width > 0)
				{
				total_flex += child->fl_width;
				total_product += (child->fl_width * child->width);
				}
			    total_size += child->width;
			    n++;
			    after=0;
			    }
			else if (al_type == 1 && ((j == 0 && column_width <= 0) || (column_width > 0 && spacing >= 0 && (int)(child->x / (column_width + spacing)) == j)))
			    {
			    /** widget in a vbox **/
			    if (child->fl_height > 0)
				{
				total_flex += child->fl_height;
				total_product += (child->fl_height * child->height);
				}
			    total_size += child->height;
			    n++;
			    after=0;
			    }
			}

		    /** No widgets in this row/col found?  End of columns/rows if so. **/
		    if (n == 0)
			{
			break;
			}

		    /** Step Two:  Scale the widgets if justification is needed. **/
		    if (!strcmp(justify_mode,"all") || (after > 0 && !strcmp(justify_mode,"filled")))
			{
			/** Determine how much total scaling is needed **/
			if (al_type == 0)
			    adj = al->width - total_size - (n-1)*spacing;
			else
			    adj = al->height - total_size - (n-1)*spacing;
			adj_remain = adj;
			n_remain = n;

			/** Loop through the widgets and scale them. **/
			for(i=0;i<wgt_cnt;i++)
			    {
			    child = sortarray[i];
			    if (al_type == 0 && ((j == 0 && row_height <= 0) || (row_height > 0 && spacing >= 0 && (int)(child->y / (row_height + spacing)) == j)))
				{
				/** hbox **/
				if (n_remain == 1)
				    this_adj = adj_remain;
				else
				    this_adj = adj * (child->fl_width * child->width) / total_product;

				/** We don't set the ->pre_* properties, in order for APOS to do its magic. **/
				child->x += (adj - adj_remain);
				child->width += this_adj;
				wgtrReverify(s, child);
				adj_remain -= this_adj;
				n_remain --;
				}
			    else if (al_type == 1 && ((j == 0 && column_width <= 0) || (column_width > 0 && spacing >= 0 && (int)(child->x / (column_width + spacing)) == j)))
				{
				/** vbox **/
				if (n_remain == 1)
				    this_adj = adj_remain;
				else
				    this_adj = adj * (child->fl_height * child->height) / total_product;

				/** We don't set the ->pre_* properties, in order for APOS to do its magic. **/
				child->y += (adj - adj_remain);
				child->height += this_adj;
				wgtrReverify(s, child);
				adj_remain -= this_adj;
				n_remain --;
				}
			    if (n_remain == 0) break;
			    }
			}
		    }
		}

	    /** Center or Right/Bottom alignment? **/
	    if (!strcmp(align, "center") || !strcmp(align, "right") || !strcmp(align, "bottom"))
		{
		if (al_type == 0)	/* hbox */
		    {
		    if (!strcmp(align, "center"))
			adj = (al->width - (maxwidth - spacing)) / 2;
		    else
			adj = (al->width - (maxwidth - spacing));
		    if (maxwidth - spacing < al->width)
			for(i=0;i<wgt_cnt;i++)
			    {
			    child = sortarray[i];
			    child->x += adj;
			    child->pre_x += adj;
			    }
		    }
		else			/* vbox */
		    {
		    if (!strcmp(align, "center"))
			adj = (al->height - (maxheight - spacing)) / 2;
		    else
			adj = (al->height - (maxheight - spacing));
		    if (maxheight - spacing < al->height)
			for(i=0;i<wgt_cnt;i++)
			    {
			    child = sortarray[i];
			    child->y += adj;
			    child->pre_y += adj;
			    }
		    }
		}
	    }

    return 0;
    }


/*** wgtalNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtalNew(pWgtrNode node)
    {

	if (!strcmp(node->Type, "widget/autolayoutspacer"))
	    {
	    if(node->fl_width < 0) node->fl_width = 5;
	    if(node->fl_height < 0) node->fl_height = 5;
	    }
	else
	    {
	    node->Flags |= WGTR_F_CONTAINER;   
	    if(node->fl_width < 0) node->fl_width = 100;
	    if(node->fl_height < 0) node->fl_height = 100;
	    }
	
    return 0;
    }


int
wgtalInitialize()
    {
    char* name="AutoLayout Driver";

	wgtrRegisterDriver(name, wgtalVerify, wgtalNew);
	wgtrAddType(name, "autolayout");
	wgtrAddType(name, "autolayoutspacer");
	wgtrAddType(name, "hbox");
	wgtrAddType(name, "vbox");

    return 0;
    }
