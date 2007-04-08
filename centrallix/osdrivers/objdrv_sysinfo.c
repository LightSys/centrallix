#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"
#include "config.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_sysinfo.c           				*/
/* Author:	Greg Beeley (GRB)      					*/
/* Creation:	March 2nd, 2005        					*/
/* Description:	The SysInfo objectsystem driver provides access to	*/
/*		information on internal Centrallix settings and status,	*/
/*		much like the /proc or /sys filesystem on Linux does.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_sysinfo.c,v 1.3 2007/04/08 03:52:01 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_sysinfo.c,v $

    $Log: objdrv_sysinfo.c,v $
    Revision 1.3  2007/04/08 03:52:01  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.2  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.1  2005/09/17 01:23:51  gbeeley
    - Adding sysinfo objectsystem driver, which is roughly analogous to
      the /proc filesystem in Linux.

 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[OBJSYS_MAX_PATH+1];
    char*	OurPath;	/* ptr to part of path we manage */
    char*	Name;		/* ptr to last path element */
    int		Flags;		/* SYS_F_xxx */
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    int		CurMethod;
    pSnNode	Node;
    pSysInfoData Sid;		/* ptr to tree of sysinfo objects */
    pXArray	ObjsArray;
    pXArray	AttrsArray;
    pXArray	MethodsArray;
    pXArray	AttrTypesArray;
    }
    SysData, *pSysData;

#define SYS_F_SUBOBJ		1	/* use subobj of the sid */


#define SYS(x) ((pSysData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pSysData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    SysQuery, *pSysQuery;

/*** GLOBALS ***/
struct
    {
    pSysInfoData	Root;
    int			StartUID;
    int			sys_thr_ids[256];
    char		sys_thr_names[256][3];
    char*		sys_thr_desc[256];
    int			sys_thr_status[256];
    char*		sys_thr_status_char[256];
    int			sys_thr_flags[256];
    char*		sys_thr_flags_char[256];
    unsigned long	sys_mt_last_tick;
    int			sys_thr_cnt;
    }
    SYS_INF;

static int is_init = 0;

int
sys_internal_RootAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    static char build[64];

	if (objname) return -1;
	if (!strcmp(attrname, "version"))
	    {
	    val->String = PACKAGE_VERSION;
	    return 0;
	    }
	else if (!strcmp(attrname, "build"))
	    {
	    snprintf(build, sizeof(build), "%d", BUILD);
	    val->String = build;
	    return 0;
	    }
	else if (!strcmp(attrname, "stability"))
	    {
	    val->String = STABILITY;
	    return 0;
	    }

    return -1;
    }

void
sys_internal_HardRestart(void* v)
    {
    int i;

	thSleep(2000);
	for(i=3;i<1024;i++) close(i);
	execvp(CxGlobals.ArgV[0], CxGlobals.ArgV);

    thExit();
    }

int
sys_internal_RootExecMethod(void* ctx, char* objname, char* methodname, char* param)
    {

	if (!strcmp(methodname, "HardRestart") && geteuid() == SYS_INF.StartUID)
	    {
	    thCreate(sys_internal_HardRestart, 0, NULL);
	    }

    return -1;
    }


int
sysInitGlobals()
    {

	if (is_init) return 0;

	/** Initialize globals **/
	memset(&SYS_INF,0,sizeof(SYS_INF));
	SYS_INF.Root = sysAllocData("/", NULL, NULL, NULL, NULL, sys_internal_RootAttrValue, sys_internal_RootExecMethod, 0);
	SYS_INF.StartUID = geteuid();
	sysAddAttrib(SYS_INF.Root, "version", DATA_T_STRING);
	sysAddAttrib(SYS_INF.Root, "build", DATA_T_STRING);
	sysAddAttrib(SYS_INF.Root, "stability", DATA_T_STRING);
	sysAddMethod(SYS_INF.Root, "HardRestart");
	SYS_INF.Root->Subtree = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(SYS_INF.Root->Subtree, 16);
	SYS_INF.Root->Context = NULL;
	SYS_INF.Root->Name = "";
	is_init = 1;

	SYS_INF.sys_mt_last_tick = 0;

    return 0;
    }


