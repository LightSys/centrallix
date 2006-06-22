
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
/* Module: 	Interface Module     					*/
/* Author:	Matt McGill (MJM)					*/
/* Creation:	July 26, 2004   					*/
/* Description:	Provies the functionality for handling interfaces in a  */
/*		general way, to be used by any Centrallix module that   */
/*		has need of support for interfaces.                    	*/
/************************************************************************/

/**CVSDATA***************************************************************

 **END-CVSDATA***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iface.h"
#include "iface_private.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "obj.h"
#include "centrallix.h"
#include "ht_render.h"

struct _IFC IFC;

/************* Functions ***********************/

/*** ifc_internal_FreeMajorVersion - frees an internal represntatino of a major version
 ***/
void
ifc_internal_FreeMajorVersion(pIfcMajorVersion v, int type)
    {
    int i, j;
	if (v)
	    {
	    if (v->Members)
		{
		for (i=0;i<IFC.NumCategories[type];i++)
		    {
		    for (j=0;j<xaCount(&(v->Members[i]));j++) nmSysFree(xaGetItem(&(v->Members[i]), j));
		    xaDeInit(&(v->Members[i]));
		    }
		nmSysFree(v->Members);
		}
	    if (v->Properties)
		{
		for (i=0;i<IFC.NumCategories[type];i++)
		    {
		    for (j=0;j<xaCount(&(v->Properties[i]));j++) nmSysFree(xaGetItem(&(v->Properties[i]), j));
		    xaDeInit(&(v->Properties[i]));
		    }
		nmSysFree(v->Properties);
		}
	    if (v->Offsets)
		{
		for (i=0;i<IFC.NumCategories[type];i++) xaDeInit(&(v->Offsets[i]));
		nmSysFree(v->Offsets);
		}
	    nmFree(v, sizeof(IfcMajorVersion));
	    }
	if (IFC_DEBUG)
	    fprintf(stderr, "ifc_internal_FreeMajorVersion()\n");
    }


#if 0
/*** ifc_internal_VerifyProperties - recursive function to be sure that one properties structure
 *** does not break backwards compatibility with another properties structure (though the new
 *** one can *add* functionality. Returns 1 if properties are ok, 0 otherwise.
 ***/
int
ifc_internal_VerfifyProperties(pObject OldProp, pObject NewProp)
    {
    pObject OldSubObj, NewSubObj;
    ObjData OldVal, NewVal;
    char    *OldName, *NewName;
    int	    OldType, NewType;

    /** check attributes **/
    OldName = objGetFirstAttr(OldProp);
    while (OldName != NULL)
	{
	OldType = objGetAttrType(OldProp, OldName);
	/** make sure the attribute is there **/
	if ( (NewType = objGetAttrType(NewProp, OldName)) < 0)
	    {
	    mssError(1, "IFC", "Break in backward-compatibility: '%s' property has attribute '%s', '%s' doesn't",
		OldProp->Pathname->Pathbuf+1, OldName, NewProp->Pathname->Pathbuf+1);
	    return 0;
	    }
	/** make sure the types match **/
	if ( NewType != OldType)
	    {
	    mssError(1, "IFC", "Break in backward-compatibility: type of attribute '%s' does not match between '%s' and '%s'",
		OldName, OldProp->Pathname->Pathbuf+1, NewProp->Pathname->Pathbuf+1);
	    return 0;
	    }
	/** make sure the values match **/
	objGetAttrValue(OldProp, OldName, OldType, &OldVal);
	objGetAttrValue(NewProp, OldName, OldType, &NewVal);
	switch (OldType)
	    {
	    case DATA_T_INTEGER:
	    case DATA_T_STRING:
	    case DATA_T_DOUBLE:
	    case DATA_T_MONEY:
	    case DATA_T_DATETIME:

	    }
	}


    /** check subobjects **/

    return 1;
    }
#endif

/*** ifc_internal_NewMajorVersion - takes an open object representing a single major version
 *** of an interface definition and creates an equivelent internal representation of that
 *** major version
 ***/
