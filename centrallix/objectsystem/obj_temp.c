#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xhashqueue.h"
#include "cxlib/xhandle.h"
#include "expression.h"
#include "cxlib/magic.h"
#include "cxlib/mtsession.h"
#include "stparse.h"
#include "cxss/cxss.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2018 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_object.c: implements the generic object access	*/
/*		methods, including open/close/create/delete.		*/
/************************************************************************/

typedef struct
    {
    pObjTemp	TempObj;
    pStructInf	Data;
    int		CurAttr;
    IntVec	IVvalue;
    StringVec	SVvalue;
    void*	VecData;
    }
    ObjTempData, *pObjTempData;

typedef struct
    {
    pStructInf	Data;
    int		ItemCnt;
    pStructInf	CurInf;
    pObjTemp	TempObj;
    }
    ObjTempQuery, *pObjTempQuery;


/*** obj_internal_CloseTempObj() - release the connection to the
 *** temporary object
 ***/
int
obj_internal_CloseTempObj(pObjTemp tmp)
    {

	tmp->LinkCnt--;
	if (tmp->LinkCnt <= 0)
	    {
	    stFreeInf((pStructInf)tmp->Data);
	    xhnFreeHandle(&OSYS.TempObjects, tmp->Handle);
	    nmFree(tmp, sizeof(ObjTemp));
	    }

    return 0;
    }


/*** objCreateTempObject() - creates a temporary object and returns a handle
 *** to the newly created object, which can then be opened using the
 *** objOpenTempObject() function, and later scheduled for deletion using
 *** objDeleteTempObject().  These "temporary" objects exist outside of any
 *** OSML session, and so their lifetime and scope is not restricted to
 *** any one session but can be shared across sessions and across time.
 ***/
handle_t
objCreateTempObject()
    {
    pObjTemp tmp;

	/** Create the temporary object **/
	tmp = (pObjTemp)nmMalloc(sizeof(ObjTemp));
	if (!tmp)
	    return XHN_INVALID_HANDLE;
	tmp->LinkCnt = 1;
	tmp->CreateCnt = 0;
	tmp->Data = stCreateStruct("temp", "system/object");
	if (!tmp->Data)
	    {
	    nmFree(tmp, sizeof(ObjTemp));
	    return XHN_INVALID_HANDLE;
	    }
	tmp->Handle = xhnAllocHandle(&OSYS.TempObjects, (void*)tmp);

    return tmp->Handle;
    }


/*** objDeleteTempObject() - deletes a temporary object.  The object will
 *** not actually be permanently deleted until all other opens and child
 *** opens and queries on the object have been closed.
 ***/
int
objDeleteTempObject(handle_t tempobj)
    {
    pObjTemp tmp = NULL;

	/** Lookup the temporary object **/
	tmp = xhnHandlePtr(&OSYS.TempObjects, tempobj);
	if (!tmp)
	    goto error;

	/** Schedule it for deletion (or delete it, if no one else has
	 ** it open).
	 **/
	if (obj_internal_CloseTempObj(tmp) < 0)
	    goto error;

	return 0;

    error:
	return -1;
    }


/*** objOpenTempObject() - opens a temp object.  The open object pointer
 *** is only valid for the session in which it is opened.
 ***/
pObject
objOpenTempObject(pObjSession session, handle_t tempobj, int mode)
    {
    pObject obj = NULL;
    pObjTemp tmp = NULL;
    pObjTempData inf = NULL;

	/** Lookup the temporary object **/
	tmp = xhnHandlePtr(&OSYS.TempObjects, tempobj);
	if (!tmp)
	    goto error;
	tmp->LinkCnt++;

	/** Allocate the linkage to the node in the temp obj **/
	inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(ObjTempData));
	inf->TempObj = tmp;
	inf->Data = (pStructInf)tmp->Data;

	/** Allocate an open object **/
	obj = obj_internal_AllocObj();
	if (!obj)
	    goto error;
	obj->Data = (void*)inf;
	obj->Driver = OSYS.TempDriver;
	obj->Session = session;
	obj->Pathname = obj_internal_NormalizePath("","");

	/** Add to open objects this session. **/
	xaAddItem(&session->OpenObjects, (void*)obj);

	return obj;

    error:
	if (inf)
	    nmFree(inf, sizeof(ObjTempData));
	if (obj)
	    obj_internal_FreeObj(obj);
	if (tmp)
	    tmp->LinkCnt--;
	return NULL;
    }


