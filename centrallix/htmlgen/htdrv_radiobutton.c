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

    $Id: htdrv_radiobutton.c,v 1.25 2004/08/04 20:03:09 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_radiobutton.c,v $

    $Log: htdrv_radiobutton.c,v $
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
int htrbRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj) {
   char* ptr;
   char name[64];
   char title[64];
   char sbuf2[200];
   char* nptr;
   //char bigbuf[4096];
   char textcolor[32];
   char main_bgcolor[32];
   char main_background[128];
   char outline_bg[64];
   pWgtrNode radiobutton_obj, sub_tree;
   int x=-1,y=-1,w,h;
   int id, i, j;
   char fieldname[32];

   if(!s->Capabilities.Dom0NS)
       {
       mssError(1,"HTRB","Netscape DOM support required");
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
   if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
      strncpy(main_bgcolor,ptr,31);
   else 
      strcpy(main_bgcolor,"");

   if (wgtrGetPropertyValue(tree,"background",DATA_T_STRING,POD(&ptr)) == 0)
      strncpy(main_background,ptr,127);
   else
      strcpy(main_background,"");

   /** Text color? **/
   if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
      snprintf(textcolor,32,"%s",ptr);
   else
      strcpy(textcolor,"black");

   /** Outline color? **/
   if (wgtrGetPropertyValue(tree,"outlinecolor",DATA_T_STRING,POD(&ptr)) == 0)
      snprintf(outline_bg,64,"%s",ptr);
   else
      strcpy(outline_bg,"black");

   /** Get name **/
   if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,63);
   name[63]=0;

   /** Get title **/
   if (wgtrGetPropertyValue(tree,"title",DATA_T_STRING,POD(&ptr)) != 0) return -1;
   memccpy(title,ptr,0,63);
   title[63] = 0;

   /** Get fieldname **/
   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) 
      {
      strncpy(fieldname,ptr,30);
      }
   else 
      { 
      fieldname[0]='\0';
      } 

   htrAddScriptInclude(s, "/sys/js/htdrv_radiobutton.js", 0);

   /** Ok, write the style header items. **/
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dparentpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,x,y,w,h,z,w,h);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dborderpane    { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,3,12,w-(2*3),h-(12+3),z+1,w-(2*3),h-(12+3));
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dcoverpane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           id,1,1,w-(2*3 +2),h-(12+3 +2),z+2,w-(2*3 +2),h-(12+3 +2));
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%dtitlepane     { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n",
           id,10,1,w/2,17,z+3);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelbuttonsetpane   { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelbuttonunsetpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanelvaluepane       { POSITION:absolute; VISIBILITY:hidden; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           5,5,12,12,z+2,12,12);
   htrAddStylesheetItem_va(s,"\t#radiobuttonpanellabelpane       { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx,%dpx); }\n",
           27,2,w-(2*3 +2+27+1),24,z+2,w-(2*3 +2+27+1),24);
   
   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);
   htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
   htrAddScriptGlobal(s, "radiobutton", "null", 0);


    /** Loop through each radiobutton and flag it NOOBJECT **/
    for (i=0;i<xaCount(&(tree->Children));i++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), i);
	radiobutton_obj->RenderFlags |= HT_WGTF_NOOBJECT;
	}
   /*
      Now lets loop through and create a style sheet for each optionpane on the
      radiobuttonpanel
   */   
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
	wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
	if (!strcmp(ptr,"widget/radiobutton"))
	    {
	    htrAddStylesheetItem_va(s,"\t#radiobuttonpanel%doption%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(%dpx, %dpx); }\n",
		    id,i,7,10+((i-1)*25)+3,w-(2*3 +2+7),25,z+2,w-(2*3 +2+7),25);
	    i++;
	    }
	}

   /** Script initialization call. **/
   if (strlen(main_bgcolor) > 0) {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(\n"
        "    %s.layers.radiobuttonpanel%dparentpane,\"%s\",1,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,\n"
	"    \"%s\",\"%s\");\n", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_bgcolor, outline_bg);
   } else if (strlen(main_background) > 0) {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(\n"
        "    %s.layers.radiobuttonpanel%dparentpane,\"%s\",2,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane,\n"
        "    %s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dtitlepane,\n"
	"    \"%s\",\"%s\");\n", nptr, parentname, id, fieldname, parentname,id,id, parentname,id,id,id, parentname,id,id,main_background, outline_bg);
   } else {
      htrAddScriptInit_va(s,"    %s = radiobuttonpanel_init(%s.layers.radiobuttonpanel%dparentpane,\"%s\",0,0,0,0,0,0);\n", nptr, parentname, id,fieldname);
   }

   htrAddEventHandler(s, "document", "MOUSEUP", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) {\n"
      "          if(ly.optionPane) {\n"
      "             if (ly.mainlayer.form) ly.mainlayer.form.FocusNotify(ly.mainlayer);\n"
      "             radiobutton_toggle(ly);\n"
      "          }\n"
      "          cn_activate(ly.mainlayer, 'Click');\n"
      "          cn_activate(ly.mainlayer, 'MouseUp');\n"
      "      }\n"
      "   }\n");

   htrAddEventHandler(s, "document", "MOUSEDOWN", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) cn_activate(ly.mainlayer, 'MouseDown');\n"
      "   }\n");
   
   htrAddEventHandler(s, "document", "MOUSEOVER", "radiobutton", "\n"
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled && !util_cur_mainlayer) {\n"
      "          cn_activate(ly.mainlayer, 'MouseOver');\n"
      "          util_cur_mainlayer = ly.mainlayer;\n"
      "      }\n"
      "   }\n");

