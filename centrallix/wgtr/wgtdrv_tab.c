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
/* Module: 	wgtr/wgtdrv_tab.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/



/*** wgttab_internal_ClientSize - shrinks one of a tab control's geometry
 *** values by the insets along that axis, yielding the size of its client
 *** area.
 ***/
static int
wgttab_internal_ClientSize(int size, int inset)
    {
	/** A negative size means "unspecified", so pass it through. **/
	if (size < 0) return size;

	/** A tab smaller than its own insets has no room left for children. **/
	if (size < inset) return 0;

	return size - inset;
    }


int
wgttab_internal_SetTabpageGeom(pWgtrNode tab, pWgtrNode container)
    {
    int i=0;
    const int count = xaCount(&(container->Children));
    const int inset_w = tab->inset_left + tab->inset_right;
    const int inset_h = tab->inset_top + tab->inset_bottom;
    pWgtrNode child;

	    for(i=0; i<count; ++i)	//loop through tab children
	        {
		    child = (pWgtrNode)(xaGetItem(&(container->Children), i));
		    if(!strcmp(child->Type, "widget/tabpage"))
		        {
			    /*** Set the geometry of the tabpage to fill the tab
			     *** control's client area, which is the area left
			     *** over inside the tab strip and the border.
			     ***/
			    child->r_x = child->r_y = 0;
			    child->r_width    = wgttab_internal_ClientSize(tab->r_width,    inset_w);
			    child->r_height   = wgttab_internal_ClientSize(tab->r_height,   inset_h);
			    child->pre_width  = wgttab_internal_ClientSize(tab->pre_width,  inset_w);
			    child->pre_height = wgttab_internal_ClientSize(tab->pre_height, inset_h);
			    child->min_width  = wgttab_internal_ClientSize(tab->min_width,  inset_w);
			    child->min_height = wgttab_internal_ClientSize(tab->min_height, inset_h);

			    child->x = child->y = 0;
			    child->width  = wgttab_internal_ClientSize(tab->width,  inset_w);
			    child->height = wgttab_internal_ClientSize(tab->height, inset_h);
		        }
		    else if ((child->Flags & WGTR_F_NONVISUAL) && (child->Flags & WGTR_F_CONTAINER))
			{
			    /** Might be more tabpages within a nonvisual container like widget/repeat **/
			    wgttab_internal_SetTabpageGeom(tab, child);
			}
		}

    return 0;
    }


/*** wgttabVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgttabVerify(pWgtrVerifySession s)
    {
    /*** Loops through the tabpage children of the tab widget and
     *** initializes the requested and actual geometry of each 
     *** one to match that of the tab widget itself, minus a few
     *** pixels to account for the border. Necessary for auto-
     *** positioning.
     ***/
    pWgtrNode tab = s->CurrWidget;
    
	if(!strcmp(tab->Type, "widget/tab"))
	    wgttab_internal_SetTabpageGeom(tab, tab);
	    
    return 0;
    }


/*** wgttabNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgttabNew(pWgtrNode node)
    {
    int tloc;
    char* ptr;
    int tw = 0, th;
    int border_width = 1;
    int strip_top = 0, strip_bottom = 0, strip_left = 0, strip_right = 0;

	node->Flags |= WGTR_F_CONTAINER;
	if(node->fl_width < 0) node->fl_width = 100;
	if(node->fl_height < 0) node->fl_height = 100;

	/** Set insets if this is a tab control widget **/
	if(!strcmp(node->Type, "widget/tab"))
	    {
	    if (wgtrGetPropertyValue(node, "tab_location", DATA_T_STRING, POD(&ptr)) == 0)
		{
		if (!strcasecmp(ptr,"top")) tloc = 0;
		else if (!strcasecmp(ptr,"bottom")) tloc = 1;
		else if (!strcasecmp(ptr,"left")) tloc = 2;
		else if (!strcasecmp(ptr,"right")) tloc = 3;
		else if (!strcasecmp(ptr,"none")) tloc = 4;
		else
		    {
		    mssError(1,"WGTTAB","%s: '%s' is not a valid tab_location",node->Name,ptr);
		    return -1;
		    }
		}
	    else
		{
		tloc = 0;
		}

	    if (wgtrGetPropertyValue(node, "tab_width", DATA_T_INTEGER, POD(&tw)) != 0 || tw <= 0)
		{
		if (tloc == 2 || tloc == 3)
		    {
		    mssError(1,"WGTTAB","%s: must specify a valid tab width when tab location is left or right",node->Name);
		    return -1;
		    }
		}

	    /*** How thick is the strip of tabs, and which edge is it on?  An
	     *** invalid tab_height is reported by htdrv_tab.c so here we just
	     *** fall back to the 24px default value.
	     ***/
	    if (wgtrGetPropertyValue(node, "tab_height", DATA_T_INTEGER, POD(&th)) != 0 || th <= 0)
		th = 24;
	    switch(tloc)
		{
		case 0:	strip_top = th; break;		/* top */
		case 1:	strip_bottom = th; break;	/* bottom */
		case 2:	strip_left = tw; break;		/* left */
		case 3:	strip_right = tw; break;	/* right */
		default: break;				/* none */
		}

	    /*** BACKWARD COMPATIBILITY HACK: in the structure file language, a
	     *** tab control's 'height' and 'width' describe only its content
	     *** area, and the strip of tabs is drawn *outside* of that, so the
	     *** widget's real footprint is larger than its requested geometry.
	     *** That contradicts every other widget where x/y/width/height are
	     *** their requested size, but changing the language to fix this
	     *** breaks every existing design with tabs.  Thus, we grow the
	     *** requested size by the thickness of the strip here to maintain
	     *** this incorrect behavior.
	     ***
	     *** Remove this hacky patch once the language is fixed to treat a
	     *** tab control's geometry as part of the tab widget.
	     ***/
	    if (node->r_width    >= 0) node->r_width    += strip_left + strip_right;
	    if (node->width      >= 0) node->width      += strip_left + strip_right;
	    if (node->pre_width  >= 0) node->pre_width  += strip_left + strip_right;
	    if (node->r_height   >= 0) node->r_height   += strip_top  + strip_bottom;
	    if (node->height     >= 0) node->height     += strip_top  + strip_bottom;
	    if (node->pre_height >= 0) node->pre_height += strip_top  + strip_bottom;

	    /*** Declare the insets around the client area, which is the
	     *** #tcNctrl layer that holds the tab pages.  Both the tab strip
	     *** and the 1px border that htdrv_tab.c draws around that layer
	     *** are inside this widget's outer box.
	     ***/
	    if (wgtrGetPropertyValue(node, "border_style", DATA_T_STRING, POD(&ptr)) == 0
		&& (strcmp(ptr, "none") == 0 || strcmp(ptr, "hidden") == 0))
		border_width = 0;
	    wgtrSetInsets(node,
		strip_top + border_width,
		strip_bottom + border_width,
		strip_left + border_width,
		strip_right + border_width
	    );
	    }

    return 0;
    }


int
wgttabInitialize()
    {
    char* name = "Tab Control / Tab Page Driver";

	wgtrRegisterDriver(name, wgttabVerify, wgttabNew);
	wgtrAddType(name, "tab");
	wgtrAddType(name, "tabpage");

    return 0;
    }
