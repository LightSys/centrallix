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
/* Module:      htdrv_radiobutton.c                                     */
/* Author:      Nathan Ehresman (NRE)                                   */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for a radiobutton panel and          */
/*              radio button.                                           */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_radiobutton.c,v 1.32 2008/01/18 23:53:30 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_radiobutton.c,v $

    $Log: htdrv_radiobutton.c,v $
    Revision 1.32  2008/01/18 23:53:30  gbeeley
    - (bugfix) fix invalid css clip:rect() declarations

    Revision 1.31  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.30  2007/04/10 17:38:06  gbeeley
    - (feature) port of radiobutton panel to mozilla.

    Revision 1.29  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.28  2006/10/16 18:34:34  gbeeley
    - (feature) ported all widgets to use widget-tree (wgtr) alone to resolve
      references on client side.  removed all named globals for widgets on
      client.  This is in preparation for component widget (static and dynamic)
      features.
    - (bugfix) changed many snprintf(%s) and strncpy(), and some sprintf(%.<n>s)
      to use strtcpy().  Also converted memccpy() to strtcpy().  A few,
      especially strncpy(), could have caused crashes before.
    - (change) eliminated need for 'parentobj' and 'parentname' parameters to
      Render functions.
    - (change) wgtr port allowed for cleanup of some code, especially the
      ScriptInit calls.
    - (feature) ported scrollbar widget to Mozilla.
    - (bugfix) fixed a couple of memory leaks in allocated data in widget
      drivers.
    - (change) modified deployment of widget tree to client to be more
      declarative (the build_wgtr function).
    - (bugfix) removed wgtdrv_templatefile.c from the build.  It is a template,
      not an actual module.

    Revision 1.27  2005/06/23 22:08:00  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.26  2005/02/26 06:42:37  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.25  2004/08/04 20:03:09  mmcgill
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

    Revision 1.24  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.23  2004/08/02 14:09:34  mmcgill
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

    Revision 1.22  2004/07/19 15:30:40  mmcgill
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

    Revision 1.21  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.20  2003/03/30 22:49:23  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.19  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.18  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.17  2002/07/30 16:09:05  pfinley
    Added Click,MouseUp,MouseDown,MouseOver,MouseOut,MouseMove events to the
    radiobutton widget.

    Note: MouseOut does not use the MOUSEOUT event.  It had to be done manually
          since MouseOut's on the whole widget can not be distingushed if a widget
          has more than one layer visible at one time. For this I added a global
          variable: util_cur_mainlayer which is set to the mainlayer of the widget
          (the layer that the event connector functions are attached to) on a
          MouseOver event.  In order to use this in other widgets, each layer in
          the widget must:  1) have a .kind property, 2) have a .mainlayer property,
          3) set util_cur_mainlayer to the mainlayer upon a MouseOver event on the
          widget, and 4) set util_cur_mainlayer back to null upon a "MouseOut"
          event. (See code for example)

    I also:
    - changed the enabled/disabled to be properties of the mainlayer rather
      than of each layer in the widget.
    - changed toggle function to only toggle if selecting a new option (eliminated
      flicker and was needed for DataChange event).
    - removed the .parentPane pointer and replaced it with mainlayer.
    [ how about that for a log message :) ]

    Revision 1.16  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.15  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.14  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.13  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.12  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.11  2002/06/19 14:57:52  lkehresman
    Fixed the bug that broke radio buttons.  It was just a simple typo.

    Revision 1.10  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.9  2002/06/06 17:12:22  jorupp
     * fix bugs in radio and dropdown related to having no form
     * work around Netscape bug related to functions not running all the way through
        -- Kardia has been tested on Linux and Windows to be really stable now....

    Revision 1.8  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.7  2002/03/09 07:46:04  lkehresman
    Brought radiobutton widget up to date:
      * enable(), disable(), readonly()
      * FocusNotify() callback issued
      * Added disabled images for radio buttons

    Revision 1.6  2002/03/05 01:55:09  lkehresman
    Added "clearvalue" method to form widgets

    Revision 1.5  2002/03/05 00:46:34  jorupp
    * Fix a problem in Luke's radiobutton fix
    * Add the corresponding checks in the form

    Revision 1.4  2002/03/05 00:31:40  lkehresman
    Implemented DataNotify form method in the radiobutton and checkbox widgets

    Revision 1.3  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.2  2002/02/23 19:35:28  lkehresman
    * Radio button widget is now forms aware.
    * Fixes a couple of oddities in the checkbox.
    * Fixed some formatting issues in the form.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:57  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int   idcnt;
} HTRB;