pIfcMajorVersion 
ifc_internal_NewMajorVersion(pObject def, int type)
    {
    int			    NumCategories, i, ExpectedMinorVersion, HighestMinorVersion, ThisMinorVersion,
			    MemberCategory, ThisOffset, NumMinorVersions;
    pIfcMajorVersion	    MajorVersion=NULL;
    pObject		    MinorObj=NULL, MemberObj=NULL;
    pObjQuery		    MinorQy=NULL, MemberQy=NULL;
    char		    *ptr, MemberName[64];
    ObjData		    Val;

	if (IFC_DEBUG)
	    fprintf(stderr, "ifc_internal_NewMajorVersion([%s], %d)\n", def->Pathname->Pathbuf, type);

	/** initialize local datastructures **/
	NumCategories = IFC.NumCategories[type];

	/** allocate memory for and initialize members of the new major version struct **/
	if ( (MajorVersion = nmMalloc(sizeof(IfcMajorVersion))) == NULL) goto error;
	memset(MajorVersion, 0, sizeof(IfcMajorVersion));
	if ( (MajorVersion->Members = nmSysMalloc(sizeof(XArray)*NumCategories)) == NULL) goto error;
	if ( (MajorVersion->Properties = nmSysMalloc(sizeof(XArray)*NumCategories)) == NULL) goto error;
	if ( (MajorVersion->Offsets = nmSysMalloc(sizeof(XArray)*NumCategories)) == NULL) goto error;
	for (i=0;i<NumCategories;i++)
	    {
	    xaInit(&(MajorVersion->Members[i]), 8);
	    xaInit(&(MajorVersion->Properties[i]), 8);
	    xaInit(&(MajorVersion->Offsets[i]), 8);
	    }
	
	/********* initial processing of each minor version **********/

	if ( (MinorQy = objOpenQuery(def, ":outer_type = 'iface/minorversion'", NULL, NULL, NULL)) == NULL)
	    {
	    mssError(0, "IFC", "Couldn't retrieve minor versions of major version '%s'", def->Pathname->Pathbuf+1);
	    goto error;
	    }

	HighestMinorVersion = -1;
	NumMinorVersions = 0;
	while ( (MinorObj = objQueryFetch(MinorQy, O_RDONLY)) != NULL)
	    {
	    /** process the name, make sure it's legal **/
	    if (objGetAttrValue(MinorObj, "name", DATA_T_STRING, &Val) < 0)
		{
		mssError(0, "IFC", "Couldn't get name for a minor version within '%s'", def->Pathname->Pathbuf+1);
		goto error;
		}
	    if (Val.String[0] != 'v' || ( (i=strtol(Val.String+1, &ptr, 10))==0 && ptr != Val.String+strlen(Val.String)))
		{
		mssError(0, "IFC", "Ilegal minor version name '%s' in '%s'", Val.String, def->Pathname->Pathbuf+1);
		goto error;
		}
	    if (IFC_DEBUG)
		fprintf(stderr, "Initial processing of '%s' minor version %d\n", def->Pathname->Pathbuf+1, i);
	    if (HighestMinorVersion < i) HighestMinorVersion = i;
	    /** check for duplicate minor versions **/
	    else if (HighestMinorVersion == i)
		{
		mssError(0, "IFC", "Duplicate entries for minor version %d in '%s'", i, def->Pathname->Pathbuf+1);
		goto error;
		}
	    NumMinorVersions++;
	    objClose(MinorObj);
	    MinorObj = NULL;
	    }
	objQueryClose(MinorQy);
	MinorQy = NULL;
	MajorVersion->NumMinorVersions = NumMinorVersions;
	
	/******* Second pass through minor versions, filling out major version struct **********/
	if ( (MinorQy = objOpenQuery(def, ":outer_type = 'iface/minorversion'", ":name desc", NULL, NULL)) == NULL)
	    {
	    mssError(0, "IFC", "Couldn't retrieve minor versions of major version '%s'", def->Pathname->Pathbuf+1);
	    goto error;
	    }

	/** I'm uncertain as to whether or not doing things in two passes could introduce
	 ** concurrency problems. I suppose it could, if there's a context switch somewhere
	 ** and the structure file containing the definition is modified to be incorrect.
	 ** I don't see an easy way to solve this problem.
	 **/
	ExpectedMinorVersion = HighestMinorVersion;
	while ( (MinorObj = objQueryFetch(MinorQy, O_RDONLY)) != NULL)
	    {
	    /** Get the version name **/
	    objGetAttrValue(MinorObj, "name", DATA_T_STRING, &Val);
	    ThisMinorVersion = atoi(Val.String+1);
	    if (IFC_DEBUG)
		fprintf(stderr, "Second processing, minor version %d\n", ThisMinorVersion);
	    if (ExpectedMinorVersion < 0)
		{
		mssError(1, "IFC", "Trying to process too many minor versions for '%s'!", def->Pathname->Pathbuf);
		goto error;
		}
	    /** make sure the version is the one we expect **/
	    if (ThisMinorVersion != ExpectedMinorVersion)
		{
		mssError(1, "IFC", "In '%s', missing minor version %d", def->Pathname->Pathbuf, ExpectedMinorVersion);
		goto error;
		}
	    /** Set all offsets **/
	    for (i=0;i<IFC.NumCategories[type];i++)
		xaSetItem(&(MajorVersion->Offsets[i]), ThisMinorVersion, (void*)xaCount(&(MajorVersion->Members[i])));
	    /** Process the members **/
	    if ( (MemberQy = objOpenQuery(MinorObj, NULL, NULL, NULL, NULL)) == NULL)
		{
		mssError(0, "IFC", "Could not retrieve the members of '%s'", MinorObj->Pathname->Pathbuf+1);
		goto error;
		}
	    while ( (MemberObj = objQueryFetch(MemberQy, O_RDONLY)) != NULL)
		{
		/** get the member name **/
		if (objGetAttrValue(MemberObj, "name", DATA_T_STRING, &Val)<0)
		    {
		    mssError(0, "IFC", "Couldn't retrieve name from '%s'", MemberObj->Pathname->Pathbuf+1);
		    goto error;
		    }
		strncpy(MemberName, Val.String, 64);
		/** check member type, get category **/
		if(objGetAttrValue(MemberObj, "outer_type", DATA_T_STRING, &Val)<0)
		    {
		    mssError(0, "IFC", "Couldn't retrieve Type from '%s'", MemberObj->Pathname->Pathbuf+1);
		    goto error;
		    }
		if (strncmp("iface/", Val.String, 6))
		    {
		    mssError(0, "IFC", "Type of '%s' must be 'iface/<category>", MemberObj->Pathname->Pathbuf+1);
		    goto error;
		    }
		for (MemberCategory=0;MemberCategory<NumCategories;MemberCategory++) 
		    if (!strcmp(IFC.CategoryNames[type][MemberCategory], Val.String+6)) break;
		if (MemberCategory == NumCategories)
		    {
		    mssError(0, "IFC", "For member '%s': category '%s' not valid for interface type '%s'", 
			MemberObj->Pathname->Pathbuf+1, Val.String+6, IFC.TypeNames[type]);
		    goto error;
		    }
		/** check for duplicates **/
		for (i=0;i<xaCount(&(MajorVersion->Members[MemberCategory]));i++)
		    {
		    if (!strcmp(xaGetItem(&(MajorVersion->Members[MemberCategory]), i), MemberName))
			{
			/** is the duplicate within this minor version, or in another minor version? **/
			if (i >= ThisOffset)
			    /** within this minor version **/
			    {
			    mssError(1, "IFC", "In '%s', member '%s' is duplicated", MinorObj->Pathname->Pathbuf+1,
				MemberName);
			    goto error;
			    }
			else
			    /** in another minor version **/
			    {
			    /** FIXME Right now this simply disallows duplicates. What it *should* do is
			     ** somehow figure out how to make properties cumulative like members are,
			     ** so that properties of a member can be added to in future minor versions.
			     ** FIXME
			     **/
			    mssError(1, "IFC", "In '%s', member '%s' is duplicated", MinorObj->Pathname->Pathbuf+1,
				MemberName);
			    goto error;
			    }
			}   /** end if (i >= ThisOffset) **/
		    } /** end for (i=0;i<xaCount(&(MajorVersion->Members[... **/
		/** add member to Members array **/
		if (IFC_DEBUG)
		    fprintf(stderr, "Adding '%s' to '%s'\n", MemberName, def->Pathname->Pathbuf);
		xaAddItem(&(MajorVersion->Members[MemberCategory]), nmSysStrdup(MemberName));
		xaAddItem(&(MajorVersion->Properties[MemberCategory]), nmSysStrdup(MemberObj->Pathname->Pathbuf+1));
		objClose(MemberObj);
		MemberObj = NULL;
		} /** end while ( (MemberObj = ... **/
	    ExpectedMinorVersion--;

	    objQueryClose(MemberQy);
	    MemberQy = NULL;
	    objClose(MinorObj);
	    MinorObj = NULL;
	    } /** end while ( (MinorObj = ... */

	objQueryClose(MinorQy);
	MinorQy = NULL;

	return MajorVersion;

    error:
	if (MemberObj) objClose(MemberObj);
	if (MemberQy) objQueryClose(MemberQy);
	if (MinorObj) objClose(MinorObj);
	if (MinorQy) objQueryClose(MinorQy);
	if (MajorVersion) ifc_internal_FreeMajorVersion(MajorVersion, type);
	return NULL;
    }


