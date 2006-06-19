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
int htddRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj) {
   char bgstr[HT_SBUF_SIZE];
   char hilight[HT_SBUF_SIZE];
   char string[HT_SBUF_SIZE];
   char fieldname[30];
   char name[64];
   char *ptr, *nptr;
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
   if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h=20;
   if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"numdisplay",DATA_T_INTEGER,POD(&num_disp)) != 0) num_disp=3;

   if (wgtrGetPropertyValue(tree,"hilight",DATA_T_STRING,POD(&ptr)) == 0) {
	snprintf(hilight,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'hilight' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0) {
	snprintf(bgstr,HT_SBUF_SIZE,"%.40s",ptr);
   } else {
	mssError(1,"HTDD","Drop Down widget must have a 'bgcolor' property");
	return -1;
   }

   if (wgtrGetPropertyValue(tree,"fieldname",DATA_T_STRING,POD(&ptr)) == 0) {
	strncpy(fieldname,ptr,30);
   } else {
	fieldname[0]='\0';
   }

    /** Get name **/
    if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
    memccpy(name,ptr,0,63);
    name[63] = 0;
    nptr = (char*)nmMalloc(strlen(name)+1);
    strcpy(nptr,name);

    /** Ok, write the style header items. **/
    htrAddStylesheetItem_va(s,"\t#dd%dbtn { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:%dpx; TOP:%dpx; HEIGHT:20px; WIDTH:%dpx; Z-INDEX:%d; }\n",id,x,y,w,z);
    htrAddStylesheetItem_va(s,"\t#dd%dcon1 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:inherit; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,h-2,z+1);
    htrAddStylesheetItem_va(s,"\t#dd%dcon2 { OVERFLOW:hidden; POSITION:absolute; VISIBILITY:hidden; LEFT:1px; TOP:1px; WIDTH:1024px; HEIGHT:%dpx; Z-INDEX:%d; }\n",id,h-2,z+1);

    htrAddScriptGlobal(s, "dd_current", "null", 0);
    htrAddScriptGlobal(s, "dd_lastkey", "null", 0);
    htrAddScriptGlobal(s, "dd_target_img", "null", 0);
    htrAddScriptGlobal(s, "dd_thum_y","0",0);
    htrAddScriptGlobal(s, "dd_timeout","null",0);
    htrAddScriptGlobal(s, "dd_click_x","0",0);
    htrAddScriptGlobal(s, "dd_click_y","0",0);
    htrAddScriptGlobal(s, "dd_incr","0",0);
    htrAddScriptGlobal(s, "dd_cur_mainlayer","null",0);
    htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

    htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
    htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
    htrAddScriptInclude(s, "/sys/js/htdrv_dropdown.js", 0);

    htrAddEventHandler(s, "document","MOUSEMOVE", "dd", 
	"\n"
	"    var ti=dd_target_img;\n"
	"    if (ti != null && ti.name == 't' && dd_current && dd_current.enabled!='disabled')\n"
	"        {\n"
	"        var pl=ti.mainlayer.PaneLayer;\n"
	"        var v=getClipHeight(pl)-(3*18)-4;\n"
	"        var new_y=dd_thum_y+(e.pageY-dd_click_y)\n"
	"        if (new_y > getPageY(pl)+20+v) new_y=getPageY(pl)+20+v;\n"
	"        if (new_y < getPageY(pl)+20) new_y=getPageY(pl)+20;\n"
	"        setPageY(ti.thum,new_y);\n"
	"        var h=dd_current.PaneLayer.h;\n"
	"        var d=h-getClipHeight(pl)+4;\n"
	"        if (d<0) d=0;\n"
	"        dd_incr = (((getRelativeY(ti.thum)-22)/(v-4))*-d)-getRelativeY(dd_current.PaneLayer.ScrLayer);\n"
	"        dd_scroll(0);\n"
	"        return false;\n"
	"        }\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'dd')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseMove');\n"
	"        dd_cur_mainlayer = null;\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEOVER", "dd", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'dd')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseOver');\n"
	"        dd_cur_mainlayer = ly.mainlayer;\n"
	"        }\n"
	"    if (ly.kind == 'dd_itm' && dd_current && dd_current.enabled=='full')\n"
	"        {\n"
	"        dd_lastkey = null;\n"
	"        dd_hilight_item(dd_current, ly.index);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEUP", "dd", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'dd')\n"
	"        {\n"
	"        cn_activate(ly.mainlayer, 'MouseUp');\n"
	"        }\n"
	"    if (dd_timeout != null)\n"
	"        {\n"
	"        clearTimeout(dd_timeout);\n"
	"        dd_timeout = null;\n"
	"        dd_incr = 0;\n"
	"        }\n"
	"    if (dd_target_img != null)\n"
	"        {\n"
	"        if (dd_target_img.kind && dd_target_img.kind.substr(0,2) == 'dd' && (dd_target_img.name == 'u' || dd_target_img.name == 'd'))\n"
	"            pg_set(dd_target_img,'src',htutil_subst_last(dd_target_img.src,\"b.gif\"));\n"
	"        dd_target_img = null;\n"
	"        }\n"
	"    if ((ly.kind == 'dd' || ly.kind == 'ddtxt') && ly.mainlayer.enabled != 'disabled')\n"
	"        {\n"
	"        dd_toggle(ly.mainlayer,false);\n"
	"        }\n"
	"\n");

    htrAddEventHandler(s, "document","MOUSEDOWN", "dd", 
	"\n"
	"    if (ly.mainlayer && ly.mainlayer.kind == 'dd') cn_activate(ly.mainlayer, 'MouseDown');\n"
	"    dd_target_img = e.target;\n"
	"    if (ly.kind == 'dd_itm' && dd_current && dd_current.enabled == 'full')\n"
	"        {\n"
	"        dd_select_item(dd_current, ly.index);\n"
	"        dd_collapse(dd_current);\n"
	"        }\n"
	"    else if (ly.kind == 'dd_sc')\n"
	"        {\n"
	"        switch(ly.name)\n"
	"            {\n"
	"            case 'u':\n"
	"                pg_set(ly,'src','/sys/images/ico13c.gif');\n"
	"                dd_incr = 8;\n"
	"                dd_scroll();\n"
	"                dd_timeout = setTimeout(dd_scroll_tm,300);\n"
	"                break;\n"
	"            case 'd':\n"
	"                pg_set(ly, 'src', '/sys/images/ico12c.gif');\n"
	"                dd_incr = -8;\n"
	"                dd_scroll();\n"
	"                dd_timeout = setTimeout(dd_scroll_tm,300);\n"
	"                break;\n"
	"            case 'b':\n"
	"                dd_incr = dd_target_img.height+36;\n"
	"                if (e.pageY > getPageY(dd_target_img.thum)+9) dd_incr = -dd_incr;\n"
	"                dd_scroll();\n"
	"                dd_timeout = setTimeout(dd_scroll_tm,300);\n"
	"                break;\n"
	"            case 't':\n"
	"                dd_click_x = e.pageX;\n"
	"                dd_click_y = e.pageY;\n"
	"                dd_thum_y = getPageY(dd_target_img.thum);\n"
	"                break;\n"
	"            }\n"
	"        }\n"
	"    else if ((ly.kind == 'dd' || ly.kind == 'ddtxt') && ly.mainlayer.enabled != 'disabled')\n"
	"        {\n"
	"        if (dd_current)\n"
	"            {\n"
	"            dd_toggle(dd_current,true);\n"
	"            dd_collapse(dd_current);\n"
	"            }\n"
	"        else\n"
	"            {\n"
	"            dd_toggle(ly.mainlayer,true);\n"
	"            dd_expand(ly.mainlayer);\n"
	"            if(ly.mainlayer.form)\n"
	"                ly.mainlayer.form.FocusNotify(ly);\n"
	"            }\n"
	"        }\n"
	"    if (dd_current && dd_current != ly && dd_current.PaneLayer != ly && (!ly.mainlayer || dd_current != ly.mainlayer))\n"
	"        {\n"
	"        dd_collapse(dd_current);\n"
	"        }\n"
	"\n");

    /** Get the mode (default to 1, dynamicpage) **/
    mode = 0;
    if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0) {
	if (!strcmp(ptr,"static")) mode = 0;
	else if (!strcmp(ptr,"dynamic_server")) mode = 1;
	else if (!strcmp(ptr,"dynamic")) mode = 2;
	else if (!strcmp(ptr,"dynamic_client")) mode = 2;
	else {
	    mssError(1,"HTDD","Dropdown widget has not specified a valid mode.");
	    return -1;
	}
    }

    sql = 0;
    if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0 && mode != 0) {
	mssError(1, "HTDD", "SQL parameter was not specified for dropdown widget");
	return -1;
    }
    /** Script initialization call. **/
    if (s->Capabilities.Dom0NS)
	{
	htrAddScriptInit_va(s,"    %s = dd_init({layer:%s.layers.dd%dbtn, c1:%s.layers.dd%dbtn.document.layers.dd%dcon1, c2:%s.layers.dd%dbtn.document.layers.dd%dcon2, background:'%s', highlight:'%s', fieldname:'%s', numDisplay:%d, mode:%d, sql:'%s', width:%d, height:%d});\n", nptr, parentname, id, parentname, id, id, parentname, id, id, bgstr, hilight, fieldname, num_disp, mode, sql, w, h);
	}
    else if (s->Capabilities.Dom1HTML)
	{
	htrAddScriptInit_va(s,"    %s = dd_init({layer:document.getElementById(\"dd%dbtn\"), c1:document.getElementById(\"dd%dcon1\"), c2:document.getElementById(\"dd%dcon2\"), background:'%s', highlight:'%s', fieldname:'%s', numDisplay:%d, mode:%d, sql:'%s', width:%d, height:%d});\n", nptr, id, id, id, bgstr, hilight, fieldname, num_disp, mode, sql, w, h);
	}

    /** Set object parent **/
    htrAddScriptInit_va(s, "    htr_set_parent(%s, \"%s\", %s);\n",
	    nptr, nptr, parentobj);

    /** HTML body <DIV> element for the layers. **/
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dbtn\">\n", id);
    htrAddBodyItem_va(s,"<TABLE width=%d cellspacing=0 cellpadding=0 border=0 bgcolor=\"%s\">\n",w, bgstr);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
    htrAddBodyItem_va(s,"       <TD align=right valign=middle><IMG SRC=/sys/images/ico15b.gif></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
    htrAddBodyItem_va(s,"   <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
    htrAddBodyItem_va(s,"       <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n");
    htrAddBodyItem_va(s,"</TABLE>\n");
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dcon1\"></DIV>\n",id);
    htrAddBodyItem_va(s,"<DIV ID=\"dd%dcon2\"></DIV>\n",id);
    htrAddBodyItem_va(s,"</DIV>\n");
    
    /* Read and initialize the dropdown items */
    if (mode == 1) {
	if ((qy = objMultiQuery(s->ObjSession, sql))) {
	    flag=0;
	    htrAddScriptInit_va(s,"    dd_add_items(%s, Array(",nptr);
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
		    str = objDataToStringTmp(type, (void*)(&od), 0);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), 0);
		}
		if (flag) htrAddScriptInit(s,",");
		htrAddScriptInit_va(s,"Array('%s',",str);
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
		    str = objDataToStringTmp(type, (void*)(&od), 0);
		} else {
		    str = objDataToStringTmp(type, (void*)(od.String), 0);
		}
		htrAddScriptInit_va(s,"'%s')", str);
		objClose(qy_obj);
		flag=1;
	    }
	    htrAddScriptInit_va(s,"));\n");
	    objQueryClose(qy);
	}
    }



    flag=0;
    for (i=0;i<xaCount(&(tree->Children));i++)
	{
	subtree = xaGetItem(&(tree->Children), i);
	if (!strcmp(subtree->Type, "widget/dropdownitem")) subtree->RenderFlags |= HT_WGTF_NOOBJECT;
	if (!strcmp(subtree->Type,"widget/dropdownitem") && mode == 0) 
	    {
	    if (wgtrGetPropertyValue(subtree,"label",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'width' property");
		return -1;
		}
	    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	    if (flag) 
		{
		xsConcatPrintf(&xs, ",");
		}
	    else 
		{
		xsInit(&xs);
		xsConcatPrintf(&xs, "    dd_add_items(%s, Array(", nptr, id);
		flag=1;
		}
	    xsConcatPrintf(&xs,"Array('%s',", string);

	    if (wgtrGetPropertyValue(subtree,"value",DATA_T_STRING,POD(&ptr)) != 0) 
		{
		mssError(1,"HTDD","Drop Down widget must have a 'value' property");
		return -1;
		}
	    memccpy(string,ptr,0,HT_SBUF_SIZE-1);
	    xsConcatPrintf(&xs,"'%s')", string);
	    } 
	else 
	    {
	    htrRenderWidget(s, subtree, z+1, parentname, nptr);
	    }
	}
    if (flag) 
	{
	xsConcatPrintf(&xs, "));\n");
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

    $Id: htdrv_dropdown.c,v 1.52 2006/06/19 15:03:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
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
