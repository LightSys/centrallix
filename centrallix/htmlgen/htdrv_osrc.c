/* vim: set sw=3: */

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

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_osrc.c                                            */
/* Author:      John Peebles & Joe Heth                                 */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for an object system                 */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_osrc.c,v 1.59 2005/06/23 22:07:59 ncolson Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
    Revision 1.59  2005/06/23 22:07:59  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.58  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.57  2004/08/04 20:03:09  mmcgill
    Major change in the way the client-side widget tree works/is built.
    Instead of overlaying a tree structure on top of the global widget objects,
    the tree is built *out of* those objects.
    *   Removed the now-unnecessary tree-building code in the ht drivers
    *   added htr_internal_BuildClientTree(), which keeps just about all the
        client-side tree-building code in one spot
    *   Added RenderFlags to the WgtrNode struct, for use by any rendering
        module in whatever way that module sees fit
    *   Added the HT_WGTF_NOOBJECT flag in ht_render, which is set by ht
        drivers that deal with widgets for which a corresponding DHTML object
        is not created - for example, a radiobuttonpanel widget has
        radiobutton child widgets - but in the client-side code there are no
        corresponding DHTML objects for those child widgets. So the
        radiobuttonpanel ht driver sets the HT_WGTF_NOOBJECT RenderFlag on
        each of those child nodes, and when the client-side widget tree is
        being built, no attempt is made to add them to the client-side tree.
    *   Tweaked the connector widget a bit - it doesn't appear that the Add
        member function needs to take an object as a parameter, since each
        connector is associated with its parent object in cn_init.
    *   *cough* Er, fixed the, um....giant unclosable unmovable textarea that
        I had been using for debug messages, so that it doesn't appear unless
        WGTR_DBG_WINDOW is defined in ht_render.c. Heh heh. Sorry about that.

    Revision 1.56  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.55  2004/08/02 14:09:34  mmcgill
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

    Revision 1.54  2004/07/19 15:30:40  mmcgill
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

    Revision 1.53  2004/06/12 03:56:05  gbeeley
    - adding preliminary support for client notification of changes to an open
      object.

    Revision 1.52  2004/02/24 20:21:57  gbeeley
    - hints .js file inclusion on form, osrc, and editbox
    - htrParamValue and htrGetBoolean utility functions
    - connector now supports runclient() expressions as a better way to
      do things for connector action params
    - global variable pollution problems fixed in some places
    - show_root option on treeview

    Revision 1.51  2003/11/30 02:09:40  gbeeley
    - adding autoquery modes to OSRC (never, onload, onfirstreveal, or
      oneachreveal)
    - adding serialized loader queue for preventing communcations with the
      server from interfering with each other (netscape bug)
    - pg_debug() writes to a "debug:" dynamic html widget via AddText()
    - obscure/reveal subsystem initial implementation

    Revision 1.50  2003/07/27 03:24:53  jorupp
     * added Mozilla support for:
     	* connector
    	* formstatus
    	* imagebutton
    	* osrc
    	* pane
    	* textbutton
     * a few bug fixes for other Mozilla support as well.

    Revision 1.49  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.48  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.47  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.46  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.45  2002/08/18 18:43:44  jorupp
     * osrc now uses double quotes to enclose the parameters passed to init -- it's safer

    Revision 1.44  2002/07/31 16:17:55  gbeeley
    OSRC relies upon ht_utils_string.js now, so I added a script include
    line to link that one in.  Before, the rtrim thing broke xml_test.app.

    Revision 1.43  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.42  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.41  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.40  2002/07/12 20:06:42  lkehresman
    Removed an alert
    *cough*

    Revision 1.39  2002/07/12 20:03:09  lkehresman
    Modified the osrc to take advantage of the new encoding ability in the
    net_http driver.  This fixes a bug that the textarea uncovered with new
    lines getting squashed.

    Revision 1.38  2002/06/24 17:28:58  jorupp
     * osrc will now close objects when they are removed from the replica

    Revision 1.37  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.36  2002/06/10 21:47:45  jorupp
     * bit of code cleanup
     * added movable borders to the dynamic table

    Revision 1.35  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.34  2002/06/06 17:12:21  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.33  2002/06/03 19:10:29  jorupp
     * =>,<=

    Revision 1.32  2002/06/03 05:45:35  jorupp
     * removing alerts

    Revision 1.31  2002/06/03 05:10:56  jorupp
     * removed a debugging message -- not needed

    Revision 1.30  2002/06/03 05:09:25  jorupp
     * impliment the form view mode correctly
     * fix scrolling back in the OSRC (from the table)
     * throw DataChange event when any data is changed

    Revision 1.29  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.28  2002/06/01 19:46:15  jorupp
     * mixed annoying problem where sometimes, OSRC would make the last record the active one instead of the first one

    Revision 1.27  2002/06/01 19:16:48  jorupp
     * down-sized the osrc's hidden layer, which was causing the page to be
        longer than it should be, which caused the up/down scrollbar to
        appear, which caused the textarea that handles keypress events to
        be off the page, which meant that it couldn't recieve keyboard focus,
        which meant that there were no keypress events.

    Revision 1.26  2002/05/31 05:03:32  jorupp
     * OSRC now can do a DoubleSync -- check kardia for an example

    Revision 1.25  2002/05/30 05:01:31  jorupp
     * OSRC has a Sync Action (best used to tie two OSRCs together on a table selection)
     * NOTE: with multiple tables in an app file, netscape seems to like to hang (the JS engine at least)
        while rendering the page.  uncomment line 1109 in htdrv_table.c to fix it (at the expense of extra alerts)
        -- I tried to figure this out, but was unsuccessful....

    Revision 1.24  2002/05/30 00:03:07  jorupp
     * this ^should^ allow nesting of the osrc and form, but who knows.....

    Revision 1.23  2002/05/06 22:29:36  jorupp
     * minor bug fixes that I found while documenting the OSRC

    Revision 1.22  2002/04/30 18:08:43  jorupp
     * more additions to the table -- now it can scroll~
     * made the osrc and form play nice with the table
     * minor changes to form sample

    Revision 1.21  2002/04/28 21:36:59  jorupp
     * only one session at a time open
     * only one query at a time open
     * disabled query close on exit, as it would crash the browser or load a blank page

    Revision 1.20  2002/04/28 06:00:38  jorupp
     * added htrAddScriptCleanup* stuff
     * added cleanup stuff to osrc

    Revision 1.19  2002/04/27 22:47:45  jorupp
     * re-wrote form and osrc interaction -- more happens now in the form
     * lots of fun stuff in the table....check form.app for an example (not completely working yet)
     * the osrc is still a little bit buggy.  If you get it screwed up, let me know how to reproduce it.

    Revision 1.18  2002/04/26 22:12:27  jheth
    Added nextPage() prevPage() functions to OSRC - Didn't test it though - something is broken. I can't make depend.

    Revision 1.17  2002/04/25 23:02:52  jorupp
     * added alternate alignment for labels (right or center should work)
     * fixed osrc/form bug

    Revision 1.16  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc

    Revision 1.15  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.14  2002/03/28 05:21:23  jorupp
     * form no longer does some redundant status checking
     * cleaned up some unneeded stuff in form
     * osrc properly impliments almost everything (will prompt on unsaved data, etc.)

    Revision 1.13  2002/03/26 06:38:05  jorupp
    osrc has two new parameters: readahead and replicasize
    osrc replica now operates on a sliding window principle (holds a range of records, instead of all between the beginning and the current one)

    Revision 1.12  2002/03/23 00:32:13  jorupp
     * osrc now can move to previous and next records
     * form now loads it's basequery automatically, and will not load if you don't have one
     * modified form test page to be a bit more interesting

    Revision 1.11  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.10  2002/03/16 05:55:14  jheth
    Added Move First/Next/Previous/Last logic
    Query obtains oid and now closes object and session

    Revision 1.9  2002/03/16 02:04:05  jheth
    osrc widget queries and passes data back to form widget

    Revision 1.8  2002/03/14 05:11:49  jorupp
     * bugfixes

    Revision 1.7  2002/03/14 03:29:51  jorupp
     * updated form to prepend a : to the fieldname when using for a query
     * updated osrc to take the query given it by the form, submit it to the server,
        iterate through the results, and store them in the replica
     * bug fixes to treeview (DOMviewer mode) -- added ability to change scaler values

    Revision 1.6  2002/03/13 01:35:02  jheth
    Re-commit of Object Source - No Alerts

    Revision 1.5  2002/03/13 01:04:32  jheth
    Partial working Object Source - Functionality added but no reliable testing

    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.2  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.1  2002/02/27 01:38:51  jheth
    Initial commit of object source

    Revision 1.5  2002/02/23 19:35:28 jpeebles/jheth 

 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTOSRC;

