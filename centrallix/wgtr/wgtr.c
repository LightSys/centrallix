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

/**CVSDATA***************************************************************

 **END-CVSDATA***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "wgtr.h"
#include "xarray.h"
#include "datatypes.h"
#include "magic.h"
#include "xhash.h"
#include "ht_render.h"


/** Generic property struct **/
typedef struct
    {
    int		    Magic;			/** Magic Number **/
    void*	    Reserved;
    char	    Name[64];
    int		    Type;
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



/** Widget Driver information structure **/
typedef struct
    {
    char	    Name[64];		    /* Driver name */
    int		    (*Verify)();	    /* Function for verifying the widget */
    int		    (*New)();		    /* Function for initializing a widget */
    XArray	    Types;		    /* Pseudo-types this widget driver will handle */
    } WgtrDriver, *pWgtrDriver;

struct
    {
    XHashTable	    Methods;		    /* deployment methods */
    XArray	    Drivers;		    /* simple driver listing */
    } WGTR;



pWgtrDriver
wgtr_internal_LookupDriver(pWgtrNode node)
    {
    int i, j;
    pWgtrDriver drv;
    char* this_type;

	for (i=0;i<xaCount(&(WGTR.Drivers));i++)
	    {
	    drv = xaGetItem(&(WGTR.Drivers), i);
	    for (j=0;j<xaCount(&(drv->Types));j++)
		{
		this_type = xaGetItem(&(drv->Types), j);
		if (!strncmp(node->Type+7, this_type, 64)) return drv;
		}
	    }
	mssError(1, "WGTR", "No driver registered for type '%s'", node->Type+7);
	return NULL;
    }


pWgtrNode 
wgtrParseObject(pObjSession s, char* path, int mode, int permission_mask, char* type)
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
	results = wgtrParseOpenObject(obj);
	objClose(obj);
        return results;
    }


pWgtrNode 
wgtrParseOpenObject(pObject obj)
    {
    pWgtrNode	this_node, child_node;
    char   name[64], type[64], * prop_name;
    int rx, ry, rwidth, rheight;
    int flx, fly, flwidth, flheight;
    int prop_type;
    ObjData	val;
    pObject child_obj;
    pObjQuery qy;

	/** check the outer_type of obj tobe sure it's a widget **/
	if (objGetAttrValue(obj, "outer_type", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "WGTR", "Couldn't get outer_type for %s", obj->Pathname->Pathbuf);
	    return NULL;
	    }
	
	if (strncmp(val.String, "widget/", 7) != 0)
	    {
	    mssError(1, "WGTR", "Object %s is not a widget", obj->Pathname->Pathbuf);
	    return NULL;
	    }
	strncpy(type, val.String, 64);

	/** get the name, r_*, fl_*, from calls to the OSML **/
	if (objGetAttrValue(obj, "name", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "WGTR", "couldn't get 'type' for %s", obj->Pathname->Pathbuf);
	    return NULL;
	    }
	strncpy(name, val.String, 64);

	/** create this node **/
	rx = ry = rwidth = rheight = flx = fly = flwidth = flheight = -1;
	if ( (this_node = wgtrNewNode(name, type, obj->Session, -1, -1, -1, -1, 100, 100, 100, 100)) == NULL)
	    {
	    mssError(0, "WGTR", "Couldn't create node %s", name);
	    return NULL;
	    }

	/** loop through attributes to fill out the properties array **/
	prop_name = objGetFirstAttr(obj);
	while (prop_name)
	    {
	    /** Get the type **/
	    if ( (prop_type = objGetAttrType(obj, prop_name)) < 0) 
		{
		mssError(0, "WGTR", "Couldn't get type for property %s", prop_name);
		goto error;
		}
	    /** get the value **/ 
	    if ( objGetAttrValue(obj, prop_name, prop_type, &val) < 0)
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
		else wgtrAddProperty(this_node, prop_name, prop_type, &val);
		}
	    else wgtrAddProperty(this_node, prop_name, prop_type, &val);

	    /** get the name of the next one **/
	    prop_name = objGetNextAttr(obj);
	    }
	
	/** initialize all other struct members **/
	this_node->x = this_node->r_x;
	this_node->y = this_node->r_y;
	this_node->width = this_node->r_width;
	this_node->height = this_node->r_height;
	
	/** loop through subobjects, and call ourselves recursively to add child nodes **/
	if ( (qy = objOpenQuery(obj, "", NULL, NULL, NULL)) != NULL)
	    {
	    while ( (child_obj = objQueryFetch(qy, O_RDONLY)))
		{
		if ( (child_node = wgtrParseOpenObject(child_obj)) != NULL) wgtrAddChild(this_node, child_node);
		else
		    {
		    mssError(0, "WGTR", "Couldn't parse subobject '%s'", child_obj->Pathname->Pathbuf);
		    objClose(child_obj);
		    objQueryClose(qy);
		    goto error;
		    }
		objClose(child_obj);
		}
	    }
	objQueryClose(qy);
	/** return the struct **/
	return this_node;

    error:
	wgtrFree(this_node);
	return NULL;
    }


