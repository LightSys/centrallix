#include "cxlibconfig-internal.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "smmalloc_private.h"
#include "magic.h"
#include "cxsec.h"

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

    $Id: smmalloc.c,v 1.2 2005/03/14 20:41:25 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/smmalloc.c,v $

    $Log: smmalloc.c,v $
    Revision 1.2  2005/03/14 20:41:25  gbeeley
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


/*** smInitialize() - init the shared memory manager.
 ***/
int 
smInitialize()
    {
    return 0;
    }


/*** smCreate() - allocate, attach to, and initialize a new shared memory
 *** segment of 'size' bytes.  NULL returned on error; errno set.
 ***/
pSmRegion 
smCreate(size_t size)
    {
    pSmRegion new_rgn;
    int shm_id = -1;
    key_t shm_key;
    int fd;
    int sem_id = -1;
    int arg;

	/** Figure out a key for the region - try a few different ways **/
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
	    {
	    /** Base it on the PID if no random device **/
	    shm_key = getpid();
	    }
	else
	    {
	    /** Try reading from the random device **/
	    if (read(fd, &shm_key, sizeof(shm_key)) != sizeof(shm_key))
		{
		/** PID fallback if read fails **/
		shm_key = getpid();
		}
	    close(fd);
	    }

	/** Keep trying until we succeed... **/
	while(shm_id == -1)
	    {
	    /** Allocate ourselves a new region **/
	    shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | 0600);
	    if (shm_id < 0 && errno != EEXIST) return NULL;
	    if (shm_id >= 0) break;

	    /** Try another key **/
	    shm_key++;
	    if (shm_key == IPC_PRIVATE) shm_key++;
	    }

	/** Ok, now attach to it **/
	new_rgn = shmat(shm_id, NULL, 0);
	if (!new_rgn) return NULL;

	/** Get the semaphore set **/
	sem_id = semget(shm_key, 1, IPC_CREAT | IPC_EXCL | 0600);
	if (sem_id < 0)
	    {
	    shmdt(new_rgn);
	    return NULL;
	    }
	arg = 0;
	semctl(sem_id, 0, SETVAL, arg);

	/** Init it **/
	smInit(new_rgn, size);

	/** Set handles **/
	new_rgn->Handle = shm_key;
	new_rgn->IntHandle = shm_id;
	new_rgn->LockHandle = sem_id;
	CXSEC_UPDATE(*new_rgn);

	/** Unlock it **/
	sm_internal_Unlock(new_rgn);

    return new_rgn;
    }


/*** smAttach() - given an existing shared memory region, attach to it.
 *** SmHandle is an opaque type that was returned by another smGetHandle()
 *** call from another process.  Returns NULL on error, which can include
 *** region not existing and not having permissions to the region.
 ***/
pSmRegion 
smAttach(SmHandle handle)
    {
    pSmRegion rgn;
    key_t shm_key = handle;
    int shm_id;

	/** Get the id for the region **/
	shm_id = shmget(shm_key, 0, 0);
	if (shm_id < 0) return NULL;

	/** Map the region **/
	rgn = shmat(shm_id, NULL, 0);
	if (!rgn) return NULL;

	/** Check it **/
	ASSERTMAGIC(rgn, MGK_SMREGION);
	assert(shm_id == rgn->IntHandle);
	if (rgn->Align != SM_BLK_ALIGN)
	    {
	    smDetach(rgn);
	    return NULL;
	    }

    return rgn;
    }


/*** smGetHandle() - return the opaque handle to a shared memory region that
 *** was created by smCreate(), or attached via smAttach().  Returns NULL if
 *** the region was only initialized via smInit() (for those regions, the
 *** program is expected to map the region manually).
 ***/
SmHandle 
smGetHandle(pSmRegion rgn)
    {
    ASSERTMAGIC(rgn, MGK_SMREGION);
    return rgn->Handle;
    }


/*** smDetach() - detach from a shared memory region given by 'rgn' by 
 *** unmapping it from the current process' address space.
 ***/
void 
smDetach(pSmRegion rgn)
    {

	shmdt((void*)rgn);

    return;
    }


/*** smDestroy() - destroy a shared memory region.  Depending on the underlying
 *** implementation, the region may hang around physically until the last
 *** process detaches from it.
 ***/
