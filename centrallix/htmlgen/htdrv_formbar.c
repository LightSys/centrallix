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
/* Module:      htdrv_formstatus.c                                      */
/* Author:      Matt McGill (MJM)                                       */
/* Creation:    July 19, 2004                                           */
/* Description: HTML Widget driver for a form bar, basically a tech     */
/*		    test for widgets adding subwidgets on the fly.      */
/************************************************************************/

/**CVSDATA***************************************************************


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int	    fb_cnt;
   int	    pane_cnt;
   int	    btn_cnt;
   int	    fs_cnt;
} HTFB;


int
htfb_internal_AddStringProp(pWgtrNode node, char* name, char* str)
    {
    ObjData val;
	
	val.String = str;
	return wgtrAddProperty(node, name, DATA_T_STRING, &val);
    }

/* 
   htfbVerify - not written yet.
*/
int htfbVerify(pHtSession s) 
    {
    pWgtrNode	this;
    pWgtrNode   subtree;
    pWgtrNode   newnode, newcn;
    ObjData	val;
    char	name[32], form_name[64], exp_txt[256];
    int x, y, width, height;

	/** Find our node **/
	this = s->VerifySession->CurrWidget;

	/** Grab our properties **/
       if (wgtrGetPropertyValue(this,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
       if (wgtrGetPropertyValue(this,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
       if (wgtrGetPropertyValue(this,"target",DATA_T_STRING, &val) != 0)
	    {
	    mssError(1, "HTFB", "Form bar '%s' must have a 'target' property", this->Name);
	    return -1;
	    }
	strncpy(form_name, val.String, 64);

       /*
       if (wgtrGetPropertyValue(this,"width",DATA_T_INTEGER,POD(&x)) != 0) x=0;
       if (wgtrGetPropertyValue(this,"height",DATA_T_INTEGER,POD(&y)) != 0) y=0;
       */
       width = 240;
       height = 30;

	/** First create the pane - that's the parent **/
	snprintf(name, 32, "fb%dpane%d", HTFB.fb_cnt, HTFB.pane_cnt++);
	if ( (subtree = wgtrNewNode(name, "widget/pane", x, y, width, height, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create pane for form bar '%s'", this->Name);
	    return -1;
	    }
	val.String = "raised";
	wgtrAddProperty(subtree, "style", DATA_T_STRING, &val);
	val.String = "/sys/images/grey_gradient3.png";
	wgtrAddProperty(subtree, "background", DATA_T_STRING, &val);
	wgtrAddChild(this, subtree);
	wgtrScheduleVerify(s->VerifySession, subtree);
/** Next the formstatus widget **/
	snprintf(name, 32, "fb%dformstatus%d", HTFB.fb_cnt, HTFB.fs_cnt++);
	if ( (newnode = wgtrNewNode(name, "widget/formstatus", 72, 4, -1, -1, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create formstatus for form bar '%s'", this->Name);
	    return -1;
	    }
	val.String = "largeflat";
	wgtrAddProperty(newnode, "style", DATA_T_STRING, &val);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s->VerifySession, newnode);

	/** Now the 'first' button **/
	snprintf(name, 32, "fb%dfirst", HTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", 8, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16aa.gif");
	htfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16ab.gif");
	htfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16ac.gif");
	htfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16ad.gif");

	snprintf(exp_txt, 256, "runclient(:%s:recid > 1)", form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val);
	    expFreeExpression(val.Generic);
	    }

	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s->VerifySession, newnode);

	/** and the connector for the 'first' button **/
	snprintf(name, 32, "fb%dfirstcn", HTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newcn, "event", "Click");
	htfb_internal_AddStringProp(newcn, "target", form_name);
	htfb_internal_AddStringProp(newcn, "action", "First");
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s->VerifySession, newcn);

	/** And the 'prev' button **/
	snprintf(name, 32, "fb%dprev", HTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", 28, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16ba.gif");
	htfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16bb.gif");
	htfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16bc.gif");
	htfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16bd.gif");

	snprintf(exp_txt, 256, "runclient(:%s:recid > 1)", form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val);
	    expFreeExpression(val.Generic);
	    }

	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s->VerifySession, newnode);

	/** and the connector for the 'prev' button **/
	snprintf(name, 32, "fb%dprevcn", HTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newcn, "event", "Click");
	htfb_internal_AddStringProp(newcn, "target", form_name);
	htfb_internal_AddStringProp(newcn, "action", "Prev");
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s->VerifySession, newcn);
	
	/** And the 'next' button **/
	snprintf(name, 32, "fb%dnext", HTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", 190, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16ca.gif");
	htfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16cb.gif");
	htfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16cc.gif");
	htfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16cd.gif");

	snprintf(exp_txt, 256, "runclient(not (:%s:recid == :%s:lastrecid))", form_name, form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val);
	    expFreeExpression(val.Generic);
	    }

	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s->VerifySession, newnode);

	/** and the connector for the 'next' button **/
	snprintf(name, 32, "fb%dnextcn", HTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newcn, "event", "Click");
	htfb_internal_AddStringProp(newcn, "target", form_name);
	htfb_internal_AddStringProp(newcn, "action", "Next");
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s->VerifySession, newcn);
	
	/** And the 'last' button **/
	snprintf(name, 32, "fb%dlast", HTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", 210, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16da.gif");
	htfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16db.gif");
	htfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16dc.gif");
	htfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16dd.gif");

	snprintf(exp_txt, 256, "runclient(not (:%s:recid == :%s:lastrecid))", form_name, form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val);
	    expFreeExpression(val.Generic);
	    }

	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s->VerifySession, newnode);

	/** and the connector for the 'last' button **/
	snprintf(name, 32, "fb%dlastcn", HTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "HTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	htfb_internal_AddStringProp(newcn, "event", "Click");
	htfb_internal_AddStringProp(newcn, "target", form_name);
	htfb_internal_AddStringProp(newcn, "action", "Last");
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s->VerifySession, newcn);
	

	HTFB.fb_cnt++;

	return 0;
    }


/* 
   htfbRender - generate the HTML code for the page.
*/
int htfbRender(pHtSession s, pWgtrNode tree, int z, char* parentname, char* parentobj) {
   int x=-1,y=-1;
   int id;
   char name[64];
   char sbuf[160], sbuf2[160];
   char* ptr;
   char* style;
   int i;

   if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
       {
       mssError(1,"HTFS","Netscape DOM or W3C DOM1 HTML and CSS1 support required");
       return -1;
       }

    for (i=0;i<xaCount(&(tree->Children));i++)
	htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+2, parentname, parentobj);
   return 0;
}


/* 
   htfsInitialize - register with the ht_render module.
*/
int htfbInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = htrAllocDriver();
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Form Bar Driver");
   strcpy(drv->WidgetName,"formbar");
   drv->Render = htfbRender;
   drv->Verify = htfbVerify;

/*
   htrAddEvent(drv,"Click");
   htrAddEvent(drv,"MouseUp");
   htrAddEvent(drv,"MouseDown");
   htrAddEvent(drv,"MouseOver");
   htrAddEvent(drv,"MouseOut");
   htrAddEvent(drv,"MouseMove");
*/
   /** Register. **/
   htrRegisterDriver(drv);

   htrAddSupport(drv, "dhtml");

    memset(&HTFB, 0, sizeof(HTFB));

   return 0;
}