/*** sysAllocData - allocate a SysInfoData structure from the given params
 ***/
pSysInfoData
sysAllocData(char* path, pXArray (*attrfn)(void*, char* objname), pXArray (*objfn)(void*), pXArray (*methfn)(void*, char* objname), int (*getfn)(void*, char* objname, char* attrname), int (*valuefn)(void*, char* objname, char* attrname, void* value), int (*execfn)(void*, char* objname, char* methname, char* methparam), int admin_only)
    {
    pSysInfoData sid;

	/** Allocate the structure **/
	sid = (pSysInfoData)nmMalloc(sizeof(SysInfoData));
	if (!sid) return NULL;

	/** Fill it in **/
	memset(sid, 0, sizeof(SysInfoData));
	sid->AttrEnumFn = attrfn;
	sid->ObjEnumFn = objfn;
	sid->MethodEnumFn = methfn;
	sid->GetAttrTypeFn = getfn;
	sid->GetAttrValueFn = valuefn;
	sid->ExecFn = execfn;
	sid->AdminOnly = admin_only;
	sid->Path = nmSysStrdup(path);

    return sid;
    }


/*** sysAddAttrib() - add a new attribute to the object
 ***/
int
sysAddAttrib(pSysInfoData sid, char* attrname, int type)
    {

	/** Can't do it if have a attr enum fn **/
	if (sid->AttrEnumFn)
	    {
	    mssError(1, "SYS", "Cannot add attr '%s' to sysinfo object '%s' that has an attribute enumerator", attrname, sid->Path);
	    return -1;
	    }
	
	/** Setup the xarray? **/
	if (!sid->Attrs)
	    {
	    sid->Attrs = (pXArray)nmMalloc(sizeof(XArray));
	    if (!sid->Attrs) return -1;
	    xaInit(sid->Attrs, 16);
	    }

	/** Add it. **/
	xaAddItem(sid->Attrs, nmSysStrdup(attrname));

	/** setup type xarray? **/
	if (!sid->AttrTypes)
	    {
	    sid->AttrTypes = (pXArray)nmMalloc(sizeof(XArray));
	    if (!sid->AttrTypes) return -1;
	    xaInit(sid->AttrTypes, 16);
	    }

	/** Add the type **/
	xaAddItem(sid->AttrTypes, (void*)type);

    return 0;
    }


/*** sysAddMethod - add a declared method on an object.
 ***/
int
sysAddMethod(pSysInfoData sid, char* methodname)
    {

	/** Can't do it if have an obj enum fn **/
	if (sid->MethodEnumFn)
	    {
	    mssError(1, "SYS", "Cannot add method '%s' to sysinfo object '%s' that has a method enumerator", methodname, sid->Path);
	    return -1;
	    }

	/** init the xarray? **/
	if (!sid->Methods)
	    {
	    sid->Methods = (pXArray)nmMalloc(sizeof(XArray));
	    if (!sid->Methods) return -1;
	    xaInit(sid->Methods, 16);
	    }

	/** Add it. **/
	xaAddItem(sid->Methods, nmSysStrdup(methodname));
	
    return 0;
    }


/*** sysAddObj() - add a subobject to a sid
 ***/
int
sysAddObj(pSysInfoData sid, char* objname)
    {

	/** Can't do it if have an obj enum fn **/
	if (sid->ObjEnumFn)
	    {
	    mssError(1, "SYS", "Cannot add object '%s' to sysinfo object '%s' that has an object enumerator", objname, sid->Path);
	    return -1;
	    }

	/** init the xarray? **/
	if (!sid->Objs)
	    {
	    sid->Objs = (pXArray)nmMalloc(sizeof(XArray));
	    if (!sid->Objs) return -1;
	    xaInit(sid->Objs, 16);
	    }

	/** Add it. **/
	xaAddItem(sid->Objs, nmSysStrdup(objname));
	
    return 0;
    }