/*** tmpOpen - temporary objects cannot be opened via objOpen because they are not
 *** anchored at any point in the actual ObjectSystem.
 ***/
void*
tmpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpOpenChild - open a child object of this one
 ***/
void*
tmpOpenChild(void* inf_v, pObject obj, char* childname, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;
    pObjTempData child_inf;
    pStructInf find_inf;

	/** Find it **/
	if (!strcmp(childname, "*") && (obj->Mode & OBJ_O_AUTONAME) && (obj->Mode & OBJ_O_CREAT))
	    find_inf = NULL;
	else
	    find_inf = stLookup(inf->Data, childname);

	/** Exists, but it is not a child object? **/
	if (find_inf && stStructType(find_inf) != ST_T_SUBGROUP)
	    return NULL;

	/** Exclusive create, but object already exists? **/
	if ((obj->Mode & OBJ_O_EXCL) && (obj->Mode & OBJ_O_EXCL) && find_inf)
	    return NULL;

	/** Create a new child object? **/
	if ((obj->Mode & OBJ_O_CREAT) && !find_inf)
	    {
	    find_inf = stAllocInf();
	    if (!find_inf)
		return NULL;
	    inf->TempObj->CreateCnt++;
	    if (!strcmp(childname, "*") && (obj->Mode & OBJ_O_AUTONAME))
		snprintf(find_inf->Name, ST_NAME_STRLEN, "%lld", inf->TempObj->CreateCnt);
	    else
		strtcpy(find_inf->Name, childname, ST_NAME_STRLEN);
	    find_inf->UsrType = nmMalloc(ST_USRTYPE_STRLEN);
	    if (!find_inf->UsrType)
		return NULL;
	    strtcpy(find_inf->UsrType, "", ST_USRTYPE_STRLEN);
	    find_inf->Flags |= ST_F_GROUP;
	    stAddInf(inf->Data, find_inf);
	    }

	/** Exists and is a child object **/
	if (find_inf)
	    {
	    child_inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	    if (!child_inf) return NULL;
	    memset(child_inf, 0, sizeof(ObjTempData));
	    child_inf->Data = find_inf;
	    child_inf->TempObj = inf->TempObj;
	    child_inf->TempObj->LinkCnt++;
	    return child_inf;
	    }

    return NULL;
    }


/*** tmpClose - close an open temp object.
 ***/
int
tmpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;

	/** Release the memory **/
	obj_internal_CloseTempObj(inf->TempObj);
	nmFree(inf, sizeof(ObjTempData));

    return 0;
    }


/*** tmpCreate - create a new object, via pathname.  Since pathnames are not used
 *** to reference temp objects, this just returns failure.
 ***/
int
tmpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** tmpDeleteObj - delete an existing object.  This removes the object from the
 *** struct inf tree.
 ***/
int
tmpDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;

	/** Trying to delete root? **/
	if (inf->Data == (pStructInf)inf->TempObj->Data)
	    {
	    mssError(1, "OSML", "Cannot delete the root of temporary object tree");
	    return -1;
	    }

	/** Delete it.  FIXME - BUG - there is a reference counting issue here,
	 ** we will need ref counting on the entire StructInf structure in order
	 ** to properly delete a subpart of a structinf tree that may have
	 ** multiple references open to it.
	 **/
	stFreeInf(inf->Data);
	 
	/** Release the inf structure **/
	tmpClose(inf_v, oxt);

    return 0;
    }


/*** tmpRead - Structure elements have no content.  Fails.
 ***/
int
tmpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //pObjTempData inf = (pObjTempData)(inf_v);
    return -1;
    }


/*** tmpWrite - Again, no content.  This fails.
 ***/
int
tmpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //pObjTempData inf = (pObjTempData)(inf_v);
    return -1;
    }


/*** tmpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
tmpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pObjTempQuery qy;

	/** Allocate the query structure **/
	qy = (pObjTempQuery)nmMalloc(sizeof(ObjTempQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(ObjTempQuery));
	qy->Data = inf->Data;
	qy->ItemCnt = 0;
	qy->TempObj = inf->TempObj;
	inf->TempObj->LinkCnt++;
    
    return (void*)qy;
    }


/*** tmpQueryFetch - get the next directory entry as an open object.
 ***/
