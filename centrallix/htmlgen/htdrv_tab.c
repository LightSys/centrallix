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

    $Id: htdrv_tab.c,v 1.2 2001/10/23 00:25:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_tab.c,v $

    $Log: htdrv_tab.c,v $
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

    	/** Get an id for this. **/
	id = (HTTAB.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTTAB","Tab widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTTAB","Tab widget must have a 'height' property");
	    return -1;
	    }

	/** Which tab is selected? **/
	if (objGetAttrValue(w_obj,"selected",POD(&ptr)) == 0)
	    {
	    sprintf(sel,"%.127s",ptr);
	    }
	else
	    {
	    strcpy(sel,"");
	    }

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
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
	if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
	    sprintf(tab_txt,"%.127s",ptr);
	else
	    strcpy(tab_txt,"black");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Ok, write the style header items. **/
	sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#tc%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y+24,w,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Add a global for the master tabs listing **/
	htrAddScriptGlobal(s, "tc_tabs", "null", 0);

	/** Function for the makeCurrent method on a tabpage. **/
	htrAddScriptFunction(s, "tc_makecurrent", "\n"
		"function tc_makecurrent()\n"
		"    {\n"
		"    if (this.zIndex > this.tabctl.zIndex) return 0;\n"
		"    for(i=0;i<this.tabctl.tabs.length;i++)\n"
		"        {\n"
		"        t = this.tabctl.tabs[i];\n"
		"        if (t != this && t.zIndex > this.tabctl.zIndex)\n"
		"            {\n"
		"            t.zIndex = this.tabctl.zIndex - 1;\n"
		"            t.tabpage.visibility = 'hidden';\n"
		"            t.document.images['tb'].src = '/sys/images/tab_lft3.gif';\n"
		"            t.pageY += 2;\n"
		"            t.pageX += 1;\n"
		"            t.clip.height = 23;\n"
		"            }\n"
		"        }\n"
		"    this.zIndex = this.tabctl.zIndex + 1;\n"
		"    this.tabpage.visibility = 'inherit';\n"
		"    this.document.images['tb'].src = '/sys/images/tab_lft2.gif';\n"
		"    this.pageY -= 2;\n"
		"    this.pageX -= 1;\n"
		"    this.clip.height = 25;\n"
		"    }\n", 0);

	/** Function for the addTab method for adding a new tab page to a tab **/
	htrAddScriptFunction(s, "tc_addtab", "\n"
		"function tc_addtab(l_tab, l_page)\n"
		"    {\n"
		"    if (this.tabs.length > 0)\n"
		"        {\n"
		"        newx = this.tabs[this.tabs.length-1].pageX + this.tabs[this.tabs.length-1].document.width + 1;\n"
		"        if (this.tabs[this.tabs.length-1].tabpage.visibility == 'inherit') newx += 1;\n"
		"        }\n"
		"    else\n"
		"        newx = this.pageX;\n"
		"    newy = this.pageY - 24;\n"
		"    if (l_page.visibility != 'inherit')\n"
		"        {\n"
		"        newx += 1;\n"
		"        newy += 2;\n"
		"        l_tab.clip.height = 23;\n"
		"        }\n"
		"    for(i=0;i<l_tab.document.images.length;i++)\n"
		"        {\n"
		"        l_tab.document.images[i].layer = l_tab;\n"
		"        l_tab.document.images[i].kind = 'tc';\n"
		"        }\n"
		"    this.tabs[this.tabs.length++] = l_tab;\n"
		"    l_tab.tabpage = l_page;\n"
		"    l_tab.tabctl = this;\n"
		"    l_tab.makeCurrent = tc_makecurrent;\n"
		"    l_tab.pageX = newx;\n"
		"    l_tab.pageY = newy;\n"
		"    l_tab.kind = 'tc';\n"
		"    l_tab.document.layer = l_tab;\n"
		"    l_tab.document.Layer = l_tab;\n"
		"    l_page.clip.width = this.clip.width-2;\n"
		"    l_page.clip.height = this.clip.height-24;\n"
		"    return l_tab;\n"
		"    }\n", 0);

	/** Tab control initializer **/
	htrAddScriptFunction(s, "tc_init", "\n"
		"function tc_init(l)\n"
		"    {\n"
		"    l.currentTab = 1;\n"
		"    l.nTabs = 0;\n"
		"    l.isShorting = 0;\n"
		"    l.tabs = new Array();\n"
		"    l.addTab = tc_addtab;\n"
		"    if (tc_tabs == null) tc_tabs = new Array();\n"
		"    tc_tabs[tc_tabs.length++] = l;\n"
		"    return l;\n"
		"    }\n", 0);

	/** Event handler for click-on-tab **/
	htrAddEventHandler(s, "document","MOUSEDOWN","tc",
		"    ly = (e.target.layer == null)?e.target:e.target.layer;\n"
		"    if (ly.kind == 'tc') ly.makeCurrent();\n");

		/*"    for(i=0;i<tc_tabs.length;i++)\n"
		"        {\n"
		"        if (e.pageX > tc_tabs[i].pageX && e.pageX < tc_tabs[i].pageX + tc_tabs[i].document.width &&\n"
		"            e.pageY > tc_tabs[i].pageY - 24 && e.pageY < tc_tabs[i].pageY)\n"
		"            {\n"
		"            for(j=0;j<tc_tabs[i].tabs.length;j++)\n"
		"                {\n"
		"                t = tc_tabs[i].tabs[j];\n"
		"                if (e.pageX > t.pageX && e.pageX < t.pageX + t.document.width && e.pageY > t.pageY && e.pageY < t.pageY + 24)\n"
		"                    {\n"
		"                    ly = (e.target.layer == null)?e.target:e.target.layer;\n"
		"                    if (t != ly) alert(show_obj(ly));\n"
		"                    t.makeCurrent();\n"
		"                    break;\n"
		"                    }\n"
		"                }\n"
		"            }\n"
		"        }\n");*/

	/** Script initialization call. **/
	sprintf(sbuf,"    %s = tc_init(%s.layers.tc%dbase);\n",
		nptr, parentname, id);
	htrAddScriptInit(s, sbuf);

	/** Check for tabpages within the tab control, to do the tabs at the top. **/
	tabcnt = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((tabpage_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(tabpage_obj,"outer_type",POD(&ptr));
		if (!strcmp(ptr,"widget/tabpage"))
		    {
		    objGetAttrValue(tabpage_obj,"name",POD(&ptr));
		    tabcnt++;
		    sprintf(sbuf,"<DIV ID=\"tc%dtab%d\">\n",id,tabcnt);
		    htrAddBodyItem(s,sbuf);
		    sprintf(sbuf,"    <TABLE cellspacing=0 cellpadding=0 border=0 %s>\n", main_bg);
		    htrAddBodyItem(s,sbuf);
		    htrAddBodyItem(s,"        <TR><TD colspan=3 background=/sys/images/white_1x1.png><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
		    if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
		        sprintf(sbuf,"        <TR><TD><IMG SRC=/sys/images/tab_lft2.gif name=tb></TD>\n");
		    else
		        sprintf(sbuf,"        <TR><TD><IMG SRC=/sys/images/tab_lft3.gif name=tb></TD>\n");
		    htrAddBodyItem(s,sbuf);
		    if (objGetAttrType(tabpage_obj,"title") == DATA_T_STRING && objGetAttrValue(tabpage_obj,"title",POD(&ptr)) == 0)
		        {
		        sprintf(sbuf,"            <TD valign=middle><FONT COLOR=%s>%s</FONT></TD>\n", tab_txt, ptr);
			}
		    else
		        {
			objGetAttrValue(tabpage_obj,"name",POD(&ptr));
		        sprintf(sbuf,"            <TD valign=middle><FONT COLOR=%s><B>&nbsp;%s&nbsp;</B></FONT></TD>\n", tab_txt, ptr);
			}
		    htrAddBodyItem(s,sbuf);
		    htrAddBodyItem(s,"            <TD><IMG SRC=/sys/images/dkgrey_1x1.png width=1 height=24></TD></TR>\n    </TABLE>\n");
		    htrAddBodyItem(s, "</DIV>\n");
		    }
		objClose(tabpage_obj);
		}
	    objQueryClose(qy);
	    }

	/** HTML body <DIV> element for the base layer. **/
	sprintf(sbuf,"<DIV ID=\"tc%dbase\">\n",id);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "        <TR><TD><IMG SRC=/sys/images/white_1x1.png></TD>\n");
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/white_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "            <TD><IMG SRC=/sys/images/white_1x1.png></TD></TR>\n");
	sprintf(sbuf, "        <TR><TD><IMG SRC=/sys/images/white_1x1.png height=%d width=1></TD>\n",h-2);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "            <TD>&nbsp;</TD>\n");
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=%d width=1></TD></TR>\n",h-2);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "        <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD>\n");
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width=%d></TD>\n",w-2);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "            <TD><IMG SRC=/sys/images/dkgrey_1x1.png></TD></TR>\n    </TABLE>\n\n");

	/** Check for tabpages within the tab control entity, this time to do the pages themselves **/
	tabcnt = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((tabpage_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(tabpage_obj,"outer_type",POD(&ptr));
		if (!strcmp(ptr,"widget/tabpage"))
		    {
		    /** First, render the tabpage and add stuff for it **/
		    objGetAttrValue(tabpage_obj,"name",POD(&ptr));
		    tabcnt++;

		    /** Add stylesheet headers for the layers (tab and tabpage) **/
		    sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
		    htrAddHeaderItem(s,sbuf);
		    if ((!*sel && tabcnt == 1) || !strcmp(sel,ptr))
		        {
		        sprintf(sbuf,"\t#tc%dtab%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,x,y,1,z+2);
		        htrAddHeaderItem(s,sbuf);
		        sprintf(sbuf,"\t#tc%dpane%d { POSITION:absolute; VISIBILITY:inherit; LEFT:1; TOP:1; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,w-2,z+2);
		        htrAddHeaderItem(s,sbuf);
			}
		    else
		        {
		        sprintf(sbuf,"\t#tc%dtab%d { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,x,y,1,z);
		        htrAddHeaderItem(s,sbuf);
		        sprintf(sbuf,"\t#tc%dpane%d { POSITION:absolute; VISIBILITY:hidden; LEFT:1; TOP:1; WIDTH:%d; Z-INDEX:%d; }\n",
			    id,tabcnt,w-2,z+2);
		        htrAddHeaderItem(s,sbuf);
			}
		    sprintf(sbuf,"    </STYLE>\n");
		    htrAddHeaderItem(s,sbuf);

		    /** Add script initialization to add a new tabpage **/
		    sprintf(sbuf,"    %s = %s.addTab(%s.layers.tc%dtab%d,%s.document.layers.tc%dpane%d);\n",
			ptr, nptr, parentname, id, tabcnt, nptr, id, tabcnt);
		    htrAddScriptInit(s, sbuf);

		    /** Add named global for the tabpage **/
		    subnptr = (char*)nmMalloc(strlen(ptr)+1);
		    strcpy(subnptr,ptr);
		    htrAddScriptGlobal(s, subnptr, "null", HTR_F_NAMEALLOC);

		    /** Add DIV section for the tabpage. **/
		    sprintf(sbuf,"<DIV ID=\"tc%dpane%d\">\n",id,tabcnt);
		    htrAddBodyItem(s, sbuf);

		    /** Now look for sub-items within the tabpage. **/
		    sprintf(sbuf,"%s.tabpage.document",subnptr);
		    sprintf(name,"%.50s.tabpage",subnptr);
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
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Tab Control / Tab Page Driver");
	strcpy(drv->WidgetName,"tab");
	drv->Render = httabRender;
	drv->Verify = httabVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

#if 00
	/** Add the 'load page' action **/
	action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	strcpy(action->Name,"LoadPage");
	xaInit(&action->Parameters,16);
	param = (pHtParam)nmSysMalloc(sizeof(HtParam));
	strcpy(param->ParamName,"Source");
	param->DataType = DATA_T_STRING;
	xaAddItem(&action->Parameters,(void*)param);
	xaAddItem(&drv->Actions,(void*)action);
#endif

	/** Register. **/
	htrRegisterDriver(drv);

	HTTAB.idcnt = 0;

    return 0;
    }
