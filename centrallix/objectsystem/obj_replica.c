#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "obj.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2004 LightSys Technology Services, Inc.		*/
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
/* Module:	obj_replica.c                                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 19, 2004                                            */
/*									*/
/* Description:	Implements OSML-level replication services at several	*/
/*		levels.  The currently supported replication mechanism	*/
/*		is the grossly rudimentary OSML notification system	*/
/*		by which changes to an open object can be requested.	*/
/*									*/
/*		Performance N.B.:  it may turn out to be a good	tweak	*/
/*		to flag *all* currently open objects and newly opened	*/
/*		objects that changes on them could trigger notifies on	*/
/*		other objects.  It eliminates doing the must-we-notify	*/
/*		hash lookup, but requires an open-object scan every	*/
/*		time a new notify is registered or one is removed	*/
/************************************************************************/



/*** obj_internal_RnLookupByObj() - looks up a notify structure
 *** by an object handle which contains the pathname.  The pathname
 *** is the real key we're after unless the obj already has a notify
 *** item on it (short cut)
 ***/
pObjReqNotify
obj_internal_RnLookupByObj(pObject obj)
    {
    pObjReqNotify notify_data;
    char* pathname;

	/** short-cut: object has notification already enabled **/
	if (obj->NotifyItem)
	    return ((pObjReqNotifyItem)(obj->NotifyItem))->NotifyStruct;

	/** look it up in the hash table **/
	pathname = obj_internal_PathPart(obj->Pathname, 0, 0);
	notify_data = (pObjReqNotify)xhLookup(&(OSYS.NotifiesByPath), pathname);
	
    return notify_data;
    }


/*** obj_internal_RnAdd() - adds a new notification item to the
 *** hash table of notifies, possibly creating the parent ObjReqNotify
 *** structure if it does not already exist.
 ***
 *** Item properties to be set by caller: Flags, CallbackFn, CallerContext,
 *** Obj, and SavedSecContext.  This routine will set NotifyStruct.
 ***/
int
obj_internal_RnAdd(pObjReqNotifyItem item)
    {
    pObjReqNotify notify_data;

	xaInit(&item->Notifications, 16);

	/** Try to lookup the notify data **/
	notify_data = obj_internal_RnLookupByObj(item->Obj);

	/** Alloc if needed **/
	if (!notify_data)
	    {
	    notify_data = (pObjReqNotify)nmMalloc(sizeof(ObjReqNotify));
	    if (!notify_data) return -ENOMEM;
	    notify_data->TotalFlags = 0;
	    strcpy(notify_data->Pathname, obj_internal_PathPart(item->Obj->Pathname, 0, 0));
	    xaInit(&(notify_data->Requests), 16);
	    xhAdd(&(OSYS.NotifiesByPath), (void*)(notify_data->Pathname), (void*)notify_data);
	    }

	/** Merge the item into the parent data **/
	notify_data->TotalFlags |= item->Flags;
	xaAddItem(&(notify_data->Requests), (void*)item);
	item->NotifyStruct = notify_data;

	/** Add it to the object **/
	item->Next = item->Obj->NotifyItem;
	item->Obj->NotifyItem = item;

    return 0;
    }


/*** obj_internal_RnUpdateTotalFlags() - updates the TotalFlags on the
 *** parent after a change in the child items
 ***/
int
obj_internal_RnUpdateTotalFlags(pObjReqNotify notify_data)
    {
    int i;

	/** Loop through each item, recalculating the total flags **/
	notify_data->TotalFlags = 0;
	for(i=0;i<notify_data->Requests.nItems;i++)
	    {
	    notify_data->TotalFlags |= ((pObjReqNotifyItem)(notify_data->Requests.Items[i]))->Flags;
	    }

    return 0;
    }


/*** obj_internal_RnDelete() - removes an existing notification item
 ***/
int
obj_internal_RnDelete(pObjReqNotifyItem item)
    {
    pObjReqNotify notify_data;
    pObjReqNotifyItem *item_ptr;
    int i;
    pObjNotification n;

	/** get the parent notify_data structure **/
	notify_data = item->NotifyStruct;

	/** Remove item from parent structure **/
	xaRemoveItem(&(notify_data->Requests), xaFindItem(&(notify_data->Requests), (void*)item));

	/** Unlink from the object **/
	item_ptr = (pObjReqNotifyItem*)&(item->Obj->NotifyItem);
	while(*item_ptr != item) item_ptr = &((*item_ptr)->Next);
	*item_ptr = (*item_ptr)->Next;
	item->Next = NULL;

	/** Was that the last item? **/
	if (notify_data->Requests.nItems == 0)
	    {
	    /** Last item - deallocate and detach notify structure **/
	    xhRemove(&(OSYS.NotifiesByPath), (void*)(notify_data->Pathname));
	    xaDeInit(&(notify_data->Requests));
	    nmFree(notify_data, sizeof(ObjReqNotify));
	    }
	else
	    {
	    /** not last item - update TotalFlags via scan **/
	    obj_internal_RnUpdateTotalFlags(notify_data);
	    }

	/** Zero-out important fields in the item **/
	item->NotifyStruct = NULL;

	for(i=0;i<item->Notifications.nItems;i++)
	    {
	    n = (pObjNotification)(item->Notifications.Items[i]);
	    if (n->Worker)
		{
		thKill(n->Worker);
		n->Worker = NULL;
		obj_internal_RnFree(n);
		}
	    }
	xaDeInit(&item->Notifications);

    return 0;
    }


