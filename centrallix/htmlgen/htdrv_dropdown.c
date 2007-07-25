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
/* Module:      htdrv_dropdown.c                                        */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a drop down list                 */
/************************************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTDD;


/* 
   htddRender - generate the HTML code for the page.
*/
int htddRender(pHtSession s, pWgtrNode tree, int z) {
   char bgstr[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char fieldname[30];
   char form[64];
   char name[64];
   char *ptr;
   char *sql;
   char *str;
   char *attr;
   int type, rval, mode, flag=0;
   int x,y,w,h;
   int id, i;
   int num_disp;
   ObjData od;
   XString xs;
   pObjQuery qy;
   pObject qy_obj;
   pWgtrNode subtree;

   if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
       {
       mssError(1,"HTDD","Netscape or W3C DOM support required");
       return -1;
       }

   /** Get an id for this. **/
   id = (HTDD.idcnt++);

   /** Get x,y of this object **/
   if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
   if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h=0;
   if (h < s->ClientInfo->ParagraphHeight+2)
	h = s->ClientInfo->ParagraphHeight+2;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"numdisplay",DATA_T_INTEGER,POD(&num_disp)) != 0) num_disp=3;

   if (wgtrGetPropertyValue(tree,"hilight",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(hilight,ptr,sizeof(hilight));
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'hilight' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(bgstr,ptr,sizeof(bgstr));
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'bgcolor' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) {
	strtcpy(fieldname,ptr,sizeof(fieldname));
   } else {
	fieldname[0]='\0';
   }

   if (wgtrGetPropertyValue(tree,"form",DATA_T_STRING,POD(&ptr)) == 0)
	strtcpy(form,ptr,sizeof(form));
   else
	form[0]='\0';

    /** Get name **/
    if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
    strtcpy(name,ptr,sizeof(name));

    /** Ok, write the style header items. **/
    htrAddStylesheetItem_va(s,"\t#dd%POSbtn { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; HEIGHT:%POSpx; WIDTH:%POSpx; Z-INDEX:%POS; }\n",id,x,y,h,w,z);
    htrAddStylesheetItem_va(s,"\t#dd%POScon1 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,h-2,z+1);
    htrAddStylesheetItem_va(s,"\t#dd%POScon2 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:hidden; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",id,h-2,z+1);

    htrAddScriptGlobal(s, "dd_current", "null", 0);
    htrAddScriptGlobal(s, "dd_lastkey", "null", 0);
    htrAddScriptGlobal(s, "dd_target_img", "null", 0);
    htrAddScriptGlobal(s, "dd_thum_y","0",0);
    htrAddScriptGlobal(s, "dd_timeout","null",0);
    htrAddScriptGlobal(s, "dd_click_x","0",0);
    htrAddScriptGlobal(s, "dd_click_y","0",0);
    htrAddScriptGlobal(s, "dd_incr","0",0);
    htrAddScriptGlobal(s, "dd_cur_mainlayer","null",0);
    htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"dd%POSbtn\")", id);
    htrAddWgtrCtrLinkage(s, tree, "_obj");

    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
    htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js", 0);

    htrAddEventHandlerFunction(s, "document","MOUSEMOVE", "dd", "dd_mousemove");
    htrAddEventHandlerFunction(s, "document","MOUSEOVER", "dd", "dd_mouseover");
    htrAddEventHandlerFunction(s, "document","MOUSEUP", "dd", "dd_mouseup");
    htrAddEventHandlerFunction(s, "document","MOUSEDOWN", "dd", "dd_mousedown");
    if (s->Capabilities.Dom1HTML)
       htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "dd", "dd_contextmenu");


    /** Get the mode (default to 1, dynamicpage) **/
    mode = 0;
    if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0) {
	if (!strcmp(ptr,"static")) mode = 0;
	else if (!strcmp(ptr,"dynamic_server")) mode = 1;
	else if (!strcmp(ptr,"dynamic")) mode = 2;
	else if (!strcmp(ptr,"dynamic_client")) mode = 2;
	else if (!strcmp(ptr,"objectsource")) mode = 3;
	else {
	    mssError(1,"HTDD","Dropdown widget has not specified a valid mode.");
	    return -1;
	}
    }

    sql = 0;
    if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0 && mode != 0 && mode != 3) {
	mssError(1, "HTDD", "SQL parameter was not specified for dropdown widget");
	return -1;
    }
    /** Script initialization call. **/
    htrAddScriptInit_va(s,"    dd_init({layer:nodes[\"%STR&SYM\"], c1:htr_subel(nodes[\"%STR&SYM\"], \"dd%POScon1\"), c2:htr_subel(nodes[\"%STR&SYM\"], \"dd%POScon2\"), background:'%STR&ESCQ', highlight:'%STR&ESCQ', fieldname:'%STR&ESCQ', numDisplay:%INT, mode:%INT, sql:'%STR&ESCQ', width:%INT, height:%INT, form:'%STR&ESCQ'});\n", name, name, id, name, id, bgstr, hilight, fieldname, num_disp, mode, sql?sql:"", w, h, form);

    /** HTML body <DIV> element for the layers. **/
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POSbtn\">\n", id);
    htrAddBodyItem_va(s,"<TABLE width=%POS cellspacing=0 cellpadding=0 border=0 bgcolor=\"%STR&HTE\">\n",w, bgstr);
    htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%POS></TD>\n",w-2);
    htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%POS width=1></TD>\n",h-2);
    htrAddBodyItem(s,   "       <TD align=right valign=middle><IMG SRC=/sys/images/ico15b.gif></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%POS width=1></TD></TR>\n",h-2);
    htrAddBodyItem(s,   "   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%POS></TD>\n",w-2);
    htrAddBodyItem(s,   "       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
    htrAddBodyItem(s,   "</TABLE>\n");
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POScon1\"></DIV>\n",id);
    htrAddBodyItem_va(s,"<DIV ID=\"dd%POScon2\"></DIV>\n",id);
    htrAddBodyItem(s,   "</DIV>\n");
    
    /* Read and initialize the dropdown items */
    if (mode == 1) {
	if ((qy = objMultiQuery(s->ObjSession, sql))) {
	    flag=0;
	    htrAddScriptInit_va(s,"    dd_add_items(nodes[\"%STR&SYM\"], [",name);
	    while ((qy_obj = objQueryFetch(qy, O_RDONLY))) {
		// Label
		attr = objGetFirstAttr(qy_obj);
		if (!attr) {
		    objClose(qy_obj);
		    objQueryClose(qy);
		    mssError(1, "HTDD", "SQL query must have two attributes: label and value.");
		    return -1;
		}
		type = objGetAttrType(qy_obj, attr);
		rval = objGetAttrValue(qy_obj, attr, type,&od);
		if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
		    str = objDataToStringTmp(type, (void*)(&od), DATA_F_QUOTED);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), DATA_F_QUOTED);
		}
		if (flag) htrAddScriptInit(s,",");
		htrAddScriptInit_va(s,"{wname:null, label:%STR,",str);
		// Value
		attr = objGetNextAttr(qy_obj);
		if (!attr) {
		    objClose(qy_obj);
		    objQueryClose(qy);
		    mssError(1, "HTDD", "SQL query must have two attributes: label and value.");
		    return -1;
		}

		type = objGetAttrType(qy_obj, attr);
		rval = objGetAttrValue(qy_obj, attr, type,&od);
		if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) {
		    str = objDataToStringTmp(type, (void*)(&od), DATA_F_QUOTED);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), DATA_F_QUOTED);
		}
		htrAddScriptInit_va(s,"value:%STR}", str);
		objClose(qy_obj);
		flag=1;
	    }
	    htrAddScriptInit(s,"]);\n");
	    objQueryClose(qy);
	}
    }
    else if(mode==3) {
	/* get objects from form */
    }


    flag=0;
    for (i=0;i<xaCount(&(tree->Children));i++)
	{
	subtree = xaGetItem(&(tree->Children), i);
	if (!strcmp(subtree->Type, "widget/dropdownitem")) 
	    subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	if (!strcmp(subtree->Type,"widget/dropdownitem") && mode == 0) 
	    {
	    if (wgtrGetPropertyValue(subtree,"label",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'width' property");
		return -1;
		}
	    strtcpy(string, ptr, sizeof(string));
	    if (flag) 
		{
		xsConcatenate(&xs, ",", 1);
		}
	    else 
		{
		xsInit(&xs);
		xsConcatQPrintf(&xs, "    dd_add_items(nodes[\"%STR&SYM\"], [", name);
		flag=1;
		}
	    wgtrGetPropertyValue(subtree,"name",DATA_T_STRING,POD(&ptr));
	    xsConcatQPrintf(&xs,"{wname:'%STR&SYM', label:'%STR&ESCQ',", ptr, string);

	    if (wgtrGetPropertyValue(subtree,"value",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'value' property");
		return -1;
		}
	    strtcpy(string,ptr, sizeof(string));
	    xsConcatQPrintf(&xs,"value:'%STR&ESCQ'}", string);
	    } 
	else 
	    {
	    htrRenderWidget(s, subtree, z+1);
	    }
	}
    if (flag) 
	{
	xsConcatenate(&xs, "]);\n", 4);
	htrAddScriptInit(s,xs.String);
	xsDeInit(&xs);
	}


    return 0;
}


/* 
   htddInitialize - register with the ht_render module.
*/
int htddInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Drop Down Widget Driver");
   strcpy(drv->WidgetName,"dropdown");
   drv->Render = htddRender;
   xaAddItem(&(drv->PseudoTypes), "dropdownitem");

   /** Register events **/
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
   htrAddEvent(drv,"DataChange");
   htrAddEvent(drv,"GetFocus");
   htrAddEvent(drv,"LoseFocus");

   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

   HTDD.idcnt = 0;

   return 0;
}

