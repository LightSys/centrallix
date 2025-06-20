#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"
#include "wgtr.h"

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
/* Module: 	htdrv_connector.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 24, 1998 					*/
/* Description:	HTML Widget driver for a 'connector', which joins an 	*/
/*		event on the parent widget with an action on another	*/
/*		specified widget.					*/
/************************************************************************/


/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCONN;


/*** htconnRender - generate the HTML code for the page.
 ***/
int
htconnRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    char* vstr;
    int vint;
    double vdbl;
    char name[64];
    char event[32] = "";
    char target[128];
    char source[128];
    char action[32] = "";
    int id, i;
    XString xs;
    pExpression code;
    int first;
    int inside_action = 0;
    /*char rpt_context[128];*/

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML )
	    {
	    mssError(1,"HTCONN","Netscape DOM or W3C DOM1HTML support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCONN.idcnt++);

	/** Inside a component-decl-action? **/
	if (tree->Parent && wgtrGetPropertyValue(tree->Parent, "outer_type", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "widget/component-decl-action"))
	    {
	    inside_action = 1;
	    wgtrGetPropertyValue(tree->Parent, "name", DATA_T_STRING, POD(&ptr));
	    strtcpy(event, ptr, sizeof(event));
	    }

	/** Get the event and action linkage information **/
	if (wgtrGetPropertyValue(tree,"event",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    if (inside_action)
		{
		mssError(1,"HTCONN","Connector inside componentdecl-action cannot have an 'event' property");
		return -1;
		}
	    strtcpy(event,ptr,sizeof(event));
	    }
	if (!*event)
	    {
	    mssError(1,"HTCONN","Connector must have an 'event' property");
	    return -1;
	    }

	if (wgtrGetPropertyValue(tree,"action",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    strtcpy(action,ptr,sizeof(action));
	    }
	if (!*action)
	    {
	    mssError(1,"HTCONN","Connector must have an 'action' property");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Are we inside a widget/repeat context? **/
	/*rpt_context[0] = '\0';
	if (!strncmp(name, WGTR_REPEAT_PREFIX, strlen(WGTR_REPEAT_PREFIX)))
	    {
	    ptr = strchr(name+strlen(WGTR_REPEAT_PREFIX),'_');
	    if (ptr)
		{
		strtcpy(rpt_context, name, sizeof(rpt_context));
		rpt_context[ptr - name] = '\0';
		}
	    }*/

	/** Source/target **/
	if (wgtrGetPropertyValue(tree,"target",DATA_T_STRING,POD(&ptr)) != 0)
	    target[0] = '\0';
	else
	    strtcpy(target,ptr,sizeof(target));

	if (wgtrGetPropertyValue(tree,"source",DATA_T_STRING,POD(&ptr)) != 0)
	    source[0] = '\0';
	else
	    {
	    if (inside_action)
		{
		mssError(1,"HTCONN","Connector inside componentdecl-action cannot have a 'source' property");
		return -1;
		}
	    strtcpy(source, ptr, sizeof(source));
	    }

	/** Build the param list **/
	xsInit(&xs);
	first = 1;
	for(ptr = wgtrFirstPropertyName(tree); ptr; ptr = wgtrNextPropertyName(tree))
	    {
	    if (!strcmp(ptr, "event") || !strcmp(ptr, "target") || !strcmp(ptr, "action") || !strcmp(ptr, "source") || !strcmp(ptr, "condition")) continue;
	    if (!first) xsConcatenate(&xs, ",", 1);
	    first = 0;
	    switch(wgtrGetPropertyType(tree, ptr))
	        {
		case DATA_T_CODE:
		    wgtrGetPropertyValue(tree, ptr, DATA_T_CODE, POD(&code));
		    xsConcatQPrintf(&xs,"%STR&SYM:{type:'exp', value:function(_context,_this,ep) { with(ep) { return ", ptr);
		    expGenerateText(code, NULL, xsWrite, &xs, '\0', "javascript", EXPR_F_RUNCLIENT);
		    xsConcatenate(&xs,"; } } }",7);
		    break;
		case DATA_T_INTEGER:
	    	    wgtrGetPropertyValue(tree, ptr, DATA_T_INTEGER,POD(&vint));
		    xsConcatQPrintf(&xs,"%STR&SYM:{type:'int', value:%INT}",ptr,vint);
		    break;
		case DATA_T_DOUBLE:
		    wgtrGetPropertyValue(tree, ptr, DATA_T_DOUBLE,POD(&vdbl));
		    xsConcatQPrintf(&xs,"%STR&SYM:{type:'dbl', value:%DBL}", ptr, vdbl);
		    break;
		case DATA_T_STRING:
	    	    wgtrGetPropertyValue(tree, ptr, DATA_T_STRING,POD(&vstr));
		    if (!strpbrk(vstr," !@#$%^&*()-=+`~;:,.<>/?'\"[]{}\\|"))
		        {
			xsConcatQPrintf(&xs, "%STR&SYM:{type:'sym', value:'%STR&SYM', namespace:ns}", ptr, vstr);
			}
		    else
		        {
			xsConcatQPrintf(&xs, "%STR&SYM:{type:'str', value:'%STR&JSSTR'}", ptr, vstr);
			}
		    break;
		}
	    }

	/** Add a script init to install the connector **/
#if 00
	if (*rpt_context && *source)
	    {
	    /** Try repeat-specific node first, then normally named node **/
	    htrAddScriptInit_va(s, "    var src=nodes[\"%STR&SYM_%STR&SYM\"];\n",
		    rpt_context,
		    source);
	    htrAddScriptInit_va(s, "    if(!src) src=nodes[\"%STR&SYM\"];\n",
		    source);
	    }
	else
	    {
	    htrAddScriptInit_va(s, "    var src=%[wgtrGetParent(nodes[\"%STR&SYM\"])%]%[nodes[\"%STR&SYM\"]%];\n",
#endif
	    /*htrAddScriptInit_va(s, "    var src=%[wgtrGetParent(wgtrGetNodeRef(ns,\"%STR&SYM\"))%]%[wgtrGetNodeRef(ns, \"%STR&SYM\")%];\n",
		    !*source, name, 
		    *source, source);*/
#if 00
	    }
	if (*rpt_context && *target)
	    {
	    htrAddScriptInit_va(s, "    var tgt=\"%STR&SYM_%STR&SYM\";\n",
		    rpt_context,
		    target);
	    htrAddScriptInit_va(s, "    if(!nodes[tgt]) tgt='%STR&SYM';\n",
		    target);
	    }
	else
	    {
#endif
	    /*htrAddScriptInit_va(s, "    var tgt=%['%STR&SYM'%]%[wgtrGetName(wgtrGetParent(wgtrGetNodeRef(ns,\"%STR&SYM\")))%];\n",
		    *target, target, 
		    !*target, name);*/
#if 00
	    }
#endif
	//htrAddScriptInit_va(s, "    src.ifcProbe(ifEvent).Connect('%STR&SYM', tgt, '%STR&SYM', {%STR});\n",
	htrAddScriptInit_va(s, "    %[wgtrGetParent(wgtrGetParent(wgtrGetNodeRef(ns,\"%STR&SYM\")))%]%[wgtrGetParent(wgtrGetNodeRef(ns,\"%STR&SYM\"))%]%[wgtrGetNodeRef(ns, \"%STR&SYM\")%].ifcProbe(ifEvent).Connect('%STR&SYM', %['%STR&SYM'%]%[wgtrGetName(wgtrGetParent(wgtrGetNodeRef(ns,\"%STR&SYM\")))%], '%STR&SYM', {%STR});\n",
		inside_action, name,
		!*source && !inside_action, name,
		*source && !inside_action, source,
		event, 
		*target, target, !*target, name,
		action,
		xs.String);
	xsDeInit(&xs);

	htrAddScriptInclude(s, "/sys/js/htdrv_connector.js", 0);

	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	/** Check for more sub-widgets within the conn entity. **/
	for (i=0;i<xaCount(&(tree->Children));i++)
	    htrRenderWidget(s, xaGetItem(&(tree->Children), i), z+1);

    return 0;
    }


/*** htconnInitialize - register with the ht_render module.
 ***/
int
htconnInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Event-Action Connector Driver");
	strcpy(drv->WidgetName,"connector");
	drv->Render = htconnRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTCONN.idcnt = 0;

    return 0;
    }
