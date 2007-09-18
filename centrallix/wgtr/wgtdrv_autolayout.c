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

/**CVSDATA***************************************************************
 

 **END-CVSDATA***********************************************************/


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
    pWgtrNode widgetarray[64];
    pWgtrNode sortarray[64];
    int ord[64];
    int xo, yo;
    int al_type = -1;
    int i, j, rpti;
    char* ptr;
    int count,rptcount=0;
    int wgt_cnt;
    int n;
    int next_wgt;
    int spacing;
    int cellsize = -1;
    int maxsize = -1;
    int possible_width, possible_height, tw, th, a1, a2;

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

	    /** Auto-set width and height of this container? **/
	    if (al->r_width < 0 && al->r_height >= 0)
		{
		al->r_width = al->width = al->pre_width = wgtrGetMaxWidth(al, al->r_height);
		}
	    else if (al->r_width >= 0 && al->r_height < 0)
		{
		al->r_height = al->height = al->pre_height = wgtrGetMaxHeight(al, al->r_width);
		}
	    else if (al->r_width < 0 && al->r_height < 0)
		{
		possible_width = wgtrGetMaxWidth(al, 2);
		possible_height = wgtrGetMaxHeight(al, 2);
		th = wgtrGetMaxHeight(al, possible_width);
		a1 = possible_width * th;
		tw = wgtrGetMaxWidth(al, possible_height);
		a2 = possible_height * tw;
		if (a1 > a2)
		    {
		    al->r_width = al->width = al->pre_width = possible_width;
		    al->r_height = al->height = al->pre_height = th;
		    }
		else
		    {
		    al->r_width = al->width = al->pre_width = tw;
		    al->r_height = al->height = al->pre_height = possible_height;
		    }
		}

	    /** Grab the widgets and put them in a list **/
	    count = xaCount(&(al->Children));
	    for(wgt_cnt=i=0;i<count;i++)
		{
		child = (pWgtrNode)(xaGetItem(&(al->Children),i));
		if(child->Flags & WGTR_F_CONTROL)
		    {
		    /** Code for the repeat widget that could be extended to work for other types
		     ** of control structres that need to 'look through' to their subwidgets
		     **/
		    rptcount = xaCount(&(child->Children));
		    if(wgt_cnt+rptcount > sizeof(widgetarray)/sizeof(pWgtrNode))
			{
			mssError(1, "WGTRAL", "Too many widgets inside '%s'",al->Name);
			return -1;
			}
		    for(rpti=0;rpti<rptcount;rpti++)
			{
			widgetarray[wgt_cnt++] = (pWgtrNode)(xaGetItem((&child->Children),rpti));
			}
		    }
		else if(child->Flags & WGTR_F_NONVISUAL)
		    {
		    mssError(1, "WGTRAL", "Cannot place nonvisual widget '%s' inside %s '%s'", 
			    child->Name, al->Type, al->Name);
		    return -1; 
		    }
		else
		    {
		    if (wgt_cnt >= sizeof(sortarray)/sizeof(pWgtrNode))
			{
			mssError(1, "WGTRAL", "Too many widgets in inside %s '%s'", al->Type, al->Name);
			return -1;
			}
		    widgetarray[wgt_cnt++] = (pWgtrNode)(xaGetItem((&al->Children),i));
		    }
		}

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
		    child->x = child->r_x = child->pre_x = xo;
		    if (child->r_y < 0)
			child->r_y = child->pre_y = child->y = 0;
		    if (child->r_height < 0 && al->r_height >= 0)
			child->r_height = child->pre_height = child->height = al->r_height;
		    if (child->r_width < 0 && cellsize >= 0)
			child->r_width = child->pre_width = child->width = cellsize;
		    if (child->r_height > maxsize)
			maxsize = child->r_height;
		    wgtrReverify(s, child);
		    if (child->r_width >= 0)
			xo += child->r_width;
		    xo += spacing;
		    }
		else if (al_type == 1)	/* vbox */
		    {
		    if (child->r_x < 0)
			child->r_x = child->pre_x = child->x = 0;
		    child->y = child->r_y = child->pre_y = yo;
		    if (child->r_width < 0 && al->r_width >= 0)
			child->r_width = child->pre_width = child->width = al->r_width;
		    if (child->r_height < 0 && cellsize >= 0)
			child->r_height = child->pre_height = child->height = cellsize;
		    if (child->r_width > maxsize)
			maxsize = child->r_width;
		    wgtrReverify(s, child);
		    if (child->r_height >= 0)
			yo += child->r_height;
		    yo += spacing;
		    }
		//mssError(1, "WGTRAL","x=%d,r_x=%d,pre_x=%d,y=%d",child->x,child->r_x,child->pre_x,child->y);
		}

	    /** Set container size if needed **/
	    if (al_type == 0)		/* hbox */
		{
		if (al->r_width < 0 && xo >= 0)
		    al->r_width = al->pre_width = al->width = xo - spacing;
		if (al->r_height < 0 && maxsize >= 0)
		    al->r_height = al->pre_height = al->height = maxsize;
		}
	    else if (al_type == 1)	/* vbox */
		{
		if (al->r_height < 0 && yo >= 0)
		    al->r_height = al->pre_height = al->height = yo - spacing;
		if (al->r_width < 0 && maxsize >= 0)
		    al->r_width = al->pre_width = al->width = maxsize;
		}

	    /** If Width/Height unspecified, occupy entire container **/
	    /*if (al->r_width < 0 || al->r_height < 0)
		{
		parent = al->Parent;
		while(parent && (parent->Flags & WGTR_F_NONVISUAL))
		    parent = parent->Parent;
		if (parent)
		    {
		    xo = (al->r_x >= 0)?al->r_x:0;
		    yo = (al->r_y >= 0)?al->r_y:0;
		    if (al->r_width < 0)
			al->width = al->pre_width = al->r_width = parent->r_width - xo;
		    if (al->r_height < 0)
			al->height = al->pre_height = al->r_height = parent->r_height - yo;
		    }
		}*/
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