/*** ifc_internal_FreeIfcDef - frees an internal representation of an interface definition
 ***/
void 
ifc_internal_FreeIfcDef(pIfcDefinition def)
    {
    int i;
    pIfcMajorVersion major;

	if (IFC_DEBUG)
	    fprintf(stderr, "ifc_internal_FreeIfcDef([%s])\n", def->Path);
	objClose(def->Obj);
	nmSysFree(def->Path);
	for (i=0;i<xaCount(&def->MajorVersions);i++)
	    if ( (major=xaGetItem(&(def->MajorVersions), i)) != NULL)
		ifc_internal_FreeMajorVersion(major, def->Type);
	xaDeInit(&(def->MajorVersions));
	nmFree(def, sizeof(IfcDefinition));
    }


/*** ifc_internal_NewIfcDef - takes a path to an interface definition and creates an
 *** internal representation of that interface
 ***/
pIfcDefinition 
ifc_internal_NewIfcDef(pObjSession s, char* path)
    {
    pIfcDefinition def;
    pObject obj;
    ObjData val;
    pObjQuery qy;
    pIfcMajorVersion v;
    int t, vnum;


	if (IFC_DEBUG)
	    fprintf(stderr, "ifc_internal_NewIfcDef('%s')\n", path);
    
	/** open top-level object **/
	if ( (obj = objOpen(s, path, O_RDONLY, 0600, "system/structure")) == NULL)
	    {
	    mssError(0, "IFC", "Couldn't open '%s' via the OSML", path);
	    return NULL;
	    }

	/** check its outer_type **/
	if (objGetAttrValue(obj, "outer_type", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "IFC", "Couldn't get outer_type for '%s'", path);
	    objClose(obj);
	    return NULL;
	    }
	if (strcmp(val.String, "iface/definition"))
	    {
	    mssError(0, "IFC", "'%s' must have an outer_type of 'iface/definition' (was %s)", path, val.String);
	    objClose(obj);
	    return NULL;
	    }

	/** get interface type **/
	if (objGetAttrValue(obj, "type", DATA_T_STRING, &val) < 0)
	    {
	    mssError(0, "IFC", "no interface type for '%s'", path);
	    objClose(obj);
	    return NULL;
	    }
	for (t=0;t<IFC_NUM_TYPES;t++)
	    {
	    if (!strcmp(val.String, IFC.TypeNames[t])) break;
	    }
	if (t == IFC_NUM_TYPES)
	    {
	    mssError(0, "IFC", "invalid type '%s' for '%s'", val.String, path);
	    objClose(obj);
	    return NULL;
	    }

	/** get memory and initialize struct **/
	def = nmMalloc(sizeof(IfcDefinition));
	def->Obj = obj;
	def->Path = nmSysStrdup(path);
	def->Type = t;
	def->ObjSession = s;
	xaInit(&(def->MajorVersions), 4);

	/** parse all major versions **/
	qy = objOpenQuery(def->Obj, ":outer_type='iface/majorversion'", NULL, NULL, NULL);
	while ( (obj = objQueryFetch(qy, O_RDONLY)) != NULL)
	    {
	    /** get name, and from it the version number **/
	    if ( objGetAttrValue(obj, "name", DATA_T_STRING, &val) < 0 || val.String[0] != 'v' ||
		 (vnum = atoi(val.String+1)) == 0)
		{
		mssError(1, "IFC", "Invalid major version number '%s' for '%s'", val.String, path);
		objClose(obj);
		objQueryClose(qy);
		ifc_internal_FreeIfcDef(def);
		return NULL;
		}
	    if ( (v = ifc_internal_NewMajorVersion(obj, t)) != NULL)
		xaSetItem(&(def->MajorVersions), vnum, v);
	    else
		{
		ifc_internal_FreeIfcDef(def);
		mssError(0, "IFC", "Couldn't parse major version '%s'", obj->Pathname->Elements[obj->Pathname->nElements-1]);
		objQueryClose(qy);
		objClose(obj);
		return NULL;
		}
	    objClose(obj);
	    }
	objQueryClose(qy);
	
	return def;
    }

