#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "xhashqueue.h"
#include "xhash.h"
#include "xarray.h"
#include "mtsession.h"
#include "newmalloc.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:      xhashqueue.c, xhashqueue.h                              */
/* Author:      Greg Beeley (GRB)                                       */
/* Creation:    January 12, 2000                                        */
/* Description: Extension of the xhash system for Centrallix that      */
/*              allows for the entries in the hash table to be auto-    */
/*              matically removed after a certain amount of time or     */
/*              accesses to the table.  Normally uses a LRU discard     */
/*              algorithm, with locking of entries to avoid possible    */
/*              race conditions.  This module is especially applicable  */
/*              as a cache controller.                                  */
/*		See the header file for more information.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xhashqueue.c,v 1.2 2003/04/03 04:32:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xhashqueue.c,v $

    $Log: xhashqueue.c,v $
    Revision 1.2  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

    Revision 1.1.1.1  2001/08/13 18:04:23  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:02:57  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/*** xhqInit - initialize a (pre-allocated) xhashqueue data structure and
 *** allocate the hash table and queue structures, etc.
 ***/
int 
xhqInit(pXHashQueue this, int maxcnt, int keylen, int hashtablesize, int (*discard_fn)(), int (*full_fn)())
    {
    
    	/** Setup the hash table first. **/
	xhInit(&(this->Table), hashtablesize, keylen);

	/** Setup this data structure **/
	this->MaxElements = maxcnt;
	this->NumElements = 0;
	this->NextItem = &(this->Queue);
	this->DiscardFn = discard_fn;
	this->FullFn = full_fn;
	this->ControlLock = syCreateSem(1, 0);
	this->Queue.Next = &(this->Queue);
	this->Queue.Prev = &(this->Queue);

    return 0;
    }


/*** xhqDeInit - shutdown a hashqueue and deallocate structures used
 *** for it.  Does not deallocate the XHashQueue structure itself.
 ***/
int 
xhqDeInit(pXHashQueue this)
    {
    pXHQElement xe;

    	/** Lock the xhq first **/
	xhqLock(this);

	/** Iterate and attempt to discard all elements. **/
	for(xe = xhqGetFirst(this); xe; xe = xhqGetNext(this))
	    {
	    this->DiscardFn(this,xe,xe->Locked);
	    xhRemove(&(this->Table), xe->KeyPtr);
	    nmFree(xe,sizeof(XHQElement));
	    }

	/** DeInit the hash table **/
	xhDeInit(&(this->Table));

	/** Deinit the semaphore **/
	syDestroySem(this->ControlLock, 0);
	this->ControlLock = NULL;

    return 0;
    }


/*** xhq_internal_DiscardOne - discard one element in the queue to make
 *** room for another.  Call with the queue locked.
 ***/
int
xhq_internal_DiscardOne(pXHashQueue this)
    {
    pXHQElement xe;

    	/** Point to the queue tail and check one by one. **/
	xe = this->Queue.Prev;
	while(xe != &(this->Queue))
	    {
	    if (xe->LinkCnt == 0)
	        {
		xhRemove(&(this->Table), xe->KeyPtr);
		if (this->DiscardFn && this->DiscardFn(this,xe,xe->Locked) < 0)
		    {
		    xhAdd(&(this->Table), xe->KeyPtr, (void*)xe);
		    }
		else if (this->DiscardFn || !xe->Locked)
		    {
		    xe->Prev->Next = xe->Next;
		    xe->Next->Prev = xe->Prev;
		    nmFree(xe,sizeof(XHQElement));
		    this->NumElements--;
		    break;
		    }
		}
	    xe = xe->Prev;
	    }
	if (xe == &(this->Queue)) return -1;

    return 0;
    }


/*** xhq_internal_EnQueue - place an XE structure at the head of the
 *** queue.  Unlink it if it is already linked elsewhere.
 ***/
int
xhq_internal_EnQueue(pXHashQueue this, pXHQElement xe)
    {

	/** Already at queue head? **/
	if (this->Queue.Next == xe) return 0;

	/** Unlink if need be. **/
	if (xe->Next) xe->Next->Prev = xe->Prev;
	if (xe->Prev) xe->Prev->Next = xe->Next;

	/** Now link into queue head **/
	xe->Next = this->Queue.Next;
	xe->Prev = &(this->Queue);
	xe->Next->Prev = xe;
	xe->Prev->Next = xe;

    return 0;
    }


/*** xhqAdd - add an element to the hashqueue, with the given key and
 *** data values.  Returns an opaque reference pointer, which must be
 *** xhqUnlink()ed by the calling routine (or xhqRemove()d, though that
 *** doesn't seem to make much sense right after xhqAdd()...)
 ***/
pXHQElement 
xhqAdd(pXHashQueue this, void* key, void* data)
    {
    pXHQElement xe;

    	/** Lock the queue. **/
	syGetSem(this->ControlLock, 1, 0);

    	/** Need to make room? **/
	while (this->NumElements >= this->MaxElements)
	    {
	    if (xhq_internal_DiscardOne(this) < 0)
	        {
		if (!this->FullFn || this->FullFn(this) < 0)
		    {
		    mssError((this->FullFn)?0:1,"XHQ","HashQueue is full, could not make room");
		    syPostSem(this->ControlLock, 1, 0);
		    return NULL;
		    }
		}
	    }

	/** Create xe structure **/
	xe = (pXHQElement)nmMalloc(sizeof(XHQElement));
	if (!xe)
	    {
	    syPostSem(this->ControlLock, 1, 0);
	    return NULL;
	    }
	xe->Locked = 0;
	xe->LinkCnt = 1;
	xe->KeyPtr = key;
	xe->DataPtr = data;
	xe->Prev = NULL;
	xe->Next = NULL;

	/** Add to LRU discard queue and hash table. **/
	if (xhAdd(&(this->Table), key, (void*)xe) < 0)
	    {
	    /** dup key! **/
	    nmFree(xe,sizeof(XHQElement));
	    syPostSem(this->ControlLock,1,0);
	    return NULL;
	    }
	xhq_internal_EnQueue(this, xe);
	this->NumElements++;
	syPostSem(this->ControlLock, 1, 0);

    return xe;
    }


