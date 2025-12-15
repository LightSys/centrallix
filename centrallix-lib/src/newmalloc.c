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
/* Module:	NewMalloc memory manager (newmalloc.c, .h)              */
/* Author:	Greg Beeley (GRB)                                       */
/*									*/
/* Description:	This module provides block-caching memory allocation	*/
/*		and debugging services, to help find memory leaks as	*/
/*		well.  It also interacts with the magic number module	*/
/*		to ensure memory allocation consistency.  Slower in 	*/
/*		debugging mode but can improve app speed dramatically	*/
/*		in implementation mode.  While blocks returned from the	*/
/*		nm* routines can be mixed-and-matched with the normall	*/
/*		libc functions, this defeats the purpose of the system	*/
/*		and leads to inaccuracies in the debugging information.	*/
/*		The returned blocks from the nmSys* functions CANNOT	*/
/*		be intermixed with normal malloc/free.  In short, if	*/
/*		you use this library, DONT use normal malloc/free at	*/
/*		all....							*/
/************************************************************************/

#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "magic.h"
#include "newmalloc.h"


/** define BUFFER_OVERFLOW_CHECKING for buffer overflow checking
***   this works off of magic numbers in the 4 bytes on either end
***   of the buffer that is returned to the user, at the cost of
***   16 bytes of memory per buffer, and a full scan of the list
***   of allocated memory twice per nmMalloc() or nmFree() call
***
*** the check can be made at any time from normal code by calling:
***   nmCheckAll()
***     -- this functions is still defined if BUFFER_OVERFLOW_CHECKING is
***        not defined, but it becomes a NOOP
**/
#ifdef BUFFER_OVERFLOW_CHECKING
typedef struct _mem
    {
    int size;
    struct _mem *next;
    int magic_start;
    /** not 'really' here **/
    // char data[size];
    // int magic_end;
    }
    MemStruct, *pMemStruct;
#define EXTRA_MEM (3*sizeof(int)+sizeof(void*))
#define MEMSTRUCT(x) ((pMemStruct)(x))
#define MEMDATA(x) ((void*)((char*)(x)+(sizeof(int)*2+sizeof(void*))))
#define ENDMAGIC(x) (*((int*)((char*)(MEMDATA(x))+MEMSTRUCT(x)->size)))
#define MEMDATATOSTRUCT(x) ((pMemStruct)((char*)(x)-(sizeof(int)*2+sizeof(void*))))
pMemStruct startMemList;
#endif

pOverlay lists[MAX_SIZE+1];
int listcnt[MAX_SIZE+1];
int outcnt[MAX_SIZE+1];
int outcnt_delta[MAX_SIZE+1];
int usagecnt[MAX_SIZE+1];
int nmFreeCnt;
int nmMallocCnt;
int nmMallocHits;
int nmMallocTooBig;
int nmMallocLargest;

#ifdef BLK_LEAK_CHECK
void* blks[MAX_BLOCKS];
int blksiz[MAX_BLOCKS];
#endif

bool is_init = false;
int (*err_fn)() = NULL;

int nmsys_outcnt[MAX_SIZE+1];
int nmsys_outcnt_delta[MAX_SIZE+1];

typedef struct _RB
    {
    int		Magic;
    struct _RB*	Next;
    char	Name[64];
    int		Size;
    }
    RegisteredBlockType, *pRegisteredBlockType;

pRegisteredBlockType blknames[MAX_SIZE+1];


void
nmInitialize()
    {
	for (int i = 0; i <= MAX_SIZE; i++) lists[i] = NULL;
	for (int i = 0; i <= MAX_SIZE; i++) listcnt[i] = 0;
	for (int i = 0; i <= MAX_SIZE; i++) outcnt[i] = 0;
	for (int i = 0; i <= MAX_SIZE; i++) outcnt_delta[i] = 0;
	for (int i = 0; i <= MAX_SIZE; i++) blknames[i] = NULL;
	for (int i = 0; i <= MAX_SIZE; i++) usagecnt[i] = 0;
	for (int i = 0; i <= MAX_SIZE; i++) nmsys_outcnt[i] = 0;
	for (int i = 0; i <= MAX_SIZE; i++) nmsys_outcnt_delta[i] = 0;
	
#ifdef BLK_LEAK_CHECK
	for (int i = 0; i < MAX_BLOCKS; i++) blks[i] = NULL;
#endif
	
	nmFreeCnt = 0;
	nmMallocCnt = 0;
	nmMallocHits = 0;
	nmMallocTooBig = 0;
	nmMallocLargest = 0;
	
#ifdef BUFFER_OVERFLOW_CHECKING
	startMemList = NULL;
#endif
	
	is_init = true;
    
    return;
    }