/*** ifcReleaseHandle - release an interface handle
 ***/
void
ifcReleaseHandle(IfcHandle handle)
    {
	if (IFC_DEBUG) 
	    fprintf(stderr, "ifcReleaseHandle('%s')\n", handle->DefPath);
/*	ifc_internal_Print(handle); */

	/** release the memory for every member and property path **/
	if (handle->LinkCount == 1)
	    {
	    /** remove this guy from the handle cache **/
	    xhRemove(&IFC.HandleCache, handle->FullPath);
	    nmSysFree(handle->FullPath);
	    nmSysFree(handle->Offsets);
	    nmFree(handle, sizeof(struct IfcHandle_t));
	    }
	else handle->LinkCount--;
    }


/*** ifc_internal_BuildHandle - builds a handle from a given interface definition
 ***/
IfcHandle
ifc_internal_BuildHandle(pIfcDefinition def, int major, int minor)
    {
    IfcHandle h;
    pIfcMajorVersion maj_v;
    int i;
    XString fullpath;

	if (IFC_DEBUG)
	    fprintf(stderr, "ifc_internal_BuildHandle(%s, %d, %d)\n", def->Path, major, minor);

	/** make sure the major version we need is there **/
	if ( major >= xaCount(&(def->MajorVersions)) || (maj_v = xaGetItem(&(def->MajorVersions), major)) == NULL)
	    {
	    mssError(1, "IFC", "Interface definition '%s' does not contain a major version %d", def->Path, major);
	    return NULL;
	    }
	
	/** allocate memory for handle **/
	if ( (h = nmMalloc(sizeof(struct IfcHandle_t))) == NULL)
	    {
	    mssError(1, "IFC", "Couldn't allocate memory for handle");
	    return NULL;
	    }
	if ( (h->Offsets = nmSysMalloc(sizeof(int)*IFC.NumCategories[def->Type])) == NULL)
	    {
	    nmFree(h, sizeof(struct IfcHandle_t));
	    mssError(1, "IFC", "Couldn't allocate memory for handle");
	    return NULL;
	    }
	h->DefPath = def->Path;
	xsInit(&fullpath);
	xsConcatPrintf(&fullpath, "%s?cx__version=%d.%d", def->Path, major, minor);
	h->FullPath = nmSysStrdup(fullpath.String);
	xsDeInit(&fullpath);
	h->ObjSession = def->ObjSession;
	h->MajorVersion = major;
	h->MinorVersion = minor;
	h->LinkCount = 1;
	h->Type = def->Type;
	h->Members = maj_v->Members;
	h->Properties = maj_v->Properties;

	/** get offsets into member and property arrays **/
	for (i=0;i<IFC.NumCategories[def->Type];i++)
	    {
	    if ( (h->Offsets[i] = (int)xaGetItem(&(maj_v->Offsets[i]), minor)) < 0)
		{
		mssError(1, "IFC", "'%s/v%d' does not contain a minor version %d", def->Path, major, minor);
		ifcReleaseHandle(h);
		return NULL;
		}
	    }


	return h;
    }