//   htrAddEventHandler(s, "document", "MOUSEOUT", "radiobutton", "\n"
//      "   }\n");
   
   htrAddEventHandler(s, "document", "MOUSEMOVE", "radiobutton", "\n"
      "   if (util_cur_mainlayer && ly.kind != 'radiobutton') {\n"						// 
      "      if (util_cur_mainlayer.mainlayer.enabled) cn_activate(util_cur_mainlayer.mainlayer, 'MouseOut');\n"	// This is MouseOut Detection!
      "      util_cur_mainlayer = null;\n"									// 
      "   }\n"												// 
      "   if (ly != null && ly.kind == 'radiobutton') {\n"
      "      if (ly.mainlayer.enabled) cn_activate(ly.mainlayer, 'MouseMove');\n"
      "   }\n");
 



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
            if (wgtrGetPropertyValue(sub_tree,"selected",DATA_T_STRING,POD(&ptr)) != 0)
		strcpy(sbuf2,"false");
            else 
		{
		memccpy(sbuf2,ptr,0,199);
		sbuf2[199] = 0;
		}

            htrAddScriptInit_va(s,"    add_radiobutton(%s.layers.radiobuttonpanel%dparentpane.layers.radiobuttonpanel%dborderpane.layers.radiobuttonpanel%dcoverpane.layers.radiobuttonpanel%doption%dpane, %s.layers.radiobuttonpanel%dparentpane, %s, %s);\n", parentname, id, id, id, id, i, parentname, id, sbuf2, nptr);
            i++;
	    }
	 else 
	    {
	    htrRenderWidget(s, sub_tree, z+1, parentname, nptr);
	    }
	}

   /*
      Do the HTML layers
   */
   htrAddBodyItem_va(s,"   <DIV ID=\"radiobuttonpanel%dparentpane\">\n", id);
   htrAddBodyItem_va(s,"      <DIV ID=\"radiobuttonpanel%dborderpane\">\n", id);
   htrAddBodyItem_va(s,"         <DIV ID=\"radiobuttonpanel%dcoverpane\">\n", id);

   /* Loop through each radio button and do the option pane and sub layers */
    i = 1;
    for (j=0;j<xaCount(&(tree->Children));j++)
	{
	radiobutton_obj = xaGetItem(&(tree->Children), j);
        wgtrGetPropertyValue(radiobutton_obj,"outer_type",DATA_T_STRING,POD(&ptr));
        if (!strcmp(ptr,"widget/radiobutton")) 
	    {
            htrAddBodyItem_va(s,"            <DIV ID=\"radiobuttonpanel%doption%dpane\">\n", id, i);
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelbuttonsetpane\"><IMG SRC=\"/sys/images/radiobutton_set.gif\"></DIV>\n");
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelbuttonunsetpane\"><IMG SRC=\"/sys/images/radiobutton_unset.gif\"></DIV>\n");
 
            wgtrGetPropertyValue(radiobutton_obj,"label",DATA_T_STRING,POD(&ptr));
            memccpy(sbuf2,ptr,0,199);
	    sbuf2[199]=0;
            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanellabelpane\" NOWRAP><FONT COLOR=\"%s\">%s</FONT></DIV>\n", textcolor, sbuf2);

	    /* use label (from above) as default value if no value given */
	    if(wgtrGetPropertyValue(radiobutton_obj,"value",DATA_T_STRING,POD(&ptr))==0)
		{
		memccpy(sbuf2,ptr,0,199);
		sbuf2[199] = 0;
		}

            htrAddBodyItem_va(s,"               <DIV ID=\"radiobuttonpanelvaluepane\" VISIBILITY=\"hidden\"><A NAME=\"%s\"></A></DIV>\n", sbuf2);
            htrAddBodyItem_va(s,"            </DIV>\n");
            i++;
	    }
	}
   
   htrAddBodyItem_va(s,"         </DIV>\n");
   htrAddBodyItem_va(s,"      </DIV>\n");
   htrAddBodyItem_va(s,"      <DIV ID=\"radiobuttonpanel%dtitlepane\"><TABLE><TR><TD NOWRAP><FONT COLOR=\"%s\">%s</FONT></TD></TR></TABLE></DIV>\n", id, textcolor, title);
   htrAddBodyItem_va(s,"   </DIV>\n");

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
