#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/mtsession.h"
#include "cxlib/exception.h"
#include "stparse.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include <sys/stat.h>
#include "obj.h"
#include "st_node.h"
#include "cxlib/magic.h"
#include "cxlib/util.h"


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
/* Module: 	st_node.c,st_node.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 17, 1998 					*/
/* Description:	Handles read/write/create of structure file nodes that	*/
/*		are currently used by the structure file driver and the	*/
/*		querytree object driver for config information.		*/
/************************************************************************/



/*** Globals.  Node cache. ***/
struct
    {
    XHashTable	NodeCache;
    int		MasterRevCnt;
    }
    SN_INF;


/*** snReadNode - reads in the contents of a structure node file
 *** and/or looks up those contents in the node cache.
 ***/
pSnNode
snReadNode(pObject obj)
    {
    pSnNode node;
    pStructInf inf;
    char* path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr + obj->SubCnt - 1);
    ObjData pod;

    	/** Check in the node cache first. **/
	node = (pSnNode)xhLookup(&(SN_INF.NodeCache),path);
	ASSERTMAGIC(node, MGK_STNODE);

	/** If found node, quickly verify the file's timestamp. **/
	if (node) 
	    {
	    if (objGetAttrValue(obj,"last_modification",DATA_T_DATETIME,&pod) == 0)
	        {
		if (memcmp(pod.DateTime, &(node->LastModification), sizeof(DateTime)) != 0)
		    {
		    xhRemove(&(SN_INF.NodeCache),path);
		    xaDeInit(&(node->Opens));
		    stFreeInf(node->Data);
		    nmFree(node, sizeof(SnNode));
		    node = NULL;
		    }
		}
	    }
	if (node) return node;

	/** Read in its contents. **/
	inf = stParseMsgGeneric(obj,objRead,0);
	if (!inf)
	    {
	    mssError(0,"SN","Could not parse structure file for node");
	    obj_internal_PathPart(obj->Pathname,0,0);
	    return NULL;
	    }

	/** Allocate the new node structure. **/
	node = (pSnNode)nmMalloc(sizeof(SnNode));
	if (!node)
	    {
	    stFreeInf(inf);
	    obj_internal_PathPart(obj->Pathname,0,0);
	    return NULL;
	    }
	node->OpenCnt = 0;
	strcpy(node->NodePath,path);
	node->Status = SN_NS_CLEAN;
	xaInit(&(node->Opens),16);
	node->Data = inf;
	SETMAGIC(node, MGK_STNODE);
	if (objGetAttrValue(obj,"last_modification",DATA_T_DATETIME,&pod) == 0)
	    {
	    memcpy(&(node->LastModification), pod.DateTime, sizeof(DateTime));
	    }
	else
	    {
	    printf("SN:  Warning - driver '%s' does not support last_modification.\n",obj->Driver->Name);
	    }
	node->RevisionCnt = (SN_INF.MasterRevCnt++);
	obj_internal_PathPart(obj->Pathname,0,0);
	if (objGetAttrValue(obj,"outer_type",DATA_T_STRING,&pod) != 0) objGetAttrValue(obj,"content_type",DATA_T_STRING,&pod);
	memccpy(node->OpenType, pod.String, 0, 63);
	node->OpenType[63] = 0;

	/** Cache the node. **/
	xhAdd(&(SN_INF.NodeCache),node->NodePath, (char*)node);

    return node;
    }


/*** snWriteNode - writes a node data structure back out to the
 *** file from which it was loaded.
 ***/
int
snWriteNode(pObject obj, pSnNode node)
    {
    ObjData pod;
    pObject new_obj;
    char* path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr + obj->SubCnt - 1);
    char* openas_path;

	ASSERTMAGIC(node, MGK_STNODE);

    	/** Make sure that the date/time hasn't changed. **/
	if (objGetAttrValue(obj,"last_modification",DATA_T_DATETIME,&pod) == 0)
	    {
	    if (memcmp(pod.DateTime, &(node->LastModification), sizeof(DateTime)) != 0)
		{
	        /** merge logic will go here. **/
		if (node->Status == SN_NS_CLEAN)
		    mssError(1,"SN","Node file was modified; in-memory copy not sync'd");
		else
		    mssError(1,"SN","Oops - Node file was modified -- cannot rewrite it...");
	        obj_internal_PathPart(obj->Pathname,0,0);
	        return -1;
		}
	    }

	/** Node not 'dirty'?  Return success now if so. **/
	if (node->Status == SN_NS_CLEAN) 
	    {
	    obj_internal_PathPart(obj->Pathname,0,0);
	    return 0;
	    }

	/** Add the open-as key to the path so it is opened raw. **/
	openas_path = nmSysMalloc(strlen(path) + 37);
	sprintf(openas_path,"%s?ls__type=application%%2foctet-stream",path);

	/** Open and truncate the file **/
	new_obj = objOpen(obj->Session, openas_path, O_WRONLY | O_TRUNC | O_CREAT, 0600, node->OpenType);
	if (!new_obj)
	    {
	    mssErrorErrno(1,"SN","Could not (re)write the node object");
	    obj_internal_PathPart(obj->Pathname,0,0);
	    return -1;
	    }

	/** Write the structure data. **/
	stGenerateMsgGeneric(new_obj,objWrite,node->Data,0);
	node->Status = SN_NS_CLEAN;
	node->RevisionCnt = (SN_INF.MasterRevCnt++);

	/** Close up and read the timestamp. **/
	objClose(new_obj);
	nmSysFree(openas_path);
	if (objGetAttrValue(obj,"last_modification",DATA_T_DATETIME,&pod) == 0)
	    {
	    memcpy(&(node->LastModification), pod.DateTime, sizeof(DateTime));
	    } 
	obj_internal_PathPart(obj->Pathname,0,0);

    return 0;
    }


