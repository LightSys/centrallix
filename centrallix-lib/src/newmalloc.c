#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "magic.h"
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
    //char data[size];
    //int magic_end;
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

int isinit=0;
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
    int i;

    	for(i=0;i<=MAX_SIZE;i++) lists[i]=NULL;
	for(i=0;i<=MAX_SIZE;i++) listcnt[i] = 0;
	for(i=0;i<=MAX_SIZE;i++) outcnt[i] = 0;
	for(i=0;i<=MAX_SIZE;i++) outcnt_delta[i] = 0;
	for(i=0;i<=MAX_SIZE;i++) blknames[i] = NULL;
	for(i=0;i<=MAX_SIZE;i++) usagecnt[i] = 0;
	for(i=0;i<=MAX_SIZE;i++) nmsys_outcnt[i] = 0;
	for(i=0;i<=MAX_SIZE;i++) nmsys_outcnt_delta[i] = 0;
#ifdef BLK_LEAK_CHECK
	for(i=0;i<MAX_BLOCKS;i++) blks[i] = NULL;
#endif
	nmFreeCnt=0;
	nmMallocCnt=0;
	nmMallocHits=0;
	nmMallocTooBig=0;
	nmMallocLargest=0;
#ifdef BUFFER_OVERFLOW_CHECKING
	startMemList=NULL;
#endif
	isinit = 1;

    return;
    }

#ifdef BUFFER_OVERFLOW_CHECKING
int
nmCheckItem(pMemStruct mem)
    {
    int ret=0;
    if(mem->magic_start!=MGK_MEMSTART)
	{
	printf("bad magic_start at %p (%p) -- 0x%08x != 0x%08x\n",MEMDATA(mem),mem,mem->magic_start,MGK_MEMSTART);
	ret = -1;
	}
    if(ENDMAGIC(mem)!=MGK_MEMEND)
	{
	printf("bad magic_end at %p (%p) -- 0x%08x != 0x%08x\n",MEMDATA(mem),mem,ENDMAGIC(mem),MGK_MEMEND);
	ret = -1;
	}
    return ret;
    }
#endif

void
nmCheckAll()
    {
#ifdef BUFFER_OVERFLOW_CHECKING
    pMemStruct mem;
    int ret=0;
    mem=startMemList;
    while(mem)
	{
	if(nmCheckItem(mem)==-1)
	    ret=-1;
	mem=mem->next;
	}
    if(ret==-1)
	{
	printf("causing segfault to halt.......\n");
	*(int*)NULL=0;
	}
#endif
    }

#ifdef BUFFER_OVERFLOW_CHECKING
void*
nmDebugMalloc(int size)
    {
    pMemStruct tmp;

    tmp = (pMemStruct)malloc(size+EXTRA_MEM);
    if(!tmp)
	return NULL;
    tmp->next = startMemList;
    startMemList=tmp;
    tmp->size=size;
    tmp->magic_start=MGK_MEMSTART;
    ENDMAGIC(tmp)=MGK_MEMEND;

    return (void*)MEMDATA(tmp);
    }

void
nmDebugFree(void *ptr)
    {
    pMemStruct tmp;
    pMemStruct prev;

    tmp = MEMDATATOSTRUCT(ptr);
    nmCheckItem(tmp);
    if(tmp==startMemList)
	{
	startMemList=tmp->next;
	}
    else
	{
	prev = startMemList;
	while(prev->next != tmp)
	    prev=prev->next;
	prev->next=tmp->next;
	}
    free(tmp);
    }

void* 
nmDebugRealloc(void *ptr,int newsize)
    {
    void *newptr;
    int oldsize;

    if(!ptr)
	return nmDebugMalloc(newsize);
    newptr=(void*)nmDebugMalloc(newsize);
    if(!newptr)
	return NULL;
    oldsize=MEMDATATOSTRUCT(ptr)->size;
    memmove(newptr,ptr,oldsize);
    nmDebugFree(ptr);
    return newptr;
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
    int i;
    pOverlay ov,del;

    	if (!isinit) nmInitialize();

    	for(i=MIN_SIZE;i<=MAX_SIZE;i++)
	    {
	    ov = lists[i];
	    while(ov)
	        {
		del = ov;
		ov = ov->Next;
		nmDebugFree(del);
		}
	    lists[i] = NULL;
	    }

    return;
    }

