/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	widget tree     					*/
/* Author:	Matt McGill (MJM)					*/
/* Creation:	July 15, 2004   					*/
/* Description:	Provides an interface for creating and manipulating  	*/
/*		widget trees, primarily used in the process of rendering*/
/*		a DHTML page from an application.                      	*/
/************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "wgtr.h"
#include "apos.h"
#include "hints.h"
#include "param.h"
#include "endorsement_utils.h"
#include "cxlib/xarray.h"
#include "cxlib/datatypes.h"
#include "cxlib/magic.h"
#include "cxlib/xhash.h"
#include "cxlib/strtcpy.h"
#include "ht_render.h"

#define WGTR_MAX_PARAMS		(24)


/*** General algorithm this module uses for loading in a component or app:
 ***
 *** wgtrParseObject
 ***     objOpen
 ***     wgtrParseOpenObject
 ***         Load in pre-specified templates
 ***         wgtr_internal_ParseOpenObject
 ***             load in any widget/parameter
 ***             wgtrLoadTemplate - for each in widget_template= setting
 ***                 wgtrParseObject
 ***             wgtr_internal_LoadAttrs (loads in attributes for the widget node)
 ***                 wgtrCheckTemplate
 ***                     wgtrCopyInTemplate
 ***             wgtr_internal_AddChildren
 ***                 objOpenQuery
 ***                     objQueryFetch
 ***                     wgtr_internal_ParseOpenObject (see above)
 ***                     wgtrAddChild
 ***         Release pre-specified templates
 ***     objClose
 ***/


/** Generic property struct **/
typedef struct
    {
    int		    Magic;			/** Magic Number **/
    void*	    Reserved;
    char	    Name[64];
    short	    Type;
    short	    Flags;
    ObjData	    Val;
    union
	{
	char	    String[16];
	MoneyType   Money;
	DateTime    Date;
	IntVec	    IV;
	StringVec   SV;
	}
	Buf;	/** buffer for various data types **/
    } ObjProperty, *pObjProperty;


#define WGTR_PROP_F_NULL	(1)


struct
    {
    XHashTable	    Methods;		    /* deployment methods */
    XArray	    Drivers;		    /* simple driver listing */
    XHashTable	    DriversByType;
    long	    SerialID;
    int		    RepeatID;		    /* global namespace suffix counter, for repeat widgets */
    } WGTR;


pWgtrNode wgtr_internal_ParseOpenObject(pObject obj, pWgtrNode templates[], pWgtrNode root, pWgtrNode parent, pParamObjects context_objlist, pWgtrClientInfo client_info, pStruct client_params, int xoffset, int yoffset, int flags, int* err, char* namespace);
int wgtr_internal_AddChildren(pObject obj, pWgtrNode this_node, pWgtrNode templates[], pWgtrNode root, pParamObjects context_objlist, pWgtrClientInfo client_info, pStruct client_params, int xoffset, int yoffset, int flags, char* namespace);
pWgtrNode wgtrLoadTemplate(pObjSession s, char* path, pWgtrClientInfo client_info, pStruct params);
int wgtrLoadAddTemplate(pObjSession s, char* path, pWgtrClientInfo client_info, pStruct params, pWgtrNode templates[]);

pWgtrDriver
wgtr_internal_LookupDriver(pWgtrNode node)
    {
    /*int i, j;*/
    pWgtrDriver drv;
    /*char* this_type;*/

    drv = (pWgtrDriver)xhLookup(&(WGTR.DriversByType), node->Type+7); //eg, node->Type == "widget/page" and node->Type+7 == "page";

	/*for (i=0;i<xaCount(&(WGTR.Drivers));i++)
	    {
	    drv = xaGetItem(&(WGTR.Drivers), i);
	    for (j=0;j<xaCount(&(drv->Types));j++)
		{
		this_type = xaGetItem(&(drv->Types), j);
		if (!strncmp(node->Type+7, this_type, 64)) return drv;
		}
	    }*/
	if (!drv) 
	    mssError(1, "WGTR", "No driver registered for type '%s'", node->Type+7);

    return drv;
    }

#if 00
int
wgtr_param_Free(pParam param)
    {

	if (param->Hints)
	    objFreeHints(param->Hints);
	if (param->Value)
	    ptodFree(param->Value);
	nmFree(param, sizeof(Param));

    return 0;
    }
#endif


/*** wgtrParseParameter - parse one 'widget/parameter' and build the
 *** corresponding data structure
 ***/
pParam
wgtrParseParameter(pObject obj, pStruct inf, pParamObjects objlist)
    {
    pParam param = NULL;
    pStruct find_inf;

	/** Create the parameter **/
	param = paramCreateFromObject(obj);
	if (!param) goto error;

	/** Value supplied by user? **/
	find_inf = stLookup_ne(inf, param->Name);
	if (find_inf && param->Value->DataType != DATA_T_CODE)
	    {
	    paramSetValueFromInfNe(param, find_inf);
	    }

	/** Check hints/default/etc **/
	if (paramEvalHints(param, objlist, NULL) < 0)
	    goto error;

#if 00
	/** Allocate **/
	param = (pWgtrAppParam)nmMalloc(sizeof(WgtrAppParam));
	if (!param) goto error;

	/** Set up hints... OK if null **/
	param->Hints = hntObjToHints(obj);

	/** Allocate the typed obj data **/
	param->Value = ptodAllocate();
	if (!param->Value)
	    goto error;

	/** Get name, type, and value **/
	if (objGetAttrValue(obj, "name", DATA_T_STRING, POD(&str)) != 0)
	    goto error;
	strtcpy(param->Name, str, sizeof(param->Name));
	if (objGetAttrValue(obj, "type", DATA_T_STRING, POD(&str)) != 0)
	    {
	    mssError(0, "WGTR", "Parameter '%s' must have a valid type", param->Name);
	    goto error;
	    }
	if (!strcmp(str,"object"))
	    t = DATA_T_STRING;
	else
	    t = objTypeID(str);
	if (t < 0)
	    {
	    mssError(0, "WGTR", "Invalid type '%s' for parameter '%s'", str, param->Name);
	    goto error;
	    }
	param->Value->DataType = t;
	/** set default value and/or verify that the given value is valid **/
	if (hntVerifyHints(param->Hints, param->Value, &str, objlist, obj->Session) < 0)
	    {
	    mssError(1, "WGTR", "Parameter '%s' specified incorrectly: %s", param->Name, str);
	    goto error;
	    }
#endif /* 00 */
	return param;

    error:
	if (param)
	    paramFree(param);
	return NULL;
    }



int
wgtr_param_GetAttrType(pParam params[], char* attrname)
    {
    int i;

	for(i=0;i<WGTR_MAX_PARAMS;i++)
	    if (params[i] && !strcmp(attrname, params[i]->Name))
		return ptodTypeOf(params[i]->Value);

    return -1;
    }


int
wgtr_param_GetAttrValue(pParam params[], char* attrname, int type, pObjData value)
    {
    int i;

	for(i=0;i<WGTR_MAX_PARAMS;i++)
	    {
	    if (params[i] && !strcmp(attrname, params[i]->Name))
		{
		if (ptodTypeOf(params[i]->Value) != type && type != DATA_T_ANY)
		    return -1;
		if (params[i]->Value->Flags & DATA_TF_NULL)
		    return 1;
		objCopyData(&(params[i]->Value->Data), value, params[i]->Value->DataType);
		return 0;
		}
	    }

    return -1;
    }


/*** Copy the properties from template 'match' into node 'tree'.  If
 *** a new template is found, and 'templates' is not null, then the new
 *** template is added to the templates[] list.
 ***/
int
wgtrCopyInTemplate(pWgtrNode tree, pObject tree_obj, pWgtrNode match, char* base_name, pWgtrClientInfo client_info, pWgtrNode templates[])
    {
    pObjProperty p;
    int t,i,j;
    ObjData val;
    char* subst_props[16];
    int n_subst = 0;
    char* prop;
    pWgtrNode subtree,new_node;
    char new_name[64];
    pExpression code;
    int rval;
    char* path;

	/** Copy in standard geometry properties **/
	if (match->r_x >= 0) tree->r_x = match->r_x;
	if (match->r_y >= 0) tree->r_y = match->r_y;
	if (match->r_width >= 0) tree->r_width = match->r_width;
	if (match->r_height >= 0) tree->r_height = match->r_height;
	if (match->fl_x >= 0) tree->fl_x = match->fl_x;
	if (match->fl_y >= 0) tree->fl_y = match->fl_y;
	if (match->fl_width >= 0) tree->fl_width = match->fl_width;
	if (match->fl_height >= 0) tree->fl_height = match->fl_height;

	/** Check for substitutions **/
	for(prop=objGetFirstAttr(tree_obj);prop;prop=objGetNextAttr(tree_obj))
	    {
	    if (!strncmp(prop,"template_",9))
		{
		subst_props[n_subst++] = prop;
		if (n_subst==16) break;
		}
	    }

	/** Copy in additional properties **/
	for (i=0;i<xaCount(&(match->Properties));i++)
	    {
	    p = (pObjProperty)xaGetItem(&(match->Properties), i);
	    t = wgtrGetPropertyType(match, p->Name);
	    rval = wgtrGetPropertyValue(match, p->Name, t, &val);

	    /** Convert templated expression and string values, if needed **/
	    code = NULL;
	    if (t == DATA_T_STRING)
		{
		for(j=0;j<n_subst;j++)
		    {
		    if (!strcmp(val.String, subst_props[j]))
			{
			objGetAttrValue(tree_obj, subst_props[j], DATA_T_STRING, &val);
			break;
			}
		    }
		}
	    else if (t == DATA_T_CODE)
		{
		code = expDuplicateExpression((pExpression)val.Generic);
		val.Generic = code;
		for(j=0;j<n_subst;j++)
		    {
		    objGetAttrValue(tree_obj, subst_props[j], DATA_T_STRING, POD(&prop));
		    expReplaceString(code, subst_props[j], prop);
		    }
		}

	    wgtrAddProperty(tree, p->Name, t, &val, rval == 1);
	    if (code) expFreeExpression(code);

	    /** Found a new template setting? **/
	    if (templates && !strcmp(p->Name, "widget_template"))
		{
		/** Load it in... **/
		if (wgtrGetPropertyValue(match, p->Name, DATA_T_STRING, POD(&path)) == 0)
		    {
		    if (wgtrLoadAddTemplate(tree_obj->Session, path, client_info, NULL, templates) < 0)
			return -1;
		    }
		}
	    }

	/** Subobjects of the template match **/
	for(i=0;i<xaCount(&(match->Children));i++)
	    {
	    subtree = (pWgtrNode)(xaGetItem(&(match->Children), i));
	    snprintf(new_name, sizeof(new_name), "%s_%s", base_name, subtree->Name);
	    if ((new_node = wgtrNewNode(new_name, subtree->Type, subtree->ObjSession, 
			subtree->r_x, subtree->r_y, subtree->r_width, subtree->r_height, 
			subtree->fl_x, subtree->fl_y, subtree->fl_width, subtree->fl_height)) == NULL)
		return -1;

	    if (wgtrSetupNode(new_node) < 0)
		{
		wgtrFree(new_node);
		return -1;
		}

	    wgtrCopyInTemplate(new_node, tree_obj, subtree, base_name, client_info, NULL);
	    wgtrAddChild(tree, new_node);
	    }

    return 0;
    }