void*
tmpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pObjTempQuery qy = ((pObjTempQuery)(qy_v));
    pObjTempData inf;

	/** Find a subgroup item **/
	while(qy->ItemCnt < qy->Data->nSubInf && 
	      stStructType(qy->Data->SubInf[qy->ItemCnt]) != ST_T_SUBGROUP) qy->ItemCnt++;

	/** No more left? **/
	if (qy->ItemCnt >= qy->Data->nSubInf)
	    return NULL;

	qy->CurInf = qy->Data->SubInf[qy->ItemCnt];

	/** Alloc the structure **/
	inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	if (!inf) return NULL;
	memset(inf, 0, sizeof(ObjTempData));
	inf->Data = qy->CurInf;
	inf->TempObj = qy->TempObj;
	inf->TempObj->LinkCnt++;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** tmpQueryClose - close the query.
 ***/
int
tmpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pObjTempQuery qy = ((pObjTempQuery)(qy_v));

	/** Free the structure **/
	obj_internal_CloseTempObj(qy->TempObj);
	nmFree(qy, sizeof(ObjTempQuery));

    return 0;
    }


/*** tmpGetAttrType - get the type (DATA_T_tmp) of an attribute by name.
 ***/
int
tmpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Lookup the subgroup inf **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf || stStructType(find_inf) != ST_T_ATTRIB) 
	    {
	    return (find_inf)?(-1):DATA_T_UNAVAILABLE;
	    }

	/** Examine the expr to determine the type **/
	t = stGetAttrType(find_inf, 0);
	if (find_inf->Value && find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (t == DATA_T_INTEGER) return DATA_T_INTVEC;
	    else return DATA_T_STRINGVEC;
	    }
	else if (find_inf->Value)
	    {
	    return t;
	    }

    return -1;
    }


/*** tmpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
tmpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int rval;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (stLookup(inf->Data, "content"))
	        val->String = "application/octet-stream";
	    else
	        val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->UsrType;
	    return 0;
	    }

	/** Look through the attribs in the subinf **/
	find_inf = stLookup(inf->Data, attrname);

	/** If annotation, and not found, return "" **/
	if (!find_inf && !strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

	/** Not found, or not an attribute? **/
	if (!find_inf) return 1;
	if (stStructType(find_inf) != ST_T_ATTRIB) return -1;

	/** Vector or scalar? **/
	if (find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (inf->VecData)
		{
		nmSysFree(inf->VecData);
		inf->VecData = NULL;
		}
	    if (stGetAttrType(find_inf, 0) == DATA_T_INTEGER)
		{
		if (datatype != DATA_T_INTVEC)
		    {
		    mssError(1,"OSML","Type mismatch getting attribute '%s' (should be intvec)", attrname);
		    return -1;
		    }
		inf->VecData = stGetValueList(find_inf, DATA_T_INTEGER, &(inf->IVvalue.nIntegers));
		val->IntVec = &(inf->IVvalue);
		val->IntVec->Integers = (int*)(inf->VecData);
		}
	    else
		{
		if (datatype != DATA_T_STRINGVEC)
		    {
		    mssError(1,"OSML","Type mismatch getting attribute '%s' (should be stringvec)", attrname);
		    return -1;
		    }
		/** FIXME - the below StringVec->Strings never gets freed **/
		inf->VecData = stGetValueList(find_inf, DATA_T_STRING, &(inf->SVvalue.nStrings));
		val->StringVec = &(inf->SVvalue);
		val->StringVec->Strings = (char**)(inf->VecData);
		}
	    return 0;
	    }
	else
	    {
	    rval = stGetAttrValue(find_inf, datatype, val, 0);
	    if (rval < 0)
		{
		mssError(1,"OSML","Type mismatch or non-existent attribute '%s'", attrname);
		}
	    return rval;
	    }

    return -1;
    }


/*** tmpGetNextAttr - get the next attribute name for this object.
 ***/
char*
tmpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);

	/** Get the next attr from the list unless last one already **/
	while(inf->CurAttr < inf->Data->nSubInf && 
	      (stStructType(inf->Data->SubInf[inf->CurAttr]) != ST_T_ATTRIB || 
	       !strcmp(inf->Data->SubInf[inf->CurAttr]->Name, "annotation"))) inf->CurAttr++;
	if (inf->CurAttr >= inf->Data->nSubInf) return NULL;

    return inf->Data->SubInf[inf->CurAttr++]->Name;
    }


