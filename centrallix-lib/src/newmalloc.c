#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "magic.h"

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

/**CVSDATA***************************************************************

    $Id: newmalloc.c,v 1.1 2001/08/13 18:04:22 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/newmalloc.c,v $

    $Log: newmalloc.c,v $
    Revision 1.1  2001/08/13 18:04:22  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:02:53  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


typedef struct _ov
    {
    int		Magic;
    struct _ov 	*Next;
    }
    Overlay,*pOverlay;

#define OVERLAY(x) ((pOverlay)(x))
#define MAX_SIZE (8192)
#define MIN_SIZE (sizeof(Overlay))
#define DUP_FREE_CHECK 1

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
	nmFreeCnt=0;
	nmMallocCnt=0;
	nmMallocHits=0;
	nmMallocTooBig=0;
	nmMallocLargest=0;

	isinit = 1;

    return;
    }


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
		free(del);
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

    	if (!isinit) nmInitialize();

	nmMallocCnt++;

    	if (size <= MAX_SIZE && size >= MIN_SIZE)
	    {
	    outcnt[size]++;
	    usagecnt[size]++;
	    if (lists[size] == NULL)
		{
		tmp = (void*)malloc(size);
		}
	    else
		{
		nmMallocHits++;
	        tmp = lists[size];
		ASSERTMAGIC(tmp,MGK_FREEMEM)
	        lists[size]=lists[size]->Next;
		listcnt[size]--;
		}
	    }
	else
	    {
	    nmMallocTooBig++;
	    if (size > nmMallocLargest) nmMallocLargest = size;
	    tmp = (void*)malloc(size);
	    }

	if (!tmp && err_fn) err_fn("Insufficient system memory for operation.");
	else OVERLAY(tmp)->Magic = MGK_ALLOCMEM;
	
    return tmp;
    }


void
nmFree(ptr,size)
    void* ptr;
    int size;
    {
    pOverlay tmp;

    	ASSERTNOTMAGIC(ptr,MGK_FREEMEM)

    	if (!ptr) return;

    	if (!isinit) nmInitialize();

	nmFreeCnt++;

    	if (size <= MAX_SIZE && size >= MIN_SIZE)
	    {
#ifdef DUP_FREE_CHECK
	    tmp = lists[size];
	    while(tmp)
	        {
		ASSERTMAGIC(tmp,MGK_FREEMEM)
		if (tmp == OVERLAY(ptr))
		    {
		    printf("Duplicate nmFree()!!!  Size = %d, Address = %8.8x\n",size,(unsigned int)ptr);
		    if (err_fn) err_fn("Internal error - duplicate nmFree() occurred.");
		    return;
		    }
		tmp = tmp->Next;
		}
#endif
	    outcnt[size]--;
	    OVERLAY(ptr)->Next = lists[size];
	    lists[size] = OVERLAY(ptr);
	    listcnt[size]++;
	    OVERLAY(ptr)->Magic = MGK_FREEMEM;
	    }
	else
	    {
	    free(ptr);
	    }

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
		    ASSERTMAGIC(blk,MGK_REGISBLK)
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
    int i;
    pRegisteredBlockType blk;

	printf("size\tdelta\tnames\n-------\t-------\t-------\n");
    	for(i=0;i<=MAX_SIZE;i++)
	    {
	    if (outcnt[i] != outcnt_delta[i])
	        {
		printf("%d\t%d\t",i,outcnt[i] - outcnt_delta[i]);
		blk = blknames[i];
		while(blk)
		    {
		    ASSERTMAGIC(blk,MGK_REGISBLK)
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
		nmsys_outcnt_delta[i] = nmsys_outcnt[i];
		}
	    }
	printf("\n");

    return;
    }


void*
nmSysMalloc(int size)
    {
#ifdef NM_USE_SYSMALLOC
    char* ptr;
    ptr = (char*)malloc(size+4);
    *(int*)ptr = size;
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]++;
    return (void*)(ptr+4);
#else
    return (void*)malloc(size);
#endif
    }

void
nmSysFree(void* ptr)
    {
#ifdef NM_USE_SYSMALLOC
    int size;
    size = *(int*)(((char*)ptr)-4);
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
    free(((char*)ptr)-4);
#else
    free(ptr);
#endif
    return;
    }

void*
nmSysRealloc(void* ptr, int newsize)
    {
#ifdef NM_USE_SYSMALLOC
    int size;
    char* newptr;
    if (!ptr) return nmSysMalloc(newsize);
    size = *(int*)(((char*)ptr)-4);
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
    newptr = (char*)realloc((((char*)ptr)-4), newsize);
    *(int*)newptr = newsize;
    if (newsize > 0 && newsize <= MAX_SIZE) nmsys_outcnt[newsize]++;
    return (void*)(newptr+4);
#else
    return (void*)realloc(ptr,newsize);
#endif
    }

char*
nmSysStrdup(char* ptr)
    {
#ifdef NM_USE_SYSMALLOC
    char* newptr;
    int n = strlen(ptr);
    newptr = (char*)nmSysMalloc(n+1);
    memcpy(newptr,ptr,n+1);
    return newptr;
#else
    return strdup(ptr);
#endif
    }