/** htrbRender - generate the HTML code for the page.  **/
int htrbRender(pHtSession s, pWgtrNode tree, int z) {
   char* ptr;
   char name[64];
   char title[64];
   char sbuf2[200];
   //char bigbuf[4096];
   char textcolor[32];
   char main_background[128];
   char outline_background[128];
   char form[64];
   pWgtrNode radiobutton_obj, sub_tree;
   int x=-1,y=-1,w,h;
   int top_offset;
   int cover_height, cover_width;
   int item_spacing;
   int id, i, j;
   int is_selected;
   int rb_cnt;
   int cover_margin;
   char fieldname[32];
   char value[64];
   char label[64];

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTRB","Netscape 4.x or W3C DOM support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTRB.idcnt++);

   /** Get x,y,w,h of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'width' property");
      return -1;
   }
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) {
      mssError(1,"HTRB","RadioButtonPanel widget must have a 'height' property");
      return -1;
   }

   /** Background color/image? **/
   htrGetBackground(tree,NULL,!s->Capabilities.Dom0NS,main_background,sizeof(main_background));

   /** Text color? **/
   if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
      strtcpy(textcolor,ptr,sizeof(textcolor));
   else
      strcpy(textcolor,"black");

   /** Outline color? **/
   htrGetBackground(tree,"outline",!s->Capabilities.Dom0NS,outline_background,sizeof(outline_background));

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(name,ptr,sizeof(name));

   /** Get title **/
   if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   strtcpy(title,ptr,sizeof(title));

   /** Get fieldname **/
   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      {
      strtcpy(fieldname,ptr,sizeof(fieldname));
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
      strtcpy(form,ptr,sizeof(form));
   else
      form[0]='\0';

   htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0);
   htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);

   /** Ok, write the style header items. **/
   top_offset = s->ClientInfo->ParagraphHeight*3/4+1;
   cover_height = h-(top_offset+3+2);
   cover_width = w-(2*3 +2);
   htrAddStylesheetItem_va(s,"\t#rb%POSparent    { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,x,y,w,h,z,w,h);
   htrAddStylesheetItem_va(s,"\t#rb%POSborder    { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,3,top_offset,w-(2*3),h-(top_offset+3),z+1,w-(2*3),h-(top_offset+3));
   htrAddStylesheetItem_va(s,"\t#rb%POScover     { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
           id,1,1,cover_width,cover_height,z+2,cover_width,cover_height);
   htrAddStylesheetItem_va(s,"\t#rb%POStitle     { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",
           id,10,1,w/2,s->ClientInfo->ParagraphHeight,z+3);
   
   htrAddScriptGlobal(s, "radiobutton", "null", 0);

   /** DOM linkages **/
   htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr,\"rb%POSparent\")",id);
   htrAddWgtrCtrLinkage_va(s, tree, "htr_subel(htr_subel(_obj,\"rb%POSborder\"),\"rb%POScover\")",id,id);

    /** Loop through each radiobutton and flag it NOOBJECT **/
    rb_cnt = 0;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
	radiobutton_obj->RenderFlags |= HT_WGTF_NOOBJECT;
	wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	if (!strcmp(ptr,"widget/radiobutton"))
	    {
	    rb_cnt++;
	    }
	}
   /*
      Now lets loop through and create a style sheet for each optionpane on the
      radiobuttonpanel
   */   
    item_spacing = 12 + s->ClientInfo->ParagraphHeight;
    cover_margin = 10;
    if (item_spacing*rb_cnt+2*cover_margin > cover_height)
	item_spacing = (cover_height-2*cover_margin)/rb_cnt;
    if (item_spacing*rb_cnt+2*cover_margin > cover_height)
	cover_margin = (cover_height-(item_spacing*rb_cnt))/2;
    if (cover_margin < 2) cover_margin = 2;
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
	wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	if (!strcmp(ptr,"widget/radiobutton"))
	    {
	    htrAddStylesheetItem_va(s,"\t#rb%POSoption%POS { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px, %POSpx, %POSpx, 0px); }\n",
		    id,i,7,cover_margin+((i-1)*item_spacing)+3,cover_width-7,item_spacing,z+2,cover_width-7,item_spacing);
	    i++;
	    }
	}

   /** Script initialization call. **/
   if (strlen(main_background) > 0) {
      htrAddScriptInit_va(s,"    radiobuttonpanel_init({\n"
        "    parentPane:nodes[\"%STR&SYM\"], fieldname:\"%STR&ESCQ\",\n"
        "    borderPane:htr_subel(nodes[\"%STR&SYM\"],\"rb%POSborder\"),\n"
        "    coverPane:htr_subel(htr_subel(nodes[\"%STR&SYM\"],\"rb%POSborder\"),\"rb%POScover\"),\n"
        "    titlePane:htr_subel(nodes[\"%STR&SYM\"],\"rb%POStitle\"),\n"
	"    mainBackground:\"%STR&ESCQ\", outlineBackground:\"%STR&ESCQ\", form:\"%STR&ESCQ\"});\n",
	    name, fieldname, name,id, name,id,id, name,id, main_background, outline_background, form);
   } else {
      htrAddScriptInit_va(s,"    radiobuttonpanel_init({parentPane:nodes[\"%STR&SYM\"], fieldname:\"%STR&ESCQ\", borderPane:0, coverPane:0, titlePane:0, mainBackground:0, outlineBackground:0, form:\"%STR&ESCQ\"});\n", name, fieldname, form);
   }

   htrAddEventHandlerFunction(s, "document", "MOUSEUP", "radiobutton", "radiobutton_mouseup");
   htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "radiobutton", "radiobutton_mousedown");
   htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "radiobutton", "radiobutton_mouseover");
   htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "radiobutton", "radiobutton_mousemove");

   /*
      Now lets loop through and add each radiobutton
   */
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	sub_tree = xaGetItem(&(tree->Children), j);
	wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
        if (!strcmp(ptr,"widget/radiobutton")) 
	    {
	    if (wgtrGetPropertyValue(sub_tree,"value",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(value, ptr, sizeof(value));
	    else
		value[0] = '\0';
	    if (wgtrGetPropertyValue(sub_tree,"label",DATA_T_STRING,POD(&ptr)) == 0)
		strtcpy(label, ptr, sizeof(label));
	    else
		label[0] = '\0';
	    is_selected = htrGetBoolean(sub_tree, "selected", 0);
	    if (is_selected < 0) is_selected = 0;
	    htrAddWgtrObjLinkage_va(s,sub_tree,"htr_subel(_parentctr,\"rb%POSoption%POS\")",id,i);
	    wgtrGetPropertyValue(sub_tree,"name",DATA_T_STRING,POD(&ptr));
            htrAddScriptInit_va(s,"    add_radiobutton(nodes[\"%STR&SYM\"], {selected:%INT, buttonset:htr_subel(nodes[\"%STR&SYM\"], \"rb%POSbuttonset%POS\"), buttonunset:htr_subel(nodes[\"%STR&SYM\"], \"rb%POSbuttonunset%POS\"), value:htr_subel(nodes[\"%STR&SYM\"], \"rb%POSvalue%POS\"), label:htr_subel(nodes[\"%STR&SYM\"], \"rb%POSlabel%POS\"), valuestr:\"%STR&ESCQ\", labelstr:\"%STR&ESCQ\"});\n", 
		    ptr, is_selected,
		    ptr, id, i, ptr, id, i,
		    ptr, id, i, ptr, id, i,
		    value, label);
            i++;
	    }
	 else 
	    {
	    htrRenderWidget(s, sub_tree, z+1);
	    }
	}

   /*
      Do the HTML layers
   */
   htrAddBodyItem_va(s,"   <DIV ID=\"rb%POSparent\">\n", id);
   htrAddBodyItem_va(s,"      <DIV ID=\"rb%POSborder\">\n", id);
   htrAddBodyItem_va(s,"         <DIV ID=\"rb%POScover\">\n", id);

   /* Loop through each radio button and do the option pane and sub layers */
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
        wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
        if (!strcmp(ptr,"widget/radiobutton")) 
	    {
	    /** CSS layers **/
	    htrAddStylesheetItem_va(s,"\t#rb%POSbuttonset%POS   { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",
		   id,i,5,2+(s->ClientInfo->ParagraphHeight-12)/2,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSbuttonunset%POS { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",
		   id,i,5,2+(s->ClientInfo->ParagraphHeight-12)/2,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSvalue%POS       { POSITION:absolute; VISIBILITY:hidden; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); }\n",
		   id,i,5,5,12,12,z+2,12,12);
	    htrAddStylesheetItem_va(s,"\t#rb%POSlabel%POS       { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; CLIP:rect(0px,%POSpx,%POSpx,0px); CURSOR:pointer; }\n",
		   id,i,27,2,cover_width-(27+1),item_spacing-1,z+2,cover_width-(27+1),item_spacing-1);

	    /** Body layers **/
            htrAddBodyItem_va(s,"            <DIV ID=\"rb%POSoption%POS\">\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSbuttonset%POS\"><IMG SRC=\"/sys/images/radiobutton_set.gif\"></DIV>\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSbuttonunset%POS\"><IMG SRC=\"/sys/images/radiobutton_unset.gif\"></DIV>\n", id, i);
 
            wgtrGetPropertyValue(radiobutton_obj,"label",DATA_T_STRING,POD(&ptr));
	    strtcpy(sbuf2,ptr,sizeof(sbuf2));
            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSlabel%POS\" NOWRAP><FONT COLOR=\"%STR&HTE\">%STR&HTE</FONT></DIV>\n", 
		    id, i, textcolor, sbuf2);

	    /* use label (from above) as default value if no value given */
	    if(wgtrGetPropertyValue(radiobutton_obj,"value",DATA_T_STRING,POD(&ptr))==0)
		{
		strtcpy(sbuf2,ptr,sizeof(sbuf2));
		}

            htrAddBodyItem_va(s,"               <DIV ID=\"rb%POSvalue%POS\" VISIBILITY=\"hidden\"><A HREF=\".\">%STR&HTE</A></DIV>\n",
		    id, i, sbuf2);
            htrAddBodyItem(s,   "            </DIV>\n");
            i++;
	    }
	}
   
   htrAddBodyItem(s,   "         </DIV>\n");
   htrAddBodyItem(s,   "      </DIV>\n");
   htrAddBodyItem_va(s,"      <DIV ID=\"rb%POStitle\"><TABLE><TR><TD NOWRAP><FONT COLOR=\"%STR&HTE\">%STR&HTE</FONT></TD></TR></TABLE></DIV>\n", id, textcolor, title);
   htrAddBodyItem(s,   "   </DIV>\n");

   return 0;
}


/** htrbInitialize - register with the ht_render module.  **/
int htrbInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML RadioButton Driver");
   strcpy(drv->WidgetName,"radiobuttonpanel");
   drv->Render = htrbRender;

   /** Events **/ 
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTRB.idcnt = 0;

   return 0;
}