/*** snNewNode - create a new structure node.
 ***/
pSnNode
snNewNode(pObject obj, char* content_type)
    {
    pSnNode new_node;
    char* ptr;
    char* path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr + obj->SubCnt - 1);

    	/** Allocate the node. **/
	new_node = (pSnNode)nmMalloc(sizeof(SnNode));
	if (!new_node) return NULL;
	SETMAGIC(new_node, MGK_STNODE);

	/** Allocate an initial inf structure **/
	ptr = strrchr(path,'/')+1;
	new_node->Data = stCreateStruct(ptr,content_type);
	new_node->Status = SN_NS_DIRTY;
	xaInit(&(new_node->Opens),16);
	strcpy(new_node->NodePath, path);
	new_node->OpenCnt = 0;
	new_node->RevisionCnt = (SN_INF.MasterRevCnt++);
	obj_internal_PathPart(obj->Pathname,0,0);

	/** Set the initial modification time **/
	objCurrentDate(&(new_node->LastModification));

    return new_node;
    }


/*** snDelete - deletes a node.
 ***/
int
snDelete(pSnNode node)
    {

	ASSERTMAGIC(node, MGK_STNODE);

    	/** Release from hash table cache. **/
	xhRemove(&(SN_INF.NodeCache), node->NodePath);

	/** Release the memory **/
	nmFree(node, sizeof(SnNode));

    return 0;
    }


/*** snGetSerial - return the serial # of a node file.  When the
 *** file is modified for any reason (reload, WriteNode, etc),
 *** the serial number changes.
 ***/
int
snGetSerial(pSnNode node)
    {
    ASSERTMAGIC(node, MGK_STNODE);
    return node->RevisionCnt;
    }


/*** snGetLastModification - return the date/time of the last modification
 *** of the node file.  Pointer is valid as long as the node remains valid.
 *** Caller should not free the pointer.
 ***/
pDateTime
snGetLastModification(pSnNode node)
    {
    return &(node->LastModification);
    }


/*** snSetParamString - set a parameter in a new node structure,
 *** either from the obj->Pathname->OpenCtl parameters, or otherwise
 *** from the given default.  Returns -1 if failed or no default
 *** given, 0 if it used the OpenCtl parameter, or 1 if it used the
 *** given default.  The default or openctl will be strdup() copied.
 ***/
int
snSetParamString(pSnNode node, pObject obj, char* paramname, char* default_val)
    {
    char* ptr;
    pStructInf attr_inf;

	ASSERTMAGIC(node, MGK_STNODE);

    	/** Look it up in the open ctl. **/
	if (stAttrValue_ne(stLookup_ne(obj->Pathname->OpenCtl[obj->SubPtr],paramname),&ptr) < 0) ptr = default_val;

	/** Didn't find a suitable value? **/
	if (!ptr) return -1;

	/** Make a copy and add to the node. **/
	/*ptrcpy = nmSysStrdup(ptr);*/
	attr_inf = stLookup(node->Data, paramname);
	if (!attr_inf) attr_inf = stAddAttr(node->Data,paramname);
	stSetAttrValue(attr_inf, DATA_T_STRING, POD(&ptr), 0);

    return (ptr == default_val)?1:0;
    }


/*** stSetParamInteger - like the above routine, but deals with integer
 *** values instead of string values...
 ***/
int
snSetParamInteger(pSnNode node, pObject obj, char* paramname, int default_val)
    {
    int val;
    int used_default = 0;
    pStructInf attr_inf;
    char* intptr;

	ASSERTMAGIC(node, MGK_STNODE);

    	/** Look it up in the open ctl. **/
	if (stAttrValue_ne(stLookup_ne(obj->Pathname->OpenCtl[obj->SubPtr],paramname),&intptr) < 0) 
	    {
	    val = default_val;
	    used_default = 1;
	    }
	else
	    {
	    val = strtoi(intptr,NULL,10);
	    }

	/** Make a copy and add to the node. **/
	attr_inf = stLookup(node->Data, paramname);
	if (!attr_inf) attr_inf = stAddAttr(node->Data,paramname);
	stSetAttrValue(attr_inf, DATA_T_INTEGER, POD(&val), 0);

    return used_default?1:0;
    }


/*** snInitialize - initialize the SN module.
 ***/
int
snInitialize()
    {
    	
	/** Zero the globals and init the node cache **/
	memset(&SN_INF, 0, sizeof(SN_INF));
	xhInit(&SN_INF.NodeCache,255,0);
	SN_INF.MasterRevCnt = 0;

    return 0;
    }

