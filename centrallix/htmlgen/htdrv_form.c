#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "wgtr.h"

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
/* Module: 	htdrv_form.c      					*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is the non-visual widget that interfaces the 	*/
/*		objectsource widget and the visual sub-widgets		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_form.c,v $
    Revision 1.54  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.53  2004/08/02 14:09:34  mmcgill
    Restructured the rendering process, in anticipation of new deployment methods
    being added in the future. The wgtr module is now the main widget-related
    module, responsible for all non-deployment-specific widget functionality.
    For example, Verifying a widget tree is non-deployment-specific, so the verify
    functions have been moved out of htmlgen and into the wgtr module.
    Changes include:
    *   Creating a new folder, wgtr/, to contain the wgtr module, including all
        wgtr drivers.
    *   Adding wgtr drivers to the widget tree module.
    *   Moving the xxxVerify() functions to the wgtr drivers in the wgtr module.
    *   Requiring all deployment methods (currently only DHTML) to register a
        Render() function with the wgtr module.
    *   Adding wgtrRender(), to abstract the details of the rendering process
        from the caller. Given a widget tree, a string representing the deployment
        method to use ("DHTML" for now), and the additional args for the rendering
        function, wgtrRender() looks up the appropriate function for the specified
        deployment method and calls it.
    *   Added xxxNew() functions to each wgtr driver, to be called when a new node
        is being created. This is primarily to allow widget drivers to declare
        the interfaces their widgets support when they are instantiated, but other
        initialization tasks can go there as well.

    Also in this commit:
    *   Fixed a typo in the inclusion guard for iface.h (most embarrasing)
    *   Fixed an overflow in objCopyData() in obj_datatypes.c that stomped on
        other stack variables.
    *   Updated net_http.c to call wgtrRender instead of htrRender(). Net drivers
        can now be completely insulated from the deployment method by the wgtr
        module.

    Revision 1.52  2004/07/19 15:30:39  mmcgill
    The DHTML generation system has been updated from the 2-step process to
    a three-step process:
        1)	Upon request for an application, a widget-tree is built from the
    	app file requested.
        2)	The tree is Verified (not actually implemented yet, since none of
    	the widget drivers have proper Verify() functions - but it's only
    	a matter of a function call in net_http.c)
        3)	The widget drivers are called on their respective parts of the
    	tree structure to generate the DHTML code, which is then sent to
    	the user.

    To support widget tree generation the WGTR module has been added. This
    module allows OSML objects to be parsed into widget-trees. The module
    also provides an API for building widget-trees from scratch, and for
    manipulating existing widget-trees.

    The Render functions of all widget drivers have been updated to make their
    calls to the WGTR module, rather than the OSML, and to take a pWgtrNode
    instead of a pObject as a parameter.

    net_internal_GET() in net_http.c has been updated to call
    wgtrParseOpenObject() to make a tree, pass that tree to htrRender(), and
    then free it.

    htrRender() in ht_render.c has been updated to take a pWgtrNode instead of
    a pObject parameter, and to make calls through the WGTR module instead of
    the OSML where appropriate. htrRenderWidget(), htrRenderSubwidgets(),
    htrGetBoolean(), etc. have also been modified appropriately.

    I have assumed in each widget driver that w_obj->Session is equivelent to
    s->ObjSession; in other words, that the object being passed in to the
    Render() function was opened via the session being passed in with the
    HtSession parameter. To my understanding this is a valid assumption.

    While I did run through the test apps and all appears to be well, it is
    possible that some bugs were introduced as a result of the modifications to
    all 30 widget drivers. If you find at any point that things are acting
    funny, that would be a good place to check.

    Revision 1.51  2004/02/24 20:21:57  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.50  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.49  2003/05/30 17:39:49  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.48  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.47  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.46  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.45  2002/07/08 23:21:38  jorupp
     * added a global object, cn_browser with two boolean properties -- netscape47 and mozilla
        The corresponding one will be set to true by the page
     * made one minor change to the form to get around the one .layers reference in the form (no .document references)
        It _should_ work, however I don't have a _simple_ form test to try it on, so it'll have to wait

    Revision 1.44  2002/06/24 17:29:44  jorupp
     * clarified an error message
     * added a potential feature, but commented it out do to implimentation difficulties

    Revision 1.43  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.42  2002/06/03 05:09:25  jorupp
     * impliment the form view mode correctly
     * fix scrolling back in the OSRC (from the table)
     * throw DataChange event when any data is changed

    Revision 1.41  2002/06/03 04:52:45  lkehresman
    Made saving throw you out of modify mode in the form.

    Revision 1.40  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.39  2002/06/01 19:35:49  jorupp
     * some more of the same problem....found where the global variable was made at -- removed it

    Revision 1.38  2002/06/01 19:33:44  jorupp
     * I don't know why this worked before, but it works now

    Revision 1.37  2002/05/31 05:03:32  jorupp
     * OSRC now can do a DoubleSync -- check kardia for an example

    Revision 1.36  2002/05/31 01:41:19  lkehresman
    Fixed form bug that hung javascript if no data was returned from a query.

    Revision 1.35  2002/05/30 04:17:21  lkehresman
    * Modified editbox to take advantage of enablenew and enablemodify
    * Fixed ChangeStatus stuff in form

    Revision 1.34  2002/05/30 04:10:49  jorupp
     * this looks better.....

    Revision 1.33  2002/05/30 04:10:02  jorupp
     * shows what I get for not actually testing my code.....

    Revision 1.32  2002/05/30 04:07:08  jorupp
     * changed interface with form elements to use enablemodify (for enable for modification)
        and enablenew (for enable for new data entry) -- useful for the disableable editbox

    Revision 1.31  2002/05/30 00:03:07  jorupp
     * this ^should^ allow nesting of the osrc and form, but who knows.....

    Revision 1.30  2002/04/30 18:08:43  jorupp
     * more additions to the table -- now it can scroll~
     * made the osrc and form play nice with the table
     * minor changes to form sample

    Revision 1.29  2002/04/28 06:28:29  jorupp
     * add a hair of speed to the form repopulation by caching column ids

    Revision 1.28  2002/04/27 22:47:45  jorupp
     * re-wrote form and osrc interaction -- more happens now in the form
     * lots of fun stuff in the table....check form.app for an example (not completely working yet)
     * the osrc is still a little bit buggy.  If you get it screwed up, let me know how to reproduce it.

    Revision 1.27  2002/04/27 06:37:45  jorupp
     * some bug fixes in the form
     * cleaned up some debugging output in the label
     * added a dynamic table widget

    Revision 1.26  2002/04/25 23:02:52  jorupp
     * added alternate alignment for labels (right or center should work)
     * fixed osrc/form bug

    Revision 1.25  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc

    Revision 1.24  2002/04/25 01:13:43  jorupp
     * increased buffer size for query in form
     * changed sybase driver to not put strings in two sets of quotes on update

    Revision 1.23  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.22  2002/04/05 06:39:12  jorupp
     * Added ReadOnly parameter to form
     * If ReadOnly is not present, updates will now work properly!
     * still need to work on stopping updates on client side when readonly is set

    Revision 1.21  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.20  2002/03/28 05:21:22  jorupp
     * form no longer does some redundant status checking
     * cleaned up some unneeded stuff in form
     * osrc properly impliments almost everything (will prompt on unsaved data, etc.)

    Revision 1.19  2002/03/23 00:32:13  jorupp
     * osrc now can move to previous and next records
     * form now loads it's basequery automatically, and will not load if you don't have one
     * modified form test page to be a bit more interesting

    Revision 1.18  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.17  2002/03/17 20:45:45  gbeeley
    Re-fixed security update introduced at file rev 1.12 but lost somehow.

    Revision 1.16  2002/03/17 03:51:03  jorupp
    * treeview now returns value on function call (in alert window)
    * implimented basics of 3-button confirm window on the form side
        still need to update several functions to use it

    Revision 1.15  2002/03/16 02:04:05  jheth
    osrc widget queries and passes data back to form widget

    Revision 1.14  2002/03/14 05:11:49  jorupp
     * bugfixes

    Revision 1.13  2002/03/14 03:29:51  jorupp
     * updated form to prepend a : to the fieldname when using for a query
     * updated osrc to take the query given it by the form, submit it to the server,
        iterate through the results, and store them in the replica
     * bug fixes to treeview (DOMviewer mode) -- added ability to change scaler values

    Revision 1.11  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.10  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working

    Revision 1.9  2002/03/05 01:55:23  jorupp
    * switch to using clearvalue() instead of setvalue('') to clear form elements
    * document basequery/basewhere

    Revision 1.8  2002/03/05 00:46:34  jorupp
    * Fix a problem in Luke's radiobutton fix
    * Add the corresponding checks in the form

    Revision 1.7  2002/03/02 21:57:00  jorupp
    * Editbox supports as many</>/=/<=/>=/<=> clauses you can fit to query for data
        (<=> is the LIKE operator)

    Revision 1.6  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.5  2002/02/27 02:37:19  jorupp
    * moved editbox I-beam movement functionality to function
    * cleaned up form, added comments, etc.

    Revision 1.4  2002/02/23 19:35:28  lkehresman
    * Radio button widget is now forms aware.
    * Fixes a couple of oddities in the checkbox.
    * Fixed some formatting issues in the form.

    Revision 1.3  2002/02/23 04:28:29  jorupp
    bug fixes in form, I-bar in editbox is reset on a setvalue()

    Revision 1.2  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.1  2002/02/21 18:15:14  jorupp
    Initial commit of form widget


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTFORM;


/*** htformRender - generate the HTML code for the form 'glue'
 ***/