void*
nmMalloc(size)
    int size;
    {
    void* tmp;
#ifdef BLK_LEAK_CHECK
    int i;
#endif

    	if (!isinit) nmInitialize();

#ifdef GLOBAL_BLK_COUNTING
	nmMallocCnt++;
#endif

#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif

    	if (size <= MAX_SIZE && size >= MIN_SIZE)
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
		ASSERTMAGIC(tmp,MGK_FREEMEM);
	        lists[size]=lists[size]->Next;
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

	if (!tmp)
	    {
	    if (err_fn) err_fn("Insufficient system memory for operation.");
	    }
	else
	    {
	    OVERLAY(tmp)->Magic = MGK_ALLOCMEM;
	    }

#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif

#ifdef BLK_LEAK_CHECK
	for(i=0;i<MAX_BLOCKS;i++)
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
nmFree(ptr,size)
    void* ptr;
    int size;
    {
#ifndef NO_BLK_CACHE
#ifdef DUP_FREE_CHECK
    pOverlay tmp;
#endif
#endif
#ifdef BLK_LEAK_CHECK
    int i;
#endif

    	ASSERTNOTMAGIC(ptr,MGK_FREEMEM);

    	if (!ptr) return;

    	if (!isinit) nmInitialize();

#ifdef GLOBAL_BLK_COUNTING
	nmFreeCnt++;
#endif

#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif

#ifdef BLK_LEAK_CHECK
	for(i=0;i<MAX_BLOCKS;i++)
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
	    tmp = lists[size];
	    while(tmp)
	        {
		ASSERTMAGIC(OVERLAY(tmp),MGK_FREEMEM);
		if (OVERLAY(tmp) == OVERLAY(ptr))
		    {
		    printf("Duplicate nmFree()!!!  Size = %d, Address = %p\n",size,ptr);
		    if (err_fn) err_fn("Internal error - duplicate nmFree() occurred.");
		    return;
		    }
		tmp = OVERLAY(tmp)->Next;
		}
#endif
#ifdef SIZED_BLK_COUNTING
	    outcnt[size]--;
#endif
	    OVERLAY(ptr)->Next = lists[size];
	    lists[size] = OVERLAY(ptr);
#ifdef SIZED_BLK_COUNTING
	    listcnt[size]++;
#endif
	    OVERLAY(ptr)->Magic = MGK_FREEMEM;
	    }
	else
	    {
#endif
	    nmDebugFree(ptr);
#ifndef NO_BLK_CACHE
	    }
#endif

#ifdef BUFFER_OVERFLOW_CHECKING
	nmCheckAll();
#endif

    return;
    }


void
nmStats()
    {

    	if (!isinit) nmInitialize();

    	printf("NewMalloc subsystem statistics:\n");
	printf("   nmMalloc: %d calls, %d hits (%3.3f%%)\n",
		nmMallocCnt,
		nmMallocHits,
		(float)nmMallocHits/(float)nmMallocCnt*100.0);
	printf("   nmFree: %d calls\n", nmFreeCnt);
	printf("   bigblks: %d too big, %d largest size\n\n",
		nmMallocTooBig,
		nmMallocLargest);

    return;
    }


void
nmRegister(int size,char* name)
    {
    pRegisteredBlockType blk;

    	if (size > MAX_SIZE) return;

    	blk = (pRegisteredBlockType)malloc(sizeof(RegisteredBlockType));
	blk->Next = blknames[size];
	blk->Size = size;
	strcpy(blk->Name,name);
	blknames[size] = blk;
	blk->Magic = MGK_REGISBLK;

    return;
    }