/*** sysFindPath() - find a sysinfodata structure that corresponds to the
 *** given path.  If "new_sid" is set, then we can create the paths that
 *** lead up to the thing, and use new_sid as the last element in the 
 *** path.
 ***/
pSysInfoData
sysFindPath(pPathname path, pSysInfoData new_sid, int start_path, int* path_cnt)
    {
    pSysInfoData sid, mk_sid;
    int i,j;
    pSysInfoData find_sid;

	/** Start at the root **/
	sid = SYS_INF.Root;
	if (path_cnt) (*path_cnt) = 0;

	/** Loop for elements in the path **/
	for(i = start_path; i<path->nElements-1; i++)
	    {
	    if (path_cnt) (*path_cnt) = (i - start_path + 1);
	    if (i == path->nElements-2 && new_sid)
		{
		/** Stop here, caller is going to add new item. **/
		if (path_cnt) (*path_cnt)--;
		break;
		}
	    /** Search for the item **/
	    find_sid = NULL;
	    for(j=0;j<sid->Subtree->nItems;j++)
		{
		find_sid = (pSysInfoData)(sid->Subtree->Items[j]);
		if (!strcmp(find_sid->Name, obj_internal_PathPart(path,i+1,1))) break;
		find_sid = NULL;
		}
	    if (find_sid)
		sid = find_sid;
	    else if (i == path->nElements-2)
		{
		if (path_cnt) (*path_cnt)--;
		break; /* might be subobj */
		}
	    else if (new_sid)
		{
		/** make it **/
		mk_sid = sysAllocData(obj_internal_PathPart(path, 1, i+1), NULL, NULL, NULL, NULL, NULL, NULL, new_sid->AdminOnly);
		obj_internal_PathPart(path, 0, 0);
		mk_sid->Name = nmSysStrdup(obj_internal_PathPart(path, i+1, 1));
		mk_sid->Context = NULL;
		mk_sid->Subtree = (pXArray)nmMalloc(sizeof(XArray));
		if (!mk_sid->Subtree) return NULL;
		xaInit(mk_sid->Subtree, 16);
		xaAddItem(sid->Subtree, (void*)mk_sid);
		sid = mk_sid;
		}
	    else
		break;
	    }

    return sid;
    }


/*** sysRegister() - register a sysinfodata structure so that it can be 
 *** accessed using the /sys/cx.sysinfo subtree
 ***/
int
sysRegister(pSysInfoData sid, void* context)
    {
    char* slashptr;
    pSysInfoData parent_sid;
    pPathname path;

	sysInitGlobals();

	/** Set the context for the subtree **/
	sid->Context = context;

	/** Figure out the name **/
	slashptr = strrchr(sid->Path, '/');
	if (!slashptr) return -1;
	sid->Name = nmSysStrdup(slashptr+1);

	/** Add the subtree xarray **/
	sid->Subtree = (pXArray)nmMalloc(sizeof(XArray));
	if (!sid->Subtree) return -1;
	xaInit(sid->Subtree, 16);

	/** Link it in **/
	path = obj_internal_NormalizePath("/",sid->Path);
	parent_sid = sysFindPath(path, sid, 0, NULL);
	xaAddItem(parent_sid->Subtree, (void*)sid);
	obj_internal_FreePath(path);

    return 0;
    }


/*** sysLoadObjsArray() - figure out the needed list of subobjects
 ***/
int
sysLoadObjsArray(pSysData inf)
    {
    pXArray xa = NULL;

	/** Static or dynamic list? **/
	if (inf->ObjsArray)
	    xa = inf->ObjsArray;
	else if (inf->Sid->Objs)
	    xa = inf->Sid->Objs;
	else if (inf->Sid->ObjEnumFn)
	    xa = inf->Sid->ObjEnumFn(inf->Sid->Context);
	inf->ObjsArray = xa;

    return 0;
    }


/*** sysLoadMethodsArray() - figure out the needed list of subobjects
 ***/