/**CVSDATA***************************************************************

    $Id: htdrv_dropdown.c,v 1.59 2007/07/25 16:54:29 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
    Revision 1.59  2007/07/25 16:54:29  gbeeley
    - (feature) allow dropdown widget to directly specify what form it is
      using instead of just defaulting to a containing form.

    Revision 1.58  2007/07/03 22:44:59  dkasper
    - Added a new mode to the dropdown called object source.

    Revision 1.57  2007/06/01 21:05:55  dkasper
    - Added hook for context menu event

    Revision 1.56  2007/04/19 21:26:49  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.55  2006/11/16 20:15:53  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.54  2006/10/27 05:57:23  gbeeley
    - (change) All widgets switched over to use event handler functions instead
      of inline event scripts in the main .app generated DHTML file.
    - (change) Reworked the way event capture is done to allow dynamically
      loaded components to hook in with the existing event handling mechanisms
      in the already-generated page.
    - (feature) Dynamic-loading of components now works.  Multiple instancing
      does not yet work.  Components need not be "rectangular", but all pieces
      of the component must share a common container.

    Revision 1.53  2006/10/16 18:34:33  gbeeley
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

    Revision 1.52  2006/06/19 15:03:02  gbeeley
    - Mozilla port.

    Revision 1.51  2005/10/09 07:46:47  gbeeley
    - (change) continued work porting dropdown to IE/Moz

    Revision 1.50  2005/06/23 22:07:58  ncolson
    Modified *_init JavaScript function call here in the HTML generator so that
    when it is executed in the generated page it no longer passes parameters as
    individual variables, but as properties of a single object, which are position
    independent. Made corresponding changes in the *.js file to pick apart the
    object once it is passed.

    Revision 1.49  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.48  2004/12/31 04:42:03  gbeeley
    - global variable pollution fixes for dropdown widget
    - use dd_collapse() for dropdown widget event script
    - fix to background image for windows

    Revision 1.47  2004/08/27 01:28:32  jorupp
     * cleaning up some compile warnings

    Revision 1.46  2004/08/04 20:03:08  mmcgill
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

    Revision 1.45  2004/08/04 01:58:56  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.44  2004/08/02 14:09:34  mmcgill
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

    Revision 1.43  2004/07/20 21:28:52  mmcgill
    *   ht_render
        -   Added code to perform verification of widget-tree prior to
            rendering.
        -   Added concept of 'pseudo-types' for widget-drivers, e.g. the
            table driver getting called for 'table-column' widgets. This is
            necessary now since the 'table-column' entry in an app file will
            actually get put into its own widget node. Pseudo-type names
            are stored in an XArray in the driver struct during the
            xxxInitialize() function of the driver, and BEFORE ANY CALLS TO
            htrAddSupport().
        -   Added htrLookupDriver() to encapsulate the process of looking up
            a driver given an HtSession and widget type
        -   Added 'pWgtrVerifySession VerifySession' to HtSession.
            WgtrVerifySession represents a 'verification context' to be used
            by the xxxVerify functions in the widget drivers to schedule new
            widgets for verification, and otherwise interact with the
            verification system.
    *   xxxVerify() functions now take a pHtSession parameter.
    *   Updated the dropdown, tab, and table widgets to register their
        pseudo-types
    *   Moved the ObjProperty out of obj.h and into wgtr.c to internalize it,
        in anticipation of converting the Wgtr module to use PTODs instead.
    *   Fixed some Wgtr module memory-leak issues
    *   Added functions wgtrScheduleVerify() and wgtrCancelVerify(). They are
        to be used in the xxxVerify() functions when a node has been
        dynamically added to the widget tree during tree verification.
    *   Added the formbar widget driver, as a demonstration of how to modify
        the widget-tree during the verification process. The formbar widget
        doesn't actually do anything during the rendering process excpet
        call htrRenderWidget on its subwidgets, but during Verify it adds
        all the widgets necessary to reproduce the 'form control pane' from
        ors.app. This will eventually be done even more efficiently with
        component widgets - this serves as a tech test.

    Revision 1.42  2004/07/19 15:30:39  mmcgill
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

    Revision 1.41  2004/06/12 03:59:00  gbeeley
    - starting to implement tree linkages to link the DHTML widgets together
      on the client in the same organization that they are in within the .app
      file on the server.

    Revision 1.40  2004/05/04 18:22:19  gbeeley
    - start of port of dropdown widget to W3C/IE

    Revision 1.39  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.38  2002/12/04 00:19:10  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.37  2002/09/27 22:26:05  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.36  2002/08/21 02:14:15  jorupp
     * updated a couple GetAttrValue calls to explicitly specify the data type, as the code was assuming a certain type anyway
     * fixed a wrong error message

    Revision 1.35  2002/08/13 19:23:01  lkehresman
    Made the dropdown less user-friendly (it used to print out "hi" when you
    clicked a button).

    Revision 1.34  2002/08/05 19:36:13  lkehresman
    Reworked the check on MOUSEUP for the dropdown so that it won't try and
    rewrite the image.src attribute unless it is the up/down scroll button.

    Revision 1.33  2002/08/05 19:20:08  lkehresman
    * Revamped the GUI for the DropDown to make it look cleaner
    * Added the function pg_resize_area() so page areas can be resized.  This
      allows the dropdown and datetime widgets to contain focus on their layer
      that extends down.  Previously it was very kludgy, now it works nicely
      by just extending the page area for that widget.
    * Reworked the dropdown widget to take advantage of the resize function
    * Added .mainlayer attributes to the editbox widget (new page functionaly
      requires .mainlayer properties soon to be standard in all widgets).

    Revision 1.32  2002/08/02 14:53:39  lkehresman
    Fixed dropdown bug that was substituting the last 5 characters of images
    with "b.gif" on MOUSEUP to unpress icon buttons.  However, this wasn't doing
    proper checking to make sure it was only happening on dropdown images, so
    all images that had mouseup events would get changed causing errors.

    Revision 1.31  2002/07/31 22:03:43  lkehresman
    Fixed mouseup issues when mouseup occurred outside the image for:
      * dropdown scroll images
      * imagebutton images

    Revision 1.30  2002/07/31 21:26:57  lkehresman
    Added support to click the area above and below the thumb image to scroll
    a page up and a page down in the dropdown widget

    Revision 1.29  2002/07/31 15:03:11  lkehresman
    Changed the default dropdown population mode to be static rather than
    dynamic_client to retain backwards compatibility with the previous
    dropdown widget revisions.

    Revision 1.28  2002/07/31 13:35:59  lkehresman
    * Made x.mainlayer always point to the top layer in dropdown
    * Fixed a netscape crash bug with the event stuff from the last revision of dropdown
    * Added a check to the page event stuff to make sure that pg_curkbdlayer is set
        before accessing the pg_curkbdlayer.getfocushandler() function. (was causing
        javascript errors before because of the special case of the dropdown widget)

    Revision 1.27  2002/07/26 18:15:40  lkehresman
    Added standard events to dropdown
    MouseUp,MouseDown,MouseOut,MouseOver,MouseMove,Click,DataChange,GetFocus,LoseFocus

    Revision 1.26  2002/07/25 15:06:47  lkehresman
    * Fixed bug where dropdown wasn't going away
    * Added enable/disable/readonly support

    Revision 1.25  2002/07/24 20:33:15  lkehresman
    Complete reworking of the dropdown widget.  Much more functionality
    (including, FINALLY, a working scrollbar).  Better interface.  More
    bugs (still working out some of the kinks).  This also has a shell
    for client-side dynamic population of the dropdown, which was the
    main reason for the restructure/rewrite.

 **END-CVSDATA***********************************************************/
