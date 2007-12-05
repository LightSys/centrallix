#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"
#include "cxlib/qprintf.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_rule.c 						*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 9, 2007   					*/
/* Description:	A rule widget adds some declarative control information	*/
/*		to another widget.  Place rule widgets as subwidgets in	*/
/*		the widget to be affected.  Types and properties of	*/
/*		rules are specific to the affected widget.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_rule.c,v 1.1 2007/12/05 18:56:18 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_rule.c,v $

    $Log: htdrv_rule.c,v $
    Revision 1.1  2007/12/05 18:56:18  gbeeley
    - (feature) adding declarative "widget/rule" widget, which has multiple
      purposes in defining rule-based behavior for different kinds of widgets.
      First use is replacing "osrc-rule" on objectsource widgets and in
      providing osrc relationships to be declared rather than handled via
      connectors.


 **END-CVSDATA***********************************************************/


/*** Rule definition - for other drivers registering rule types
 *** with this module
 ***/
typedef struct _RD
    {
    struct _RD*	Next;
    char	Ruletype[32];
    int		nParams;
    char*	ParamNames[32];
    int		ParamTypes[32];
    }
    HtRuleDefinition, *pHtRuleDefinition;


/** globals **/
static struct 
    {
    int			idcnt;
    pHtRuleDefinition	RuletypeList;
    }
    HTRULE;


/*** htruleRender - generate the HTML code for the page.
 ***/
int
htruleRender(pHtSession s, pWgtrNode tree, int z)
    {
    int id;
    char* nptr;
    char* ptr;
    pXString xs = NULL;
    int rval = -1;
    char* attrname;
    char* ruletype;
    int first;
    int t;
    ObjData od;
    pExpression code;
    pHtRuleDefinition def;
    int i, found;

    	/** Get an id for this. **/
	id = (HTRULE.idcnt++);

	xs = (pXString)nmMalloc(sizeof(XString));
	if (!xs) goto error;
	xsInit(xs);

	wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&nptr));

	/** Build the list of rule properties **/
	if (wgtrGetPropertyValue(tree, "ruletype", DATA_T_STRING, POD(&ruletype)) != 0)
	    {
	    mssError(1,"HTRULE","'ruletype' property required for widget/rule");
	    goto error;
	    }
	for(def = HTRULE.RuletypeList;def;def=def->Next)
	    {
	    if (!strcmp(ruletype, def->Ruletype))
		break;
	    }
	if (!def)
	    {
	    mssError(1,"HTRULE","widget/rule '%s': '%s' is not a valid rule type", nptr, ruletype);
	    goto error;
	    }

	xsCopy(xs, "{", 1);
	first = 1;
	for(attrname=wgtrFirstPropertyName(tree);attrname;attrname=wgtrNextPropertyName(tree))
	    {
	    /** no need to include this one **/
	    if (!strcmp(attrname,"ruletype")) continue;

	    /** make sure it is a valid attribute **/
	    found = -1;
	    for(i=0;i<def->nParams;i++)
		{
		if (!strcmp(attrname, def->ParamNames[i]))
		    {
		    found = i;
		    break;
		    }
		}

	    /** simply ignore any invalid attrs **/
	    if (found < 0) continue;

	    /** add it **/
	    if (xsConcatQPrintf(xs, "%[, %]%STR&SYM:", !first, attrname) > 0)
		{
		t = wgtrGetPropertyType(tree, attrname);
		if (t <= 0 || (t != def->ParamTypes[found] && def->ParamTypes[found] != HT_DATA_T_BOOLEAN))
		    xsConcatenate(xs, "null", 4);
		else
		    {
		    if (def->ParamTypes[found] == HT_DATA_T_BOOLEAN)
			xsConcatQPrintf(xs, "%INT", htrGetBoolean(tree, attrname, -1));
		    else if (wgtrGetPropertyValue(tree, attrname, t, &od) != 0)
			xsConcatenate(xs, "null", 4);
		    else
			{
			if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE)
			    ptr = objDataToStringTmp(t, (void*)(&od), DATA_F_QUOTED);
			else if (t == DATA_T_CODE)
			    {
			    wgtrGetPropertyValue(tree,attrname,DATA_T_CODE,POD(&code));
			    htrAddExpression(s, nptr, attrname, code);
			    ptr = "null";
			    }
			else
			    ptr = objDataToStringTmp(t, (void*)(od.Generic), DATA_F_QUOTED);
			xsConcatenate(xs, ptr, -1);
			}
		    }
		first = 0;
		}
	    }
	xsConcatenate(xs, "}", 1);

	/** Script include to add functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_rule.js", 0);

	/** Use parent container for subobjects... **/
	htrAddWgtrCtrLinkage(s, tree, "_parentctr");

	/** Script Init **/
	htrAddScriptInit_va(s, "    rl_init(nodes[\"%STR&SYM\"], \"%STR&ESCQ\", %STR);\n", nptr, ruletype, xs->String);
	htrAddScriptInit_va(s, "    wgtrGetParent(nodes[\"%STR&SYM\"]).addRule(nodes[\"%STR&SYM\"]);\n", nptr, nptr);

	/** mark this node as not being associated with a DHTML object **/
	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	rval = 0;

    error:
	if (xs)
	    {
	    xsDeInit(xs);
	    nmFree(xs, sizeof(XString));
	    }

    return rval;
    }


/*** htruleRegister() - register a new type of rule with the rule module
 ***/
int
htruleRegister(char* ruletype, ...)
    {
    pHtRuleDefinition ruledef;
    va_list va;
    char* attrname;
    int attrtype;

	/** Alloc and set up the definition **/
	ruledef = (pHtRuleDefinition)nmMalloc(sizeof(HtRuleDefinition));
	if (!ruledef) return -1;
	strtcpy(ruledef->Ruletype, ruletype, sizeof(ruledef->Ruletype));
	ruledef->nParams = 0;

	/** Get the possible attributes for the rule **/
	va_start(va, ruletype);
	while(va && (attrname = va_arg(va, char*)) != NULL)
	    {
	    if (va)
		{
		attrtype = va_arg(va, int);
		if (ruledef->nParams < sizeof(ruledef->ParamNames)/sizeof(char*))
		    {
		    ruledef->ParamNames[ruledef->nParams] = nmSysStrdup(attrname);
		    ruledef->ParamTypes[ruledef->nParams] = attrtype;
		    ruledef->nParams++;
		    }
		}
	    }
	va_end(va);

	/** Add it to the list **/
	ruledef->Next = HTRULE.RuletypeList;
	HTRULE.RuletypeList = ruledef;

    return 0;
    }


/*** htruleInitialize - register with the ht_render module.
 ***/
int
htruleInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"Declarative Rule Widget Driver");
	strcpy(drv->WidgetName,"rule");
	drv->Render = htruleRender;

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTRULE.idcnt = 0;
	HTRULE.RuletypeList = NULL;

    return 0;
    }