/*** xhqLookup - find an element in the hash table and return it.  The 
 *** returned opaque pointer can then be either Removed or Unlinked.
 ***/
pXHQElement 
xhqLookup(pXHashQueue this, void* key)
    {
    pXHQElement xe;

    	/** Lock the queue first **/
	syGetSem(this->ControlLock, 1, 0);

	/** Lookup the item. **/
	xe = (pXHQElement)xhLookup(&(this->Table), key);
	if (xe) xe->LinkCnt++;
	
	syPostSem(this->ControlLock, 1, 0);

    return xe;
    }


/*** xhqGetData - return the data pointer for an opaque reference pointer.
 *** the data pointer is the one passed to xhqAdd.
 ***/
void* 
xhqGetData(pXHashQueue this, pXHQElement item, int flags)
    {
    return item->DataPtr;
    }


/*** xhqRemove - remove an element from the hashqueue.  If the discard
 *** function (if any) is NOT to be called, use XHQ_UF_NOCALLDISCARD in
 *** the 'flags' parameter.
 ***/
int 
xhqRemove(pXHashQueue this, pXHQElement item, int flags)
    {

    	/** Linked? **/
	if (item->LinkCnt != 1)
	    {
	    mssError(1,"XHQ","Inappropriate link count %d for xhqRemove", item->LinkCnt);
	    return -1;
	    }

    	/** Lock the queue. **/
	if (!(flags & XHQ_UF_PRELOCKED)) syGetSem(this->ControlLock, 1, 0);

	/** Call discard function? **/
	xhRemove(&(this->Table), item->KeyPtr);
	if (!(flags & XHQ_UF_NOCALLDISCARD) && this->DiscardFn)
	    {
	    if (this->DiscardFn(this, item, item->Locked) < 0)
	        {
		xhAdd(&(this->Table), item->KeyPtr, (void*)item);
		if (!(flags & XHQ_UF_PRELOCKED)) syPostSem(this->ControlLock, 1, 0);
		return -1;
		}
	    }
	else if (item->Locked)
	    {
	    xhAdd(&(this->Table), item->KeyPtr, (void*)item);
	    if (!(flags & XHQ_UF_PRELOCKED)) syPostSem(this->ControlLock, 1, 0);
	    return -1;
	    }

	/** Unlink the element and free it. **/
	item->Next->Prev = item->Prev;
	item->Prev->Next = item->Next;
	nmFree(item,sizeof(XHQElement));
	this->NumElements--;
	if (!(flags & XHQ_UF_PRELOCKED)) syPostSem(this->ControlLock, 1, 0);

    return 0;
    }


/*** xhqUnlink - unlink from an element.
 **/
int 
xhqUnlink(pXHashQueue this, pXHQElement item, int flags)
    {

	if (!(flags & XHQ_UF_PRELOCKED)) syGetSem(this->ControlLock, 1, 0);
	xhq_internal_EnQueue(this,item);
        item->LinkCnt--;
	if (!(flags & XHQ_UF_PRELOCKED)) syPostSem(this->ControlLock, 1, 0);

    return 0;
    }


/*** xhqLink - link to an element.
 ***/
int
xhqLink(pXHashQueue this, pXHQElement item, int flags)
    {

	if (!(flags & XHQ_UF_PRELOCKED)) syGetSem(this->ControlLock, 1, 0);
        item->LinkCnt++;
	if (!(flags & XHQ_UF_PRELOCKED)) syPostSem(this->ControlLock, 1, 0);

    return 0;
    }


/*** xhqLock - lock the entire hashqueue.
 ***/
int 
xhqLock(pXHashQueue this)
    {
    syGetSem(this->ControlLock, 1, 0);
    return 0;
    }


/*** xhqUnlock - unlock the hashqueue.
 ***/
int 
xhqUnlock(pXHashQueue this)
    {
    syPostSem(this->ControlLock, 1, 0);
    return 0;
    }


/*** xhqLockItem - lock a particular item into memory so it normally
 *** cannot be discarded without further processing first.
 ***/
int
xhqLockItem(pXHQElement item, int flags)
    {
    item->Locked = 1;
    return 0;
    }


/*** xhqUnlockItem - unlock a particular item after it has been
 *** locked.
 ***/
int
xhqUnlockItem(pXHQElement item, int flags)
    {
    item->Locked = 0;
    return 0;
    }


/*** xhqGetFirst - return the first element in the hashqueue, as
 *** a preparation to iterate through the rest of them using xhqGetNext.
 *** Returns NULL if no items in the hashqueue.  Call with queue locked.
 ***/
pXHQElement 
xhqGetFirst(pXHashQueue this)
    {
    pXHQElement xe;

	this->NextItem = this->Queue.Next;
	xe = xhqGetNext(this);

    return xe;
    }


/*** xhqGetNext - get the next element in the hashqueue, as iterating
 *** through the elements.  Returns NULL if no more elements.  Call with
 *** the queue locked.
 ***/
pXHQElement 
xhqGetNext(pXHashQueue this)
    {
    pXHQElement xe;

    	xe = this->NextItem;
	if (xe == &(this->Queue)) return NULL;
	this->NextItem = xe->Next;

    return xe;
    }