int
htformRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char tabmode[64];
    int id, i;
    char* nptr;
    int allowquery, allownew, allowmodify, allowview, allownodata, multienter;
    char _3bconfirmwindow[30];
    int readonly;
//    pObject sub_w_obj;
//    pObjQuery qy;

	/** form widget should work on any browser **/
    
    	/** Get an id for this. **/
	id = (HTFORM.idcnt++);

	/** Get params. **/
	if (wgtrGetPropertyValue(tree,"AllowQuery",DATA_T_INTEGER,POD(&allowquery)) != 0) 
	    allowquery=1;
	if (wgtrGetPropertyValue(tree,"AllowNew",DATA_T_INTEGER,POD(&allownew)) != 0) 
	    allownew=1;
	if (wgtrGetPropertyValue(tree,"AllowModify",DATA_T_INTEGER,POD(&allowmodify)) != 0) 
	    allowmodify=1;
	if (wgtrGetPropertyValue(tree,"AllowView",DATA_T_INTEGER,POD(&allowview)) != 0) 
	    allowview=1;
	if (wgtrGetPropertyValue(tree,"AllowNoData",DATA_T_INTEGER,POD(&allownodata)) != 0) 
	    allownodata=1;
	/** The way I read the specs -- overriding this resides in 
	 **   the code, not here -- JDR **/
	if (wgtrGetPropertyValue(tree,"MultiEnter",DATA_T_INTEGER,POD(&multienter)) != 0) 
	    multienter=0;
	if (wgtrGetPropertyValue(tree,"TabMode",DATA_T_STRING,POD(tabmode)) != 0) 
	    tabmode[0]='\0';
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
	    snprintf(_3bconfirmwindow,30,"%s",ptr);
	else
	    strcpy(_3bconfirmwindow,"null");

	
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

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** create our instance variable **/
	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

	/** Script include to add functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_form.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

	/** Write out the init line for this instance of the form
	 **   the name of this instance was defined to be global up above
	 **   and fm_current is defined in htdrv_page.c 
	 **/
	htrAddScriptInit_va(s,"\n    %s=form_init(%i,%i,%i,%i,%i,%i,'%s',%s,%i);\n",
		name,allowquery,allownew,allowmodify,allowview,allownodata,multienter,name,
		_3bconfirmwindow,readonly);
	htrAddScriptInit_va(s,"    %s.ChangeMode('NoData');\n",name);
	htrAddScriptInit_va(s,"    %s.oldform=fm_current;\n",name);
	htrAddScriptInit_va(s,"    fm_current=%s;\n",name);

    htrAddScriptWgtr(s, "    // htdrv_form.c\n");
    /** Add this node to the widget tree **/
    htrAddScriptWgtr_va(s, "    child_node = new WgtrNode('%s', '%s', %s, false)\n", tree->Name, tree->Type, nptr);
    htrAddScriptWgtr_va(s, "    wgtrAddChild(curr_node[0], child_node);\n");

    /** make ourself the current node for our children **/
    htrAddScriptWgtr(s, "    curr_node.unshift(child_node);\n\n");


	/** Check for and render all subobjects. **/
	/** non-visual, don't consume a z level **/

	/**
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", DATA_T_STRING,POD(&ptr));
		if (strcmp(ptr,"widget/connector") == 0)
		    htrRenderWidget(s, sub_w_obj, z, "", name);
		else
		    htrRenderWidget(s, sub_w_obj, z, parentname, parentobj);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }
	**/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    if (strcmp(tree->Type, "widget/connector") == 0)
		htrRenderWidget(s, xaGetItem(&(tree->Children), i), z, "", name);
	    else
		htrRenderWidget(s, xaGetItem(&(tree->Children), i), z, parentname, name);
	    }
	
	htrAddScriptInit_va(s,"    fm_current=%s.oldform;\n",name);
    /** make our parent the current node again **/
    htrAddScriptWgtr(s, "    curr_node.shift();\n\n");

    return 0;
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

	HTFORM.idcnt = 0;

    return 0;
    }