int
sysLoadMethodsArray(pSysData inf)
    {
    pXArray xa = NULL;

	/** Static or dynamic list? **/
	if (inf->MethodsArray)
	    xa = inf->MethodsArray;
	else if (inf->Sid->Methods)
	    xa = inf->Sid->Methods;
	else if (inf->Sid->MethodEnumFn)
	    xa = inf->Sid->MethodEnumFn(inf->Sid->Context, (inf->Flags & SYS_F_SUBOBJ)?inf->Name:NULL);
	inf->MethodsArray = xa;
	if (!xa) return -1;

    return 0;
    }


/*** sysLoadAttrsArray() - figure out list of attributes
 ***/
int
sysLoadAttrsArray(pSysData inf)
    {
    pXArray xa = NULL, txa = NULL;
    int i,t;

	/** Static or dynamic list? **/
	if (inf->AttrsArray)
	    xa = inf->AttrsArray;
	else if (inf->Sid->Attrs)
	    xa = inf->Sid->Attrs;
	else if (inf->Sid->AttrEnumFn)
	    xa = inf->Sid->AttrEnumFn(inf->Sid->Context, (inf->Flags & SYS_F_SUBOBJ)?inf->Name:NULL);
	inf->AttrsArray = xa;
	if (!xa) return -1;

	if (inf->AttrTypesArray)
	    txa = inf->AttrTypesArray;
	else if (inf->Sid->AttrTypes)
	    txa = inf->Sid->AttrTypes;
	else
	    {
	    txa = (pXArray)nmMalloc(sizeof(XArray));
	    xaInit(txa, xa->nItems);
	    for(i=0;i<xa->nItems;i++)
		{
		if (!inf->Sid->GetAttrTypeFn) 
		    t = -1;
		else
		    t = inf->Sid->GetAttrTypeFn(inf->Sid->Context, (inf->Flags & SYS_F_SUBOBJ)?inf->Name:NULL, xa->Items[i]);
		xaAddItem(txa, (void*)t);
		}
	    }
	inf->AttrTypesArray = txa;

    return 0;
    }


/*** sysFindObj - does the object with the given name exist within
 *** the sid?
 ***/
int
sysFindObj(pSysData inf, char* objname)
    {
    int i, rval = -1;
    pXArray xa;

	/** Get the list of subobjects **/
	sysLoadObjsArray(inf);
	xa = inf->ObjsArray;
	if (!xa) return -1;

	/** Look for it **/
	for(i=0;i<xa->nItems;i++)
	    {
	    if (!strcmp(objname, xa->Items[i])) 
		{
		rval = 0;
		break;
		}
	    }

    return rval;
    }


/*** sysOpen - open an object.
 ***/
void*
sysOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSysData inf = NULL;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
    pStructInf verify_inf;
    int len;

	/** Allocate the structure **/
	inf = (pSysData)nmMalloc(sizeof(SysData));
	if (!inf) goto error;
	memset(inf,0,sizeof(SysData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		mssError(0,"SYS","Could not create new node object");
		goto error;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    mssError(0,"SYS","Could not open structure file");
	    goto error;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** Verify node's stated path --
	 ** when we do signed node objects, this will ensure it is at a
	 ** kosher location in the overall objectsystem tree.
	 **/
	verify_inf = stLookup(inf->Node->Data, "verify_path");
	if (!verify_inf)
	    {
	    mssError(1,"SYS","Node object must contain verify_path attribute");
	    goto error;
	    }
	ptr = NULL;
	stAttrValue(verify_inf, NULL, &ptr, 0);
	if (!ptr || strcmp(ptr, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr)+1))
	    {
	    mssError(1,"SYS","Node object verify_path '%s' does not match open path '%s'!",
		    ptr, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr)+1);
	    goto error;
	    }

	/** Find the correct sid to associate with this. **/
	obj_internal_PathPart(obj->Pathname, 0, 0);
	inf->Sid = sysFindPath(obj->Pathname, NULL, obj->SubPtr-1, &len);
	if (!inf->Sid) goto error;
	obj->SubCnt = len + 1;

	/** Try to open next obj in the path **/
	if (obj->SubPtr + obj->SubCnt < obj->Pathname->nElements)
	    {
	    if (sysFindObj(inf, obj_internal_PathPart(obj->Pathname,obj->SubPtr + obj->SubCnt,1)) == 0)
		{
		inf->Flags |= SYS_F_SUBOBJ;
		obj->SubCnt++;
		}
	    }
	inf->OurPath = obj_internal_PathPart(obj->Pathname, obj->SubPtr, obj->SubCnt-1);
	if (!inf->OurPath) inf->OurPath = "";
	inf->OurPath = nmSysStrdup(inf->OurPath);
	obj_internal_PathPart(obj->Pathname, 0, 0);
	ptr = strrchr(inf->OurPath, '/');
	if (ptr) inf->Name = ptr+1;
	else inf->Name = inf->OurPath;

	return (void*)inf;

    error:
	if (inf) sysClose(inf, oxt);
	return NULL;
    }


