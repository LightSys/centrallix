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

    $Id: htdrv_pane.c,v 1.1 2001/08/13 18:00:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_pane.c,v $

    $Log: htdrv_pane.c,v $
    Revision 1.1  2001/08/13 18:00:50  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTPN;


/*** htpnVerify - not written yet.
 ***/
int
htpnVerify()
    {
    return 0;
    }


/*** htpnRender - generate the HTML code for the page.
 ***/
int
htpnRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char sbuf2[160];
    char main_bg[128];
    pObject sub_w_obj;
    pObjQuery qy;
    int x=-1,y=-1,w,h;
    int id;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;

    	/** Get an id for this. **/
	id = (HTPN.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTPN","Pane widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTPN","Pane widget must have a 'height' property");
	    return -1;
	    }

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Style of pane - raised/lowered **/
	if (objGetAttrValue(w_obj,"style",POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
	if (is_raised)
	    {
	    c1 = "white_1x1.png";
	    c2 = "dkgrey_1x1.png";
	    }
	else
	    {
	    c1 = "dkgrey_1x1.png";
	    c2 = "white_1x1.png";
	    }

	/** Ok, write the style header items. **/
	sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#pn%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#pn%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Pane initializer **/
	htrAddScriptFunction(s, "pn_init", "\n"
		"function pn_init(l,ml)\n"
		"    {\n"
		"    l.mainlayer = ml;\n"
		"    l.document.Layer = l;\n"
		"    return l;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	sprintf(sbuf,"    %s = pn_init(%s.layers.pn%dbase, %s.layers.pn%dbase.document.layers.pn%dmain);\n",
		nptr, parentname, id, parentname, id, id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> element for the base layer. **/
	sprintf(sbuf,"<DIV ID=\"pn%dbase\">\n",id);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"pn%dmain\">\n",id);
	htrAddBodyItem(s, sbuf);

	/** Check for objects within the pane. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    sprintf(sbuf,"%s.mainlayer.document",nptr);
	    sprintf(sbuf2,"%s.mainlayer",nptr);
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
	        htrRenderWidget(s, sub_w_obj, z+2, sbuf, sbuf2);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV></DIV>\n");

    return 0;
    }


/*** htpnInitialize - register with the ht_render module.
 ***/
int
htpnInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Pane Driver");
	strcpy(drv->WidgetName,"pane");
	drv->Render = htpnRender;
	drv->Verify = htpnVerify;
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

	HTPN.idcnt = 0;

    return 0;
    }