#ifdef BUFFER_OVERFLOW_CHECKING
int
nmCheckItem(pMemStruct mem)
    {
    int ret = 0;
    
	if (mem->magic_start != MGK_MEMSTART)
	    {
	    fprintf(stderr,
		"Bad magic_start at %p (%p) -- 0x%08x != 0x%08x\n",
		MEMDATA(mem), mem, mem->magic_start, MGK_MEMSTART
	    );
	    ret = -1;
	    }
	
	if (ENDMAGIC(mem) != MGK_MEMEND)
	    {
	    fprintf(stderr,
		"Bad magic_end at %p (%p) -- 0x%08x != 0x%08x\n",
		MEMDATA(mem), mem, ENDMAGIC(mem), MGK_MEMEND
	    );
	    ret = -1;
	    }
    
    return ret;
    }
#endif


void
nmCheckAll()
    {
#ifdef BUFFER_OVERFLOW_CHECKING
    int ret = 0;
    
	for (pMemStruct mem = startMemList; mem != NULL; mem = mem->next)
	    if (nmCheckItem(mem) == -1) ret = -1;
	
	if (ret == -1)
	    {
	    printf("causing segfault to halt.......\n");
	    *(int*)NULL = 0;
	    }
#endif
    
    return;
    }


#ifdef BUFFER_OVERFLOW_CHECKING
void*
nmDebugMalloc(int size)
    {
	pMemStruct tmp = (pMemStruct)malloc(size + EXTRA_MEM);
	if(tmp == NULL) return NULL;
	
	tmp->size = size;
	tmp->magic_start = MGK_MEMSTART;
	ENDMAGIC(tmp) = MGK_MEMEND;
	
	tmp->next = startMemList;
	startMemList = tmp;

    return (void*)MEMDATA(tmp);
    }


void
nmDebugFree(void* ptr)
    {
	pMemStruct tmp = MEMDATATOSTRUCT(ptr);
	
	nmCheckItem(tmp);
	
	if (tmp == startMemList)
	    {
	    startMemList = tmp->next;
	    }
	else
	    {
	    pMemStruct prev = startMemList;
	    while (prev->next != tmp)
		prev = prev->next;
	    
	    prev->next = tmp->next;
	    }
	
	free(tmp);
    
    return;
    }


void*
nmDebugRealloc(void* ptr, int new_size)
    {
	if (ptr == NULL) return nmDebugMalloc(new_size);
	
	void* new_ptr = (void*)nmDebugMalloc(new_size);
	if (new_ptr == NULL) return NULL;	
	
	int old_size = MEMDATATOSTRUCT(ptr)->size;
	memmove(new_ptr, ptr, old_size);
	
	nmDebugFree(ptr);
    
    return new_ptr;
    }
#else
 #define nmDebugMalloc(size) malloc(size)
 #define nmDebugFree(ptr) free(ptr)
 #define nmDebugRealloc(ptr,size) realloc(ptr,size)
#endif


void
nmSetErrFunction(int (*error_fn)())
    {
	err_fn = error_fn;
    
    return;
    }


void
nmClear()
    {
	if (!is_init) nmInitialize();
	
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    pOverlay ov = lists[size];
	    while (ov != NULL)
		{
		pOverlay del = ov;
		ov = ov->Next;
		nmDebugFree(del);
		}
	    lists[size] = NULL;
	    }
    
    return;
    }


