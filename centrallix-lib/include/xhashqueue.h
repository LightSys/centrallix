#ifndef _XHASHQUEUE_H
#define _XHASHQUEUE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:      xhashqueue.c, xhashqueue.h                              */
/* Author:      Greg Beeley (GRB)                                       */
/* Creation:    January 12, 2000                                        */
/* Description: Extension of the xhash system for Centrallix that 	*/
/*		allows for the entries in the hash table to be auto-	*/
/*		matically removed after a certain amount of time or	*/
/*		accesses to the table.  Normally uses a LRU discard	*/
/*		algorithm, with locking of entries to avoid possible	*/
/*		race conditions.  This module is especially applicable	*/
/*		as a cache controller.					*/
/*									*/
/* Usage:	This module is used by first creating a hashqueue with	*/
/*		xhqInit, using the hashqueue as described below, and	*/
/*		then de-initializing the thing with xhqDeInit.  The	*/
/*		XHashQueue datastructure itself must be pre-allocated.	*/
/*									*/
/*		To add an item to the queue, use xhqAdd.  A reference	*/
/*		pointer is returned, which should be treated as opaque.	*/
/*		with this reference pointer, you can get the pointer	*/
/*		to the item's data using xhqGetData, lock the item in	*/
/*		memory using xhqLockItem, or unlock it to free it to be	*/
/*		discarded at some point, using xhqUnlockItem.  When 	*/
/*		done with the reference to the item, call xhqUnlink.	*/
/*		This is necessary to prevent discarding of an item	*/
/*		while a routine is using it.  Such link counting is	*/
/*		different from the lock/unlock feature for items; the	*/
/*		latter is used to hold an item in memory because some	*/
/*		further processing on it needs to take place at some	*/
/*		later point, even though the reference is unlinked.	*/
/*									*/
/*		To find an item in the queue, use xhqLookup.  A 	*/
/*		reference pointer is returned; see the Add section above*/
/*		for more information.					*/
/*									*/
/*		To remove an item, first it must be added or looked up.	*/
/*		Then xhqRemove can be called in lieu of xhqUnlink.  If	*/
/*		the discard_fn SHOULD NOT be called as a result of the	*/
/*		xhqRemove, include XHQ_UF_NOCALLDISCARD in flags.	*/
/*									*/
/*		To iterate through items in the hashqueue, first lock	*/
/*		the queue using xhqLock, iterate using xhqGetFirst and	*/
/*		xhqGetNext, and then unlock using xhqUnlock.		*/
/*									*/
/*		To perform operations on the queue during iteration	*/
/*		while the queue is externally locked, pass the flag	*/
/*		XHQ_UF_PRELOCKED to the routines that handle an element.*/
/*									*/
/*		When an element is to be discarded, the discard_fn	*/
/*		is called with the queue and the element as the first	*/
/*		and second parameters.  The hashqueue is locked, so 	*/
/*		operations can be performed with the PRELOCKED flag.	*/
/*		The discard routine SHOULD NOT FREE XHQElement MEMORY,	*/
/*		but rather only consider the data from xhqGetData.	*/
/*		The third parameter passed is a status indication of	*/
/*		whether the element is xhqLockItem'ed.  Most discard	*/
/*		routines will fail if this is true (1).  Other such	*/
/*		routines will attempt to tidy things up so the element	*/
/*		can be successfully discarded.	The function should	*/
/*		return 0 on success, -1 on failure to discard.		*/
/*									*/
/*		The full_fn is called when an attempt is made to add	*/
/*		an element to the queue, but MaxElements has been 	*/
/*		reached and no elements could be successfully discarded.*/
/*		This function should return 0 if it was able to free up	*/
/*		some elements, or -1 if no more elements were able to	*/
/*		be freed and the xhqAdd should fail.  This function is 	*/
/*		called with the queue locked.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xhashqueue.h,v 1.2 2005/02/26 04:32:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xhashqueue.h,v $

    $Log: xhashqueue.h,v $
    Revision 1.2  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#ifdef CXLIB_INTERNAL
#include "xhash.h"
#include "xarray.h"
#include "mtask.h"
#else
#include "cxlib/xhash.h"
#include "cxlib/xarray.h"
#include "cxlib/mtask.h"
#endif


/*** Structure for an element in the hashqueue ***/
typedef struct _HE
    {
    struct _HE*	Next;
    struct _HE*	Prev;
    void*	DataPtr;	/* pointer to data area */
    void*	KeyPtr;		/* pointer to key value for hash */
    int		LinkCnt;	/* references to this element */
    int		Locked;
    }
    XHQElement, *pXHQElement;

/*** Structure for the hashqueue ***/
typedef struct _HQ
    {
    XHashTable	Table;		/* hash table for lookups */
    XHQElement	Queue;		/* queue for LRU discard */
    int		NumElements;	/* current element count */
    int		MaxElements;	/* maximum elements permitted */
    pSemaphore	ControlLock;	/* sync access to the hashqueue */
    pXHQElement	NextItem;	/* for getfirst/getnext */
    int		(*DiscardFn)();	/* function called on discard of element */
    int		(*FullFn)();	/* function called on discard of element */
    }
    XHashQueue, *pXHashQueue;


#define XHQ_UF_PRELOCKED	1
#define XHQ_UF_NOCALLDISCARD	2

/*** Functions used to access the hashqueue ***/
int xhqInit(pXHashQueue this, int maxcnt, int keylen, int hashtablesize, int (*discard_fn)(), int (*full_fn)());
int xhqDeInit(pXHashQueue this);
pXHQElement xhqAdd(pXHashQueue this, void* key, void* data);
pXHQElement xhqLookup(pXHashQueue this, void* key);
void* xhqGetData(pXHashQueue this, pXHQElement item, int flags);
int xhqRemove(pXHashQueue this, pXHQElement item, int flags);
int xhqUnlink(pXHashQueue this, pXHQElement item, int flags);
int xhqLink(pXHashQueue this, pXHQElement item, int flags);
int xhqLock(pXHashQueue this);
int xhqUnlock(pXHashQueue this);
int xhqLockItem(pXHQElement item, int flags);
int xhqUnlockItem(pXHQElement item, int flags);
pXHQElement xhqGetFirst(pXHashQueue this);
pXHQElement xhqGetNext(pXHashQueue this);


#endif /* not defined _XHASHQUEUE_H */

