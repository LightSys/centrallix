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
/* Module: 	htdrv_terminal.c      					*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is visual widget that emulates a vt100 terminal 	*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_terminal.c,v $
    Revision 1.6  2004/08/04 20:03:11  mmcgill
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

    Revision 1.5  2004/08/04 01:58:57  mmcgill
    Added code to ht_render and the ht drivers to build a representation of
    the widget tree on the client-side, linking each node to its corresponding
    widget object or layer. Also fixed a couple bugs that were introduced
    by switching to rendering off the widget tree.

    Revision 1.4  2004/08/02 14:09:35  mmcgill
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

    Revision 1.3  2004/07/19 15:30:41  mmcgill
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

    Revision 1.2  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.1  2002/12/24 09:51:56  jorupp
     * yep, this is what it looks like -- inital commit of the terminal widget :)
       -- the first Mozilla-only widget
     * it's not even close to a 'real' form yet -- mozilla takes so much CPU time
       rendering the table that it's pretty useless


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTERM;


/*** httermRender - generate the HTML code for the form 'glue'
 ***/
int
httermRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    int id;
    char* nptr;
    pWgtrNode sub_tree;
    pObjQuery qy;
#define MAX_COLORS 8
#define MAX_COLOR_LEN 32
    char colors[MAX_COLORS][MAX_COLOR_LEN]={"black","red","green","yellow","blue","purple","aqua","white"};
    int i;
    XString source;
    int rows, cols, fontsize, x, y;

	/** our first Mozilla-only widget :) **/
	if(!s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTERM","W3C DOM Level 1 support required");
	    return -1;
	    }


    	/** Get an id for this. **/
	id = (HTTERM.idcnt++);

	/** read the color specs, leaving the defaults if they don't exist **/
	for(i=0;i<MAX_COLORS;i++)
	    {
	    char color[32];
	    sprintf(color,"color%i",i);
	    if (wgtrGetPropertyValue(tree,color,DATA_T_STRING,POD(&ptr)) == 0) 
		{
		strncpy(colors[i],ptr,MAX_COLOR_LEN);
		colors[i][MAX_COLOR_LEN-1]='\0';
		}
	    }

	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(0,"TERM","x is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&y)) != 0) 
	    {
	    mssError(0,"TERM","y is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"rows",DATA_T_INTEGER,POD(&rows)) != 0) 
	    {
	    mssError(0,"TERM","rows is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"cols",DATA_T_INTEGER,POD(&cols)) != 0) 
	    {
	    mssError(0,"TERM","cols is required");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"fontsize",DATA_T_INTEGER,POD(&fontsize)) != 0) 
	    {
	    fontsize = 12;
	    }

	xsInit(&source);

	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    xsCopy(&source,ptr,strlen(ptr));
	    }
	else
	    {
	    xsDeInit(&source);
	    mssError(0,"TERM","source is required");
	    return -1;
	    }
	
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
	htrAddScriptInclude(s, "/sys/js/htdrv_terminal.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** HTML body <IFRAME> element to use as the base **/
	htrAddBodyItem_va(s,"    <DIV ID=\"term%dbase\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%dreader\"></IFRAME>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%dwriter\"></IFRAME>\n",id);
	
	/** write the stylesheet header element **/
	htrAddStylesheetItem_va(s,"        #term%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%i; TOP:%i;  WIDTH:%i; HEIGHT:%i; Z-INDEX:%i; }\n",id,x,y,cols*fontsize,rows*fontsize,z);
	htrAddStylesheetItem_va(s,"        #term%dreader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        #term%dwriter { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        .fixed%d {font-family: fixed; }\n",id);

	/** init line **/
	htrAddScriptInit_va(s,"    %s=terminal_init(%s,%d,'%s',%d,%d,new Array(",name,parentname,id,source.String,rows,cols);
	for(i=0;i<MAX_COLORS;i++)
	    {
	    if(i!=0)
		htrAddScriptInit(s,",");
	    htrAddScriptInit_va(s,"'%s'",colors[i]);
	    }
	htrAddScriptInit(s,"));\n");




	/** Check for and render all subobjects. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (strcmp(ptr,"widget/connector") == 0)
		htrRenderWidget(s, sub_tree, z+1, "", name);
	    else
		/** probably shouldn't render anything other than connectors, but who knows... **/
		htrRenderWidget(s, sub_tree, z+1, parentname, parentobj);
	    }
	

    return 0;
    }


/*** httermInitialize - register with the ht_render module.
 ***/
int
httermInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Terminal Widget");
	strcpy(drv->WidgetName,"terminal");
	drv->Render = httermRender;


	/** Add our actions **/
	htrAddAction(drv,"Disconnect");
	htrAddAction(drv,"Connect");

	/** Add our Events **/
	htrAddEvent(drv,"ConnectionOpen");
	htrAddEvent(drv,"ConnectionClose");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTERM.idcnt = 0;

    return 0;
    }