void 
smDestroy(pSmRegion rgn)
    {
    struct shmid_ds shminfo;
    int shm_id;

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** Get the id for the region **/
	if (rgn->IntHandle != 0 && rgn->Handle != 0)
	    {
	    /** Mark it for destruction **/
	    shm_id = rgn->IntHandle;
	    shmctl(shm_id, IPC_RMID, &shminfo);
	    }
	if (rgn->LockHandle != 0)
	    {
	    semctl(rgn->LockHandle, 0, IPC_RMID);
	    }

	/** Now detach. **/
	smDetach(rgn);

    return;
    }


/*** sm_internal_Lock() - lock access to the shared memory management
 *** metadata structures to prevent corruption
 ***/
int
sm_internal_Lock(pSmRegion rgn)
    {
    struct sembuf ops;

	/** Verify semaphore count... **/
	assert(semctl(rgn->LockHandle, 0, GETVAL) <= 1);

	ops.sem_num = 0;
	ops.sem_op = -1; /* wait on one count */
	ops.sem_flg = 0;
	while (semop(rgn->LockHandle, &ops, 1) < 0)
	    {
	    if (errno != EINTR && errno != EAGAIN) return -1;
	    }

    return 0;
    }


/*** sm_internal_Unlock() - unlock exclusive access to shared memory
 *** management metadata structures.
 ***/
int
sm_internal_Unlock(pSmRegion rgn)
    {
    struct sembuf ops;

	/** Verify that we are locked exactly once **/
	assert(semctl(rgn->LockHandle, 0, GETVAL) == 0);

	ops.sem_num = 0;
	ops.sem_op = 1; /* post one count */
	ops.sem_flg = 0;
	while (semop(rgn->LockHandle, &ops, 1) < 0)
	    {
	    if (errno != EINTR && errno != EAGAIN) return -1;
	    }

    return 0;
    }


/*** Shared memory management functions ***/
pSmRegion 
smInit(void* ptr, size_t size)
    {
    pSmRegion rgn = (pSmRegion)ptr;
    pSmBlock blk;
    int minsize;
    int i;

	/** size big enough? **/
	minsize = sizeof(SmRegion);
	minsize = (minsize + (SM_BLK_ALIGN-1)) & ~(SM_BLK_ALIGN-1);
	if (size <= minsize + sizeof(SmBlock)) return NULL;

	/** Set up the region **/
	SETMAGIC(rgn, MGK_SMREGION);
	rgn->Handle = SM_HANDLE_NONE;
	rgn->IntHandle = SM_HANDLE_NONE;
	rgn->Size = size;
	rgn->AllocSize = 0;
	rgn->nBlocks = 1;
	rgn->nAllocBlocks = 0;
	for(i=0;i<SM_MAX_FREEREQFN;i++)
	    {
	    rgn->FreeReqFns[i] = NULL;
	    rgn->FreeReqPids[i] = 0;
	    }

	/** Build the initial memory block **/
	blk = (pSmBlock)(((char*)rgn)+minsize);
	rgn->FreeBlkHead = SM_TOREL(rgn,blk);
	rgn->FreeBlkTail = rgn->FreeBlkHead;
	rgn->BlkHead = rgn->FreeBlkHead;
	rgn->BlkTail = rgn->FreeBlkHead;
	rgn->Align = SM_BLK_ALIGN;

	CXSEC_INIT(*rgn);

	SETMAGIC(blk, MGK_SMBLOCK);
	blk->Region = SM_GENOFFSET(pSmRegion, blk, rgn);
	blk->FreeBlkNext = NULL;
	blk->FreeBlkPrev = NULL;
	blk->BlkNext = NULL;
	blk->BlkPrev = NULL;
	blk->Size = size - minsize - sizeof(SmBlock);
	blk->Finalize = NULL;
	blk->LinkCnt = -1;

	CXSEC_INIT(*blk);

    return rgn;
    }


/*** sm_internal_Verify() - reverify the block chain...
 ***/