void*
nmMalloc(int size)
    {
    void* tmp = NULL;
    
	if (!is_init) nmInitialize();
	
#ifdef GLOBAL_BLK_COUNTING
	nmMallocCnt++;
#endif
	
#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif
	
	if (MIN_SIZE <= size && size <= MAX_SIZE)
	    {
#ifdef SIZED_BLK_COUNTING
	    outcnt[size]++;
	    usagecnt[size]++;
#endif
	    
	    if (lists[size] == NULL)
		{
		tmp = (void*)nmDebugMalloc(size);
		}
	    else
		{
#ifdef GLOBAL_BLK_COUNTING
		nmMallocHits++;
#endif
		
		tmp = lists[size];
		ASSERTMAGIC(tmp, MGK_FREEMEM);
		lists[size] = lists[size]->Next;
		
#ifdef SIZED_BLK_COUNTING
		listcnt[size]--;
#endif
		}
	    }
	else
	    {
#ifdef GLOBAL_BLK_COUNTING
	    nmMallocTooBig++;
	    if (size > nmMallocLargest) nmMallocLargest = size;
#endif
	    
	    tmp = (void*)nmDebugMalloc(size);
	    }
	
	if (tmp == NULL)
	    {
	    if (err_fn) err_fn("Insufficient system memory for operation.");
	    }
	else
	    {
	    if (size >= MIN_SIZE)
		OVERLAY(tmp)->Magic = MGK_ALLOCMEM;
	    }
	
#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif
	
#ifdef BLK_LEAK_CHECK
	for (int i = 0; i < MAX_BLOCKS; i++)
	    {
	    if (blks[i] == NULL)
		{
		blks[i] = tmp;
		blksiz[i] = size;
		break;
		}
	    }
#endif
    
    return tmp;
    }


void
nmFree(void* ptr, int size)
    {
	if (!is_init) nmInitialize();
	
	if (size >= MIN_SIZE)
	    ASSERTNOTMAGIC(ptr, MGK_FREEMEM);
	
	if (ptr == NULL) return;
	
#ifdef GLOBAL_BLK_COUNTING
	nmFreeCnt++;
#endif
	
#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif
	
#ifdef BLK_LEAK_CHECK
	for (int i = 0; i < MAX_BLOCKS; i++)
	    {
	    if (blks[i] == ptr)
		{
		blks[i] = NULL;
		blksiz[i] = 0;
		break;
		}
	    }
#endif
	
#ifndef NO_BLK_CACHE
	if (size <= MAX_SIZE && size >= MIN_SIZE)
	    {
 #ifdef DUP_FREE_CHECK
	    for (pOverlay tmp = lists[size]; tmp != NULL; tmp = OVERLAY(tmp)->Next)
		{
		ASSERTMAGIC(OVERLAY(tmp),MGK_FREEMEM);
		if (OVERLAY(tmp) == OVERLAY(ptr))
		    {
		    printf("Duplicate nmFree()!!!  Size = %d, Address = %p\n", size, ptr);
		    if (err_fn) err_fn("Internal error - duplicate nmFree() occurred.");
		    return;
		    }
		}
 #endif
	    
	    OVERLAY(ptr)->Magic = MGK_FREEMEM;
	    OVERLAY(ptr)->Next = lists[size];
	    lists[size] = OVERLAY(ptr);
	    ptr = NULL;
	    
 #ifdef SIZED_BLK_COUNTING
	    outcnt[size]--;
	    listcnt[size]++;
 #endif
	    }
#endif
	
	if (ptr != NULL)
	    {
	    nmDebugFree(ptr);
	    ptr = NULL;
	    }
	
#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif
    
    return;
    }


void
nmStats()
    {
	if (!is_init) nmInitialize();
	
	printf(
	    "NewMalloc subsystem statistics:\n"
	    "   nmMalloc: %d calls, %d hits (%3.3f%%)\n"
	    "   nmFree: %d calls\n"
	    "   bigblks: %d too big, %d largest size\n\n",
	    nmMallocCnt, nmMallocHits, (float)nmMallocHits / (float)nmMallocCnt * 100.0,
	    nmFreeCnt,
	    nmMallocTooBig, nmMallocLargest
	    );
    
    return;
    }


void
nmRegister(int size, char* name)
    {
    pRegisteredBlockType blk = NULL;
    
	if (size > MAX_SIZE) return;
	
	blk = (pRegisteredBlockType)malloc(sizeof(RegisteredBlockType));
	if (blk == NULL) return;
	blk->Next = blknames[size];
	blknames[size] = blk;
	
	blk->Magic = MGK_REGISBLK;
	blk->Size = size;
	strcpy(blk->Name, name);
    
    return;
    }