/*** See if a template applies to the given node.  We also pass in the
 *** templates[] array - if that is not NULL, then this routine will add
 *** any newly-found templates to the list, up to WGTR_MAX_TEMPLATE.
 ***
 *** Returns: -1 on error, 0 on success, or 1 on success if a new template
 *** was found and added.
 ***/
int
wgtrCheckTemplate(pWgtrNode tree, pObject tree_obj, pWgtrNode template, char* class, pWgtrClientInfo client_info, pWgtrNode templates[])
    {
    pWgtrNode match, search;
    char* tpl_class;
    int i;
    char* wgt_path = NULL;
    char* tpl_path = NULL;
    int rval;

	/** Search through the template and see if we have a match. **/
	match = NULL;
	wgtrGetPropertyValue(tree, "path", DATA_T_STRING, POD(&wgt_path));
	for (i=0;i<xaCount(&(template->Children));i++) 
	    {
	    search = (pWgtrNode)xaGetItem(&(template->Children), i);
	    if (!strcmp(search->Type, tree->Type))
		{
		tpl_class = NULL;
		wgtrGetPropertyValue(search, "widget_class", DATA_T_STRING, POD(&tpl_class));
		if (!tpl_class || (tpl_class && class && !strcmp(tpl_class, class)))
		    {
		    if (!strcmp(tree->Type, "widget/component"))
			wgtrGetPropertyValue(search, "path", DATA_T_STRING, POD(&tpl_path));
		    if (wgt_path && tpl_path && strcmp(wgt_path, tpl_path))
			continue;
		    match = search;
		    rval = wgtrCopyInTemplate(tree, tree_obj, match, tree->Name, client_info, templates);
		    if (rval < 0)
			return rval;
		    }
		}
	    }

	/** Did we find one? **/
	/*if (match)
	    return wgtrCopyInTemplate(tree, tree_obj, match, tree->Name, templates);*/

    return 0;
    }


/*** Load the template and add it to our list of templates, if there is room.
 ***/
int
wgtrLoadAddTemplate(pObjSession s, char* path, pWgtrClientInfo client_info, pStruct params, pWgtrNode templates[])
    {
    int i;

	/** Scan through the list **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    {
	    /** A place to add it? **/
	    if (templates && !templates[i])
		{
		/** Load it in. **/
		templates[i] = wgtrLoadTemplate(s, path, client_info, params);
		if (!templates[i])
		    return -1;
		/*printf("loaded template %s\n", path);*/
		return 0;
		}

	    /** Check duplicates **/
	    if (templates && !strcmp(path, templates[i]->ThisTemplatePath))
		{
		mssError(1,"WGTR","Warning - cannot add duplicate template '%s'", path);
		return 0;
		}
	    }

	mssError(1,"WGTR","Cannot add template '%s' - too many templates already loaded.", path);

    return -1;
    }


pWgtrNode
wgtrLoadTemplate(pObjSession s, char* path, pWgtrClientInfo client_info, pStruct params)
    {
    pWgtrNode template;

	template = wgtrParseObject(s, path, OBJ_O_RDONLY, 0600, "system/structure", params, client_info, WGTR_PF_NOTEMPLATE | WGTR_PF_NOSECURITY);
	if (!template) return NULL;
	if (strcmp(template->Type, "widget/template"))
	    {
	    wgtrFree(template);
	    mssError(1, "WGTR", "Object '%s' does not appear to be a widget template.", path);
	    return NULL;
	    }
	template->ThisTemplatePath = nmSysStrdup(path);

    return template;
    }


pWgtrNode 
wgtrParseObject(pObjSession s, char* path, int mode, int permission_mask, char* type, pStruct params, pWgtrClientInfo client_info, int flags)
    {
    pObject obj;
    pWgtrNode results;
    
	/** attempt to open OSML object **/
	if ( (obj = objOpen(s, path, mode, permission_mask, type)) == NULL)
	    {
	    mssError(0, "WGTR", "Couldn't open %s", path);
	    return NULL;
	    }

	/** call wgtrParseOpenObject and return results **/
	results = wgtrParseOpenObject(obj, params, client_info, flags);
	objClose(obj);
        return results;
    }


int
wgtr_internal_GetTypeAndName(pObject obj, char* name, size_t name_len, char* type, size_t type_len)
    {
    ObjData val;

	/** Get type and verify it **/
	if (objGetAttrValue(obj, "outer_type", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "WGTR", "Couldn't get outer_type for %s", obj->Pathname->Pathbuf);
	    goto error;
	    }
	
	if (strncmp(val.String, "widget/", 7) != 0)
	    {
	    mssError(1, "WGTR", "Object %s is not a widget", obj->Pathname->Pathbuf);
	    goto error;
	    }
	strtcpy(type, val.String, type_len);

	/** get the name from the OSML **/
	if (objGetAttrValue(obj, "name", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "WGTR", "Bark!  Couldn't get name of %s", obj->Pathname->Pathbuf);
	    goto error;
	    }
	strtcpy(name, val.String, name_len);

	return 0;

    error:
	return -1;
    }


/*** wgtr_internal_LoadAttrs - load in the attributes for the widget, and
 *** apply an templates that match the widget.
 ***/
pWgtrNode
wgtr_internal_LoadAttrs(pObject obj, char* name, char* type, pWgtrNode templates[], pWgtrNode root, int xoffset, int yoffset, pWgtrClientInfo client_info, pStruct client_params)
    {
    pWgtrNode this_node = NULL;
    char* class;
    char* prop_name;
    char* ptr;
    ObjData val;
    int prop_type;
    int rval;
    int i,j;
    pStruct one_param;
    int already_used;
    pWgtrNode sub_node;
    /*pWgtrNode my_templates[WGTR_MAX_TEMPLATE];*/

	/*memcpy(my_templates, templates, sizeof(my_templates));*/

	/** create this node **/
	if ( (this_node = wgtrNewNode(name, type, obj->Session, -1, -1, -1, -1, 100, 100, -1, -1)) == NULL)
	    {
	    mssError(0, "WGTR", "Couldn't create node %s", name);
	    goto error;
	    }

	/** Copy in template data **/
	class = NULL;
	objGetAttrValue(obj, "widget_class", DATA_T_STRING, POD(&class));
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (templates[i])
		{
		wgtrCheckTemplate(this_node, obj, templates[i], class, client_info, templates);
		this_node->TemplatePaths[i] = nmSysStrdup(templates[i]->ThisTemplatePath);
		}

	/** Copy in top-level params? **/
	if (!strcmp(type, "widget/component") && objGetAttrValue(obj, "use_toplevel_params", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr, "yes") && client_params)
	    {
	    for(i=0;i<client_params->nSubInf;i++)
		{
		one_param = client_params->SubInf[i];
		if (!strcmp(one_param->Name, "mode") || !strcmp(one_param->Name, "path") || !strcmp(one_param->Name, "width") || !strcmp(one_param->Name, "height") || !strcmp(one_param->Name, "auto_destroy") || !strcmp(one_param->Name, "multiple_instantiation"))
		    continue;
		already_used = 0;
		for(j=0;j<root->Children.nItems;j++)
		    {
		    sub_node = (pWgtrNode)(root->Children.Items[j]);
		    if (!strcmp(sub_node->Type, "widget/parameter") && !strcmp(one_param->Name, sub_node->Name))
			{
			already_used = 1;
			break;
			}
		    }
		if (!already_used)
		    {
		    ptr = NULL;
		    if (stAttrValue_ne(one_param, &ptr) == 0 && ptr)
			wgtrAddProperty(this_node, one_param->Name, DATA_T_STRING, POD(&ptr), 0);
		    }
		}
	    }
	
	/** loop through attributes to fill out the properties array **/
	for(prop_name = objGetFirstAttr(obj); prop_name; prop_name = objGetNextAttr(obj))
	    {
	    /** properties to ignore **/
	    if (!strcmp(prop_name, "condition") || !strcmp(prop_name, "cond_add_children"))
		continue;

	    /** Get the type **/
	    if ( (prop_type = objGetAttrType(obj, prop_name)) < 0) 
		{
		mssError(0, "WGTR", "Couldn't get type for property %s", prop_name);
		goto error;
		}
	    /** get the value **/ 
	    if ((rval = objGetAttrValue(obj, prop_name, prop_type, &val)) < 0)
		{
		mssError(0, "WGTR", "Couldn't get value for property %s", prop_name);
		goto error;
		}

	    /** add property to node **/

	    /** see if it's a property we want to alias for easy access **/
	    if (prop_type == DATA_T_INTEGER)
		{
		if (!strcmp(prop_name,"x")) this_node->r_x = val.Integer;
		else if (!strcmp(prop_name,"y")) this_node->r_y = val.Integer;
		else if (!strcmp(prop_name,"width")) this_node->r_width = val.Integer;
		else if (!strcmp(prop_name,"height")) this_node->r_height = val.Integer;
		else if (!strcmp(prop_name,"fl_x")) this_node->fl_x = val.Integer;
		else if (!strcmp(prop_name,"fl_y")) this_node->fl_y = val.Integer;
		else if (!strcmp(prop_name,"fl_width")) this_node->fl_width = val.Integer;
		else if (!strcmp(prop_name,"fl_height")) this_node->fl_height = val.Integer;
		else wgtrAddProperty(this_node, prop_name, prop_type, &val, rval == 1);
		}
	    else wgtrAddProperty(this_node, prop_name, prop_type, &val, rval == 1);
	    }

	/** Add offsets? **/
	if (this_node->r_x >= 0) this_node->r_x += xoffset;
	if (this_node->r_y >= 0) this_node->r_y += yoffset;
	
	/** initialize all other struct members **/
	this_node->x = this_node->r_x;
	this_node->y = this_node->r_y;
	this_node->width = this_node->r_width;
	this_node->height = this_node->r_height;
	this_node->pre_x = this_node->r_x;
	this_node->pre_y = this_node->r_y;
	this_node->pre_width = this_node->r_width;
	this_node->pre_height = this_node->r_height;
	if (root)
	    this_node->Root = root;
	else
	    this_node->Root = this_node;
	this_node->ClientInfo = client_info;

	/** Setup (call driver New function) **/
	if (wgtrSetupNode(this_node) < 0)
	    goto error;

	/*for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (templates[i] != my_templates[i])
		wgtrFree(my_templates[i]);*/

	return this_node;

    error:
	/*for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (templates[i] != my_templates[i])
		wgtrFree(my_templates[i]);*/

	if (this_node)
	    wgtrFree(this_node);

	return NULL;
    }