int
sm_internal_Verify(pSmRegion rgn)
    {
    pSmBlock blk, prev_blk, next_blk;

	/** Start at head. **/
	blk = SM_TOABS(rgn,rgn->BlkHead);
	prev_blk = SM_TOABS(rgn, NULL);
	next_blk = SM_TOABS(rgn, blk->BlkNext);

	/** Loop **/
	while (SM_TOREL(rgn,blk))
	    {
	    ASSERTMAGIC(blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*blk);
	    assert(SM_TOABS(rgn,blk->BlkPrev) == prev_blk);
	    assert(SM_TOABS(rgn,blk->BlkNext) == next_blk);
	    if (SM_TOREL(rgn,next_blk)) assert(SM_TOABS(rgn,next_blk->BlkPrev) == blk);
	    if (SM_TOREL(rgn,prev_blk)) assert(SM_TOABS(rgn,prev_blk->BlkNext) == blk);
	    if (SM_TOREL(rgn,prev_blk)) assert(((char*)prev_blk) + prev_blk->Size + sizeof(SmBlock) == ((char*)blk));
	    if (SM_TOREL(rgn,next_blk)) assert(((char*)blk) + blk->Size + sizeof(SmBlock) == ((char*)next_blk));
	    if (!SM_TOREL(rgn,prev_blk)) assert(SM_TOABS(rgn,rgn->BlkHead) == blk);
	    if (!SM_TOREL(rgn,next_blk)) assert(SM_TOABS(rgn,rgn->BlkTail) == blk);
	    prev_blk = blk;
	    blk = next_blk;
	    next_blk = SM_TOABS(rgn, blk->BlkNext);
	    }

	/** Loop **/
	blk = SM_TOABS(rgn,rgn->FreeBlkHead);
	prev_blk = SM_TOABS(rgn, NULL);
	if (blk) next_blk = SM_TOABS(rgn, blk->FreeBlkNext);
	else next_blk = SM_TOABS(rgn, NULL);

	while (SM_TOREL(rgn,blk))
	    {
	    ASSERTMAGIC(blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*blk);
	    assert(SM_TOABS(rgn,blk->FreeBlkPrev) == prev_blk);
	    assert(SM_TOABS(rgn,blk->FreeBlkNext) == next_blk);
	    if (SM_TOREL(rgn,next_blk)) assert(SM_TOABS(rgn,next_blk->FreeBlkPrev) == blk);
	    if (SM_TOREL(rgn,prev_blk)) assert(SM_TOABS(rgn,prev_blk->FreeBlkNext) == blk);
	    if (!SM_TOREL(rgn,prev_blk)) assert(SM_TOABS(rgn,rgn->FreeBlkHead) == blk);
	    if (!SM_TOREL(rgn,next_blk)) assert(SM_TOABS(rgn,rgn->FreeBlkTail) == blk);
	    prev_blk = blk;
	    blk = next_blk;
	    next_blk = SM_TOABS(rgn, blk->FreeBlkNext);
	    }

    return 0;
    }


/*** sm_internal_SplitBlock() - splits a SmBlock into two parts, the
 *** first of which is of a given size, the second contains the rest.
 ***
 *** ENTRY: region lock must be held.
 *** EXIT: block and region CXSEC-DS checksums *not* updated on blk,
 *** but are updated on 'remainder' block that is created.
 ***/