void
nmDebug()
    {
    int i;
    pRegisteredBlockType blk;

	printf("size\tout\tcache\tusage\tnames\n");
    	for(i=0;i<MAX_SIZE;i++)
	    {
	    if (usagecnt[i] != 0)
	        {
		printf("%d\t%d\t%d\t%d\t",i,outcnt[i],listcnt[i],usagecnt[i]);
		blk = blknames[i];
		while(blk)
		    {
		    ASSERTMAGIC(blk,MGK_REGISBLK);
		    printf("%s ", blk->Name);
		    blk = blk->Next;
		    }
		printf("\n");
		}
	    }
	printf("\n-----\n");
	printf("size\toutcnt\n-------\t-------\n");
	for(i=0;i<=MAX_SIZE;i++)
	    {
	    if (nmsys_outcnt[i]) printf("%d\t%d\n",i,nmsys_outcnt[i]);
	    }
	printf("\n");

    return;
    }


void
nmDeltas()
    {
    int i, total;
    pRegisteredBlockType blk;

	total = 0;
	printf("size\tdelta\tnames\n-------\t-------\t-------\n");
    	for(i=0;i<=MAX_SIZE;i++)
	    {
	    if (outcnt[i] != outcnt_delta[i])
	        {
		printf("%d\t%d\t",i,outcnt[i] - outcnt_delta[i]);
		total += (i * (outcnt[i] - outcnt_delta[i]));
		blk = blknames[i];
		while(blk)
		    {
		    ASSERTMAGIC(blk,MGK_REGISBLK);
		    printf("%s ", blk->Name);
		    blk = blk->Next;
		    }
		printf("\n");
		outcnt_delta[i] = outcnt[i];
		}
	    }
	printf("\nsize\tdelta\n-------\t-------\n");
    	for(i=0;i<=MAX_SIZE;i++)
	    {
	    if (nmsys_outcnt[i] != nmsys_outcnt_delta[i])
	        {
		printf("%d\t%d\n",i,nmsys_outcnt[i] - nmsys_outcnt_delta[i]);
		total += (i * (nmsys_outcnt[i] - nmsys_outcnt_delta[i]));
		nmsys_outcnt_delta[i] = nmsys_outcnt[i];
		}
	    }
	printf("\n");
	printf("delta %d total bytes\n", total);

    return;
    }


void*
nmSysMalloc(int size)
    {
#ifdef NM_USE_SYSMALLOC
    char* ptr;
    ptr = (char*)nmDebugMalloc(size+sizeof(int));
    if (!ptr) return NULL;
    *(int*)ptr = size;
#ifdef SIZED_BLK_COUNTING
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]++;
#endif
    return (void*)(ptr+sizeof(int));
#else
    return (void*)nmDebugMalloc(size);
#endif
    }

void
nmSysFree(void* ptr)
    {
#ifdef NM_USE_SYSMALLOC
#ifdef SIZED_BLK_COUNTING
    int size;
    size = *(int*)(((char*)ptr)-sizeof(int));
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
#endif
    nmDebugFree(((char*)ptr)-sizeof(int));
#else
    nmDebugFree(ptr);
#endif
    return;
    }

void*
nmSysRealloc(void* ptr, int newsize)
    {
#ifdef NM_USE_SYSMALLOC
#ifdef SIZED_BLK_COUNTING
    int size;
#endif
    char* newptr;
    if (!ptr) return nmSysMalloc(newsize);
#ifdef SIZED_BLK_COUNTING
    size = *(int*)(((char*)ptr)-sizeof(int));
#endif
    newptr = (char*)nmDebugRealloc((((char*)ptr)-sizeof(int)), newsize+sizeof(int));
    if (!newptr) return NULL;
#ifdef SIZED_BLK_COUNTING
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
#endif
    *(int*)newptr = newsize;
#ifdef SIZED_BLK_COUNTING
    if (newsize > 0 && newsize <= MAX_SIZE) nmsys_outcnt[newsize]++;
#endif
    return (void*)(newptr+sizeof(int));
#else
    return (void*)nmDebugRealloc(ptr,newsize);
#endif
    }

char*
nmSysStrdup(const char* ptr)
    {
#ifdef NM_USE_SYSMALLOC
    char* newptr;
    int n = strlen(ptr);
    newptr = (char*)nmSysMalloc(n+1);
    if (!newptr) return NULL;
    memcpy(newptr,ptr,n+1);
    return newptr;
#else
    return strdup(ptr);
#endif
    }