int
wgtr_internal_AddChildren(pObject obj, pWgtrNode this_node, pWgtrNode templates[], pWgtrNode root, pParamObjects context_objlist, pWgtrClientInfo client_info, pStruct client_params, int xoffset, int yoffset, int flags, char* namespace)
    {
    pObject child_obj = NULL;
    pObjQuery qy = NULL;
    int t, rval;
    pWgtrNode child_node = NULL;
    ObjData val;
    int nxo=0, nyo=0;
    char name[64];
    /*char rptname[64];*/
    char type[64];
    int errstat;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"WGTR","Could not load widget tree: resource exhaustion occurred");
	    goto error;
	    }

	/** loop through subobjects, and call ourselves recursively to add
	 ** child nodes.
	 **/
	if ( (qy = objOpenQuery(obj, "", NULL, NULL, NULL)) != NULL)
	    {
	    while ( (child_obj = objQueryFetch(qy, O_RDONLY)))
		{
		/** Conditional rendering? **/
		t = objGetAttrType(child_obj, "condition");
		if (t != DATA_T_INTEGER || (rval=objGetAttrValue(child_obj, "condition", t, &val)) != 0 || val.Integer != 0)
		    {
		    /** Parse the widget subtree and add it to this_node. **/
		    child_node = wgtr_internal_ParseOpenObject(child_obj,templates,this_node->Root, this_node, context_objlist, client_info, client_params, xoffset, yoffset, flags, &errstat, namespace);
		    if (child_node != NULL)
			{
			/** Add it. **/
			wgtrAddChild(this_node, child_node);
			child_node = NULL;
			}
		    else if (errstat < 0)
			{
			/** Could not render child, and it is an error.  Stop now. **/
			mssError(0, "WGTR", "Couldn't parse subobject '%s'", child_obj->Pathname->Pathbuf);
			goto error;
			}
		    else
			{
			/** Could not render child, but it is not an error, so we just
			 ** continue on without this child being added (same as if the
			 ** condition had failed)
			 **/
			objClose(child_obj);
			child_obj = NULL;
			continue;
			}
		    }
		else if (t == DATA_T_INTEGER && rval == 0 && val.Integer == 0)
		    {
		    /** The condition is false.  However, if cond_add_children is set,
		     ** then we add the grandchildren anyhow.
		     **/
		    t = objGetAttrType(child_obj, "cond_add_children");
		    if (t == DATA_T_INTEGER || t == DATA_T_STRING)
			{
			rval = objGetAttrValue(child_obj, "cond_add_children", t, &val);
			if (rval == 0 && ((t == DATA_T_INTEGER && val.Integer != 0) ||
				(t == DATA_T_STRING && !strcasecmp(val.String, "yes"))))
			    {
			    /** Get the type and name of the widget.  We need to temporarily load in
			     ** the widget, in order to get its geometry.  Then we free the widget
			     ** rather than inserting it into the widget tree.  The geometry is used
			     ** to properly position its children.
			     **/
			    if (wgtr_internal_GetTypeAndName(child_obj, name, sizeof(name), type, sizeof(type)) < 0)
				goto error;

			    /** Load in the attributes for the widget and copy in the templates. **/
			    if ((child_node = wgtr_internal_LoadAttrs(child_obj, name, type, templates, root, xoffset, yoffset, client_info, client_params)) == NULL)
				goto error;

			    /** compute actual offsets that would have been used had the
			     ** widget really been rendered.
			     **/
			    nxo = child_node->x + child_node->left;
			    nyo = child_node->y + child_node->top;

			    /** Now we no longer need the widget. **/
			    wgtrFree(child_node);
			    child_node = NULL;

			    /** Add the grandchildren without actually rendering the child node
			     ** in question here.
			     **/
			    if (wgtr_internal_AddChildren(child_obj, this_node, templates, this_node->Root, context_objlist, client_info, client_params, xoffset + nxo, yoffset + nyo, flags, namespace) < 0)
				goto error;
			    }
			}
		    }
		objClose(child_obj);
		child_obj = NULL;
		}
	    objQueryClose(qy);
	    qy = NULL;
	    }

	return 0;

    error:
	if (child_node)
	    wgtrFree(child_node);
	if (child_obj)
	    objClose(child_obj);
	if (qy)
	    objQueryClose(qy);
	return -1;
    }



/*** wgtr_internal_CheckLoadTemplates - check to see if there are
 *** any templates specified for this widget.
 ***/
int
wgtr_internal_CheckLoadTemplates(pObject obj, pWgtrClientInfo client_info, pStruct client_params, pWgtrNode my_templates[])
    {
    ObjData val;
    int i;

	/** App author specified a template? **/
	if (objGetAttrType(obj, "widget_template") == DATA_T_STRINGVEC)
	    {
	    /** Stringvec - more than one template was specified **/
	    if (objGetAttrValue(obj, "widget_template", DATA_T_STRINGVEC, &val) == 0)
		{
		for(i=0;i<val.StringVec->nStrings; i++)
		    {
		    if (wgtrLoadAddTemplate(obj->Session, val.StringVec->Strings[i], client_info, client_params, my_templates) < 0)
			{
			mssError(0, "WGTR", "Could not load widget_template '%s'", val.StringVec->Strings[i]);
			return -1;
			}
		    }
		}
	    }
	else if (objGetAttrValue(obj, "widget_template", DATA_T_STRING, &val) == 0)
	    {
	    /** Just one template specified. **/
	    if (wgtrLoadAddTemplate(obj->Session, val.String, client_info, client_params, my_templates) < 0)
		{
		mssError(0, "WGTR", "Could not load widget_template '%s'", val.String);
		return -1;
		}
	    }

    return 0;
    }


/*** wgtr_internal_LoadEndorsements - look for security endorsements specified in
 *** the app or component.
 ***/
int
wgtr_internal_LoadEndorsements(pObject obj, pWgtrNode node)
    {
    char* endorsement_sql;
    pObjQuery eqy;
    pObject eobj;
    char* one_endorsement;
    char* one_context;

	/** Check for endorsements to load **/
	if (wgtrGetPropertyValue(node, "add_endorsements_sql", DATA_T_STRING, POD(&endorsement_sql)) == 0)
	    {
	    /** Run the SQL to fetch the endorsements **/
	    eqy = objMultiQuery(obj->Session, endorsement_sql, NULL, 0);
	    if (eqy)
		{
		/** Loop through returned rows **/
		while ((eobj = objQueryFetch(eqy, O_RDONLY)) != NULL)
		    {
		    if (objGetAttrValue(eobj, "endorsement", DATA_T_STRING, POD(&one_endorsement)) == 0)
			{
			if (objGetAttrValue(eobj, "context", DATA_T_STRING, POD(&one_context)) == 0)
			    {
			    cxssAddEndorsement(one_endorsement, one_context);
			    }
			}
		    objClose(eobj);
		    }
		objQueryClose(eqy);
		}
	    }

    return 0;
    }


/*** wgtr_internal_VerifyEndorsements() - see if this widget is requiring certain
 *** security endorsements.  If so, check them.  Returns 0 if endorsements pass or
 *** no endorsements are required.  Returns 1 if endorsements are required but it
 *** is not considered an "error" (just don't give the user the widget).  Returns
 *** -1 if endorsements are required, the user doesn't have them, and it is an
 *** error condition that should prevent the rendering of the entire application.
 ***/
int
wgtr_internal_VerifyEndorsements(pWgtrNode node)
    {
    char* endorsement_name;
    char* endorsement_action;
    int err;
    int endorsement_ok = 1;

	/** Check endorsements **/
	if (endVerifyEndorsements(node, wgtrGetPropertyValue, &endorsement_name) < 0)
	    endorsement_ok = 0;

	/** Don't have it - error or not?  If we are trying to render the top level of
	 ** a page or component, it is by default an error.  Otherwise, this is by default
	 ** something we ignore - i.e. just leave the widget out of the application.
	 **/
	if (!endorsement_ok)
	    {
	    if (!strcmp(node->Type, "widget/page") || !strcmp(node->Type, "widget/component-decl"))
		err = -1;
	    else
		err = 1;

	    /** App author may have specified explicitly how to handle it. **/
	    if (wgtrGetPropertyValue(node, "missing_endorsement_action", DATA_T_STRING, POD(&endorsement_action)) == 0)
		{
		if (!strcmp(endorsement_action,"error"))
		    err = -1;
		else if (!strcmp(endorsement_action,"ignore"))
		    err = 1;
		}

	    if (err == -1)
		mssError(1,"WGTR","Missing security endorsement '%s' is required by widget '%s'", endorsement_name, node->Name);
	    return err;
	    }

    return 0;
    }


/*** wgtr_internal_ParseOpenObject - recursively load in one widget and
 *** its child widget trees.
 ***/