/*** sysClose - close an open object.
 ***/
int
sysClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSysData inf = SYS(inf_v);

	/** allocated name **/
	if (inf->OurPath) nmSysFree(inf->OurPath);

	/** temporary xarray for list of objects **/
	if (inf->Sid && inf->AttrsArray && inf->AttrsArray != inf->Sid->Attrs)
	    {
	    xaDeInit(inf->AttrsArray);
	    nmFree(inf->AttrsArray, sizeof(XArray));
	    xaDeInit(inf->AttrTypesArray);
	    nmFree(inf->AttrTypesArray, sizeof(XArray));
	    }

	/** temporary xarray for list of objects **/
	if (inf->Sid && inf->ObjsArray && inf->ObjsArray != inf->Sid->Objs)
	    {
	    xaDeInit(inf->ObjsArray);
	    nmFree(inf->ObjsArray, sizeof(XArray));
	    }

    	/** Write the node first, if need be. **/
	if (inf->Node)
	    {
	    snWriteNode(inf->Obj->Prev, inf->Node);
	    inf->Node->OpenCnt --;
	    }
	
	/** Release the memory **/
	nmFree(inf,sizeof(SysData));

    return 0;
    }


/*** sysCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
sysCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = sysOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	sysClose(inf, oxt);

    return 0;
    }


/*** sysDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
sysDelete(pObject obj, pObjTrxTree* oxt)
    {
    pSysData inf;
    int is_empty = 1;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pSysData)sysOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		sysClose(inf, oxt);
		mssError(1,"SYS","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		sysClose(inf, oxt);
		mssError(1,"SYS","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(SysData));

    return 0;
    }


/*** sysRead - No content yet for system info objects.
 ***/
int
sysRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pSysData inf = SYS(inf_v);*/
    return -1;
    }


/*** sysWrite - No content yet for system info objects.
 ***/
int
sysWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pSysData inf = SYS(inf_v);*/
    return -1;
    }