/*** obj_internal_RnCreateAttrNotify() - create an attribute-update notification
 *** structure.
 ***/
pObjNotification
obj_internal_RnCreateAttrNotify(pObjReqNotifyItem item, char* attrname, pTObjData newvalue)
    {
    pObjNotification n;

	/** Create the notification structure **/
	n = (pObjNotification)nmMalloc(sizeof(ObjNotification));
	if (!n) return NULL;
	n->Obj = item->Obj;
	n->Context = item->CallerContext;
	n->Worker = NULL;
	n->__Item = item;
	n->What = OBJ_RN_F_ATTRIB;
	memccpy(n->Name, attrname, 0, OBJSYS_MAX_ATTR-1);
	n->Name[OBJSYS_MAX_ATTR-1] = '\0';
	memcpy(&(n->NewAttrValue), newvalue, sizeof(TObjData));
	switch(newvalue->DataType)
	    {
	    case DATA_T_STRING:
		n->PtrSize = strlen(newvalue->Data.String)+1;
		break;
	    case DATA_T_BINARY:
		n->PtrSize = newvalue->Data.Binary.Size;
		break;
	    case DATA_T_DATETIME:
		n->PtrSize = sizeof(DateTime);
		break;
	    case DATA_T_MONEY:
		n->PtrSize = sizeof(MoneyType);
		break;
	    default:
		n->PtrSize = 0;
		break;
	    }
	if (n->PtrSize)
	    {
	    n->Ptr = nmSysMalloc(n->PtrSize);
	    memcpy(n->Ptr, newvalue->Data.Generic, n->PtrSize);
	    }
	else
	    {
	    n->Ptr = NULL;
	    }

    return n;
    }


/*** obj_internal_RnFree() - free an objNotification structure
 ***/
int
obj_internal_RnFree(pObjNotification n)
    {
    
	if (n->Ptr) nmSysFree(n->Ptr);
	nmFree(n, sizeof(ObjNotification));

    return 0;
    }


/*** obj_internal_RnSendNotify() - send a notification callback; this routine
 *** is the new thread entry point.
 ***/
void
obj_internal_RnSendNotify(void* v)
    {
    pObjNotification n = (pObjNotification)v;

	/** Set up thread context **/
	thSetName(NULL, "[Rn Callback]");
	thSetSecContext(NULL, &(n->__Item->SavedSecContext));

	/** do the callback **/
	n->Worker = NULL;
	xaRemoveItem(&(n->__Item->Notifications), xaFindItem(&(n->__Item->Notifications), (void*)n));
	n->__Item->CallbackFn(n);

	/** Free memory used for the callback **/
	obj_internal_RnFree(n);

    thExit();
    }


/*** obj_internal_RnNotifyAttrib() - notifies all requestors listening for
 *** changes to this object that an ATTRIBUTE value of the object has changed,
 *** or that an attribute has been added.
 ***/
int
obj_internal_RnNotifyAttrib(pObject this, char* attrname, pTObjData newvalue, int send_this)
    {
    pObjReqNotify notify_data;
    pObjNotification n;
    int i;
    pObjReqNotifyItem one_item;

	printf("RnNotifyAttrib: %s: ", attrname);
	ptodPrint(newvalue);

	/** GRB FIXME - disabling until we can make this work correctly. **/
	return 0;

	/** Lookup notify information **/
	notify_data = obj_internal_RnLookupByObj(this);
	if (!notify_data || !(notify_data->TotalFlags & OBJ_RN_F_ATTRIB)) 
	    return 0;

	/** Loop through requestors **/
	for(i=0;i<notify_data->Requests.nItems;i++)
	    {
	    one_item = (pObjReqNotifyItem)(notify_data->Requests.Items[i]);
	    if (!(one_item->Flags & OBJ_RN_F_ATTRIB)) continue;

	    /** Don't send notify if *this* object **/
	    if (this == one_item->Obj && !send_this) continue;

	    n = obj_internal_RnCreateAttrNotify(one_item, attrname, newvalue);
	    if (!n) return -ENOMEM;

	    /** Send the notify **/
	    xaAddItem(&one_item->Notifications, (void*)n);
	    n->Worker = thCreate(obj_internal_RnSendNotify, 0, (void*)n);
	    }
	    
    return 0;
    }