pWgtrNode 
wgtr_internal_ParseOpenObject(pObject obj, pWgtrNode templates[], pWgtrNode root, pWgtrNode parent, pParamObjects context_objlist, pWgtrClientInfo client_info, pStruct client_params, int xoffset, int yoffset, int flags, int* err, char* namespace)
    {
    pWgtrNode	this_node = NULL;
    pParam param;
    char   name[64], type[64];
    pObject child_obj = NULL, rptrow = NULL;
    pObjQuery qy = NULL,rptqy = NULL;
    pWgtrNode my_templates[WGTR_MAX_TEMPLATE];
    pParam paramlist[WGTR_MAX_PARAMS];
    int n_params;
    int created_objlist = 0;
    int i;
    ObjData rptqysql;
    int new_sec_context = 0;
    int rval;
    char rpt_ns[64];

	/** Default state on error exit **/
	*err = -1;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"WGTR","Could not load widget tree: resource exhaustion occurred");
	    goto error;
	    }

	/** Make a copy of the list of templates we're using.  That way, we
	 ** can add to the list temporarily without affecting our caller's
	 ** copy of the list.
	 **/
	memcpy(my_templates, templates, sizeof(my_templates));

	/** check the outer_type of obj to be sure it's a widget **/
	if (wgtr_internal_GetTypeAndName(obj, name, sizeof(name), type, sizeof(type)) < 0)
	    goto error;

	/** Before we do anything else, examine any application parameters
	 ** that could be present.
	 **/
	if (!context_objlist && (!strcmp(type,"widget/page") || !strcmp(type,"widget/component-decl") || !strcmp(type,"widget/template")))
	    {
	    /** Set up the object list to be used by the parameters **/
	    context_objlist = expCreateParamList();
	    if (!context_objlist) 
		goto error;
	    context_objlist->Session = obj->Session;
	    n_params = 0;
	    memset(paramlist, 0, sizeof(paramlist));
	    created_objlist = 1;
	    expAddParamToList(context_objlist, "this", (void*)paramlist, 0);
	    expSetParamFunctions(context_objlist, "this", wgtr_param_GetAttrType, wgtr_param_GetAttrValue, NULL);

	    /** Query for any parameters that are listed. **/
	    if ( (qy = objOpenQuery(obj, ":outer_type = 'widget/parameter'", NULL, NULL, NULL)) != NULL)
		{
		while ( (child_obj = objQueryFetch(qy, O_RDONLY)) != NULL)
		    {
		    /** Too many? **/
		    if (n_params >= sizeof(paramlist)/sizeof(pParam))
			{
			mssError(1, "WGTR", "Too many parameters for application '%s'", name);
			goto error;
			}

		    /** Parse one parameter. **/
		    if (context_objlist)
			{
			param = wgtrParseParameter(child_obj, client_params, context_objlist);
			if (!param)
			    goto error;
			paramlist[n_params] = param;
			n_params++;
			}
		    objClose(child_obj);
		    child_obj = NULL;
		    }
		objQueryClose(qy);
		qy = NULL;
		}

	    /** Set the evaluation context for expressions within this object.  This
	     ** allows runserver() type expressions (DATA_T_CODE data type) to evaluate
	     ** based on our list of parameters that we just built.
	     **/
	    objSetEvalContext(obj, context_objlist);
	    }

	/** Load new templates? **/
	if (!(flags & WGTR_PF_NOTEMPLATE))
	    {
	    if (wgtr_internal_CheckLoadTemplates(obj, client_info, client_params, my_templates) < 0)
		goto error;
	    }

	/** Load in the attributes for the widget and copy in the template **/
	if ((this_node = wgtr_internal_LoadAttrs(obj, name, type, my_templates, root, xoffset, yoffset, client_info, client_params)) == NULL)
	    goto error;

	/** Set root namespace if this is the root node **/
	if (root == NULL)
	    {
	    strtcpy(this_node->Namespace, this_node->DName, sizeof(this_node->Namespace));
	    namespace = this_node->Namespace;
	    }
	else if (namespace)
	    strtcpy(this_node->Namespace, namespace, sizeof(this_node->Namespace));

	/** Now that the templates and attributes are loaded, we can do the security check **/
	if (!(flags & WGTR_PF_NOSECURITY))
	    {
	    if (!strcmp(type,"widget/page") || !strcmp(type,"widget/component-decl"))
		{
		/** Start the new context for this component/app **/
		/*cxssPushContext();
		new_sec_context = 1;*/

		/** Load our available endorsements for this app/component **/
		wgtr_internal_LoadEndorsements(obj, this_node);
		}

	    /** Check for required endorsements **/
	    rval = wgtr_internal_VerifyEndorsements(this_node);

	    /** Endorsement failed but it is not an error? **/
	    if (rval == 1)
		*err = 0;

	    /** Could not add this widget? **/
	    if (rval != 0)
		goto error;
	    }

	/** If this is a visual widget, clear the x/y offsets. **/
	if (!(this_node->Flags & WGTR_F_NONVISUAL))
	    {
	    xoffset = 0;
	    yoffset = 0;
	    }

	/** Add all of the child widgets under this one. **/
	if (!strcmp(type,"widget/repeat"))
	    {
	    /** Repeat widget.  Get the SQL statement. **/
	    if(objGetAttrValue(obj,"sql",DATA_T_STRING,&rptqysql) != 0)
		{
		mssError(1,"WGTR","Repeat widget %s must have a sql attribute", this_node->Name);
		goto error;
		}

	    /** Run a query, retrieving one row per each time that we need to
	     ** repeat the list of child widgets under the widget/repeat.
	     **/
	    if((rptqy = objMultiQuery(obj->Session,rptqysql.String,NULL,0)) != NULL)
		{
		/** Allow runserver() expressions to reference the repeat widget
		 ** itself, to be able to get the attributes from the SQL query.
		 **/
		expAddParamToList(context_objlist,this_node->Name,NULL,0);

		/** Get the SQL results for the repeat **/
		while((rptrow = objQueryFetch(rptqy, O_RDONLY)) != NULL)
		    {
		    /** The RepeatID is what gives this particular repeated subtree
		     ** a unique namespace, to avoid name conflicts between repeated
		     ** widgets.
		     **/
		    snprintf(rpt_ns, sizeof(rpt_ns), "%s_%6.6x", root->DName, (WGTR.RepeatID++) & 0xffffff);
		    expModifyParam(context_objlist,this_node->Name, rptrow);
		   
		    /** Add the children of the repeat widget **/
		    if (wgtr_internal_AddChildren(obj, this_node, my_templates, this_node->Root, context_objlist, client_info, client_params, xoffset, yoffset, flags, rpt_ns) < 0)
			goto error;

		    objClose(rptrow);
		    rptrow = NULL;
		    }
		expRemoveParamFromList(context_objlist, this_node->Name);
		objQueryClose(rptqy);
		rptqy = NULL;
		}
	    }
	else
	    {
	    /** Not a repeat widget.  Just add the children.  Once. **/
	    if (wgtr_internal_AddChildren(obj, this_node, my_templates, this_node->Root, context_objlist, client_info, client_params, xoffset, yoffset, flags, namespace) < 0)
		goto error;
	    }

	/** Free the template if we created it here. **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (my_templates[i] != templates[i])
		wgtrFree(my_templates[i]);

	/** Free up the context objlist, if need be **/
	if (created_objlist)
	    {
	    objSetEvalContext(obj, NULL);
	    expFreeParamList(context_objlist);
	    for(i=0;i<n_params;i++) paramFree(paramlist[i]);
	    }

	/** Release the security context? **/
	if (new_sec_context) cxssPopContext();

	/** return the completed node and subtree **/
	*err = 0;
	return this_node;

    error:
	if (new_sec_context) cxssPopContext();
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (my_templates[i] != templates[i])
		wgtrFree(my_templates[i]);
	if (rptrow)
	    objClose(rptrow);
	if (rptqy)
	    objQueryClose(rptqy);
	if (this_node)
	    wgtrFree(this_node);
	if (created_objlist)
	    {
	    objSetEvalContext(obj, NULL);
	    expFreeParamList(context_objlist);
	    for(i=0;i<n_params;i++) paramFree(paramlist[i]);
	    }
	if (child_obj)
	    objClose(child_obj);
	if (qy)
	    objQueryClose(qy);
	return NULL;
    }


pWgtrNode
wgtrParseOpenObject(pObject obj, pStruct params, pWgtrClientInfo client_info, int flags)
    {
    pWgtrNode template_arr[WGTR_MAX_TEMPLATE];
    pWgtrNode tree;
    int i;
    int errstat;

	/** Load templates? **/
	memset(template_arr, 0, sizeof(template_arr));
	if (client_info && client_info->Templates && !(flags & WGTR_PF_NOTEMPLATE))
	    {
	    for(i=0;i<WGTR_MAX_TEMPLATE;i++)
		if (client_info->Templates[i])
		    {
		    if (wgtrLoadAddTemplate(obj->Session, client_info->Templates[i], client_info, params, template_arr) < 0)
			return NULL;
		    }
	    }

	/** Load in the widget tree, using the templates **/
	tree = wgtr_internal_ParseOpenObject(obj, template_arr, NULL, NULL, NULL, client_info, params, 0, 0, flags, &errstat, NULL);

	/** Free memory used by templates **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (template_arr[i])
		wgtrFree(template_arr[i]);

    return tree;
    }


void wgtr_internal_FreeProperty(pObjProperty prop)
    {
	if (!prop) return;

	switch (prop->Type)
	    {
	    case DATA_T_STRING:
		if (prop->Val.String) nmSysFree(prop->Val.String);
		break;
	    case DATA_T_CODE:
		if (prop->Val.Generic) expFreeExpression((pExpression)prop->Val.Generic);
		break;
	    }
	nmFree(prop, sizeof(ObjProperty));

    }

void 
wgtrFree(pWgtrNode tree)
    {
    int i;
    pObjProperty p;
    IfcHandle ifc;

	ASSERTMAGIC(tree, MGK_WGTR);

	if (!tree) return;

	/** free all children **/
	for (i=0;i<xaCount(&(tree->Children));i++) 
	    {
	    wgtrFree(xaGetItem(&(tree->Children), i));
	    }
	xaDeInit(&(tree->Children));

	/** free all properties **/
	for (i=0;i<xaCount(&(tree->Properties));i++)
	    {
	    p = xaGetItem(&(tree->Properties), i);
	    if (p) wgtr_internal_FreeProperty(p);
	    }
	xaDeInit(&(tree->Properties));

	/** free the interface handles **/
	for (i=0;i<xaCount(&(tree->Interfaces));i++)
	    {
	    ifc = xaGetItem(&(tree->Interfaces), i);
	    if (ifc) ifcReleaseHandle(ifc);
	    }
	xaDeInit(&(tree->Interfaces));

	/** free the node itself **/
	for(i=0;i<WGTR_MAX_TEMPLATE;i++)
	    if (tree->TemplatePaths[i]) nmSysFree(tree->TemplatePaths[i]);
	if (tree->ThisTemplatePath) nmSysFree(tree->ThisTemplatePath);
	nmFree(tree, sizeof(WgtrNode));
    }