/*** tmpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
tmpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = tmpGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** tmpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
tmpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int t;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    strcpy(inf->Data->Name, val->String);
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    strcpy(inf->Data->UsrType, val->String);
	    return 0;
	    }

	/** Otherwise, set the integer or string value **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf) 
	    {
	    find_inf = stAddAttr(inf->Data, attrname);
	    if (!find_inf)
		{
		mssError(1,"OSML","Could not add attribute '%s' to object", attrname);
		return -1;
		}
	    }
	if (stStructType(find_inf) != ST_T_ATTRIB) return -1;

	/** Set value of attribute **/
	t = stGetAttrType(find_inf, 0);
	if (t > 0 && datatype != t)
	    {
	    mssError(1,"OSML","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
		    attrname, obj_type_names[datatype], obj_type_names[t]);
	    return -1;
	    }
	if (stSetAttrValue(find_inf, datatype, val, 0) < 0)
	    {
	    mssError(1,"OSML","Could not set attribute '%s'", attrname);
	    return -1;
	    }

    return 0;
    }


/*** tmpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
tmpAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf new_inf;
    char* ptr;

    	/** Add the attribute **/
	new_inf = stAddAttr(inf->Data, attrname);
	if (type == DATA_T_STRING)
	    {
	    ptr = (char*)nmSysStrdup(val->String);
	    stAddValue(new_inf, ptr, 0);
	    }
	else if (type == DATA_T_INTEGER)
	    {
	    stAddValue(new_inf, NULL, val->Integer);
	    }
	else
	    {
	    stAddValue(new_inf, NULL, 0);
	    }

    return 0;
    }


/*** tmpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
tmpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** tmpGetFirstMethod -- return name of First method available on the object.
 ***/
char*
tmpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** tmpGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
tmpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** tmpExecuteMethod - Execute a method, by name.
 ***/
int
tmpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** tmpPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
tmpPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
tmpInfo(void* inf_v, pObjectInfo info)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    int i;

	/** Setup the flags, and we know the subobject count btw **/
	memset(info, sizeof(ObjectInfo), 0);
	info->Flags = (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_SUBOBJ_CNT_KNOWN |
		OBJ_INFO_F_CAN_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
		OBJ_INFO_F_NO_CONTENT | OBJ_INFO_F_SUPPORTS_INHERITANCE | OBJ_INFO_F_TEMPORARY);
	info->nSubobjects = 0;
	for(i=0;i<inf->Data->nSubInf;i++)
	    {
	    if (stStructType(inf->Data->SubInf[i]) == ST_T_SUBGROUP) info->nSubobjects++;
	    }
	if (info->nSubobjects)
	    info->Flags |= OBJ_INFO_F_HAS_SUBOBJ;
	else
	    info->Flags |= OBJ_INFO_F_NO_SUBOBJ;

    return 0;
    }


/*** tmpCommit - commit any changes made to the underlying data source.
 ***/
int
tmpCommit(void* inf_v, pObjTrxTree* oxt)
    {
    return 0;
    }


/*** temp object handler initialization ***/
int
tmpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Setup the structure **/
	strcpy(drv->Name,"TMP - Temporary objects driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/tempobj");

	/** Setup the function references. **/
	drv->Open = tmpOpen;
	drv->OpenChild = tmpOpenChild;
	drv->Close = tmpClose;
	drv->Create = tmpCreate;
	drv->Delete = NULL;
	drv->DeleteObj = tmpDeleteObj;
	drv->OpenQuery = tmpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = tmpQueryFetch;
	drv->QueryClose = tmpQueryClose;
	drv->Read = tmpRead;
	drv->Write = tmpWrite;
	drv->GetAttrType = tmpGetAttrType;
	drv->GetAttrValue = tmpGetAttrValue;
	drv->GetFirstAttr = tmpGetFirstAttr;
	drv->GetNextAttr = tmpGetNextAttr;
	drv->SetAttrValue = tmpSetAttrValue;
	drv->AddAttr = tmpAddAttr;
	drv->OpenAttr = tmpOpenAttr;
	drv->GetFirstMethod = tmpGetFirstMethod;
	drv->GetNextMethod = tmpGetNextMethod;
	drv->ExecuteMethod = tmpExecuteMethod;
	drv->Info = tmpInfo;

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	OSYS.TempDriver = drv;

    return 0;
    }