/*** obj_internal_RnSendInit() - send the initial state as a series of 
 *** notification callbacks; this routine is a new thread entry point.
 ***
 *** No need to set the security context here as the thread inherits
 *** it correctly from the caller.
 ***/
void
obj_internal_RnSendInit(void* v)
    {
    pObjReqNotifyItem item = (pObjReqNotifyItem)v;
    char* one_attr;
    TObjData tod;
    pObjNotification n;

	/** Send list of attributes **/
	one_attr = objGetFirstAttr(item->Obj);
	while(one_attr)
	    {
	    tod.DataType = objGetAttrType(item->Obj, one_attr);
	    if (tod.DataType > 0)
		{
		tod.Flags = 0;
		if (objGetAttrValue(item->Obj, one_attr, tod.DataType, &(tod.Data)) == 1)
		    tod.Flags |= DATA_TF_NULL;
		n = obj_internal_RnCreateAttrNotify(item, one_attr, &tod);
		item->CallbackFn(n);
		obj_internal_RnFree(n);
		}
	    one_attr = objGetNextAttr(item->Obj);
	    }

    thExit();
    }


/*** objRequestNotify() - this routine causes the OSML to invoke a 
 *** callback routine when certain aspects of an object have changed.
 *** This can be used to implement a rudimentary form of replication
 *** that keeps a second copy of an object in sync.  The notification
 *** is automatically cancelled when the object is closed.
 ***
 *** The callback function is invoked within the security context of
 *** the original caller of this function, not under the security
 *** context of the thread doing the reported modification.
 ***
 *** parameter 'what' = flags OBJ_RN_F_xxx
 ***/
int
objRequestNotify(pObject this, int (*callback_fn)(), void* context, int what)
    {
    pObjReqNotifyItem item = NULL;
    int rval;

	/** Object already has an item? **/
	if (this->NotifyItem) item = this->NotifyItem;

	/** Search for a match? **/
	while (item && item->CallerContext != context) item=item->Next;

	/** Deleting notifies? **/
	if (!what && item)
	    {
	    obj_internal_RnDelete(item);
	    nmFree(item, sizeof(ObjReqNotifyItem));
	    return 0;
	    }

	/** Modifying notifies? **/
	if (item && item->Flags != what)
	    {
	    item->Flags = what;
	    obj_internal_RnUpdateTotalFlags(item->NotifyStruct);
	    }

	/** New notify? **/
	if (!item)
	    {
	    item = (pObjReqNotifyItem)nmMalloc(sizeof(ObjReqNotifyItem));
	    if (!item) return -ENOMEM;
	    item->Obj = this;
	    item->Flags = what;
	    item->CallerContext = context;
	    item->CallbackFn = callback_fn;
	    item->Next = NULL;
	    thGetSecContext(NULL, &(item->SavedSecContext));
	    if ((rval = obj_internal_RnAdd(item)) < 0)
		{
		nmFree(item, sizeof(ObjReqNotifyItem));
		return rval;
		}
	    }

	/** Send initial attr/content/subobj data? **/
	if (what & OBJ_RN_F_INIT)
	    {
	    thCreate(obj_internal_RnSendInit, 0, (void*)item);
	    }

    return 0;
    }


/*** objDriverAttrEvent() - this function is called by an ObjectSystem driver
 *** to indicate that an attribute has been modified on a given object.  This
 *** routine is not a part of the upper-level OSML API and should not be used
 *** by anything other than an objectsystem driver.
 ***
 *** Set 'send_this' to 0 if events should not occur on the current object (if
 *** the object was just explicitly modified), or to 1 if events should occur
 *** on 'obj' (if the driver found out that the attribute changed in some
 *** other manner).
 ***/
int
objDriverAttrEvent(pObject obj, char* attr_name, pTObjData value, int send_this)
    {
    TObjData newval;
    int rval;

	/** Need to fetch the new value? **/
	if (!value)
	    {
	    value = &newval;
	    value->Flags = 0;
	    value->DataType = objGetAttrType(obj, attr_name);
	    rval = objGetAttrValue(obj, attr_name, value->DataType, &(value->Data));
	    if (rval == 1) value->Flags |= DATA_TF_NULL;
	    }

    return obj_internal_RnNotifyAttrib(obj, attr_name, value, send_this);
    }

