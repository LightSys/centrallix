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
/* Module: 	htdrv_tab.c             				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 28, 1998 					*/
/* Description:	HTML Widget driver for a tab control.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_tab.c,v 1.16 2003/11/15 19:59:57 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_tab.c,v $

    $Log: htdrv_tab.c,v $
    Revision 1.16  2003/11/15 19:59:57  gbeeley
    Adding support for tabs on any of 4 sides of tab control

    Revision 1.15  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.14  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.13  2002/08/13 01:43:56  gbeeley
    Updating htdrv_tab to use the new OSML API for objGetAttrValue().

    Revision 1.12  2002/07/30 19:04:45  lkehresman
    * Added standard events to tab widget
    * Converted tab widget to use standard mainlayer and layer properties

    Revision 1.11  2002/07/23 15:22:39  mcancel
    Changing htrAddHeaderItem to htrAddStylesheetItem for a couple of files
    that missed the API change - for adding style sheet definitions.

    Revision 1.10  2002/07/20 19:44:25  lkehresman
    Event handlers now have the variable "ly" defined as the target layer
    and it will be global for all the events.  We were finding that nearly
    every widget defined this themselves, so I just made it global to save
    some variables and a lot of lines of duplicate code.

    Revision 1.9  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.8  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.7  2002/07/16 17:52:01  lkehresman
    Updated widget drivers to use include files

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2001/10/23 02:21:09  gbeeley
    Fixed incorrect auto-setting of tabpage clip height.

    Revision 1.2  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTAB;


enum httab_locations { Top=0, Bottom=1, Left=2, Right=3 };


/*** httabVerify - not written yet.
 ***/
int
httabVerify()
    {
    return 0;
    }


/*** httabRender - generate the HTML code for the page.
 ***/