enum htosrc_autoquery_types { Never=0, OnLoad=1, OnFirstReveal=2, OnEachReveal=3  };

/* 
   htosrcRender - generate the HTML code for the page.
   
   Don't know what this is, but we're keeping it for now - JJP, JDH
*/
int
htosrcRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
   {
   int id;
   char name[40];
   char *ptr;
   char *nptr;
   int readahead;
   int scrollahead;
   int replicasize;
   char *sql;
   char *filter;
   char *baseobj;
   pWgtrNode sub_tree;
//   pObjQuery qy;
   enum htosrc_autoquery_types aq;
   int receive_updates;
   int count, i;

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTOSRC","Netscape DOM or W3C DOM1 HTML support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTOSRC.idcnt++);

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,39);
   name[39] = 0;

   if (wgtrGetPropertyValue(tree,"replicasize",DATA_T_INTEGER,POD(&replicasize)) != 0)
      replicasize=6;
   if (wgtrGetPropertyValue(tree,"readahead",DATA_T_INTEGER,POD(&readahead)) != 0)
      readahead=replicasize/2;
   if (wgtrGetPropertyValue(tree,"scrollahead",DATA_T_INTEGER,POD(&scrollahead)) != 0)
      scrollahead=readahead;

   /** try to catch mistakes that would probably make Netscape REALLY buggy... **/
   if(replicasize==1 && readahead==0) readahead=1;
   if(replicasize==1 && scrollahead==0) scrollahead=1;
   if(readahead>replicasize) replicasize=readahead;
   if(scrollahead>replicasize) replicasize=scrollahead;
   if(scrollahead<1) scrollahead=1;
   if(replicasize<1 || readahead<1)
      {
      mssError(1,"HTOSRC","You must give positive integer for replicasize and readahead");
      return -1;
      }

   /** Query autostart types **/
   if (wgtrGetPropertyValue(tree,"autoquery",DATA_T_STRING,POD(&ptr)) == 0)
      {
      if (!strcasecmp(ptr,"onLoad")) aq = OnLoad;
      else if (!strcasecmp(ptr,"onFirstReveal")) aq = OnFirstReveal;
      else if (!strcasecmp(ptr,"onEachReveal")) aq = OnEachReveal;
      else if (!strcasecmp(ptr,"never")) aq = Never;
      else
	 {
	 mssError(1,"HTOSRC","Invalid autostart type '%s' for objectsource '%s'",ptr,name);
	 return -1;
	 }
      }
   else
      {
      aq = OnFirstReveal;
      }

   /** Get replication updates from server? **/
   receive_updates = htrGetBoolean(tree, "receive_updates", 0);

   if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&ptr)) == 0)
      {
      sql=nmMalloc(strlen(ptr)+1);
      strcpy(sql,ptr);
      }
   else
      {
      mssError(1,"HTOSRC","You must give a sql parameter");
      return -1;
      }

   if (wgtrGetPropertyValue(tree,"baseobj",DATA_T_STRING,POD(&ptr)) == 0)
      {
      baseobj = nmMalloc(strlen(ptr)+1);
      strcpy(baseobj, ptr);
      }
   else
      {
      baseobj = NULL;
      }

   if (wgtrGetPropertyValue(tree,"filter",DATA_T_STRING,POD(&ptr)) == 0)
      {
      filter=nmMalloc(strlen(ptr)+1);
      strcpy(filter,ptr);
      }
   else
      {
      filter=nmMalloc(1);
      filter[0]='\0';
      }



   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);

   /** create our instance variable **/
   //htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"        #osrc%dloader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:1;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);

   /** Script initialization call. **/
   if(s->Capabilities.Dom0NS)
      {
      htrAddScriptInit_va(s,"    %s=osrc_init({loader:%s.layers.osrc%dloader, readahead:%i, scrollahead:%i, replicasize:%i, sql:\"%s\", filter:\"%s\", baseobj:\"%s\", name:\"%s\", autoquery:%d, requestupdates:%d});\n",
	    name,parentname, id,readahead,scrollahead,replicasize,sql,filter,baseobj?baseobj:"",name,aq,receive_updates);
      }
   else if(s->Capabilities.Dom1HTML)
      {
      htrAddScriptInit_va(s,"    %s=osrc_init({loader:document.getElementById('osrc%dloader'), readahead:%i, scrollahead:%i, replicasize:%i, sql:\"%s\", filter:\"%s\", baseobj:\"%s\", name:\"%s\", autoquery:%d, requestupdates:%d});\n",
	    name, id,readahead,scrollahead,replicasize,sql,filter,baseobj?baseobj:"",name,aq,receive_updates);
      }
   else
      {
      mssError(1,"HTOSRC","Cannot render for this browser");
      }
   //htrAddScriptCleanup_va(s,"    %s.layers.osrc%dloader.cleanup();\n", parentname, id);

   htrAddScriptInclude(s, "/sys/js/htdrv_osrc.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_hints.js", 0);

   htrAddScriptInit_va(s,"    %s.oldosrc=osrc_current;\n",name);
   htrAddScriptInit_va(s,"    osrc_current=%s;\n",name);

   /** HTML body element for the frame **/
   htrAddBodyItemLayerStart(s,HTR_LAYER_F_DYNAMIC,"osrc%dloader",id);
   htrAddBodyItemLayerEnd(s,HTR_LAYER_F_DYNAMIC);
   htrAddBodyItem(s, "\n");





   /**
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy)
   {
   while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
       {
       wgtrGetPropertyValue(sub_w_obj, "outer_type", DATA_T_STRING,POD(&ptr));
       if (strcmp(ptr,"widget/connector") == 0)
	   htrRenderWidget(s, sub_w_obj, z, "", name);
       else
	   htrRenderWidget(s, sub_w_obj, z, parentname, parentobj);
       objClose(sub_w_obj);
       }
   objQueryClose(qy);
   }
   **/
    count = xaCount(&(tree->Children));
    for (i=0;i<count;i++)
	{
	sub_tree = xaGetItem(&(tree->Children), i);
	if (strcmp(sub_tree->Type, "widget/connector") == 0)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z, "", name);
	else
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z, parentname, parentobj);
	}


   /** We set osrc_current=null so that orphans can't find us  **/
   htrAddScriptInit(s, "    //osrc_current.InitQuery();\n");
   htrAddScriptInit_va(s,"    osrc_current=%s.oldosrc;\n\n",name);


   return 0;
}


/* 
   htosrcInitialize - register with the ht_render module.
*/
int htosrcInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML OSRC Driver");
   strcpy(drv->WidgetName,"osrc");
   drv->Render = htosrcRender;

   /** Add actions **/
   htrAddAction(drv,"Clear");
   htrAddAction(drv,"Query");
   htrAddAction(drv,"Delete");
   htrAddAction(drv,"Create");
   htrAddAction(drv,"Modify");

   htrAddAction(drv,"Sync");
   htrAddAction(drv,"ReverseSync");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTOSRC.idcnt = 0;

   return 0;
}