pWgtrIterator 
wgtrGetIterator(pWgtrNode tree, int traversal_type)
    {
	ASSERTMAGIC(tree, MGK_WGTR);
	return NULL;
    }


pWgtrNode 
wgtrNext(pWgtrIterator itr)
    {
	return NULL;
    }


void 
wgtrFreeIterator(pWgtrIterator itr)
    {
    }


int 
wgtrGetPropertyType(pWgtrNode widget, char* name)
    {
    int i, count;
    pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	if (!strcmp(name, "name")) return DATA_T_STRING;
	else if (!strcmp(name, "outer_type")) return DATA_T_STRING;
	else if (!strncmp(name, "r_",2) || !strncmp(name, "fl_", 3)) return DATA_T_INTEGER;
	else if (!strcmp(name, "x") || !strcmp(name, "y") || !strcmp(name, "width") || !strcmp(name, "height"))
	    return DATA_T_INTEGER;
	count = xaCount(&(widget->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(widget->Properties), i);
	    if (prop && !strcmp(name, prop->Name)) break;
	    }
	if (i == count) return -1;
	
	return prop->Type;
    }


int 
wgtrGetPropertyValue(pWgtrNode widget, char* name, int datatype, pObjData val)
    {
	int i, count;
	pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	/** first check for values we have aliased **/
	if (datatype == DATA_T_INTEGER)
	    {
	    if (!strcmp(name, "x")) { val->Integer = widget->x; return (val->Integer == -1)?1:0; }
	    if (!strcmp(name, "y")) { val->Integer = widget->y; return (val->Integer == -1)?1:0; }
	    if (!strcmp(name, "width")) { val->Integer = widget->width; return (val->Integer == -1)?1:0; }
	    if (!strcmp(name, "height")) { val->Integer = widget->height; return (val->Integer == -1)?1:0; }
	    if (!strncmp(name, "r_", 2))
		{
		if (!strcmp(name+2, "x")) { val->Integer = widget->r_x; return 0; }
		if (!strcmp(name+2, "y")) { val->Integer = widget->r_y; return 0; }
		if (!strcmp(name+2, "width")) { val->Integer = widget->r_width; return 0; }
		if (!strcmp(name+2, "height")) { val->Integer = widget->r_height; return 0; }
		}
	    else if (!strncmp(name, "fl_", 3))
		{
		if (!strcmp(name+3, "x")) { val->Integer = widget->fl_x; return 0; }
		if (!strcmp(name+3, "y")) { val->Integer = widget->fl_y; return 0; }
		if (!strcmp(name+3, "width")) { val->Integer = widget->fl_width; return 0; }
		if (!strcmp(name+3, "height")) { val->Integer = widget->fl_height; return 0; }
		}
	    }
	else if (datatype == DATA_T_STRING)
	    {
	    if (!strcmp(name, "name")) { val->String = widget->Name; return 0; }
	    else if (!strcmp(name, "outer_type")) { val->String = widget->Type; return 0; }
	    }
	    
	/** if we didn't find it there, loop through the list of properties until we do **/
	count = xaCount(&(widget->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(widget->Properties), i);
	    if (prop && !strcmp(name, prop->Name)) break;
	    }
	if (i == count || datatype != prop->Type) return -1;

	if (prop->Flags & WGTR_PROP_F_NULL) return 1;

	objCopyData(&(prop->Val), val, prop->Type);

	return 0;
    }


char* 
wgtrFirstPropertyName(pWgtrNode widget)
    {
	ASSERTMAGIC(widget, MGK_WGTR);
	widget->CurrProperty = 0;
	return wgtrNextPropertyName(widget);
    }


char* 
wgtrNextPropertyName(pWgtrNode widget)
    {
    pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	if (widget->CurrProperty == xaCount(&(widget->Properties))) return NULL;
	if ( (prop = xaGetItem(&(widget->Properties), widget->CurrProperty++)) == NULL) return NULL;
	return prop->Name;
    }

    
int 
wgtrAddProperty(pWgtrNode widget, char* name, int datatype, pObjData val, int isnull)
    /** XXX Should this check for duplicates? **/
    {
    pObjProperty prop, old_prop;
    int i;

	ASSERTMAGIC(widget, MGK_WGTR);
	/** Get the memory **/
	if ( (prop = (pObjProperty)nmMalloc(sizeof(ObjProperty))) == NULL) return -1;

	memset(prop, 0, sizeof(ObjProperty));
	strtcpy(prop->Name, name, sizeof(prop->Name));
	prop->Type = datatype;

	if (isnull)
	    {
	    prop->Flags |= WGTR_PROP_F_NULL;
	    }
	else
	    {
	    /** Make sure the value is assigned correctly. A little nasty when these
	     ** two interfaces meet
	     **/
	    switch (datatype)
		/** XXX Is this right? XXX **/
		{
		case DATA_T_INTEGER: 
		    prop->Val.Integer = val->Integer; 
		    break;
		case DATA_T_STRING: 
		    prop->Val.String = nmSysStrdup(val->String); 
		    break;
		case DATA_T_DOUBLE: 
		    prop->Val.Double = val->Double; 
		    break;
		case DATA_T_DATETIME: 
		    prop->Buf.Date = *(val->DateTime);
		    prop->Val.DateTime = &(prop->Buf.Date);
		    break;	
		case DATA_T_MONEY:
		    prop->Buf.Money = *(val->Money);
		    prop->Val.Money = &(prop->Buf.Money);
		    break;
		case DATA_T_INTVEC:
		    prop->Buf.IV = *(val->IntVec);
		    prop->Val.IntVec = &(prop->Buf.IV);
		    break;
		case DATA_T_STRINGVEC:
		    prop->Buf.SV = *(val->StringVec);
		    prop->Val.StringVec = &(prop->Buf.SV);
		    break;
		case DATA_T_CODE:
		    prop->Val.Generic = (void*)expDuplicateExpression((pExpression)val->Generic);    
		    break;
		}
	    }

	/** Remove existing property? **/
	for (i=0;i<xaCount(&(widget->Properties));i++)
	    {
	    old_prop = xaGetItem(&(widget->Properties), i);
	    if (old_prop && !strcmp(name, old_prop->Name))
		{
		xaRemoveItem(&(widget->Properties), i);
		wgtr_internal_FreeProperty(old_prop);
		break;
		}
	    }

	/** Assign the property to the node **/
	xaAddItem(&(widget->Properties), prop);
	return 0;
    }

    
int 
wgtrDeleteProperty(pWgtrNode widget, char* name)
    {
    int i, count;
    pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	count = xaCount(&(widget->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(widget->Properties), i);
	    if (prop && !strcmp(prop->Name, name)) break;
	    }
	if (i == count) return -1;
	xaRemoveItem(&(widget->Properties), i);

	wgtr_internal_FreeProperty(prop);

	return 0;
    }

    
int 
wgtrSetProperty(pWgtrNode widget, char* name, int datatype, pObjData val)
    {
    int i, count;
    pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	count = xaCount(&(widget->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(widget->Properties), i);
	    if (prop && !strcmp(prop->Name, name)) break;
	    }
	if (i == count) return -1;

	/** Clean up whatever was there before **/
	if (prop->Type == DATA_T_STRING && prop->Val.String)
	    {
	    nmSysFree(prop->Val.String);
	    prop->Val.String = NULL;
	    }

	prop->Type = datatype;
	switch (datatype)
	    /** XXX Is this right? XXX **/
	    {
	    case DATA_T_INTEGER: prop->Val.Integer = val->Integer; break;
	    case DATA_T_STRING: prop->Val.String = nmSysStrdup(val->String); break;
	    case DATA_T_DOUBLE: prop->Val.Double = val->Double; break;
	    case DATA_T_DATETIME: 
		prop->Buf.Date = *(val->DateTime);
		prop->Val.DateTime = &(prop->Buf.Date);
		break;	
	    case DATA_T_MONEY:
		prop->Buf.Money = *(val->Money);
		prop->Val.Money = &(prop->Buf.Money);
		break;
	    case DATA_T_INTVEC:
		prop->Buf.IV = *(val->IntVec);
		prop->Val.IntVec = &(prop->Buf.IV);
		break;
	    case DATA_T_STRINGVEC:
		prop->Buf.SV = *(val->StringVec);
		prop->Val.StringVec = &(prop->Buf.SV);
		break;
	    }
	return 0;
    }


pWgtrNode 
wgtrNewNode(	char* name, char* type, pObjSession s,
		int rx, int ry, int rwidth, int rheight,
		int flx, int fly, int flwidth, int flheight)
    {
    pWgtrNode node;

	if ( (node = (pWgtrNode)nmMalloc(sizeof(WgtrNode))) == NULL)
	    {
	    mssError(0, "WGTR", "Couldn't allocate memory for new node");
	    return NULL;
	    }
	memset(node, 0, sizeof(WgtrNode));
	SETMAGIC(node, MGK_WGTR);

	strtcpy(node->Name, name, sizeof(node->Name));
	strtcpy(node->Type, type, sizeof(node->Name));
	snprintf(node->DName, sizeof(node->DName), "w%8.8lx", WGTR.SerialID++);
	node->x = node->r_x = rx;
	node->y = node->r_y = ry;
	node->width = node->r_width = rwidth;
	node->height = node->r_height = rheight;
	node->fl_x = flx;
	node->fl_y = fly;
	node->fl_width = flwidth;
	node->fl_height = flheight;
	node->ObjSession = s;
	node->Parent = NULL;
	node->min_height = 0;
	node->min_width = 0;
	node->LayoutGrid = NULL;
	node->Root = node;  /* this will change when it is added as a child */
	node->DMPrivate = NULL;
	node->top = node->bottom = node->right = node->left = 0;
	node->Verified = 0;

	xaInit(&(node->Properties), 16);
	xaInit(&(node->Children), 16);
	xaInit(&(node->Interfaces), 8);

	return node;
    }


