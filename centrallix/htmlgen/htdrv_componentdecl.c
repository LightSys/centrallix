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
#include "hints.h"
#include "cxsec.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_componentdecl.c             			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 11, 2003					*/
/* Description:	HTML Widget driver for component widget definition,	*/
/*		which is used to declare a component, its structure,	*/
/*		and its interface/params.  Use the normal component	*/
/*		widget (htdrv_component.c) to insert a component into	*/
/*		an application.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_componentdecl.c,v 1.1 2004/02/24 19:59:30 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_componentdecl.c,v $

    $Log: htdrv_componentdecl.c,v $
    Revision 1.1  2004/02/24 19:59:30  gbeeley
    - adding component-declaration widget driver
    - adding image widget driver
    - adding app-level presentation hints pseudo-widget driver


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTCMPD;


/** structure defining a param to the component **/
typedef struct
    {
    pObjPresentationHints	Hints;
    TObjData			TypedObjData;
    char*			StrVal;
    char*			Name;
    }
    HTCmpdParam, *pHTCmpdParam;


/*** htcmpdVerify - not written yet.
 ***/
int
htcmpdVerify()
    {
    return 0;
    }


/*** htcmpdRender - generate the HTML code for the component.
 ***/
int
htcmpdRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char subobj_name[64];
    int id;
    char* nptr;
    pObject subobj = NULL;
    pObjQuery subobj_qy = NULL;
    XArray attrs;
    pHTCmpdParam param;
    int i,t;
    int rval = 0;
    int is_visual = 1;

	/** Verify capabilities **/
	if(!s->Capabilities.Dom0NS && !(s->Capabilities.Dom1HTML && s->Capabilities.CSS1))
	    {
	    mssError(1,"HTCMPD","Either Netscape DOM or W3C DOM1 HTML and W3C CSS support required");
	    return -1;
	    }

    	/** Get an id for this. **/
	id = (HTCMPD.idcnt++);

	/** Is this a visual component? **/
	if ((is_visual = htrGetBoolean(w_obj, "visual", 1)) < 0)
	    {
	    return -1;
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;
	if (cxsecVerifySymbol(name) < 0)
	    {
	    mssError(1,"HTCMPD","Invalid name '%s' for component", name);
	    return -1;
	    }

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Include the js module **/
	htrAddScriptInclude(s, "/sys/js/htdrv_componentdecl.js", 0);

	/** Init component **/
	htrAddScriptInit_va(s, "    %s = cmpd_init(%d);\n", name, is_visual);

	/** Hunt for parameters for this component **/
	xaInit(&attrs, 16);
	if ((subobj_qy = objOpenQuery(w_obj,":outer_type == 'system/parameter'",NULL,NULL,NULL)) != NULL)
	    {
	    /** Loop through each param we get **/
	    while((subobj = objQueryFetch(subobj_qy, O_RDONLY)) != NULL)
		{
		param = (pHTCmpdParam)nmMalloc(sizeof(HTCmpdParam));
		if (!param) break;
		xaAddItem(&attrs, param);

		/** Get component parameter name **/
		objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&ptr));
		param->Name = nmSysStrdup(ptr);
		if (cxsecVerifySymbol(param->Name) < 0)
		    {
		    mssError(1,"HTCMPD","Invalid name '%s' for parameter in component '%s'", param->Name, name);
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Does the param have a value? **/
		param->StrVal = htrParamValue(s, param->Name);

		/** Get component type **/
		if (objGetAttrValue(subobj, "type", DATA_T_STRING, POD(&ptr)) == 0)
		    {
		    t = objTypeID(ptr);
		    if (t < 0)
			{
			mssError(1,"HTCMPD","Component '%s' parameter '%s' has unknown/invalid data type '%s'", name, param->Name, ptr);
			rval = -1;
			goto htcmpd_cleanup;
			}
		    param->TypedObjData.DataType = t;
		    }
		else
		    {
		    /** default type is string **/
		    param->TypedObjData.DataType = DATA_T_STRING;
		    }

		/** Get hints **/
		param->Hints = hntObjToHints(subobj);
		if (!param->Hints)
		    {
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Close the object **/
		objClose(subobj);
		subobj = NULL;
		}
	    objQueryClose(subobj_qy);
	    subobj_qy = NULL;
	    }

	/** Build the typed pod values for each data value **/
	for(i=0;i<attrs.nItems;i++)
	    {
	    param = (pHTCmpdParam)(attrs.Items[i]);
	    if (!param->StrVal)
		{
		param->TypedObjData.Flags = DATA_TF_NULL;
		}
	    else
		{
		param->TypedObjData.Flags = 0;
		switch(param->TypedObjData.DataType)
		    {
		    case DATA_T_STRING:
			param->TypedObjData.Data.String = param->StrVal;
			break;
		    case DATA_T_INTEGER:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to integer", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Integer = strtol(param->StrVal,&ptr,10);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to integer", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    case DATA_T_DOUBLE:
			if (!param->StrVal[0])
			    {
			    mssError(1,"HTCMPD","Failed to convert empty string for param '%s' to double", param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			param->TypedObjData.Data.Double = strtod(param->StrVal,&ptr,10);
			if (*ptr)
			    {
			    mssError(1,"HTCMPD","Failed to convert value '%s' for param '%s' to double", param->StrVal, param->Name);
			    rval = -1;
			    goto htcmpd_cleanup;
			    }
			break;
		    default:
			mssError(1,"HTCMPD","Unsupported type for param '%s'", param->Name);
			rval= -1;
			goto htcmpd_cleanup;
		    }
		}

	    /** Verify the thing **/
	    if (hntVerifyHints(param->Hints, &(param->TypedObjData), &ptr, NULL) < 0)
		{
		mssError(1,"HTCMPD","Invalid value '%s' for component '%s' param '%s': %s", param->StrVal, name, param->Name, ptr);
		rval = -1;
		goto htcmpd_cleanup;
		}
	    }

	/** Add actions, events, and client properties **/
	if ((subobj_qy=objOpenQuery(w_obj,NULL,NULL,NULL,NULL)) != NULL)
	    {
	    while((subobj=objQueryFetch(subobj_qy,O_RDONLY)) != NULL)
		{
		/** Get component action/event/cprop name **/
		objGetAttrValue(subobj, "name", DATA_T_STRING, POD(&ptr));
		memccpy(subobj_name, ptr, 0, sizeof(subobj_name)-1);
		subobj_name[sizeof(subobj_name)-1] = '\0';
		if (cxsecVerifySymbol(subobj_name) < 0)
		    {
		    mssError(1,"HTCMPD","Invalid name '%s' for action/event/cprop in component '%s'", subobj_name, name);
		    rval = -1;
		    goto htcmpd_cleanup;
		    }

		/** Get type **/
		objGetAttrValue(subobj, "outer_type", DATA_T_STRING, POD(&ptr));
		if (!strcmp(ptr,"widget/component-decl-action"))
		    htrAddScriptInit_va(s, "    %s.addAction('%s');\n", name, subobj_name);
		else if (!strcmp(ptr,"widget/component-decl-event"))
		    htrAddScriptInit_va(s, "    %s.addEvent('%s');\n", name, subobj_name);
		else if (!strcmp(ptr,"widget/component-decl-cprop"))
		    htrAddScriptInit_va(s, "    %s.addProp('%s');\n", name, subobj_name);

		objClose(subobj);
		subobj = NULL;
		}
	    objQueryClose(subobj_qy);
	    subobj_qy = NULL;
	    }

	/** End init for component **/
	htrAddScriptInit_va(s, "    cmpd_endinit(%s);\n", name);

    htcmpd_cleanup:
	if (subobj) objClose(subobj);
	if (subobj_qy) objQueryClose(subobj_qy);
	for(i=0;i<attrs.nItems;i++)
	    {
	    if (attrs.Items[i])
		{
		param = (pHTCmpdParam)(attrs.Items[i]);
		if (param->Hints) objFreeHints(param->Hints);
		if (param->Name) nmSysFree(param->Name);
		nmFree(param, sizeof(HTCmpdParam));
		}
	    }
	xaDeInit(&attrs);

    return rval;
    }


/*** htcmpdInitialize - register with the ht_render module.
 ***/
int
htcmpdInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Component Declaration Driver");
	strcpy(drv->WidgetName,"component-decl");
	drv->Render = htcmpdRender;
	drv->Verify = htcmpdVerify;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");

	/** Register. **/
	htrRegisterDriver(drv);

	/** Declare support for DHTML user interface class **/
	htrAddSupport(drv, "dhtml");

	HTCMPD.idcnt = 0;

    return 0;
    }