/*** ifcGetHandle - given an OSML path, returns a handle to the interface at that path
 ***/
IfcHandle
ifcGetHandle(pObjSession s, char* ref)
    {
    IfcHandle handle;
    char path[512], *ver, *ptr;
    int major, minor;
    pIfcDefinition def;

	if (IFC_DEBUG)
	    fprintf(stderr, "ifcGetHandle('%s')\n", ref);

	/** get the absolute path **/
	if (ref[0] == '/') strncpy(path, ref, 512);
	else snprintf(path, 512, "%s/%s", IFC.IfaceDir, ref);

	/** check for a cached handle **/
	if ( (handle = (IfcHandle)xhLookup(&(IFC.HandleCache), path)) != NULL)
	    {
	    handle->LinkCount++;
	    return handle;
	    }

	/** split path into interface and version info **/
	ver = strchr(path, '?');
	if (ver == NULL)
	    {
	    mssError(1, "IFC", "Tried to get handle, but didn't specify major or minor version!");
	    return NULL;
	    }
	if (ver == path)
	    {
	    mssError(1, "IFC", "Syntax error: reference to handle can't start with '?'");
	    return NULL;
	    }
	ver[0] = '\0';
	ver++;
	if ( (ptr = strtok(ver, "=")) == NULL || strcmp(ptr, "cx__version") || (ptr = strtok(NULL, ".")) == NULL )
	    {
	    mssError(1, "IFC", "Syntax error - couldn't parse interface reference");
	    return NULL;
	    }
	major = atoi(ptr);
	if ( (ptr = strtok(NULL, ".")) == NULL)
	    {
	    mssError(1, "IFC", "Syntax error: invalid version number");
	    return NULL;
	    }
	minor = atoi(ptr);

	/** check to see if the definition has already been parsed **/
	if ( (def = (pIfcDefinition)xhLookup(&(IFC.Definitions), path)) == NULL)
	    {
	    /** parse the interface definition **/
	    if ( (def = ifc_internal_NewIfcDef(s, path)) == NULL)
		{
		mssError(0, "IFC", "Couldn't parse interface definition '%s'", path);
		return NULL;
		}
	    if (xhAdd(&(IFC.Definitions), def->Path, (char*)def) < 0) mssError(0, "IFC", "Couldn't cache");
	    }
	/** we have the interface definition. Now let's build a handle from it **/
	if ( (handle = ifc_internal_BuildHandle(def, major, minor)) == NULL)
	    {
	    mssError(0, "IFC", "Coulnd't build handle from interface definition '%s'", path);
	    return NULL;
	    }
	/** get the absolute path again, since we destroyed it above **/
	if (ref[0] == '/') strncpy(path, ref, 512);
	else snprintf(path, 512, "%s/%s", IFC.IfaceDir, ref);
	/** cache the handle **/
	xhAdd(&(IFC.HandleCache), handle->FullPath, (void*)handle);
	return handle;
    }