int
wgtrSetupNode(pWgtrNode node)
    {
    pWgtrDriver drv;

	/** look up the 'new' function and call it on the now-init'd struct **/
	if ( (drv = wgtr_internal_LookupDriver(node)) == NULL) 
	    {
	    return -1;
	    }
	if (drv->New(node) < 0)
	    {
	    mssError(1, "WGTR", "Error initializing new widget node '%s'", node->Name);
	    return -1;
	    }

    return 0;
    }

    
int 
wgtrDeleteChild(pWgtrNode widget, char* child_name)
    {
    int i;
    pWgtrNode child;

	ASSERTMAGIC(widget, MGK_WGTR);
	for (i=0;i<xaCount(&(widget->Children));i++)
	    {
		child = xaGetItem(&(widget->Children), i);
		if (!strcmp(child->Name, child_name)) break;
	    }
	if (i == xaCount(&(widget->Children))) return -1;

	xaRemoveItem(&(widget->Children), i);
	wgtrFree(child);
	return 0;
    }

    
int 
wgtrAddChild(pWgtrNode widget, pWgtrNode child)
    {
	ASSERTMAGIC(widget, MGK_WGTR);
	xaAddItem(&(widget->Children), child);
	child->Parent = widget;
	child->Root = widget->Root;
	if (child->Namespace[0] == '\0')
	    strtcpy(child->Namespace, widget->Namespace, sizeof(child->Namespace));
	return 0;
    }


void
wgtr_internal_Indent(int indent)
    {
    int i;

	for (i=0;i<indent;i++) fprintf(stderr, " ");
    }


pWgtrNode 
wgtrFirstChild(pWgtrNode tree)
    {
	ASSERTMAGIC(tree, MGK_WGTR);
	tree->CurrChild = 0;
	return wgtrNextChild(tree);
    }


pWgtrNode
wgtrNextChild(pWgtrNode tree)
    {
	ASSERTMAGIC(tree, MGK_WGTR);
	if (tree->CurrChild >= xaCount(&(tree->Children))) return NULL;
	return xaGetItem(&(tree->Children), tree->CurrChild++);
    }


pExpression
wgtrGetExpr(pWgtrNode widget, char* attrname)
    {
    pExpression exp;
    int t;
    ObjData pod;

	/** Check data type **/
	t = wgtrGetPropertyType(widget, attrname);
	if (t < 0) return NULL;
	if (t == DATA_T_CODE)
	    {
	    /** Get code directly **/
	    if (wgtrGetPropertyValue(widget, attrname, t, POD(&exp)) != 0) return NULL;
	    exp = expDuplicateExpression(exp);
	    }
	else
	    {
	    /** Build exp from pod **/
	    if (wgtrGetPropertyValue(widget, attrname, t, &pod) != 0) return NULL;
	    exp = expPodToExpression(&pod, t, NULL);
	    }

	return exp;
    }


char*
wgtr_internal_GetString(pWgtrNode wgt, char* attrname)
    {
    char* str;
	
	if (wgtrGetPropertyValue(wgt, attrname, DATA_T_STRING, POD(&str)) != 0)
	    return NULL;
	str = nmSysStrdup(str);

	return str;
    }


/** shamelessly copied from utulities/hints.c, with some modifications **/

extern int hnt_internal_SetStyleItem(pObjPresentationHints ph, char* style);

pObjPresentationHints
wgtrWgtToHints(pWgtrNode widget)
    {
    pObjPresentationHints ph;
    int t, i;
    char* ptr;
    int n;
    ObjData od;

	/** Allocate a new ph structure **/
	ph = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
	if (!ph) return NULL;
	memset(ph, 0, sizeof(ObjPresentationHints));
	xaInit(&(ph->EnumList),16);

	/** expressions **/
	ph->Constraint = wgtrGetExpr(widget, "constraint");
	ph->DefaultExpr = wgtrGetExpr(widget, "default");
	ph->MinValue = wgtrGetExpr(widget, "min");
	ph->MaxValue = wgtrGetExpr(widget, "max");

	/** enum list? **/
	t = wgtrGetPropertyType(widget, "enumlist");
	if (t >= 0)
	    {
	    if (wgtrGetPropertyValue(widget, "enumlist", t, &od) != 0) t = -1;
	    }
	if (t >= 0)
	    {
	    switch (t)
		{
		case DATA_T_STRINGVEC:
		    for (i=0;i<od.StringVec->nStrings;i++)
			xaAddItem(&(ph->EnumList), nmSysStrdup(od.StringVec->Strings[i]));
		    break;
		case DATA_T_INTVEC:
		    for (i=0;i<od.IntVec->nIntegers;i++)
			{
			ptr = nmSysMalloc(16);
			snprintf(ptr, 16, "%d", od.IntVec->Integers[i]);
			xaAddItem(&(ph->EnumList), ptr);
			}
		    break;
		case DATA_T_INTEGER:
		    ptr = nmSysMalloc(16);
		    snprintf(ptr, 16, "%d", od.Integer);
		    xaAddItem(&(ph->EnumList), ptr);
		    break;
		case DATA_T_STRING:
		    xaAddItem(&(ph->EnumList), nmSysStrdup(od.String));
		    break;
		default:
		    mssError(1, "WGTR", "Couldn't convert widget to Hitns structure: Invalid type for enumlist!");
		    objFreeHints(ph);
		    return NULL;
		}
	    }
	/** String type hint info **/
	ph->EnumQuery = wgtr_internal_GetString(widget, "enumquery");
	ph->AllowChars = wgtr_internal_GetString(widget, "allowchars");
	ph->BadChars = wgtr_internal_GetString(widget, "badchars");
	ph->Format = wgtr_internal_GetString(widget, "format");
	ph->GroupName = wgtr_internal_GetString(widget, "groupname");
	ph->FriendlyName = wgtr_internal_GetString(widget, "description");

	/** Int type hint info **/
	if (wgtrGetPropertyValue(widget, "length", DATA_T_INTEGER, POD(&n)) == 0)
	    ph->Length = n;
	if (wgtrGetPropertyValue(widget, "width", DATA_T_INTEGER, POD(&n)) == 0)
	    ph->VisualLength = n;
	if (wgtrGetPropertyValue(widget, "height", DATA_T_INTEGER, POD(&n)) == 0)
	    ph->VisualLength2 = n;
	else
	    ph->VisualLength2 = 1;
	if (wgtrGetPropertyValue(widget, "groupid", DATA_T_INTEGER, POD(&n)) == 0)
	    ph->GroupID = n;

	/** Read-only bits **/
	t = wgtrGetPropertyType(widget, "readonlybits");
	if (t>0)
	    {
	    if (wgtrGetPropertyValue(widget, "readonlybits", t, &od) != 0) t = -1;
	    }
	if (t>=0)
	    {
	    switch(t)
		{
		case DATA_T_INTEGER:
		    ph->BitmaskRO = (1<<od.Integer);
		    break;
		case DATA_T_INTVEC:
		    ph->BitmaskRO = 0;
		    for (i=0;i<od.IntVec->nIntegers;i++)
			ph->BitmaskRO |= (1<<od.IntVec->Integers[i]);
		    break;
		default:
		    mssError(1, "WGTR", "Couldn't convert widget to hint structure - invalid type for readonlybits!");
		    objFreeHints(ph);
		    return NULL;
		}
	    }
	/** Style **/
	t = wgtrGetPropertyType(widget, "style");
	if (t >= 0)
	    {
	    if (wgtrGetPropertyValue(widget, "style", t, &od) != 0) t = -1;
	    }
	i=0;
	if (t == DATA_T_STRING || t == DATA_T_STRINGVEC)
	    {
	    while (1)
		{
		/** String or StringVec? **/
		if (t == DATA_T_STRING)
		    {
		    ptr = od.String;
		    }
		else
		    {
		    if (od.StringVec->nStrings == 0) break;
		    ptr = od.StringVec->Strings[i];
		    }
		/** Check style settings **/
		hnt_internal_SetStyleItem(ph, ptr);

		if (t == DATA_T_STRING || i >= od.StringVec->nStrings-1) break;
		i++;
		}
	    }

	return ph;
    }

void
wgtrPrint(pWgtrNode tree, int indent)
    {
    ObjData val;
    char* name;
    int type;
    pWgtrNode subobj;

	ASSERTMAGIC(tree, MGK_WGTR);
	/** first print the object name **/
	wgtr_internal_Indent(indent);
	fprintf(stderr, "Widget: %s Type: %s (%d,%d %dx%d)\n", tree->Name, tree->Type, tree->x, tree->y, tree->width, tree->height);

	/** now print the properties **/
	name = wgtrFirstPropertyName(tree);
	indent += 4;
	while (name)
	    { 
	    type = wgtrGetPropertyType(tree, name);
	    wgtrGetPropertyValue(tree, name, type, &val);
	    wgtr_internal_Indent(indent);
	    fprintf(stderr, "Property: %s Type: ", name);
	    switch (type)
		{
		case DATA_T_STRING: fprintf(stderr, "String Value: \"%s\"\n", val.String); break;
		case DATA_T_INTEGER: fprintf(stderr, "Integer Value: %d\n", val.Integer); break;
		case DATA_T_DOUBLE: fprintf(stderr, "Double Value: %f\n", val.Double); break;
		case DATA_T_MONEY: fprintf(stderr, "Money\n"); break;
		case DATA_T_DATETIME: fprintf(stderr, "Datetime\n"); break;
		case DATA_T_INTVEC: fprintf(stderr, "IntVec\n"); break;
		case DATA_T_STRINGVEC: fprintf(stderr, "StringVec\n"); break;
		default: fprintf(stderr, "?\n"); break;
		}
	    name  = wgtrNextPropertyName(tree);
	    }
	
	/** now print the sub-objects **/
	subobj = wgtrFirstChild(tree);
	while (subobj)
	    {
	    wgtrPrint(subobj, indent);
	    subobj = wgtrNextChild(tree);
	    }
    }


