#ifndef _SMMALLOC_PRIVATE_H
#define _SMMALLOC_PRIVATE_H

#ifdef CXLIB_INTERNAL
#include "smmalloc.h"
#include "cxsec.h"
#else
#include "cxlib/smmalloc.h"
#include "cxlib/cxsec.h"
#endif

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

    $Id: smmalloc_private.h,v 1.2 2005/03/14 20:41:24 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/smmalloc_private.h,v $

    $Log: smmalloc_private.h,v $
    Revision 1.2  2005/03/14 20:41:24  gbeeley
    - changed configuration to allow different levels of hardening (mainly, so
      asserts can be enabled without enabling the ds checksum stuff).
    - initial working version of the smmalloc (shared memory malloc) module.
    - test suite for smmalloc "make test".
    - results from test suite run on 1.4GHz Athlon, GCC 2.96, RH73
    - smmalloc not actually tested between two processes yet.
    - TO-FIX: smmalloc interprocess locking needs to be reworked to prefer
      spinlocks where doable instead of using sysv semaphores which are SLOW.

    Revision 1.1  2005/02/26 04:32:59  gbeeley
    - continued implementation of the shared-memory malloc module

    Revision 1.1  2005/02/06 05:08:01  gbeeley
    - Adding interface spec for shared memory management

 **END-CVSDATA***********************************************************/


/*** alignment for blocks; must be a power of 2.  This also
 *** sets the minimum amount of address space that is allocated
 *** for each request.
 ***/
#define SM_BLK_ALIGN	(8)

#define SM_MAX_FREEREQFN	(32)

/*** SmBlock - allocated or unallocated memory block.
 ***
 *** Note: LinkCnt == 0 means that the block is in the process of
 *** being freed.  LinkCnt == -1 means it is free.
 ***/
typedef struct _SM_B
    {
    int		Magic;
    CXSEC_DS_BEGIN;
    pSmRegion	Region;
    struct _SM_B* FreeBlkNext;
    struct _SM_B* FreeBlkPrev;
    struct _SM_B* BlkNext;
    struct _SM_B* BlkPrev;
    size_t	Size;
    int		LinkCnt;
    pSmFinalizeFn Finalize;		/* finalize routine */
    int		FinalizePid;		/* process in which to run the finalize routine */
    CXSEC_DS_END;
    }
    SmBlock, *pSmBlock;


/*** SmRegion - shared memory region ***/
struct _SM_T 
    {
    int		Magic;
    int		Lock;			/* unused with current sysv sem implementation */
    CXSEC_DS_BEGIN;
    SmHandle	Handle;
    SmHandle	IntHandle;		/* only used in some implementations */
    SmHandle	LockHandle;
    pSmBlock	FreeBlkHead;
    pSmBlock	FreeBlkTail;
    pSmBlock	BlkHead;
    pSmBlock	BlkTail;
    pSmFreeReqFn FreeReqFns[SM_MAX_FREEREQFN];
    int		FreeReqPids[SM_MAX_FREEREQFN];
    int		nFreeReqFns;
    int		Align;			/* alignment - used just to verify */
    size_t	Size;			/* total region size */
    size_t	AllocSize;
    int		nBlocks;
    int		nAllocBlocks;
    CXSEC_DS_END;
    };

int sm_internal_Lock(pSmRegion rgn);
int sm_internal_Unlock(pSmRegion rgn);


/** offset logic is used for region ptrs within the region **/
#define SM_OFFSET(pt,p,o) (pt)(((char*)p) + (ssize_t)(o))
#define SM_GENOFFSET(pt,p,tp) (pt)(((char*)tp) - ((char*)p))

/** abs/rel logic is used for block ptrs **/
#define SM_TOREL(r,p) ((void*)(((char*)p) - ((char*)r)))
#define SM_TOABS(r,p) ((void*)(((char*)r) + ((size_t)p)))

#endif /* not defined _SMMALLOC_PRIVATE_H */