/*** sysOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
sysOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pSysData inf = SYS(inf_v);
    pSysQuery qy;

	/** Allocate the query structure **/
	qy = (pSysQuery)nmMalloc(sizeof(SysQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(SysQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	sysLoadObjsArray(inf);
	query->Flags &= ~(OBJ_QY_F_FULLSORT | OBJ_QY_F_FULLQUERY);
    
    return (void*)qy;
    }


/*** sysQueryFetch - get the next directory entry as an open object.
 ***/
void*
sysQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pSysQuery qy = ((pSysQuery)(qy_v));
    pSysData inf;
    char* new_obj_name;
    pSysInfoData subsid;
    int id;
    char* ptr;

	/** Already a subobj via objsarray? **/
	if (qy->Data->Flags & SYS_F_SUBOBJ) return NULL;

	/** Alloc the structure **/
	inf = (pSysData)nmMalloc(sizeof(SysData));
	if (!inf) return NULL;
	memset(inf, 0, sizeof(SysData));

	/** First, are we within the Objects array? **/
	if (qy->Data->ObjsArray && qy->ItemCnt < qy->Data->ObjsArray->nItems)
	    {
	    /** yes **/
	    new_obj_name = qy->Data->ObjsArray->Items[qy->ItemCnt];
	    inf->Sid = qy->Data->Sid;
	    inf->Flags |= SYS_F_SUBOBJ;
	    }
	else
	    {
	    /** no, look for a subtree sid **/
	    if (qy->Data->ObjsArray)
		id = qy->ItemCnt - qy->Data->ObjsArray->nItems;
	    else
		id = qy->ItemCnt;
	    subsid = (pSysInfoData)xaGetItem(qy->Data->Sid->Subtree, id);
	    if (!subsid)
		{
		/** no more items **/
		sysClose(inf, oxt);
		return NULL;
		}
	    inf->Sid = subsid;
	    new_obj_name = subsid->Name;
	    }

	/** Build the filename. **/
	if (obj_internal_AddToPath(obj->Pathname, new_obj_name) < 0)
	    {
	    sysClose(inf, oxt);
	    return NULL;
	    }

	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

	inf->OurPath = nmSysStrdup(obj_internal_PathPart(obj->Pathname, obj->SubPtr, obj->SubCnt));
	obj_internal_PathPart(obj->Pathname, 0, 0);
	ptr = strrchr(inf->OurPath, '/');
	if (ptr) inf->Name = ptr+1;
	else inf->Name = inf->OurPath;

    return (void*)inf;
    }


/*** sysQueryClose - close the query.
 ***/
int
sysQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(SysQuery));

    return 0;
    }


/*** sysGetAttrType - get the type (DATA_T_sys) of an attribute by name.
 ***/
int
sysGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pSysData inf = SYS(inf_v);
    int i,t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/
	sysLoadAttrsArray(inf);
	if (!inf->AttrsArray) return -1;
	for(i=0;i<inf->AttrsArray->nItems;i++)
	    {
	    if (!strcmp(inf->AttrsArray->Items[i], attrname))
		{
		t = (int)(inf->AttrTypesArray->Items[i]);
		return t;
		}
	    }

    return -1;
    }


/*** sysGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
sysGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pSysData inf = SYS(inf_v);
    int i,t;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    val->String = inf->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    val->String = "system/void";
	    return 0;
	    }

	/** Object type. **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "application/x-sysinfo";
	    return 0;
	    }

	/** Check for our own attrs **/
	sysLoadAttrsArray(inf);
	if (inf->AttrsArray)
	  for(i=0;i<inf->AttrsArray->nItems;i++)
	    {
	    if (!strcmp(inf->AttrsArray->Items[i], attrname))
		{
		t = (int)(inf->AttrTypesArray->Items[i]);
		if (t != datatype)
		    {
		    mssError(1,"SYS","Type mismatch requesting attribute '%s'", attrname);
		    return -1;
		    }
		if (!inf->Sid->GetAttrValueFn) return 1; /* null */
		return inf->Sid->GetAttrValueFn(inf->Sid->Context, (inf->Flags & SYS_F_SUBOBJ)?inf->Name:NULL, attrname, val);
		}
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	mssError(1,"SYS","Could not locate requested attribute");

    return -1;
    }


/*** sysGetNextAttr - get the next attribute name for this object.
 ***/
char*
sysGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSysData inf = SYS(inf_v);

	if (inf->AttrsArray && inf->CurAttr < inf->AttrsArray->nItems)
	    {
	    return inf->AttrsArray->Items[inf->CurAttr++];
	    }

    return NULL;
    }


/*** sysGetFirstAttr - get the first attribute name for this object.
 ***/
char*
sysGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pSysData inf = SYS(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	sysLoadAttrsArray(inf);
	ptr = sysGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** sysSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
sysSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    /*pSysData inf = SYS(inf_v);*/

	/** Choose the attr name **/
	/** Changing name of object? **/
	if (!strcmp(attrname,"name"))
	    {
	    return -1;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    return -1;
	    }
	if (!strcmp(attrname,"outer_type"))
	    {
	    return -1;
	    }

    return -1;
    }