int
httabRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char tab_txt[128];
    char main_bg[128];
    /*char inactive_bg[128];*/
    char sel[128];
    pObject tabpage_obj,sub_w_obj;
    pObjQuery qy,subqy;
    int x=-1,y=-1,w,h;
    int id,tabcnt;
    char* nptr;
    char* subnptr;
    enum httab_locations tloc;
    int tab_width = 0;
    int xoffset,yoffset,xtoffset, ytoffset;

	if(!s->Capabilities.Dom0NS)
	    {
	    mssError(1,"HTTAB","NS4 DOM Support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTTAB.idcnt++);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",DATA_T_INTEGER,POD(&w)) != 0) 
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",DATA_T_INTEGER,POD(&h)) != 0)
	    {
	    mssError(0,"HTTAB","Tab widget must have a 'height' property");
	    return -1;
	    }

	/** Which side are the tabs on? **/
	if (objGetAttrValue(w_obj,"tab_location",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcasecmp(ptr,"top")) tloc = Top;
	    else if (!strcasecmp(ptr,"bottom")) tloc = Bottom;
	    else if (!strcasecmp(ptr,"left")) tloc = Left;
	    else if (!strcasecmp(ptr,"right")) tloc = Right;
	    else
		{
		mssError(1,"HTTAB","%s: '%s' is not a valid tab_location",name,ptr);
		return -1;
		}
	    }
	else
	    {
	    tloc = Top;
	    }

	/** How wide should left/right tabs be? **/
	if (objGetAttrValue(w_obj,"tab_width",DATA_T_INTEGER,POD(&tab_width)) != 0)
	    {
	    if (tloc == Right || tloc == Left)
		{
		mssError(1,"HTTAB","%s: tab_width must be specified with tab_location of left or right", name);
		return -1;
		}
	    }
	else
	    {
	    if (tab_width < 0) tab_width = 0;
	    }

	/** Which tab is selected? **/
	if (objGetAttrValue(w_obj,"selected",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    snprintf(sel,128,"%s",ptr);
	    }
	else
	    {
	    strcpy(sel,"");
	    }

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Inactive tab color/image? **/
	/*if (objGetAttrValue(w_obj,"inactive_bgcolor",POD(&ptr)) == 0)
	    sprintf(inactive_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"inactive_background",POD(&ptr)) == 0)
	    sprintf(inactive_bg,"background='%.110s'",ptr);
	else
	    strcpy(inactive_bg,"");*/

	/** Text color? **/
	if (objGetAttrValue(w_obj,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    sprintf(tab_txt,"%.127s",ptr);
	else
	    strcpy(tab_txt,"black");

	/** Determine offset to actual tab pages **/
	switch(tloc)
	    {
	    case Top:    xoffset = 0;         yoffset = 24; xtoffset = 0; ytoffset = 0; break;
	    case Bottom: xoffset = 0;         yoffset = 0;  xtoffset = 0; ytoffset = h; break;
	    case Right:  xoffset = 0;         yoffset = 0;  xtoffset = w; ytoffset = 0; break;
	    case Left:   xoffset = tab_width; yoffset = 0;  xtoffset = 0; ytoffset = 0; break;
	    }

	/** Ok, write the style header items. **/
	htrAddStylesheetItem_va(s,"\t#tc%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x+xoffset,y+yoffset,w,z+1);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Script include **/
	htrAddScriptInclude(s, "/sys/js/htdrv_tab.js", 0);

	/** Add a global for the master tabs listing **/
	htrAddScriptGlobal(s, "tc_tabs", "null", 0);
	htrAddScriptGlobal(s, "tc_cur_mainlayer", "null", 0);

	/** Event handler for click-on-tab **/
	htrAddEventHandler(s, "document","MOUSEDOWN","tc",
		"    if (ly.mainlayer && ly.mainlayer.kind == 'tc') cn_activate(ly.mainlayer, 'MouseDown');\n"
		"    if (ly.kind == 'tc') ly.makeCurrent();\n");

	htrAddEventHandler(s, "document","MOUSEUP","tc",
		"    if (ly.mainlayer && ly.mainlayer.kind == 'tc') cn_activate(ly.mainlayer, 'MouseUp');\n");

	htrAddEventHandler(s, "document","MOUSEMOVE","tc",
		"    if (!ly.mainlayer || ly.mainlayer.kind != 'tc')\n"
		"        {\n"
		"        if (tc_cur_mainlayer) cn_activate(tc_cur_mainlayer, 'MouseOut');\n"
		"        tc_cur_mainlayer = null;\n"
		"        }\n"
		"    else if (ly.kind && ly.kind.substr(0,2) == 'tc')\n"
		"        {\n"
		"        cn_activate(ly.mainlayer, 'MouseMove');\n"
		"        }\n");

	htrAddEventHandler(s, "document","MOUSEOVER","tc",
		"    if (ly.kind && ly.kind.substr(0,2) == 'tc')\n"
		"        {\n"
		"        cn_activate(ly.mainlayer, 'MouseOver');\n"
		"        tc_cur_mainlayer = ly.mainlayer;\n"
		"        }\n");

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    %s = tc_init(%s.layers.tc%dbase, %d);\n",
		nptr, parentname, id, tloc);

	/** Check for tabpages within the tab control, to do the tabs at the top. **/
	tabcnt = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((tabpage_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(tabpage_obj,"outer_type",DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"widget/tabpage"))
		    {
		    objGetAttrValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
		    tabcnt++;
		    htrAddBodyItem_va(s,"<DIV ID=\"tc%dtab%d\">\n",id,tabcnt);
		    htrAddBodyItem_va(s,"    <TABLE cellspacing=0 cellpadding=0 border=0 width=%d %s>\n", tab_width, main_bg);
		    if (tloc != Bottom) 
			htrAddBodyItem_va(s,"        <TR><TD colspan=%d background=/sys/images/white_1x1.png><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n", (tloc == Top || tloc == Bottom)?3:2);
		    htrAddBodyItem(s,"        <TR>");
		    if (tloc != Right)
			{
			htrAddBodyItem(s,"<TD><IMG SRC=/sys/images/white_1x1.png height=24 width=1>");
			if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
			    htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft2.gif name=tb height=24></TD>\n");
			else
			    htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft3.gif name=tb height=24></TD>\n");
			}
		    if (objGetAttrValue(tabpage_obj,"title",DATA_T_STRING,POD(&ptr)) == 0)
		        {
		        htrAddBodyItem_va(s,"            <TD valign=middle align=center><FONT COLOR=%s><b>&nbsp;%s&nbsp;</b></FONT></TD>\n", tab_txt, ptr);
			}
		    else
		        {
			objGetAttrValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
		        htrAddBodyItem_va(s,"            <TD valign=middle align=center><FONT COLOR=%s><B>&nbsp;%s&nbsp;</B></FONT></TD>\n", tab_txt, ptr);
			}
		    if (tloc != Left)
			htrAddBodyItem(s,"           <TD align=right>");
		    if (tloc == Right)
			{
			if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
			    htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft2.gif name=tb height=24>");
			else
			    htrAddBodyItem(s,"<IMG SRC=/sys/images/tab_lft3.gif name=tb height=24>");
			}
		    if (tloc != Left)
			htrAddBodyItem(s,"<IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=24></TD>\n");
		    htrAddBodyItem(s,"            </TR>\n");
		    if (tloc != Top) 
			htrAddBodyItem_va(s,"        <TR><TD colspan=%d background=/sys/images/dkgrey_1x1.png><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n", (tloc == Top || tloc == Bottom)?3:2);
		    htrAddBodyItem(s,"    </TABLE>\n");
		    htrAddBodyItem(s, "</DIV>\n");
		    }
		objClose(tabpage_obj);
		}
	    objQueryClose(qy);
	    }

	/** HTML body <DIV> element for the base layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"tc%dbase\">\n",id);
	htrAddBodyItem_va(s,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
	htrAddBodyItem_va(s,"            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem_va(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n    </TABLE>\n\n");

	/** Check for tabpages within the tab control entity, this time to do the pages themselves **/
	tabcnt = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((tabpage_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(tabpage_obj,"outer_type",DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"widget/tabpage"))
		    {
		    /** First, render the tabpage and add stuff for it **/
		    objGetAttrValue(tabpage_obj,"name",DATA_T_STRING,POD(&ptr));
		    tabcnt++;

		    /** Add stylesheet headers for the layers (tab and tabpage) **/
		    if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
		        {
		        htrAddStylesheetItem_va(s,"\t#tc%dtab%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,x+xtoffset,y+ytoffset,1,z+2);
		        htrAddStylesheetItem_va(s,"\t#tc%dpane%d { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,w-2,z+2);
			}
		    else
		        {
		        htrAddStylesheetItem_va(s,"\t#tc%dtab%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,x,y,1,z);
		        htrAddStylesheetItem_va(s,"\t#tc%dpane%d { POSITION:absolute; VISIBILITY:hidden; LEFT:1; TOP:1; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,w-2,z+2);
			}

		    /** Add script initialization to add a new tabpage **/
		    htrAddScriptInit_va(s,"    %s = %s.addTab(%s.layers.tc%dtab%d,%s.document.layers.tc%dpane%d,%s);\n",
			ptr, nptr, parentname, id, tabcnt, nptr, id, tabcnt, nptr);

		    /** Add named global for the tabpage **/
		    subnptr = (char*)nmMalloc(strlen(ptr)+1);
		    strcpy(subnptr,ptr);
		    htrAddScriptGlobal(s, subnptr, "null", HTR_F_NAMEALLOC);

		    /** Add DIV section for the tabpage. **/
		    htrAddBodyItem_va(s,"<DIV ID=\"tc%dpane%d\">\n",id,tabcnt);

		    /** Now look for sub-items within the tabpage. **/
		    snprintf(sbuf,160,"%s.tabpage.document",subnptr);
		    snprintf(name,64,"%s.tabpage",subnptr);
		    subqy = objOpenQuery(tabpage_obj,"",NULL,NULL,NULL);
		    if (subqy)
		        {
			while((sub_w_obj = objQueryFetch(subqy, O_RDONLY)))
			    {
		            htrRenderWidget(s, sub_w_obj, z+3, sbuf, name);
			    objClose(sub_w_obj);
			    }
			}
		    objQueryClose(subqy);
		    htrAddBodyItem(s, "</DIV>\n");
		    }
		else if (!strcmp(ptr,"widget/connector"))
		    {
		    snprintf(sbuf,160,"%s.mainlayer.document",nptr);
		    snprintf(name,64,"%s.mainlayer",nptr);
		    htrRenderWidget(s, tabpage_obj, z+2, sbuf, name);
		    }
		objClose(tabpage_obj);
		}

	    objQueryClose(qy);
	    }

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** httabInitialize - register with the ht_render module.
 ***/
int
httabInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Tab Control / Tab Page Driver");
	strcpy(drv->WidgetName,"tab");
	drv->Render = httabRender;
	drv->Verify = httabVerify;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTAB.idcnt = 0;

    return 0;
    }
