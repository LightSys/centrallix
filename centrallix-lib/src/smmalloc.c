#include "cxlibconfig-internal.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "smmalloc_private.h"

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

    $Id: smmalloc.c,v 1.1 2005/02/26 04:32:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/smmalloc.c,v $

    $Log: smmalloc.c,v $
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
	    shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL);
	    if (shm_id < 0 && errno != EEXIST) return NULL;
	    if (shm_id >= 0) break;

	    /** Try another key **/
	    shm_key++;
	    if (shm_key == IPC_PRIVATE) shm_key++;
	    }

	/** Ok, now attach to it **/
	new_rgn = shmat(shm_id, NULL, 0);
	if (!new_rgn) return NULL;

	/** Init it **/
	smInit(new_rgn, size);
	new_rgn->Handle = shm_key;
	new_rgn->IntHandle = shm_id;

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
    key_t shm_key;
    int shm_id;

	ASSERTMAGIC(rgn, MGK_SMREGION);

	/** Get the id for the region **/
	if (rgn->IntHandle > 0 && rgn->Handle > 0)
	    {
	    /** Mark it for destruction **/
	    shmctl(shm_id, IPC_RMID, &shminfo);
	    }

	/** Now detach. **/
	smDetach(rgn);

    return;
    }


/*** Shared memory management functions ***/
pSmRegion 
smInit(void* ptr, size_t size)
    {
    pSmRegion rgn = (pSmRegion)ptr;
    pSmBlock blk;
    int minsize;

	/** size big enough? **/
	minsize = sizeof(SmRegion);
	minsize = (minsize + 63) & ~63;
	if (size <= minsize + sizeof(SmBlock)) return NULL;

	/** Set up the region **/
	SETMAGIC(rgn, MGK_SMREGION);
	rgn->Handle = SM_HANDLE_NONE;
	rgn->IntHandle = SM_HANDLE_NONE;
	rgn->Size = size;
	rgn->AllocSize = 0;
	rgn->nBlocks = 1;
	rgn->nAllocBlocks = 0;

	/** Build the initial memory block **/
	blk = (pSmBlock)(((char*)rgn)+minsize);
	rgn->FreeBlkHead = blk;
	rgn->FreeBlkTail = blk;
	rgn->BlkHead = blk;
	rgn->BlkTail = blk;

	CXSEC_INIT(rgn);

	SETMAGIC(blk, MGK_SMBLOCK);
	blk->Region = rgn;
	blk->FreeBlkNext = NULL;
	blk->FreeBlkPrev = NULL;
	blk->BlkNext = NULL;
	blk->BlkPrev = NULL;
	blk->Size = size - minsize;

	CXSEC_INIT(blk);

    return rgn;
    }

void* smMalloc(pSmRegion rgn, size_t size);
void smSetFinalize(void* ptr, pSmFinalizeFn fn);
void* smLinkTo(void* ptr);
void smFree(void* ptr);
void smDeInit(pSmRegion rgn);


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