/*** sysAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
sysAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    /*pSysData inf = SYS(inf_v);*/
    return -1;
    }


/*** sysOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
sysOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** sysGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
sysGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    pSysData inf = SYS(inf_v);

	if (inf->MethodsArray && inf->CurMethod < inf->MethodsArray->nItems)
	    {
	    return inf->MethodsArray->Items[inf->CurMethod++];
	    }

    return NULL;
    }


/*** sysGetFirstMethod -- return name of First method available on the object.
 ***/
char*
sysGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    pSysData inf = SYS(inf_v);
    char* ptr;

	/** Set the current method. **/
	inf->CurMethod = 0;

	/** Return the next one. **/
	sysLoadMethodsArray(inf);
	ptr = sysGetNextMethod(inf_v, oxt);

    return ptr;
    }


/*** sysExecuteMethod - Execute a method, by name.
 ***/
int
sysExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    pSysData inf = SYS(inf_v);
    int i;

	/** Get the methods... **/
	sysLoadMethodsArray(inf);

	/** Look it up **/
	if (inf->MethodsArray)
	    {
	    for(i=0;i<inf->MethodsArray->nItems;i++)
		{
		if (!strcmp(inf->MethodsArray->Items[i], methodname))
		    {
		    if (!inf->Sid->ExecFn) return 1; /* null */
		    return inf->Sid->ExecFn(inf->Sid->Context, (inf->Flags & SYS_F_SUBOBJ)?inf->Name:NULL, methodname, param->String);
		    }
		}
	    }

    return -1;
    }


/*** sysPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
sysPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    /** No hints yet on this **/
    return NULL;
    }


/*** sysInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
sysInfo(void* inf_v, pObjectInfo info_struct)
    {
    /*pSysData inf = SYS(inf_v);*/
    
	memset(info_struct, sizeof(ObjectInfo), 0);

    return 0;
    }


/*** sysCommit - commit any changes made to the underlying data source.
 ***/
int
sysCommit(void* inf_v, pObjTrxTree* oxt)
    {
    /** no uncommitted changes yet **/
    return 0;
    }

pXArray
sys_internal_MtaskAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "description");
	xaAddItem(xa, "status");
	xaAddItem(xa, "flags");

    return xa;
    }


int
sys_internal_MtaskLoad()
    {
    int i;

	if (mtLastTick() <= SYS_INF.sys_mt_last_tick) return 0;

	SYS_INF.sys_mt_last_tick = mtLastTick();
	SYS_INF.sys_thr_cnt = thGetThreadList(256, SYS_INF.sys_thr_ids, SYS_INF.sys_thr_desc, SYS_INF.sys_thr_status, SYS_INF.sys_thr_flags);
	for(i=0;i<SYS_INF.sys_thr_cnt;i++) 
	    {
	    /** name **/
	    snprintf(SYS_INF.sys_thr_names[i], 3, "%2.2X", SYS_INF.sys_thr_ids[i]);

	    /** status **/
	    switch(SYS_INF.sys_thr_status[i])
		{
		case THR_S_EXECUTING:
		    SYS_INF.sys_thr_status_char[i] = "executing";
		    break;
		case THR_S_RUNNABLE:
		    SYS_INF.sys_thr_status_char[i] = "runnable";
		    break;
		case THR_S_BLOCKED:
		    SYS_INF.sys_thr_status_char[i] = "blocked";
		    break;
		default:
		    SYS_INF.sys_thr_status_char[i] = "unknown";
		    break;
		}

	    /** flags **/
	    if ((SYS_INF.sys_thr_flags[i] & THR_F_STARTING) && (SYS_INF.sys_thr_flags[i] & THR_F_IGNPIPE))
		SYS_INF.sys_thr_flags_char[i] = "starting, ignsigpipe";
	    else if (SYS_INF.sys_thr_flags[i] & THR_F_STARTING)
		SYS_INF.sys_thr_flags_char[i] = "starting";
	    else if (SYS_INF.sys_thr_flags[i] & THR_F_IGNPIPE)
		SYS_INF.sys_thr_flags_char[i] = "ignsigpipe";
	    else
		SYS_INF.sys_thr_flags_char[i] = "";
	    }

    return 0;
    }