int
sm_internal_SplitBlock(pSmBlock blk, int size)
    {
    pSmBlock new_blk;
    pSmBlock tmp_blk;
    pSmRegion rgn = SM_OFFSET(pSmRegion, blk, blk->Region);

	/** set up new blk ptr **/
	ASSERTMAGIC(rgn, MGK_SMREGION);
	assert(blk->Size > size + sizeof(SmBlock));
	new_blk = (pSmBlock)(((char*)blk) + size + sizeof(SmBlock));
	new_blk->Size = blk->Size - size - sizeof(SmBlock);
	new_blk->Region = SM_GENOFFSET(pSmRegion, new_blk, rgn);
	new_blk->LinkCnt = -1;
	SETMAGIC(new_blk, MGK_SMBLOCK);
	blk->Size = size;

	/** Set up linkages: free block list **/
	new_blk->FreeBlkNext = blk->FreeBlkNext;
	new_blk->FreeBlkPrev = SM_TOREL(rgn,blk);
	blk->FreeBlkNext = SM_TOREL(rgn,new_blk);
	if (new_blk->FreeBlkNext) 
	    {
	    tmp_blk = SM_TOABS(rgn,new_blk->FreeBlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->FreeBlkPrev = SM_TOREL(rgn,new_blk);
	    }

	/** Set up linkages: all blocks list **/
	new_blk->BlkNext = blk->BlkNext;
	new_blk->BlkPrev = SM_TOREL(rgn,blk);
	blk->BlkNext = SM_TOREL(rgn,new_blk);
	if (new_blk->BlkNext)
	    {
	    tmp_blk = SM_TOABS(rgn,new_blk->BlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    if (new_blk->BlkNext != new_blk->FreeBlkNext)
		{
		CXSEC_VERIFY(*tmp_blk);
		}
	    tmp_blk->BlkPrev = SM_TOREL(rgn,new_blk);
	    }

	/** Region head/tail ptrs affected? **/
	if (rgn->BlkTail == SM_TOREL(rgn,blk)) rgn->BlkTail = SM_TOREL(rgn,new_blk);
	if (rgn->FreeBlkTail == SM_TOREL(rgn,blk)) rgn->FreeBlkTail = SM_TOREL(rgn,new_blk);
	rgn->nBlocks++;

	/** update ds checksums on new blk and next one too **/
	CXSEC_INIT(*new_blk);
	if (new_blk->BlkNext) CXSEC_UPDATE(*(pSmBlock)SM_TOABS(rgn,new_blk->BlkNext));
	if (new_blk->FreeBlkNext && new_blk->FreeBlkNext != new_blk->BlkNext) CXSEC_UPDATE(*(pSmBlock)SM_TOABS(rgn,new_blk->FreeBlkNext));

    return 0;
    }


/*** sm_internal_Free() - return a block to the free list.
 ***
 *** ENTRY: region lock must be held.
 *** EXIT: block and region cxsec ds sums *not* updated; sums on
 *** other blocks involved in the operation are updated.
 ***/
int
sm_internal_Free(pSmBlock blk)
    {
    pSmBlock prev_blk, next_blk ;
    pSmRegion rgn = SM_OFFSET(pSmRegion, blk, blk->Region);

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** Find the place to insert it **/
	next_blk = blk;
	while(SM_TOREL(rgn,next_blk) && next_blk->LinkCnt != -1)
	    {
	    ASSERTMAGIC(next_blk, MGK_SMBLOCK);
	    next_blk = SM_TOABS(rgn, next_blk->BlkNext);
	    }
	if (SM_TOREL(rgn,next_blk))
	    prev_blk = SM_TOABS(rgn,next_blk->FreeBlkPrev);
	else
	    prev_blk = SM_TOABS(rgn,rgn->FreeBlkTail);

	/** Update linkages **/
	if (SM_TOREL(rgn,next_blk))
	    {
	    CXSEC_VERIFY(*next_blk);
	    next_blk->FreeBlkPrev = SM_TOREL(rgn,blk);
	    CXSEC_UPDATE(*next_blk);
	    }
	if (SM_TOREL(rgn,prev_blk))
	    {
	    CXSEC_VERIFY(*prev_blk);
	    prev_blk->FreeBlkNext = SM_TOREL(rgn,blk);
	    CXSEC_UPDATE(*prev_blk);
	    }
	blk->FreeBlkNext = SM_TOREL(rgn,next_blk);
	blk->FreeBlkPrev = SM_TOREL(rgn,prev_blk);

	/** update region head / tail **/
	if (rgn->FreeBlkHead == SM_TOREL(rgn,next_blk)) rgn->FreeBlkHead = SM_TOREL(rgn,blk);
	if (rgn->FreeBlkTail == SM_TOREL(rgn,prev_blk)) rgn->FreeBlkTail = SM_TOREL(rgn,blk);

    return 0;
    }


/*** sm_internal_UnFree() - remove a block from the free list.
 ***
 *** ENTRY: region lock must be held.
 *** EXIT: block and region CXSEC-DS checksums *not* updated.
 ***/
int
sm_internal_UnFree(pSmBlock blk)
    {
    pSmBlock tmp_blk;
    pSmRegion rgn = SM_OFFSET(pSmRegion, blk, blk->Region);

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** unlink **/
	if (blk->FreeBlkPrev) 
	    {
	    tmp_blk = SM_TOABS(rgn, blk->FreeBlkPrev);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->FreeBlkNext = blk->FreeBlkNext;
	    CXSEC_UPDATE(*tmp_blk);
	    }
	if (blk->FreeBlkNext) 
	    {
	    tmp_blk = SM_TOABS(rgn, blk->FreeBlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->FreeBlkPrev = blk->FreeBlkPrev;
	    CXSEC_UPDATE(*tmp_blk);
	    }

	/** update region head / tail **/
	if (rgn->FreeBlkHead == SM_TOREL(rgn,blk)) rgn->FreeBlkHead = blk->FreeBlkNext;
	if (rgn->FreeBlkTail == SM_TOREL(rgn,blk)) rgn->FreeBlkTail = blk->FreeBlkPrev;

	blk->FreeBlkPrev = NULL;
	blk->FreeBlkNext = NULL;

    return 0;
    }


/*** sm_internal_CombineWithNext() - combines this block and the next
 *** block (which must be free) into one block.  The alloc/free status
 *** of the resulting block depends on the current block.
 ***
 *** ENTRY: region lock must be held.  Next blk must be free.
 *** EXIT: region and block cxsec not updated, other blocks are updated.
 ***/
int
sm_internal_CombineWithNext(pSmBlock blk)
    {
    pSmBlock next_blk, tmp_blk;
    pSmRegion rgn = SM_OFFSET(pSmRegion, blk, blk->Region);

	/** Some basic checking **/
	ASSERTMAGIC(rgn, MGK_SMREGION);
	assert(blk->BlkNext);
	next_blk = SM_TOABS(rgn, blk->BlkNext);
	ASSERTMAGIC(next_blk, MGK_SMBLOCK);
	assert(next_blk->LinkCnt == -1);

	/** Adjust the freelist **/
	CXSEC_UPDATE(*blk);
	if (sm_internal_UnFree(next_blk) < 0) return -1;

	/** Adjust the other linkages **/
	if (next_blk->BlkNext)
	    {
	    tmp_blk = SM_TOABS(rgn,next_blk->BlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->BlkPrev = SM_TOREL(rgn,blk);
	    CXSEC_UPDATE(*tmp_blk);
	    }
	blk->BlkNext = next_blk->BlkNext;

	/** Adjust block size **/
	blk->Size += (sizeof(SmBlock) + next_blk->Size);

	/** invalidate the hdr for the removed block **/
	SETMAGIC(next_blk, 0);
	rgn->nBlocks--;

	/** Adjust region head/tail **/
	if (rgn->FreeBlkTail == SM_TOREL(rgn,next_blk)) rgn->FreeBlkTail = SM_TOREL(rgn,blk);
	if (rgn->BlkTail == SM_TOREL(rgn,next_blk)) rgn->BlkTail = SM_TOREL(rgn,blk);

    return 0;
    }


/*** sm_internal_ResizeTo() - resize the current block by stealing some
 *** space from the next block, which must be free.
 ***
 *** ENTRY: region lock must be held.  Next blk must be free.
 *** EXIT: region and block cxsec not updated, but other blocks involved
 *** in the operation are updated.
 ***/
int
sm_internal_ResizeTo(pSmBlock blk, size_t new_size)
    {
    pSmBlock next_blk, tmp_blk, old_next_blk;
    pSmRegion rgn = SM_OFFSET(pSmRegion, blk, blk->Region);
    size_t offset;

	/** check **/
	ASSERTMAGIC(rgn, MGK_SMREGION);
	assert(blk->BlkNext);
	next_blk = SM_TOABS(rgn,blk->BlkNext);
	ASSERTMAGIC(next_blk, MGK_SMBLOCK);
	assert(next_blk->LinkCnt == -1);
	if (new_size <= blk->Size) return 0; /* don't shrink it */
	assert(new_size <= blk->Size + next_blk->Size - SM_BLK_ALIGN);
	CXSEC_VERIFY(*next_blk);

	/** compute blk move offset amount. **/
	offset = new_size - blk->Size;

	/** Adjust free list? **/
	if (next_blk->FreeBlkNext)
	    {
	    tmp_blk = SM_TOABS(rgn,next_blk->FreeBlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->FreeBlkPrev = (pSmBlock)(((char*)tmp_blk->FreeBlkPrev) + offset);
	    CXSEC_UPDATE(*tmp_blk);
	    }
	if (next_blk->FreeBlkPrev)
	    {
	    tmp_blk = SM_TOABS(rgn,next_blk->FreeBlkPrev);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->FreeBlkNext = (pSmBlock)(((char*)tmp_blk->FreeBlkNext) + offset);
	    CXSEC_UPDATE(*tmp_blk);
	    }

	/** Adjust main list? **/
	if (next_blk->BlkNext)
	    {
	    tmp_blk = SM_TOABS(rgn,next_blk->BlkNext);
	    ASSERTMAGIC(tmp_blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*tmp_blk);
	    tmp_blk->BlkPrev = (pSmBlock)(((char*)tmp_blk->BlkPrev) + offset);
	    CXSEC_UPDATE(*tmp_blk);
	    }
	blk->BlkNext = (pSmBlock)(((char*)blk->BlkNext) + offset);
	blk->Size += offset;
	next_blk->Size -= offset;

	/** move the blk header **/
	old_next_blk = next_blk;
	next_blk = (pSmBlock)(((char*)old_next_blk) + offset);
	ASSERTMAGIC(old_next_blk, MGK_SMBLOCK);
	SETMAGIC(old_next_blk, 0);
	memmove(next_blk, old_next_blk, sizeof(SmBlock));
	SETMAGIC(next_blk, MGK_SMBLOCK);
	next_blk->Region = SM_GENOFFSET(pSmRegion, next_blk, rgn);
	CXSEC_UPDATE(*next_blk);

	/** Region tail ptrs affected? **/
	if (rgn->FreeBlkTail == SM_TOREL(rgn,old_next_blk)) rgn->FreeBlkTail = SM_TOREL(rgn,next_blk);
	if (rgn->BlkTail == SM_TOREL(rgn,old_next_blk)) rgn->BlkTail = SM_TOREL(rgn,next_blk);
	if (rgn->FreeBlkHead == SM_TOREL(rgn,old_next_blk)) rgn->FreeBlkHead = SM_TOREL(rgn,next_blk);

    return 0;
    }


/*** smMalloc() - allocate a new block of memory from the shared
 *** memory region.  Pointer returned is absolute rather than being
 *** relative to the start of the region.
 ***/
void* 
smMalloc(pSmRegion rgn, size_t size)
    {
    void* ptr;
    pSmBlock blk;

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** Lock the region **/
	if (sm_internal_Lock(rgn) < 0) return NULL;

	CXSEC_VERIFY(*rgn);

	/** Round up **/
	size = (size + (SM_BLK_ALIGN-1)) & ~(SM_BLK_ALIGN-1);

	/** Scan for a suitable block **/
	for(blk = SM_TOABS(rgn,rgn->FreeBlkHead); SM_TOREL(rgn,blk) && blk->Size < size; blk=SM_TOABS(rgn,blk->FreeBlkNext))
	    {
	    /** use ISNTMAGIC just in case we have to unlock before croaking **/
	    if (ISNTMAGIC(blk, MGK_SMBLOCK))
		{
		sm_internal_Unlock(rgn);
		ASSERTMAGIC(blk, MGK_SMBLOCK);

		/** 'should' never get here; unlocked so return immediately **/
		return NULL;
		}
	    }
	if (!SM_TOREL(rgn,blk)) 
	    goto error_notmodified;
	CXSEC_VERIFY(*blk);
	assert(blk->LinkCnt == -1);

	/** Need to split block in two? (would there be usable Size left?) **/
	if (blk->Size - size >= SM_BLK_ALIGN + sizeof(SmBlock))
	    {
	    if (sm_internal_SplitBlock(blk, size) < 0)
		goto error;
	    }

	/** Pull block from the free chain **/
	if (sm_internal_UnFree(blk) < 0)
	    goto error;

	/** Update region statistics **/
	rgn->nAllocBlocks++;
	rgn->AllocSize += blk->Size;
	CXSEC_UPDATE(*rgn);
	sm_internal_Unlock(rgn);
	
	/** Set up allocated block info... **/
	blk->LinkCnt = 1;
	blk->Finalize = NULL;
	CXSEC_UPDATE(*blk);

	/** Compute ptr to data area of blk **/
	ptr = ((char*)blk) + sizeof(SmBlock);

	return ptr;

    error:
	/** unlock and tail outta here **/
	CXSEC_UPDATE(*rgn);
    error_notmodified:
	sm_internal_Unlock(rgn);
	return NULL;
    }


/*** smLinkTo() - link to an existing allocated memory area.
 ***/
void* 
smLinkTo(void* ptr)
    {
    pSmBlock blk;

	/** Check the actual block data structure **/
	if (!ptr) return NULL;
	blk = (pSmBlock)(((char*)ptr) - sizeof(SmBlock));
	ASSERTMAGIC(blk, MGK_SMBLOCK);
	CXSEC_VERIFY(*blk);
	if (blk->LinkCnt <= 0) return NULL; /* not allocated */

	/** Increment link cnt **/
	blk->LinkCnt++;
	CXSEC_UPDATE(*blk);

    return ptr;
    }


/*** smRealloc() - expand a given allocated area.  In order to make it
 *** bigger, we may have to move to a different block somewhere else.
 ***/
void* 
smRealloc(void* ptr, size_t new_size)
    {
    pSmBlock blk,tmp_blk;
    pSmRegion rgn;
    void* new_ptr;
    size_t old_size;

	/** Round **/
	new_size = (new_size + (SM_BLK_ALIGN-1)) & ~(SM_BLK_ALIGN-1);

	/** Check the actual block data structure **/
	if (!ptr) return NULL;
	blk = (pSmBlock)(((char*)ptr) - sizeof(SmBlock));
	ASSERTMAGIC(blk, MGK_SMBLOCK);
	CXSEC_VERIFY(*blk);
	if (blk->LinkCnt != 1) return NULL; /* fail if not linked exactly *once*  */

	/** This block is still OK? **/
	if (new_size <= blk->Size && new_size + sizeof(SmBlock) + SM_BLK_ALIGN > blk->Size)
	    return ptr;

	/** ok, we are going to have to lock the region **/
	rgn = SM_OFFSET(pSmRegion, blk, blk->Region);
	ASSERTMAGIC(rgn, MGK_SMREGION);
	if (sm_internal_Lock(rgn) < 0) return NULL;
	CXSEC_VERIFY(*rgn);

	/** Can we resize next block and snatch part of it? **/
	tmp_blk = SM_TOABS(rgn,blk->BlkNext);
	if (tmp_blk->LinkCnt >= 0 || new_size > blk->Size + sizeof(SmBlock) + tmp_blk->Size)
	    {
	    /** nope - just malloc, copy, and free. **/
	    sm_internal_Unlock(rgn);
	    new_ptr = smMalloc(rgn,new_size);
	    if (!new_ptr) return NULL;
	    memcpy(new_ptr, ptr, (blk->Size > new_size)?(new_size):(blk->Size));
	    smFree(ptr);
	    return new_ptr;
	    }

	/** Ok, move (or eliminate) the split point between the blocks **/
	old_size = blk->Size;
	if (new_size + SM_BLK_ALIGN > blk->Size + tmp_blk->Size)
	    {
	    /** Combine the blocks (not enough room would be left over) **/
	    if (sm_internal_CombineWithNext(blk) < 0)
		goto error;
	    }
	else
	    {
	    /** Move block boundary (resize this blk and next blk) **/
	    if (sm_internal_ResizeTo(blk, new_size) < 0)
		goto error;
	    }

	/** Update statistics and return. **/
	CXSEC_UPDATE(*blk);
	rgn->AllocSize += (blk->Size - old_size);
	CXSEC_UPDATE(*rgn);
	sm_internal_Unlock(rgn);
	return ptr;

    error:
	sm_internal_Unlock(rgn);
	return NULL;
    }


/*** smSetFinalize() - install a finalization handler, which will
 *** be called in the current process' context when the link count
 *** goes to zero on smFree().  **note** that the finalize routine
 *** might be called asynchronously, esp. if the last free occurred
 *** in a different process.
 ***/
void
smSetFinalize(void* ptr, pSmFinalizeFn fn)
    {
    pSmBlock blk;

	/** Check the actual block data structure **/
	if (!ptr) return;
	blk = (pSmBlock)(((char*)ptr) - sizeof(SmBlock));
	ASSERTMAGIC(blk, MGK_SMBLOCK);
	CXSEC_VERIFY(*blk);
	if (blk->LinkCnt <= 0) return; /* not allocated */

	blk->Finalize = fn;
	blk->FinalizePid = getpid();
	CXSEC_UPDATE(*blk);

    return;
    }


/*** smSetFreeReq() - this installs a callback function that the SM
 *** subsystem can invoke if it would like to free up some memory in
 *** the region.
 ***/
void
smSetFreeReq(pSmRegion rgn, pSmFreeReqFn fn)
    {
    int i,p;

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** Lock the region **/
	if (sm_internal_Lock(rgn) < 0) return;
	CXSEC_VERIFY(*rgn);

	/** Search for an open slot **/
	p = getpid();
	for(i=0;i<SM_MAX_FREEREQFN;i++)
	    {
	    if (rgn->FreeReqPids[i] == 0 || rgn->FreeReqPids[i] == p)
		{
		rgn->FreeReqFns[i] = fn;
		if (fn)
		    rgn->FreeReqPids[i] = p;
		else
		    rgn->FreeReqPids[i] = 0;
		break;
		}
	    }

	CXSEC_UPDATE(*rgn);
	sm_internal_Unlock(rgn);

    return;
    }


/*** smFree() - decrement the reference count to an allocated block, and if the
 *** ref count goes to 0, free it.
 ***
 *** note: Finalize not yet implemented.
 ***/
void 
smFree(void* ptr)
    {
    pSmBlock blk;
    pSmRegion rgn;
    int size;

	/** Check the actual block data structure **/
	if (!ptr) return;
	blk = (pSmBlock)(((char*)ptr) - sizeof(SmBlock));
	ASSERTMAGIC(blk, MGK_SMBLOCK);
	CXSEC_VERIFY(*blk);
	if (blk->LinkCnt <= 0) return; /* not allocated */

	/** Lock the region **/
	rgn = SM_OFFSET(pSmRegion, blk, blk->Region);
	ASSERTMAGIC(rgn, MGK_SMREGION);
	if (sm_internal_Lock(rgn) < 0) return;
	CXSEC_VERIFY(*rgn);

	/** Check ref count **/
	blk->LinkCnt--;
	if (blk->LinkCnt > 0)
	    {
	    CXSEC_UPDATE(*blk);
	    sm_internal_Unlock(rgn);
	    return;
	    }

	/** Ok, ref cnt went to zero.  Free the thing. **/
	size = blk->Size;
	if (sm_internal_Free(blk) < 0)
	    goto error;
	blk->LinkCnt = -1;

	/** Try to combine block with prev and next **/
	if (blk->FreeBlkNext && blk->FreeBlkNext == blk->BlkNext)
	    {
	    /** combine with next block **/
	    if (sm_internal_CombineWithNext(blk) < 0)
		goto error_rgnmodified;
	    }
	if (blk->FreeBlkPrev && blk->FreeBlkPrev == blk->BlkPrev)
	    {
	    /** combine with previous block **/
	    CXSEC_UPDATE(*blk);
	    blk = SM_TOABS(rgn,blk->FreeBlkPrev);
	    ASSERTMAGIC(blk, MGK_SMBLOCK);
	    CXSEC_VERIFY(*blk);
	    if (sm_internal_CombineWithNext(blk) < 0)
		goto error_rgnmodified;
	    }
	CXSEC_UPDATE(*blk);

	/** Update statistics and get outta here **/
	rgn->nAllocBlocks--;
	rgn->AllocSize -= size;
	CXSEC_UPDATE(*rgn);
	sm_internal_Unlock(rgn);
	return;

    error_rgnmodified:
	CXSEC_UPDATE(*rgn);
    error:
	CXSEC_UPDATE(*blk);
	return;
    }


/*** smDeInit() - deinitialize a region; this is invoked once an existing 
 *** shared memory region is to no longer be used (by any process).
 ***/
void 
smDeInit(pSmRegion rgn)
    {

	ASSERTMAGIC(rgn, MGK_SMREGION);
	SETMAGIC(rgn, 0);

    return;
    }


/*** smToRel() - convert an absolute pointer to a relative one.  Takes a 
 *** pointer to an area in the shared memory region and converts it to a
 *** pointer relative to that shared memory region, for portability between
 *** processes with different mapping start addresses.  Use this function on
 *** a pointer to shared memory before placing the pointer where another 
 *** process might see it (in shared memory or in a message queue, etc.).
 ***/
void* 
smToRel(pSmRegion rgn, void* absolute_ptr)
    {
    return SM_TOREL(rgn, absolute_ptr);
    }


/*** smToAbs() - takes a relative pointer (returned by smToRel) and converts
 *** it back to an absolute address that is usable in the current process
 *** context.  Use this on a pointer to shared memory that you have received
 *** from another process or that came from an area where other processes might
 *** see the pointer.
 ***/
void* 
smToAbs(pSmRegion rgn, void* relative_ptr)
    {
    return SM_TOABS(rgn, relative_ptr);
    }