int 
wgtr_internal_BuildVerifyQueue(pWgtrVerifySession vs, pWgtrNode node)
    {
    int i;

	wgtrScheduleVerify(vs, node);
	for (i=0;i<xaCount(&(node->Children));i++)
	    wgtr_internal_BuildVerifyQueue(vs, xaGetItem(&(node->Children), i));
	return 0;
    }

int
wgtrVerify(pWgtrNode tree, pWgtrClientInfo client_info)
    {
    WgtrVerifySession vs;
    pWgtrDriver drv;
    XArray Names;
    char name_ns[128];
    int i;

	/** initialize datastructures **/
	vs.Tree = tree;
	xaInit(&(vs.VerifyQueue), 128);
	xaInit(&Names, 128);

	/** Set top-level width and height **/
	if (tree->width < client_info->MinWidth) tree->width = client_info->MinWidth;
	if (tree->width > client_info->MaxWidth) tree->width = client_info->MaxWidth;
	if (tree->height < client_info->MinHeight) tree->height = client_info->MinHeight;
	if (tree->height > client_info->MaxHeight) tree->height = client_info->MaxHeight;
	vs.ClientInfo = client_info;

	/** Build the verification queue **/
	wgtr_internal_BuildVerifyQueue(&vs, tree);
	vs.NumWidgets = xaCount(&(vs.VerifyQueue));

	/** Iterate through the queue **/
	for (vs.CurrWidgetIndex=0;vs.CurrWidgetIndex<vs.NumWidgets;vs.CurrWidgetIndex++)
	    {
	    /** Get the next node **/
	    vs.CurrWidget = xaGetItem(&(vs.VerifyQueue), vs.CurrWidgetIndex);

	    /** Make sure its name is unique **/
	    for (i=0;i<xaCount(&Names);i++)
		{
		snprintf(name_ns, sizeof(name_ns), "%s:%s", vs.CurrWidget->Namespace, vs.CurrWidget->Name);
		if (!strcmp(name_ns, xaGetItem(&Names, i)))
		    {
		    mssError(1, "WGTR", "Widget name '%s' is not unique - widget names must be unique within an application",
				vs.CurrWidget->Name);
		    goto error;
		    }
		}
	    xaAddItem(&Names, nmSysStrdup(name_ns));

	    /** Get the driver for this node **/
	    drv = wgtr_internal_LookupDriver(vs.CurrWidget);
	    if (!drv)
		{
		mssError(1, "WGTR", "Unknown widget object type '%s' for widget '%s'", 
		    vs.CurrWidget->Type, vs.CurrWidget->Name);
		goto error;
		}

	    /** Verify the widget **/
	    if (drv->Verify && (drv->Verify(&vs) < 0))
		{
		mssError(0, "WGTR", "Couldn't verify widget '%s'", vs.CurrWidget->Name);
		goto error;
		}
	    else vs.CurrWidget->Verified = 1;
	    }

	/** Auto-position the widget tree **/
	if(aposAutoPositionWidgetTree(tree) < 0)
	    {
	    mssError(0, "WGTR", "Couldn't auto-position widget tree");
	    goto error;
	    }
	
	/** free up data structures **/
	for(i=0;i<xaCount(&Names);i++)
	    nmSysFree(xaGetItem(&Names, i));
	xaDeInit(&Names);
	xaDeInit(&(vs.VerifyQueue));

	return 0;
error:
	xaDeInit(&Names);
	xaDeInit(&(vs.VerifyQueue));
	return -1;
    }


int 
wgtrScheduleVerify(pWgtrVerifySession vs, pWgtrNode widget)
    {
	xaAddItem(&(vs->VerifyQueue), widget);
	vs->NumWidgets++;
	return 0;
    }


int
wgtrReverify(pWgtrVerifySession vs, pWgtrNode widget)
    {
	if (widget->Verified)
	    {
	    widget->Verified = 0;
	    return wgtrScheduleVerify(vs, widget);
	    }
	return 0;
    }


int 
wgtrCancelVerify(pWgtrVerifySession vs, pWgtrNode widget)
    {
    int i;

	/** find the widget **/
	if ( (i=xaFindItem(&(vs->VerifyQueue), widget)) < 0) 
	    {
	    mssError(1, "WGTR", "wgtrCancelVerify() - couldn't find widget '%s'", widget->Name);
	    return -1;
	    }

	/** if we've already verified it, or we're currently verifying it, this fails **/
	if (i <= vs->CurrWidgetIndex || widget->Verified) 
	    {
	    mssError(1, "WGTR", "wgtrCancelVerify() - widget '%s' already verified", widget->Name);
	    return -1;
	    }

	/** remove the widget from the queue **/
	xaRemoveItem(&(vs->VerifyQueue), i);
	return 0;
    }



int
wgtrInitialize()
    {
	/** init datastructures for handling drivers **/
	xaInit(&(WGTR.Drivers), 64);
	xhInit(&(WGTR.DriversByType), 127, 0);
	xhInit(&(WGTR.Methods), 5, 0);

	WGTR.SerialID = lrand48();
	WGTR.RepeatID = 1;
	
	/** init datastructures for auto-positioning **/
	aposInit();
	
	/** call the initialization routines of all the widget drivers. I suppose it's a
	 ** little weird to do things this way - if they're going to be init'ed from here,
	 ** why have 'drivers'? Why not just hard-code them? Well, maybe later on they'll
	 ** become dynamically loaded modules. Then it'll make more sense
	 **/
	wgtalrtInitialize();
	wgtbtnInitialize();
	wgtcaInitialize();
	wgtcbInitialize();
	wgtchtInitialize();
	wgtclInitialize();
	wgtcmpInitialize();
	wgtcmpdInitialize();
	wgtconnInitialize();
	wgtdtInitialize();
	wgtddInitialize();
	wgtebInitialize();
	wgtexInitialize();
	wgtfbInitialize();
	wgtformInitialize();
	wgtfsInitialize();
	wgtfuInitialize();
	wgtsetInitialize();
	wgthintInitialize();
	wgthtmlInitialize();
	wgtibtnInitialize();
	wgtimgInitialize();
	wgtlblInitialize();
	wgtmenuInitialize();
	wgtocInitialize();
	wgtmapInitialize();
	wgtosrcInitialize();
	wgtpageInitialize();
	wgtpnInitialize();
	wgtrbInitialize();
	wgtrptInitialize();
	wgtsbInitialize();
	wgtspaneInitialize();
	wgtspnrInitialize();
	wgtosmlInitialize();
	wgttabInitialize();
	wgttblInitialize();
	wgttermInitialize();
	wgttxInitialize();
	wgttbtnInitialize();
	wgttmInitialize();
	wgttreeInitialize();
	wgtvblInitialize();
	wgtwinInitialize();
	wgttplInitialize();
	wgtpaInitialize();
	wgtalInitialize();
	wgtmsInitialize();
	wgtruleInitialize();

    return 0;
    }


/*** wgtrRegisterDriver - registers a driver 
 ***/
int 
wgtrRegisterDriver(char* name, int (*Verify)(), int (*New)())
    {
    int i;
    pWgtrDriver drv;

	/** make sure it's not already there **/
	for (i=0;i<xaCount(&(WGTR.Drivers));i++)
	    {
	    if (!strcmp(name, ((pWgtrDriver)xaGetItem(&(WGTR.Drivers), i))->Name))
		{
		mssError(1, "WGTR", "Tried to register driver '%s' twice", name);
		return -1;
		}
	    }
	
	/** allocate memory **/
	if ( (drv = nmMalloc(sizeof(WgtrDriver))) == NULL)
	    {
	    mssError(1, "WGTR", "Couldn't allocate memory for new widget driver");
	    return -1;
	    }
	memset(drv, 0, sizeof(WgtrDriver));
	strtcpy(drv->Name, name, sizeof(drv->Name));
	drv->New = New;
	drv->Verify = Verify;
	xaInit(&(drv->Types), 4);

	xaAddItem(&(WGTR.Drivers), drv);

	return 0;
    }


/*** wgtrAddType - associates a widget type with a wgtr driver 
 ***/
int 
wgtrAddType(char* name, char* type_name)
    {
    pWgtrDriver drv;
    int i;

	for (i=0;i<xaCount(&(WGTR.Drivers));i++)
	    {
	    drv = xaGetItem(&(WGTR.Drivers), i);
	    if (!strncmp(drv->Name, name, 64)) break;
	    }
	if (i == xaCount(&(WGTR.Drivers))) return -1;
	xaAddItem(&(drv->Types), nmSysStrdup(type_name));
	xhAdd(&(WGTR.DriversByType), nmSysStrdup(type_name), (void*)drv);
	return 0;
    }


/*** wgtrAddDeploymentMethod - associates a render function with a deployment method
 ***/
int
wgtrAddDeploymentMethod(char* method, int (*Render)(pFile, pObjSession, pWgtrNode, pStruct))
    {
	xhAdd(&(WGTR.Methods), nmSysStrdup(method), (void*)Render);
	return 0;
    }


/*** wgtrRender - call the appropriate function for the given deployment method
 ***/

int
wgtrRender(void* stream, int (*stream_write)(void*, char*, int, int, int), pObjSession obj_s, pWgtrNode tree, pStruct params, pWgtrClientInfo c_info, char* method)
    {
    int	    (*Render)(void*, int (*)(void*, char*, int, int, int), pObjSession, pWgtrNode, pStruct, pWgtrClientInfo);

	if ( (Render = (void*)xhLookup(&(WGTR.Methods), method)) == NULL)
	    {
	    mssError(1, "WGTR", "Couldn't render widget tree '%s': no render function for '%s'", tree->Name, method);
	    return -1;
	    }
	return Render(stream, stream_write, obj_s, tree, params, c_info);
    }


/*** wgtrImplementsInterface - adds an interface definition to the list if interfaces 
 *** implemented by this widget
 ***/
