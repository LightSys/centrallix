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