void wgtr_internal_FreeProperty(pObjProperty prop)
    {
	if (!prop) return;

	switch (prop->Type)
	    {
	    case DATA_T_STRING:
		nmSysFree(prop->Val.String);
		break;
	    case DATA_T_CODE:
		expFreeExpression((pExpression)prop->Val.Generic);
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
	else if (!strcmp(name, "type") || !strcmp(name, "outer_type")) return DATA_T_STRING;
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
	    if (!strcmp(name, "x")) { val->Integer = widget->x; return 0; }
	    if (!strcmp(name, "y")) { val->Integer = widget->y; return 0; }
	    if (!strcmp(name, "width")) { val->Integer = widget->width; return 0; }
	    if (!strcmp(name, "height")) { val->Integer = widget->height; return 0; }
	    if (!strncmp(name, "r_", 2))
		{
		if (!strcmp(name+2, "x")) { val->Integer = widget->r_x; return 0; }
		if (!strcmp(name+2, "y")) { val->Integer = widget->r_y; return 0; }
		if (!strcmp(name+2, "width")) { val->Integer = widget->r_width; return 0; }
		if (!strcmp(name+2, "height")) { val->Integer = widget->r_height; return 0; }
		}
	    else if (!strncmp(name, "fl_", 3))
		{
		if (!strcmp(name+2, "x")) { val->Integer = widget->fl_x; return 0; }
		if (!strcmp(name+2, "y")) { val->Integer = widget->fl_y; return 0; }
		if (!strcmp(name+2, "width")) { val->Integer = widget->fl_width; return 0; }
		if (!strcmp(name+2, "height")) { val->Integer = widget->fl_height; return 0; }
		}
	    }
	else if (datatype == DATA_T_STRING)
	    {
	    if (!strcmp(name, "name")) { val->String = widget->Name; return 0; }
	    else if (!strcmp(name, "type") || !strcmp(name, "outer_type")) { val->String = widget->Type; return 0; }
	    }
	    
	/** if we didn't find it there, loop through the list of properties until we do **/
	count = xaCount(&(widget->Properties));
	for (i=0;i<count;i++)
	    {
	    prop = xaGetItem(&(widget->Properties), i);
	    if (prop && !strcmp(name, prop->Name)) break;
	    }
	if (i == count || datatype != prop->Type) return -1;

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
wgtrAddProperty(pWgtrNode widget, char* name, int datatype, pObjData val)
    /** XXX Should this check for duplicates? **/
    {
    pObjProperty prop;

	ASSERTMAGIC(widget, MGK_WGTR);
	/** Get the memory **/
	if ( (prop = (pObjProperty)nmMalloc(sizeof(ObjProperty))) == NULL) return -1;

	memset(prop, 0, sizeof(ObjProperty));
	strncpy(prop->Name, name, 64);
	prop->Type = datatype;
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
    pWgtrDriver drv;

	if ( (node = (pWgtrNode)nmMalloc(sizeof(WgtrNode))) == NULL)
	    {
	    mssError(0, "WGTR", "Couldn't allocate memory for new node");
	    return NULL;
	    }
	memset(node, 0, sizeof(WgtrNode));
	SETMAGIC(node, MGK_WGTR);

	strncpy(node->Name, name, 64);
	strncpy(node->Type, type, 64);
	node->x = node->r_x = rx;
	node->y = node->r_y = ry;
	node->width = node->r_width = rwidth;
	node->height = node->r_height = rheight;
	node->fl_x = flx;
	node->fl_y = fly;
	node->fl_width = flwidth;
	node->fl_height = flheight;
	node->ObjSession = s;

	xaInit(&(node->Properties), 16);
	xaInit(&(node->Children), 16);
	xaInit(&(node->Interfaces), 8);

	/** look up the 'new' function and call it on the now-init'd struct **/
	if ( (drv = wgtr_internal_LookupDriver(node)) == NULL) 
	    {
	    wgtrFree(node);
	    return NULL;
	    }
	if (drv->New(node) < 0)
	    {
	    mssError(1, "WGTR", "Error initializing new widget node '%s'", node->Name);
	    wgtrFree(node);
	    return NULL;
	    }

	return node;
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
	    exp = expPodToExpression(&pod, t);
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
	if (1 == DATA_T_STRING || t == DATA_T_STRINGVEC)
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

		if (t == DATA_T_STRING || i >= od.StringVec->nStrings-1); break;
		i++;
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
	fprintf(stderr, "Widget: %s Type: %s\n", tree->Name, tree->Type);

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
wgtrVerify(pWgtrNode tree, int minw, int minh, int maxw, int maxh)
    {
    WgtrVerifySession vs;
    pWgtrDriver drv;
    XArray Names;
    int i;

	/** initialize datastructures **/
	vs.Tree = tree;
	xaInit(&(vs.VerifyQueue), 128);
	xaInit(&Names, 128);

	/** Set top-level width and height **/
	if (tree->width < minw) tree->width = minw;
	if (tree->width > maxw) tree->width = maxw;
	if (tree->height < minh) tree->height = minh;
	if (tree->height > maxh) tree->height = maxh;

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
		if (!strcmp(vs.CurrWidget->Name, xaGetItem(&Names, i)))
		    {
		    mssError(1, "WGTR", "Widget name '%s' is not unique - widget names must be unique within an application",
				vs.CurrWidget->Name);
		    goto error;
		    }
		}
	    xaAddItem(&Names, vs.CurrWidget->Name);

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

	/** free up data structures **/
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
	xhInit(&(WGTR.Methods), 5, 0);

	/** call the initialization routines of all the widget drivers. I suppose it's a
	 ** little weird to do things this way - if they're going to be init'ed from here,
	 ** why have 'drivers'? Why not just hard-code them? Well, maybe later on they'll
	 ** become dynamically loaded modules. Then it'll make more sense
	 **/
	wgtalrtInitialize();
	wgtcaInitialize();
	wgtcbInitialize();
	wgtclInitialize();
	wgtcmpdInitialize();
	wgtconnInitialize();
	wgtdtInitialize();
	wgtddInitialize();
	wgtebInitialize();
	wgtexInitialize();
	wgtfbInitialize();
	wgtformInitialize();
	wgtfsInitialize();
	wgtsetInitialize();
	wgthintInitialize();
	wgthtmlInitialize();
	wgtibtnInitialize();
	wgtimgInitialize();
	wgtlblInitialize();
	wgtmenuInitialize();
	wgtosrcInitialize();
	wgtpageInitialize();
	wgtpnInitialize();
	wgtrbInitialize();
	wgtrmtInitialize();
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
	    if (!strcmp(drv->Name, ((pWgtrDriver)xaGetItem(&(WGTR.Drivers), i))->Name))
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
	strncpy(drv->Name, name, 64);
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
wgtrRender(pFile output, pObjSession obj_s, pWgtrNode tree, pStruct params, char* method)
    {
    int	    (*Render)(pFile, pObjSession, pWgtrNode, pStruct);

	if ( ((void*)Render = xhLookup(&(WGTR.Methods), method)) == NULL)
	    {
	    mssError(1, "WGTR", "Couldn't render widget tree '%s': no render function for '%s'", tree->Name, method);
	    return -1;
	    }
	return Render(output, obj_s, tree, params);
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