int 
wgtrImplementsInterface(pWgtrNode this, char* iface_ref)
    {
    IfcHandle h;

	if ( (h = ifcGetHandle(this->ObjSession, iface_ref)) == NULL)
	    {
	    mssError(0, "IBTN", "Couldn't get interface handle to '%s'", iface_ref);
	    return -1;
	    }
	xaAddItem(&(this->Interfaces), h);
	
	return 0;
    }


/*** wgtrSetDMPrivateData() - specify opaque data used by the deployment method
 *** module.  WGTR doesn't care what this data is, and does not manage it.
 ***/
int
wgtrSetDMPrivateData(pWgtrNode tree, void* data)
    {
    tree->DMPrivate = data;
    return 0;
    }


/*** wgtrGetDMPrivateData() - obtain the previously-set deployment method
 *** specific data value for the given widget.
 ***/
void*
wgtrGetDMPrivateData(pWgtrNode tree)
    {
    return tree->DMPrivate;
    }


/*** wgtrGetRootDName() - returns the deployment name of the root of the tree
 ***/
char*
wgtrGetRootDName(pWgtrNode tree)
    {
    return tree->Root->DName;
    }


/*** wgtrGetDName() - returns the deployment name of the specified node
 ***/
char*
wgtrGetDName(pWgtrNode tree)
    {
    return tree->DName;
    }


/*** wgtrRenameProperty() - if a property exists in a widget, then rename it
 *** to a new name.  This is used to provide (for a time) backwards compat.
 *** for deprecated property names.
 ***/
int
wgtrRenameProperty(pWgtrNode tree, char* oldname, char* newname)
    {
    int i, count;
    int oldid = -1;
    pObjProperty prop;

	count = xaCount(&(tree->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(tree->Properties), i);
	    if (prop && !strcmp(oldname, prop->Name))
		oldid = i;
	    if (prop && !strcmp(newname, prop->Name))
		{
		/** already exists! **/
		return -1;
		}
	    }

	if (oldid < 0) return 0;

	/** Rename it **/
	prop = xaGetItem(&(tree->Properties), oldid);
	strtcpy(prop->Name, newname, sizeof(prop->Name));

	mssError(1, "WGTR", "WARNING: deprecated property '%s' used for widget '%s'; please use '%s' in the future.", oldname, tree->Name, newname);

    return 0;
    }


/*** wgtrGetContainerHeight() - returns the height of the (visual) container
 *** of a widget.
 ***/
int
wgtrGetContainerHeight(pWgtrNode tree)
    {
    int c_height;
    do  {
	tree = tree->Parent;
	} while(tree->Parent && (tree->Flags & WGTR_F_NONVISUAL));
    c_height = tree->height - 2;
    if (!strcmp(tree->Type, "widget/childwindow"))
	c_height -= 24;
    else if (!strcmp(tree->Type, "widget/scrollpane"))
	c_height += 2;
    return c_height;
    }


/*** wgtrGetContainerWidth() - returns the width of the (visual) container
 *** of a widget.
 ***/
int
wgtrGetContainerWidth(pWgtrNode tree)
    {
    int c_width;
    do  {
	tree = tree->Parent;
	} while(tree->Parent && (tree->Flags & WGTR_F_NONVISUAL));
    c_width = tree->width - 2;
    if (!strcmp(tree->Type, "widget/scrollpane"))
	c_width = c_width + 2 - 18;
    return c_width;
    }


/*** wgtrGetTemplatePath() - return the path to the nth template used for
 *** a widget.  Up to WGTR_MAX_TEMPLATE templates are possible.
 ***/
char*
wgtrGetTemplatePath(pWgtrNode tree, int n)
    {
    if (n < 0 || n >= WGTR_MAX_TEMPLATE) return NULL;
    return tree->TemplatePaths[n];
    }


/*** wgtrMoveChildren() - given a widget tree node, adjust the positioning
 *** of all children by the given amount.  If some children are nonvisual,
 *** then this function adjusts the children in those nonvisual containers,
 *** as well.
 ***/
int
wgtrMoveChildren(pWgtrNode tree, int x_offset, int y_offset)
    {
    int cnt = xaCount(&tree->Children);
    int i;
    pWgtrNode child;

	for(i=0;i<cnt;i++)
	    {
	    child = xaGetItem(&tree->Children, i);
	    if (child->Flags & WGTR_F_NONVISUAL)
		{
		wgtrMoveChildren(child, x_offset, y_offset);
		}
	    else
		{
		child->x += x_offset;
		child->pre_x += x_offset;
		child->y += y_offset;
		child->pre_y += y_offset;
		}
	    }

    return 0;
    }

/*** wgtrRenderObject() - does a wgtrParseOpenObject, wgtrVerify, and
 *** wgtrRender, as well as handling any application defined parameters
 *** and expressions.
 ***/
int
wgtrRenderObject(void* stream, int (*stream_write)(void*, char*, int, int, int), pObjSession s, pObject obj, pStruct app_params, pWgtrClientInfo client_info, char* method)
    {
    pWgtrNode tree;
    int rval;


    if(! (tree = wgtrParseOpenObject(obj, app_params, client_info, 0)))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}
    if(! (wgtrMergeOverlays(tree, objGetPathname(obj), client_info->AppPath, client_info->Overlays, client_info->Templates) >= 0))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}
    if(! (wgtrVerify(tree, client_info) >= 0))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}

    rval = wgtrRender(stream, stream_write, s, tree, app_params, client_info, method);

    if(tree) wgtrFree(tree);
    return rval;
    }


/*** wgtrMergeOverlays() - given a list of overlays, merge them on top of the
 *** given widget tree, according to standard overlay/inherit semantics
 ***/
int
wgtrMergeOverlays(pWgtrNode tree, char* objpath, char* app_path, char* overlays[], char* templates[])
    {
    return 0;
    }

pWgtrNode
wgtrGetRoot(pWgtrNode tree)
    {
    return tree->Root;
    }

int
wgtr_internal_GetMatchingChildList_r(pWgtrNode parent, char* childtype, pWgtrNode* list, int* n_items, int max_items)
    {
    int i;
    pWgtrNode child;

	for(i=0;i<parent->Children.nItems;i++)
	    {
	    if (*n_items >= max_items) return 0;
	    child = (pWgtrNode)(parent->Children.Items[i]);
	    if (!childtype || !strcmp(child->Type, childtype))
		{
		list[(*n_items)++] = child;
		}
	    else if (child->Flags & WGTR_F_NONVISUAL)
		{
		wgtr_internal_GetMatchingChildList_r(child, childtype, list, n_items, max_items);
		}
	    }

    return 0;
    }

int
wgtrGetMatchingChildList(pWgtrNode parent, char* childtype, pWgtrNode* list, int max_items)
    {
    int n_matches;

	n_matches = 0;
	wgtr_internal_GetMatchingChildList_r(parent, childtype, list, &n_matches, max_items);

    return n_matches;
    }


int
wgtr_internal_GetMaxWidth_r(pWgtrNode widget, pWgtrNode search, int width, int height)
    {
    int cnt, i;
    pWgtrNode search_child;

	cnt = xaCount(&search->Children);
	for(i=0;i<cnt;i++)
	    {
	    search_child = xaGetItem(&search->Children, i);
	    if (search_child == widget) continue;

	    if (!(search_child->Flags & WGTR_F_NONVISUAL))
		{
		if (search_child->y <= widget->y + height &&
		    search_child->y + search_child->height >= widget->y &&
		    search_child->x > widget->x && search_child->x < widget->x + width)
		    {
		    width = search_child->x - widget->x;
		    }
		}
	    else if (search_child->Flags & WGTR_F_CONTAINER)
		{
		width = wgtr_internal_GetMaxWidth_r(widget, search_child, width, height);
		}
	    }

    return width;
    }


/*** wgtrGetMaxWidth - determine the maximum width that a widget of the given
 *** height can be without overlapping other visual widgets.
 ***/
int
wgtrGetMaxWidth(pWgtrNode widget, int height)
    {
    int w;
    pWgtrNode parent;

	/** absolute max width = container width minus x location **/
	parent = widget->Parent;
	while (parent && parent->Parent && (parent->Flags & WGTR_F_NONVISUAL))
	    parent = parent->Parent;
	if(!parent) return 0;
	w = parent->width - widget->x;

	/** search siblings and descendents of siblings **/
	w = wgtr_internal_GetMaxWidth_r(widget, parent, w, height);

    return w;
    }


int
wgtr_internal_GetMaxHeight_r(pWgtrNode widget, pWgtrNode search, int width, int height)
    {
    int cnt, i;
    pWgtrNode search_child;

	cnt = xaCount(&search->Children);
	for(i=0;i<cnt;i++)
	    {
	    search_child = xaGetItem(&search->Children, i);
	    if (search_child == widget) continue;

	    if (!(search_child->Flags & WGTR_F_NONVISUAL))
		{
		if (search_child->x <= widget->x + width &&
		    search_child->x + search_child->width >= widget->x &&
		    search_child->y > widget->y && search_child->y < widget->y + height)
		    {
		    height = search_child->y - widget->y;
		    }
		}
	    else if (search_child->Flags & WGTR_F_CONTAINER)
		{
		height = wgtr_internal_GetMaxHeight_r(widget, search_child, width, height);
		}
	    }

    return height;
    }


/*** wgtrGetMaxHeight - determine the maximum height that a widget of the given
 *** width can be without overlapping other visual widgets.
 ***/
int
wgtrGetMaxHeight(pWgtrNode widget, int width)
    {
    int h;
    pWgtrNode parent;

	/** absolute max width = container width minus x location **/
	parent = widget->Parent;
	while (parent && parent->Parent && (parent->Flags & WGTR_F_NONVISUAL))
	    parent = parent->Parent;
	if(!parent) return 0;
	h = parent->height - widget->y;

	/** search siblings and descendents of siblings **/
	h = wgtr_internal_GetMaxHeight_r(widget, parent, width, h);
	if (h > WGTR_DEFAULT_SPACING)
	    {
	    h -= WGTR_DEFAULT_SPACING;
	    }

    return h;
    }


/*** wgtrGetName - return the name of the widget
 ***/
char*
wgtrGetName(pWgtrNode widget)
    {
    return widget->Name;
    }


/*** wgtrGetNamespace - return the namespace of the widget
 ***/
char*
wgtrGetNamespace(pWgtrNode widget)
    {
    return widget->Namespace;
    }