pXArray
sys_internal_MtaskObjList(void* ctx)
    {
    pXArray xa;
    int i;

	sys_internal_MtaskLoad();
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 256);
	for(i=0;i<SYS_INF.sys_thr_cnt;i++) 
	    xaAddItem(xa, SYS_INF.sys_thr_names[i]);

    return xa;
    }

int
sys_internal_MtaskAttrType(void* ctx, char* objname, char* attrname)
    {
	
	sys_internal_MtaskLoad();
	if (!objname) return -1;
	if (!strcmp(attrname, "description")) return DATA_T_STRING;
	else if (!strcmp(attrname, "status")) return DATA_T_STRING;
	else if (!strcmp(attrname, "flags")) return DATA_T_STRING;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;

    return -1;
    }


int
sys_internal_MtaskAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    int n;

	sys_internal_MtaskLoad();
	if (!objname) return -1;
	n = strtol(objname, NULL, 16);
	if (n < 0 || n >= SYS_INF.sys_thr_cnt) return -1;
	if (!strcmp(attrname, "description"))
	    {
	    val->String = SYS_INF.sys_thr_desc[n];
	    return 0;
	    }
	else if (!strcmp(attrname, "status"))
	    {
	    val->String = SYS_INF.sys_thr_status_char[n];
	    return 0;
	    }
	else if (!strcmp(attrname, "flags"))
	    {
	    val->String = SYS_INF.sys_thr_flags_char[n];
	    return 0;
	    }
	else if (!strcmp(attrname, "name"))
	    {
	    val->String = SYS_INF.sys_thr_names[n];
	    return 0;
	    }

    return -1;
    }


/*** sys_internal_RegisterMtask() - register a handler for listing
 *** mtask threads
 ***/
int
sys_internal_RegisterMtask()
    {
    pSysInfoData si;

	/** thread list **/
	si = sysAllocData("/tasks", sys_internal_MtaskAttrList, sys_internal_MtaskObjList, NULL, sys_internal_MtaskAttrType, sys_internal_MtaskAttrValue, NULL, 0);
	sysRegister(si, NULL);

    return 0;
    }


/*** sysInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
sysInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	sysInitGlobals();

	/** Setup the structure **/
	strcpy(drv->Name,"SYS - System Info ObjectSystem Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/x-sysinfo");

	/** Setup the function references. **/
	drv->Open = sysOpen;
	drv->Close = sysClose;
	drv->Create = sysCreate;
	drv->Delete = sysDelete;
	drv->OpenQuery = sysOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = sysQueryFetch;
	drv->QueryClose = sysQueryClose;
	drv->Read = sysRead;
	drv->Write = sysWrite;
	drv->GetAttrType = sysGetAttrType;
	drv->GetAttrValue = sysGetAttrValue;
	drv->GetFirstAttr = sysGetFirstAttr;
	drv->GetNextAttr = sysGetNextAttr;
	drv->SetAttrValue = sysSetAttrValue;
	drv->AddAttr = sysAddAttr;
	drv->OpenAttr = sysOpenAttr;
	drv->GetFirstMethod = sysGetFirstMethod;
	drv->GetNextMethod = sysGetNextMethod;
	drv->ExecuteMethod = sysExecuteMethod;
	drv->PresentationHints = sysPresentationHints;
	drv->Info = sysInfo;
	drv->Commit = sysCommit;

	nmRegister(sizeof(SysData),"SysData");
	nmRegister(sizeof(SysQuery),"SysQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

	/** Register internals stuff... **/
	sys_internal_RegisterMtask();

    return 0;
    }

MODULE_INIT(sysInitialize);
MODULE_PREFIX("sys");
MODULE_DESC("System Info ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