void
nmDebug()
    {
	printf("size\tout\tcache\tusage\tnames\n");
	
	for (size_t size = MIN_SIZE; size < MAX_SIZE; size++)
	    {
	    if (usagecnt[size] == 0) continue;
	    
	    printf("%ld\t%d\t%d\t%d\t", size, outcnt[size], listcnt[size], usagecnt[size]);
	    
	    for (pRegisteredBlockType blk = blknames[size]; blk != NULL; blk = blk->Next)
		{
		ASSERTMAGIC(blk, MGK_REGISBLK);
		printf("%s ", blk->Name);
		}
	    
	    printf("\n");
	    }
	
	printf("\n-----\n");
	printf("size\toutcnt\n-------\t-------\n");
	
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    if (nmsys_outcnt[size] == 0) continue;
	    
	    printf("%ld\t%d\n", size, nmsys_outcnt[size]);
	    }
	printf("\n");
    
    return;
    }


void
nmDeltas()
    {
	printf("size\tdelta\tnames\n-------\t-------\t-------\n");
	
	int total_delta = 0;
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    if (outcnt[size] == outcnt_delta[size]) continue;
	    
	    printf("%ld\t%d\t", size, outcnt[size] - outcnt_delta[size]);
	    total_delta += (size * (outcnt[size] - outcnt_delta[size]));
	    
	    for (pRegisteredBlockType blk = blknames[size]; blk != NULL; blk = blk->Next)
		{
		ASSERTMAGIC(blk, MGK_REGISBLK);
		printf("%s ", blk->Name);
		}
	    
	    printf("\n");
	    
	    outcnt_delta[size] = outcnt[size];
	    }
	
	printf("\nsize\tdelta\n-------\t-------\n");
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    if (nmsys_outcnt[size] == nmsys_outcnt_delta[size]) continue;
	    
	    printf("%ld\t%d\n", size, nmsys_outcnt[size] - nmsys_outcnt_delta[size]);
	    total_delta += (size * (nmsys_outcnt[size] - nmsys_outcnt_delta[size]));
	    nmsys_outcnt_delta[size] = nmsys_outcnt[size];
	    }
	printf("\n");
	
	printf("delta %d total bytes\n", total_delta);
    
    return;
    }


void*
nmSysMalloc(int size)
    {
#ifndef NM_USE_SYSMALLOC
	return (void*)nmDebugMalloc(size);
#else
	
	char* ptr = (char*)nmDebugMalloc(sizeof(int) + size);
	if (ptr == NULL) return NULL;
	
	*(int*)ptr = size;
	
 #ifdef SIZED_BLK_COUNTING
	if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]++;
 #endif
	
	return (void*)(sizeof(int) + ptr);
#endif
    
    return NULL; /** Unreachable. **/
    }


void
nmSysFree(void* ptr)
    {
#ifndef NM_USE_SYSMALLOC
	nmDebugFree(ptr);
#else
	
 #ifdef SIZED_BLK_COUNTING
	int size;
	size = *(int*)(((char*)ptr)-sizeof(int));
	if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
 #endif
	
	nmDebugFree(((char*)ptr) - sizeof(int));
#endif
    
    return;
    }


void*
nmSysRealloc(void* ptr, int new_size)
    {
#ifndef NM_USE_SYSMALLOC
	return (void*)nmDebugRealloc(ptr, new_size);
#else
	
	if (ptr == NULL) return nmSysMalloc(new_size);
	
	void* buffer_ptr = ((char*)ptr) - sizeof(int);
	const int size = *(int*)buffer_ptr;
	if (size == new_size) return ptr;
	
	char* new_ptr = (char*)nmDebugRealloc(buffer_ptr, sizeof(int) + new_size);
	if (new_ptr == NULL) return NULL;
	
	*(int*)new_ptr = new_size;
	
 #ifdef SIZED_BLK_COUNTING
	if (0 < size && size <= MAX_SIZE) nmsys_outcnt[size]--;
	if (0 < new_size && new_size <= MAX_SIZE) nmsys_outcnt[new_size]++;
 #endif
	
	return (void*)(sizeof(int) + new_ptr);
#endif
	
    return NULL; /** Unreachable. **/
    }


char*
nmSysStrdup(const char* str)
    {
#ifndef NM_USE_SYSMALLOC
	return strdup(str);
#else
	
	size_t n = strlen(str) + 1u;
	char* new_str = (char*)nmSysMalloc(n);
	if (new_str == NULL) return NULL;
	
	memcpy(new_str, str, n);
	
	return new_str;
#endif
    
    return NULL; /** Unreachable. **/
    }
