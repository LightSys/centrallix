#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
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
/* Module: 	htdrv_form.c      					*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is the non-visual widget that interfaces the 	*/
/*		objectsource widget and the visual sub-widgets		*/
/* See centrallix-sysdoc/HTFormsInterface.md for more information. */
/************************************************************************/


/*** htformRender - generate the HTML code for the form 'glue'
 ***/
int
htformRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char name[64];
    char tabmode[64];
    char osrc[64];
    char link_next[64];
    char link_next_within[64];
    char link_prev[64];
    char link_prev_within[64];
    char interlock_with[256];
    int i, t;
    int allowquery, allownew, allowmodify, allowview, allownodata, multienter, allowdelete, confirmdelete;
    int confirmdiscard, allowmerge;
    int allowobscure = 0;
    int autofocus;
    char _3bconfirmwindow[30];
    int readonly;
    int tro;
    int enter_mode;
    pStringVec sv;

	/** form widget should work on any browser **/

	/** Get params. **/
	allowquery = htrGetBoolean(tree, "allow_query", 1);
	allownew = htrGetBoolean(tree, "allow_new", 1);
	allowmodify = htrGetBoolean(tree, "allow_modify", 1);
	allowview = htrGetBoolean(tree, "allow_view", 1);
	allownodata = htrGetBoolean(tree, "allow_nodata", 1);
	allowdelete = htrGetBoolean(tree, "allow_delete", 1);
	confirmdelete = htrGetBoolean(tree, "confirm_delete", 1);

	confirmdiscard = htrGetBoolean(tree, "confirm_discard", 1);

	allowmerge = htrGetBoolean(tree, "allow_merge", 0);

	autofocus = htrGetBoolean(tree, "auto_focus", 1);

	tro = htrGetBoolean(tree, "tab_revealed_only", 0);

	if (wgtrGetPropertyValue(tree,"enter_mode",DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr, "save"))
		enter_mode = 0;
	    else if (!strcasecmp(ptr, "nextfield"))
		enter_mode = 1;
	    else if (!strcasecmp(ptr, "lastsave"))
		enter_mode = 2;
	    else
		enter_mode = 0;
	    }
	else
	    {
	    /** save **/
	    enter_mode = 0;
	    }

	/** The way I read the specs -- overriding this resides in 
	 **   the code, not here -- JDR **/
	if (wgtrGetPropertyValue(tree,"MultiEnter",DATA_T_INTEGER,POD(&multienter)) != 0) 
	    multienter=0;
	if (wgtrGetPropertyValue(tree,"TabMode",DATA_T_STRING,POD(&ptr)) != 0) 
	    tabmode[0]='\0';
	else
	    strtcpy(tabmode, ptr,sizeof(tabmode));
	if (wgtrGetPropertyValue(tree,"ReadOnly",DATA_T_INTEGER,POD(&readonly)) != 0) 
	    readonly=0;

	/*** 03/16/02 Jonathan Rupp
	 ***   added _3bconfirmwindow, the name of a window that has 
	 ***     three button objects, _3bConfirmCancel, 
	 ***     _3bConfirmCancel, and _3bConfirmCancel.  There can
	 ***     be no connectors attached to these buttons.
	 ***     this window should be set to hidden -- the form
	 ***     will make it visible when needed, and hide it again
	 ***     afterwards.
	 ***   There is a Action to test this new functionality:
	 ***     .Actiontest3bconfirm()
	 ***     -- once this is used elsewhere, this function will
	 ***        be removed
	 ***/
	
	if (wgtrGetPropertyValue(tree,"_3bconfirmwindow",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(_3bconfirmwindow,ptr,sizeof(_3bconfirmwindow));
	else
	    strcpy(_3bconfirmwindow,"null");

	if (wgtrGetPropertyValue(tree,"objectsource",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(osrc,ptr,sizeof(osrc));
	else
	    strcpy(osrc,"");
	
	if (wgtrGetPropertyValue(tree,"next_form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(link_next,ptr,sizeof(link_next));
	else
	    strcpy(link_next,"");
	
	if (wgtrGetPropertyValue(tree,"next_form_within",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(link_next_within,ptr,sizeof(link_next_within));
	else
	    strcpy(link_next_within,"");
	
	if (wgtrGetPropertyValue(tree,"prev_form",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(link_prev,ptr,sizeof(link_prev));
	else
	    strcpy(link_prev,"");
	
	if (wgtrGetPropertyValue(tree,"prev_form_within",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(link_prev_within,ptr,sizeof(link_prev_within));
	else
	    strcpy(link_prev_within,"");
	
	/*** 
	 *** (03/01/02) Jonathan Rupp -- added two new paramters
	 ***      basequery -- the part of the SQL statement that is never
	 ***        modified, even in QBF form (do not terminate with ';')
	 ***      basewhere -- an initial WHERE clause (do not include WHERE)
	 ***   example:
	 ***     basequery="SELECT a,b,c from data"
	 ***     +- no WHERE clause by default, can be added in QBF
	 ***     basequery="SELECT a,b,c from data"
	 ***     basewhere="a=true"
	 ***     +- by default, only show fields where a is true
	 ***          QBF will override
	 ***     basequery="SELECT a,b,c from data WHERE b=false"
	 ***     +- only will show rows where b is false
	 ***          can't be overridden in QBF, can be added to
	 ***     basequery="SELECT a,b,c from data WHERE b=false"
	 ***     basewhere="c=true"
	 ***     +- only will show rows where b is false
	 ***          in addition, at start, will only show where c is true
	 ***          condition (c=true) can be overridden, (b=false) can't
	 ***/

	/** Should we allow obscures for this form? **/
	allowobscure = htrGetBoolean(tree, "allow_obscure", 0);

	/** Interlocked forms (only one in interlock group allowed to edit/create **/
	interlock_with[0] = '\0';
	if ((t = wgtrGetPropertyType(tree,"interlock_with")) == DATA_T_STRING)
	    {
	    if (wgtrGetPropertyValue(tree,"interlock_with",DATA_T_STRING, POD(&ptr)) == 0)
		strtcpy(interlock_with, ptr, sizeof(interlock_with));
	    }
	else if (t == DATA_T_STRINGVEC)
	    {
	    if (wgtrGetPropertyValue(tree,"interlock_with",DATA_T_STRINGVEC, POD(&sv)) == 0)
		{
		for(i=0;i<sv->nStrings;i++)
		    {
		    if(i)
			strtcat(interlock_with, ",", sizeof(interlock_with));
		    strtcat(interlock_with, sv->Strings[i], sizeof(interlock_with));
		    }
		}
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** create our instance variable **/
	if (htrAddWgtrCtrLinkage(s, tree, "_parentctr") != 0) 
	    {
	    mssError(0, "HTFORM", "Failed to add container linkage.");
	    goto err;
	    }

	/** Script include to add functions **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0)) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_form.js", 0)) goto err;

	/** Write out the init line for this instance of the form
	 **   the name of this instance was defined to be global up above
	 **   and fm_current is defined in htdrv_page.c 
	 **/
	const int null_confirm_window = (strcmp(_3bconfirmwindow, "null") == 0);
	const int no_osrc = (osrc == NULL || osrc[0] == '\0');
	const int no_link_next = (link_next == NULL || link_next[0] == '\0');
	const int no_link_next_within = (link_next_within == NULL || link_next_within[0] == '\0');
	const int no_link_prev = (link_prev == NULL || link_prev[0] == '\0');
	const int no_link_prev_within = (link_prev_within == NULL || link_prev_within[0] == '\0');
	if (htrAddScriptInit_va(s,
	    "\t{ "
		"const node = wgtrGetNodeRef(ns, '%STR&SYM'); "
		"form_init(node, { "
		    "aq:%INT, an:%INT, am:%INT, av:%INT, and:%INT, ad:%INT, "
		    "cd:%INT, cdis:%INT, amrg:%INT, me:%INT, name:'%STR&SYM', "
		    "_3b:%[wgtrGetNodeRef(ns, '%STR&SYM')%]%[null%], "
		    "ro:%INT, ao:%INT, af:%INT, "
		    "osrc:%['%STR&SYM'%]%[null%], "
		    "tro:%INT, em:%INT, "
		    "nf:%['%STR&SYM'%]%[null%], "
		    "nfw:%['%STR&SYM'%]%[null%], "
		    "pf:%['%STR&SYM'%]%[null%], "
		    "pfw:%['%STR&SYM'%]%[null%], "
		    "il:'%STR&JSSTR', "
		"});"
		"node.ChangeMode('NoData');"
	    "}\n",
	    name,
	    allowquery, allownew, allowmodify, allowview, allownodata, allowdelete,
	    confirmdelete, confirmdiscard, allowmerge, multienter, name,
	    (!null_confirm_window), _3bconfirmwindow, (null_confirm_window),
	    readonly, allowobscure, autofocus,
	    (!no_osrc), osrc, (no_osrc),
	    tro, enter_mode,
	    (!no_link_next), link_next, (no_link_next),
	    (!no_link_next_within), link_next_within, (no_link_next_within),
	    (!no_link_prev), link_prev, (no_link_prev),
	    (!no_link_prev_within), link_prev_within, (no_link_prev_within),
	    interlock_with
	) != 0) 
	    {
	    mssError(0, "HTFORM", "Failed to render child widgets.");
	    goto err;
	    }

	/** Render children. **/
	if (htrRenderSubwidgets(s, tree, z) != 0) goto err;

	return 0;

    err:
	mssError(0, "HTFORM",
	    "Failed to render \"%s\":\"%s\".",
	    tree->Name, tree->Type
	);
	return -1;
    }


/*** htformInitialize - register with the ht_render module.
 ***/
int
htformInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Form Widget");
	strcpy(drv->WidgetName,"form");
	drv->Render = htformRender;

	/** Add our actions **/
	htrAddAction(drv,"Clear");
	htrAddAction(drv,"Delete");
	htrAddAction(drv,"Discard");
	htrAddAction(drv,"Edit");
	htrAddAction(drv,"First");
	htrAddAction(drv,"Last");
	htrAddAction(drv,"New");
	htrAddAction(drv,"Next");
	htrAddAction(drv,"Prev");
	htrAddAction(drv,"Query");
	htrAddAction(drv,"QueryExec");
	htrAddAction(drv,"QueryToggle");
	htrAddAction(drv,"Save");

	/* these don't really do much, since the form doesn't have a layer, so nothing can find it... */
	htrAddEvent(drv,"StatusChange");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"NoData");
	htrAddEvent(drv,"View");
	htrAddEvent(drv,"Modify");
	htrAddEvent(drv,"Query");
	htrAddEvent(drv,"QueryExec");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

    return 0;
    }
