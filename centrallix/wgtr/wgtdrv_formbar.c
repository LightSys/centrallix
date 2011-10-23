#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "wgtr.h"
#include "expression.h"

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
/* Module: 	wgtr/wgtdrv_formbar.c						*/
/* Author:	Matt McGill (MJM)		 			*/
/* Creation:	June 30, 2004						*/
/* Description:								*/
/************************************************************************/

/** globals **/

static struct {
   int	    fb_cnt;
   int	    pane_cnt;
   int	    btn_cnt;
   int	    fs_cnt;
} WGTFB;


int
wgtfb_internal_AddStringProp(pWgtrNode node, char* name, char* str)
    {
    ObjData val;
	
	val.String = str;
	return wgtrAddProperty(node, name, DATA_T_STRING, &val, 0);
    }


/*** wgtfbVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int wgtfbVerify(pWgtrVerifySession s) 
    {
    pWgtrNode	this;
    pWgtrNode   subtree;
    pWgtrNode   newnode, newcn;
    ObjData	val;
    char	name[32], form_name[64], exp_txt[256];
    int x, y, width, height;
    pObjSession os;

	/** Find our node **/
	this = s->CurrWidget;
	os = this->ObjSession;

	/** Grab our properties **/
       if (wgtrGetPropertyValue(this,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
       if (wgtrGetPropertyValue(this,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
       if (wgtrGetPropertyValue(this,"target",DATA_T_STRING, &val) != 0)
	    {
	    mssError(1, "WGTFB", "Form bar '%s' must have a 'target' property", this->Name);
	    return -1;
	    }
	strtcpy(form_name, val.String, sizeof(form_name));

       /*
       if (wgtrGetPropertyValue(this,"width",DATA_T_INTEGER,POD(&x)) != 0) x=0;
       if (wgtrGetPropertyValue(this,"height",DATA_T_INTEGER,POD(&y)) != 0) y=0;
       */
       width = 240;
       height = 30;

	/** First create the pane - that's the parent **/
	snprintf(name, 32, "fb%dpane%d", WGTFB.fb_cnt, WGTFB.pane_cnt++);
	if ( (subtree = wgtrNewNode(name, "widget/pane", os, x, y, width, height, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create pane for form bar '%s'", this->Name);
	    return -1;
	    }
	val.String = "raised";
	wgtrAddProperty(subtree, "style", DATA_T_STRING, &val, 0);
	val.String = "/sys/images/grey_gradient3.png";
	wgtrAddProperty(subtree, "background", DATA_T_STRING, &val, 0);
	wgtrSetupNode(subtree);
	wgtrAddChild(this, subtree);
	wgtrScheduleVerify(s, subtree);
	/** Next the formstatus widget **/
	snprintf(name, 32, "fb%dformstatus%d", WGTFB.fb_cnt, WGTFB.fs_cnt++);
	if ( (newnode = wgtrNewNode(name, "widget/formstatus", os, 72, 4, -1, -1, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create formstatus for form bar '%s'", this->Name);
	    return -1;
	    }
	val.String = "largeflat";
	wgtrAddProperty(newnode, "style", DATA_T_STRING, &val, 0);
	wgtrSetupNode(newnode);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s, newnode);

	/** Now the 'first' button **/
	snprintf(name, 32, "fb%dfirst", WGTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", os, 8, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16aa.gif");
	wgtfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16ab.gif");
	wgtfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16ac.gif");
	wgtfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16ad.gif");

	snprintf(exp_txt, 256, "runclient(:%s:recid > 1)", form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val, 0);
	    expFreeExpression(val.Generic);
	    }

	wgtrSetupNode(newnode);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s, newnode);

	/** and the connector for the 'first' button **/
	snprintf(name, 32, "fb%dfirstcn", WGTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", os, 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newcn, "event", "Click");
	wgtfb_internal_AddStringProp(newcn, "target", form_name);
	wgtfb_internal_AddStringProp(newcn, "action", "First");
	wgtrSetupNode(newcn);
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s, newcn);

	/** And the 'prev' button **/
	snprintf(name, 32, "fb%dprev", WGTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", os, 28, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16ba.gif");
	wgtfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16bb.gif");
	wgtfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16bc.gif");
	wgtfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16bd.gif");

	snprintf(exp_txt, 256, "runclient(:%s:recid > 1)", form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val, 0);
	    expFreeExpression(val.Generic);
	    }

	wgtrSetupNode(newnode);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s, newnode);

	/** and the connector for the 'prev' button **/
	snprintf(name, 32, "fb%dprevcn", WGTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", os, 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newcn, "event", "Click");
	wgtfb_internal_AddStringProp(newcn, "target", form_name);
	wgtfb_internal_AddStringProp(newcn, "action", "Prev");
	wgtrSetupNode(newcn);
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s, newcn);
	
	/** And the 'next' button **/
	snprintf(name, 32, "fb%dnext", WGTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", os, 190, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16ca.gif");
	wgtfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16cb.gif");
	wgtfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16cc.gif");
	wgtfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16cd.gif");

	snprintf(exp_txt, 256, "runclient(not (:%s:recid == :%s:lastrecid))", form_name, form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val, 0);
	    expFreeExpression(val.Generic);
	    }

	wgtrSetupNode(newnode);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s, newnode);

	/** and the connector for the 'next' button **/
	snprintf(name, 32, "fb%dnextcn", WGTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", os, 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newcn, "event", "Click");
	wgtfb_internal_AddStringProp(newcn, "target", form_name);
	wgtfb_internal_AddStringProp(newcn, "action", "Next");
	wgtrSetupNode(newcn);
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s, newcn);
	
	/** And the 'last' button **/
	snprintf(name, 32, "fb%dlast", WGTFB.fb_cnt);
	if ( (newnode = wgtrNewNode(name, "widget/imagebutton", os, 210, 5, 18, 18, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create imagebutton for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newnode, "image", "/sys/images/ico16da.gif");
	wgtfb_internal_AddStringProp(newnode, "pointimage", "/sys/images/ico16db.gif");
	wgtfb_internal_AddStringProp(newnode, "clickimage", "/sys/images/ico16dc.gif");
	wgtfb_internal_AddStringProp(newnode, "disabledimage", "/sys/images/ico16dd.gif");

	snprintf(exp_txt, 256, "runclient(not (:%s:recid == :%s:lastrecid))", form_name, form_name);
	if ( (val.Generic = expCompileExpression(exp_txt, NULL, 0, 0)) != NULL)
	    {
	    wgtrAddProperty(newnode, "enabled", DATA_T_CODE, &val, 0);
	    expFreeExpression(val.Generic);
	    }

	wgtrSetupNode(newnode);
	wgtrAddChild(subtree, newnode);
	wgtrScheduleVerify(s, newnode);

	/** and the connector for the 'last' button **/
	snprintf(name, 32, "fb%dlastcn", WGTFB.fb_cnt);
	if ( (newcn = wgtrNewNode(name, "widget/connector", os, 0, 0, 0, 0, 0, 0, 0, 0)) == NULL)
	    {
	    mssError(0, "WGTFB", "Couldn't create connector for form bar '%s'", this->Name);
	    return -1;
	    }
	wgtfb_internal_AddStringProp(newcn, "event", "Click");
	wgtfb_internal_AddStringProp(newcn, "target", form_name);
	wgtfb_internal_AddStringProp(newcn, "action", "Last");
	wgtrSetupNode(newcn);
	wgtrAddChild(newnode, newcn);
	wgtrScheduleVerify(s, newcn);
	

	WGTFB.fb_cnt++;

	return 0;
    }



/*** wgtfbNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtfbNew(pWgtrNode node)
    {   
	if(node->fl_width < 0) node->fl_width = 0;
	if(node->fl_height < 0) node->fl_height = 0;
	node->width = node->r_width = 240;
	node->height = node->r_height = 30;
	
    return 0;
    }


int
wgtfbInitialize()
    {
    char* name="Form Bar Driver";

	wgtrRegisterDriver(name, wgtfbVerify, wgtfbNew);
	wgtrAddType(name, "formbar");

	WGTFB.fb_cnt = 0;
	WGTFB.pane_cnt = 0;
	WGTFB.btn_cnt = 0;
	WGTFB.fs_cnt = 0;

	return 0;
    }