/*** ifc_internal_Print - print an interface handle. for testing.
 ***/
int
ifc_internal_Print(IfcHandle handle)
    {
    int i, j;
    char* name, *prop_path;

	if (IFC_DEBUG)
	    fprintf(stderr, "Printing handle to '%s'\n", handle->DefPath);
	
	for (i=0;i<IFC.NumCategories[handle->Type];i++)
	    {
	    if (IFC_DEBUG)
		fprintf(stderr, "\t%s: \n", IFC.CategoryNames[handle->Type][i]);
	    for (j=handle->Offsets[i];j<xaCount(&(handle->Members[i]));j++)
		{
		name = xaGetItem(&(handle->Members[i]), j);
		prop_path = xaGetItem(&(handle->Properties[i]), j);
		if (IFC_DEBUG)
		    fprintf(stderr, "\t\tMember: '%s'\t\tProperties object: '%s'\n", name, prop_path);
		}
	    }
	return 0;
    }

/*** ifcInitialize - initialize the Interface Module
 ***/
int
ifcInitialize()
    {

	/** Initialize some globals **/
	xhInit(&(IFC.Definitions), 31, 0);
	xhInit(&(IFC.HandleCache), IFC_CACHE_SIZE, 0);

	/** Multiple-type support **/
	/** IF YOU'RE ADDING A TYPE, HERE'S WHERE TO ADD THINGS **/

	/** Widget Interface Type **/
	IFC.NumCategories[IFC_T_WIDGET] = 5;
	IFC.TypeNames[IFC_T_WIDGET] = "widget";
	IFC.CategoryNames[IFC_T_WIDGET] = nmSysMalloc(IFC.NumCategories[IFC_T_WIDGET]*sizeof(void*));
	IFC.CategoryNames[IFC_T_WIDGET][IFC_CAT_WIDGET_PARAM] = "param";
	IFC.CategoryNames[IFC_T_WIDGET][IFC_CAT_WIDGET_EVENT] = "event";
	IFC.CategoryNames[IFC_T_WIDGET][IFC_CAT_WIDGET_ACTION] = "action";
	IFC.CategoryNames[IFC_T_WIDGET][IFC_CAT_WIDGET_PROP] = "property";
	IFC.CategoryNames[IFC_T_WIDGET][IFC_CAT_WIDGET_CONTAINER] = "container";


	/** Pull the base interface directory from the config file **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "iface_dir"), NULL, &IFC.IfaceDir, 0) < 0)
	    {
	    mssError(0, "IFC", "iface_dir not set in centrallix.conf, defaulting to /etc/iface");
	    IFC.IfaceDir = "/ifc";
	    }
//	fprintf(stderr, "IfaceDir = %s\n", IFC.IfaceDir);

	return 0;
    }


/*** ifcContains - check to see if a given category of a handle contains the given member
 ***/
int 
ifcContains(IfcHandle h, int category, char* member)
    {
    int i, count;
    
	/** make sure that's a legit category for the given type **/
	if (category < 0 || category >= IFC.NumCategories[h->Type])
	    {
	    mssError(1, "IFC", "illegal category number '%d' for category '%s' of handle '%s'",
		category, IFC.CategoryNames[h->Type], h->DefPath);
	    return -1;
	    }
	
	/** see if the member actually exists **/
	count = xaCount(&(h->Members[category]));
	for (i=h->Offsets[category];i<count;i++)
	    if (!strcmp(xaGetItem(&(h->Members[category]), i), member)) break;
	
	return (count != i);
    }


/*** ifcGetProperties - get the object holding the properties of the member of the given interface.
 *** Caller is responsible for closing the object.
 ***/
pObject 
ifcGetProperties(IfcHandle h, int category, char* member)
    {
    int i, count;

	/** make sure that's a legit category for the given type **/
	if (category < 0 || category >= IFC.NumCategories[h->Type])
	    {
	    mssError(1, "IFC", "illegal category number '%d' for category '%s' of handle '%s'",
		category, IFC.CategoryNames[h->Type], h->DefPath);
	    return NULL;
	    }
	
	/** see if the member actually exists **/
	count = xaCount(&(h->Members[category]));
	for (i=h->Offsets[category];i<count;i++)
	    if (!strcmp(xaGetItem(&(h->Members[category]), i), member)) break;

	if (count == i)
	    {
	    mssError(1, "IFC", "'%s' is not a member of category '%s' of handle '%s v%d.%d'", member,
		IFC.CategoryNames[h->Type], h->DefPath, h->MajorVersion, h->MinorVersion);
	    return NULL;
	    }
	return objOpen(h->ObjSession, xaGetItem(&(h->Properties[category]), i), O_RDONLY, 0600, "system/structure");
    }


/*** ifcIsSubset - returns 1 if h2 is a subset of h1, 0 if it is not, and -1 on error.
 ***	This is based solely on versioning.
 ***/
int 
ifcIsSubset(IfcHandle h1, IfcHandle h2)	
    {
	/** first make sure that they've got the same base interface definition **/
	if (strcmp(h1->DefPath, h2->DefPath)) return 0;

	/** major version must be the same **/
	if (h1->MajorVersion != h2->MajorVersion) return 0;

	/** minor version of h1 must be >= minor version of h2 **/
	return (h1->MinorVersion >= h2->MinorVersion);
    }


