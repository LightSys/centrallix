#ifndef _SMMALLOC_H
#define _SMMALLOC_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	smmalloc.c, smmalloc.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 2005 					*/
/* Description: Shared memory segment memory manager.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: smmalloc.h,v 1.2 2005/03/14 20:41:24 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/smmalloc.h,v $

    $Log: smmalloc.h,v $
    Revision 1.2  2005/03/14 20:41:24  gbeeley
    - changed configuration to allow different levels of hardening (mainly, so
      asserts can be enabled without enabling the ds checksum stuff).
    - initial working version of the smmalloc (shared memory malloc) module.
    - test suite for smmalloc "make test".
    - results from test suite run on 1.4GHz Athlon, GCC 2.96, RH73
    - smmalloc not actually tested between two processes yet.
    - TO-FIX: smmalloc interprocess locking needs to be reworked to prefer
      spinlocks where doable instead of using sysv semaphores which are SLOW.

    Revision 1.1  2005/02/06 05:08:01  gbeeley
    - Adding interface spec for shared memory management

 **END-CVSDATA***********************************************************/

#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct _SM_T SmRegion, *pSmRegion;
typedef void (*pSmFinalizeFn)(void*, size_t);
typedef int (*pSmFreeReqFn)(pSmRegion, size_t);
typedef long long SmHandle;

#define SM_HANDLE_NONE		((SmHandle)(0LL))


/*** Shared memory manager system functions ***/
int smInitialize();


/*** Shared memory allocation & setup ***/
pSmRegion smCreate(size_t size);
pSmRegion smAttach(SmHandle handle);
SmHandle smGetHandle(pSmRegion rgn);
void smDetach(pSmRegion rgn);
void smDestroy(pSmRegion rgn);


/*** Shared memory management functions ***/
pSmRegion smInit(void* ptr, size_t size);
void* smMalloc(pSmRegion rgn, size_t size);
void* smRealloc(void* ptr, size_t new_size);
void smSetFinalize(void* ptr, pSmFinalizeFn fn);
void smSetFreeReq(pSmRegion rgn, pSmFreeReqFn fn);
void* smLinkTo(void* ptr);
void smFree(void* ptr);
void smDeInit(pSmRegion rgn);


/*** Relative pointer management functions ***/
void* smToRel(pSmRegion rgn, void* absolute_ptr);
void* smToAbs(pSmRegion rgn, void* relative_ptr);

#endif /* not defined _SMMALLOC_H */

