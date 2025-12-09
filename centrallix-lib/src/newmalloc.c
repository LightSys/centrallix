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
/* Module:	NewMalloc memory manager (newmalloc.c, .h)		*/
/* Author:	Greg Beeley (GRB)					*/
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


/*** BUFFER_OVERFLOW_CHECKING adds 4 bytes of magic data to either end of
 *** the memory buffer returned to the user by nmMalloc().  This allows us
 *** to detect clobbered memory at the cost of increasing memory overhead
 *** by 16 bytes per allocated buffer, and requires a full scan of the
 *** allocated memory list twice in each nmMalloc() or nmFree() call.
 ***
 *** This check can also be run manually by calling nmCheckAll().
 *** 
 *** Note: nmCheckAll() is still defined if BUFFER_OVERFLOW_CHECKING is not
 *** 	defined (it is a NOOP).
 ***/
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

/** List of overlay structs, used for caching allocated memory. **/
pOverlay lists[MAX_SIZE+1]; /* TODO: Greg - Is this 65KB global variable a problem? (On the stack, it would be...) */
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


/** Initialize the NewMalloc subsystem. **/
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
    
    nmFreeCnt=0;
    nmMallocCnt=0;
    nmMallocHits=0;
    nmMallocTooBig=0;
    nmMallocLargest=0;
    #ifdef BUFFER_OVERFLOW_CHECKING
    startMemList=NULL;
    #endif
    
    /** Done. **/
    is_init = true;
    }


#ifdef BUFFER_OVERFLOW_CHECKING
/*** Check the before and after magic values on a MemStruct to detect memory
 *** buffer overflows.
 *** 
 *** @param mem A pointer to the memory struct to check.
 *** @returns 0 on success, or -1 if an overflow is detected.
 ***/
int
nmCheckItem(pMemStruct mem)
    {
    int ret = 0;
    
    /** Check the starting magic value. **/
    if (mem->magic_start != MGK_MEMSTART)
	{
	fprintf(stderr,
	    "Bad magic_start at %p (%p) -- 0x%08x != 0x%08x\n",
	    MEMDATA(mem), mem, mem->magic_start, MGK_MEMSTART
	);
	ret = -1;
	}
    
    /** Check the ending magic value. **/
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

/*** Check the before and after magic values on all MemStructs to detect memory
 *** buffer overflows.  Causes a seg fault if such an overflow is detected.
 ***/
void
nmCheckAll()
    {
    #ifdef BUFFER_OVERFLOW_CHECKING
    int ret = 0;
    
    /** Traverse the memory list and check each item. **/
    for (pMemStruct mem = startMemList; mem != NULL; mem = mem->next)
	if (nmCheckItem(mem) == -1) ret = -1;
    
    /** Handle error. **/
    if (ret == -1)
	{
	printf("causing segfault to halt.......\n");
	*(int*)NULL = 0;
	}
    #endif
    }


#ifdef BUFFER_OVERFLOW_CHECKING
/*** Allocate new memory, using before and after magic values.
 *** 
 *** @param size The size of the memory buffer to be allocated.
 *** @returns A pointer to the start of the allocated memory buffer.
 ***/
void*
nmDebugMalloc(int size)
    {
    /** Allocate space for the data. **/
    pMemStruct tmp = (pMemStruct)malloc(size + EXTRA_MEM);
    if(tmp == NULL) return NULL;
    
    /** Initialize data in the memory struct (including magic values). **/
    tmp->size = size;
    tmp->magic_start = MGK_MEMSTART;
    ENDMAGIC(tmp) = MGK_MEMEND;
    
    /** Prepend this mem struct to the MemList linked list. **/
    tmp->next = startMemList;
    startMemList = tmp;

    /** Return the memory data for the user to use. **/
    return (void*)MEMDATA(tmp);
    }


/*** Free a memory buffer allocated using `nmDebugMalloc()`.  The before and
 *** after magic values are checked and a warning is displayed if an overflow
 *** has occurred (although this does not halt the program or function).
 *** 
 *** @param ptr A pointer to the memory to be freed.
 ***/
void
nmDebugFree(void* ptr)
    {
    /** Get the mem struct for the item being freed. **/
    pMemStruct tmp = MEMDATATOSTRUCT(ptr);
    
    /** Verify that our data hasn't been clobbered. **/
    nmCheckItem(tmp);
    
    /** Remove the item from the linked list. **/
    if (tmp == startMemList)
	{ /* Item is at the start. */
	startMemList = tmp->next;
	}
    else
	{ /* Item is not at the start. */
	/** Traverse the linked list to find the previous item. **/
	pMemStruct prev = startMemList;
	while (prev->next != tmp)
	    prev = prev->next;
	
	/** Fix the gap that will be left by freeing this item. **/
	prev->next = tmp->next;
	}
    
    /** Free the item. **/
    free(tmp);
    }


/*** Reallocates a memory block from `nmDebugMalloc()` (or `nmDebugRealloc()`)
 *** to a new size, maintaining as much data as possible.  Data loss only
 *** occurs if the new size is smaller, in which case bits are lost starting
 *** at the end of the buffer.
 *** 
 *** @param ptr A pointer to the current buffer (deallocated by this call).
 *** @param new_size The size that the buffer should be after this call.
 *** @returns The new buffer, or NULL if an error occurs.
 ***/
void*
nmDebugRealloc(void* ptr, int new_size)
    {
    /** Behaves as nmDebugMalloc() if there is no target pointer. **/
    if (ptr == NULL) return nmDebugMalloc(new_size);
    
    /** Allocate new data. **/
    void* new_ptr = (void*)nmDebugMalloc(new_size);
    if (new_ptr == NULL) return NULL;	
    
    /** Move the old data. **/
    int old_size = MEMDATATOSTRUCT(ptr)->size;
    memmove(new_ptr, ptr, old_size);
    
    /** Free the old allocation. **/
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
    }


/*** Clear the allocated memory block cache.  This deallocates all memory
 *** blocks that were marked as unused by a call to `nmFree()`, but were
 *** moved to the cache instead of being freed to the OS memory pool.
 ***/
void
nmClear()
    {
    if (!is_init) nmInitialize();
    
    /** Iterate over each overlay list in the cache and clear it. **/
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
    }


/*** Allocate memory using block caching.  The allocated block may be supplied
 *** by the cache (if available), or requested from the operating system.
 *** 
 *** @attention Should only be used for memory that will benefit from caching
 *** 	(e.g. a struct of a constant size).  Dynamically sized memory (such as
 *** 	variable-length strings) should be allocated using nmSysMalloc() to
 *** 	avoid overhead from unnecessary caching.
 *** 
 *** @param size The size of the memory block to be allocated.
 *** @returns A pointer to the start of the allocated memory block.
 ***/
void*
nmMalloc(int size)
    {
    void* tmp;
    if (!is_init) nmInitialize();
    
    /** Handle counting. **/
    #ifdef GLOBAL_BLK_COUNTING
    nmMallocCnt++;
    #endif
    
    /** Handle buffer overflow check. **/
    #ifdef BUFFER_OVERFLOW_CHECKING
    nmCheckAll();
    #endif
    
    /** Use caching if the size can be cached. **/
    if (MIN_SIZE <= size && size <= MAX_SIZE)
	{
	/** Handle counting. **/
	#ifdef SIZED_BLK_COUNTING
	outcnt[size]++;
	usagecnt[size]++;
	#endif
	
	/** Cache check. **/
	if (lists[size] == NULL)
	    { /* Miss. */
	    tmp = (void*)nmDebugMalloc(size);
	    }
	else
	    { /* Hit. */
	    /** Handle counting. **/
	    #ifdef GLOBAL_BLK_COUNTING
	    nmMallocHits++;
	    #endif
	    
	    /** Pop allocated memory off of the start of the cache list. **/
	    tmp = lists[size];
	    ASSERTMAGIC(tmp, MGK_FREEMEM);
	    lists[size] = lists[size]->Next;
	    
	    /** Handle counting. **/
	    #ifdef SIZED_BLK_COUNTING
	    listcnt[size]--;
	    #endif
	    }
	}
    else
	{
	/** Handle counting. **/
	#ifdef GLOBAL_BLK_COUNTING
	nmMallocTooBig++; /* TODO: Greg - Couldn't the memory also be too small? */
	if (size > nmMallocLargest) nmMallocLargest = size;
	#endif
	
	/** Caching isn't supported for this size: Allocate memory normally. **/
	tmp = (void*)nmDebugMalloc(size);
	}
    
    /** TODO: Greg - We might need more docs for overlays. **/
    /*** It seems like this code block is doing too many different things. It
     *** uses overlays, which I don't fully understand, so I wasn't able to
     *** simplify it.
     ***/
    if (tmp == NULL)
	{
	if (err_fn) err_fn("Insufficient system memory for operation.");
	}
    else
	{
	if (size >= MIN_SIZE)
	    OVERLAY(tmp)->Magic = MGK_ALLOCMEM;
	}
    
    /** Handle overflow check. **/
    #ifdef BUFFER_OVERFLOW_CHECKING
    nmCheckAll();
    #endif
    
    /** Handle memory leak checks. **/
    #ifdef BLK_LEAK_CHECK
    /** Find the next open index in the blocks array and record this block. **/
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
    
    /** Return the allocated memory. **/
    return tmp;
    }


/*** Free a memory buffer allocated using `nmMalloc()`.  This buffer may be
 *** deallocated into the operating system memory pool, or it may be cached
 *** for reuse with `nmMalloc()`.
 *** 
 *** @attention Be EXTREMELY careful not to provide an incorrect size value.
 *** 	This can lead to memory blocks being cached incorrectly, which causes
 *** 	errors to occur when they are reallocated, possibly FAR AWAY from the
 *** 	original source of the bug.
 *** 
 *** @param ptr A pointer to the memory to be freed.
 *** @param size The size of the memory block to be freed. (Note: Even though
 *** 	the OS does store this value, the C memory manager does not make it
 *** 	available to us, so it must be provided for this function to run.)
 ***/
void
nmFree(void* ptr, int size)
    {
    if (!is_init) nmInitialize();
    
    /*** If there should be an overlay, assert that it does NOT indicate that
     *** this memory was already freed and returned to the cache.
     ***/
    if (size >= MIN_SIZE)
	ASSERTNOTMAGIC(ptr, MGK_FREEMEM);
    
    /** If the pointer is null, no work needed. **/
    if (ptr == NULL) return;
    
    /** Handle counting. **/
    #ifdef GLOBAL_BLK_COUNTING
    nmFreeCnt++;
    #endif
    
    /** Handle overflow check. **/
    #ifdef BUFFER_OVERFLOW_CHECKING
    nmCheckAll();
    #endif
    
    /** Handle memory leak check. **/
    #ifdef BLK_LEAK_CHECK
    /** Find this block in the blocks array and remove it. **/
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
    
    /** Check for the block cache if it is enabled. **/
    #ifndef NO_BLK_CACHE
    if (size <= MAX_SIZE && size >= MIN_SIZE)
	{
	/** Handle duplicate free check. **/
	#ifdef DUP_FREE_CHECK
	/** Search the freed memory cache to see if this memory is there. **/
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
	
	/** Add the freed memory to the cache. **/
	OVERLAY(ptr)->Magic = MGK_FREEMEM;
	OVERLAY(ptr)->Next = lists[size];
	lists[size] = OVERLAY(ptr);
	ptr = NULL;
	
	/** Handle counting. **/
	#ifdef SIZED_BLK_COUNTING
	outcnt[size]--;
	listcnt[size]++;
	#endif
	}
    #endif
    
    /** Free the block if it was not consumed by the cache. **/
    if (ptr != NULL)
	{
	nmDebugFree(ptr);
	ptr = NULL;
	}
    
    /** Handle overflow check. **/
    #ifdef BUFFER_OVERFLOW_CHECKING
    nmCheckAll();
    #endif
    }


/** Print stats about the NewMalloc subsystem, for debugging. **/
void
nmStats()
    {
    if (!is_init) nmInitialize();
    
    /** Warn if statistics are not being tracked. **/
    #ifndef GLOBAL_BLK_COUNTING
    printf("Warning: GLOBAL_BLK_COUNTING is disabled.\n");
    #endif
    
    /** Print subsystem stats. **/
    printf(
	"NewMalloc subsystem statistics:\n"
	"   nmMalloc: %d calls, %d hits (%3.3f%%)\n"
	"   nmFree: %d calls\n"
	"   bigblks: %d too big, %d largest size\n\n",
	nmMallocCnt, nmMallocHits, (float)nmMallocHits / (float)nmMallocCnt * 100.0,
	nmFreeCnt,
	nmMallocTooBig, nmMallocLargest
    );
    }


/** Register a new memory size with a name, for debugging. **/
void
nmRegister(int size, char* name)
    {
    pRegisteredBlockType blk;
    
    /** Ignore blocks too large to be cached. **/
    if (size > MAX_SIZE) return;
    
    /** Prepend a new RegisteredBlockType record to the list of records for this size. **/
    blk = (pRegisteredBlockType)malloc(sizeof(RegisteredBlockType));
    if (blk == NULL) return;
    blk->Next = blknames[size];
    blknames[size] = blk;
    
    /** Initialize values for this record. **/
    blk->Magic = MGK_REGISBLK;
    blk->Size = size;
    strcpy(blk->Name, name);
    }

/*** Print the registered names for a block of data of a given size to stdout.
 *** 
 *** @param size The size of block to query.
 ***/
void
nmPrintNames(int size)
    {
    /*** Traverse the linked list that holds all registered names for the given
     *** size and print each one.
     **/
    for (pRegisteredBlockType blk = blknames[size]; blk != NULL; blk = blk->Next)
	{
	ASSERTMAGIC(blk, MGK_REGISBLK);
	printf("%s ", blk->Name);
	}
    }


/** Print debug information about the newmalloc system. **/
void
nmDebug()
    {
    /** Print the header for the block sizes table. **/
    printf("size\tout\tcache\tusage\tnames\n");
    
    /** Iterate through each possible block size. **/
    for (size_t size = MIN_SIZE; size < MAX_SIZE; size++)
	{
	/** Skip unused block sizes. **/
	if (usagecnt[size] == 0) continue;
	
	/** Print stats about this block size. **/
	printf("%ld\t%d\t%d\t%d\t", size, outcnt[size], listcnt[size], usagecnt[size]);
	
	/** Print each name for this block size. **/
	nmPrintNames(size);
	
	printf("\n");
	}
    
    /** Print the header for the nmSysXYZ() info table. **/
    printf("\n-----\n");
    printf("size\toutcnt\n-------\t-------\n");
    
    /** Iterate through each possible block size. **/
    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	{
	/** Skip unused block sizes. **/
	if (nmsys_outcnt[size] == 0) continue;
	
	/** Print the nmSysXYZ() block information. **/
	printf("%ld\t%d\n", size, nmsys_outcnt[size]);
	}
    printf("\n");
    }


/** Print debug information about the newmalloc system. **/
void
nmDeltas()
    {
    /** Print the header for the block size deltas table. **/
    printf("size\tdelta\tnames\n-------\t-------\t-------\n");
    
    /** Iterate through each possible block size. **/
    int total_delta = 0;
    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	{
	/** Skip entries where there is no change. **/
	if (outcnt[size] == outcnt_delta[size]) continue;
	
	/** Print the change and add it to the total_delta. **/
	printf("%ld\t%d\t", size, outcnt[size] - outcnt_delta[size]);
	total_delta += (size * (outcnt[size] - outcnt_delta[size]));
	
	/** Print each name for this block size from the linked list. **/
	nmPrintNames(size);
	
	/** End of line. **/
	printf("\n");
	
	/** Reset the delta. **/
	outcnt_delta[size] = outcnt[size];
	}
    
    /** Print the header for the nmSysXYZ() info table. **/
    printf("\nsize\tdelta\n-------\t-------\n");
    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	{
	if (nmsys_outcnt[size] != nmsys_outcnt_delta[size]) continue;
	
	printf("%ld\t%d\n", size, nmsys_outcnt[size] - nmsys_outcnt_delta[size]);
	total_delta += (size * (nmsys_outcnt[size] - nmsys_outcnt_delta[size]));
	nmsys_outcnt_delta[size] = nmsys_outcnt[size];
	}
    printf("\n");
    
    /** Print the total delta. **/
    /** TODO: Israel - Change this to use snprint_bytes() once that function is available. **/
    printf("delta %d total bytes\n", total_delta);
    }


/*** Allocate memory without using block caching.  The size of the allocated
 *** memory is stored at the start of the memory block for debugging.
 *** 
 *** @attention Should be used for memory that will NOT benefit from caching
 *** 	(e.g. a variable length string).  Consistently sized memory (such as
 *** 	a struct of a constant size) should be allocated using nmMalloc() to
 *** 	improve performance.
 *** 
 *** @param size The size of the memory block to be allocated.
 *** @returns A pointer to the start of the allocated memory block.
 ***/
void*
nmSysMalloc(int size)
    {
    /** Fallback if sysMalloc() is disabled. **/
    #ifndef NM_USE_SYSMALLOC
    return (void*)nmDebugMalloc(size);
    #else
    
    /** Allocate the requested space, plus the initial size int. **/
    char* ptr = (char*)nmDebugMalloc(sizeof(int) + size);
    if (ptr == NULL) return NULL;
    
    /** Set the size int. **/
    *(int*)ptr = size;
    
    /** Update sized block counting, if necessary. **/
    #ifdef SIZED_BLK_COUNTING
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]++;
    #endif
    
    /** Return the allocated memory (starting after the size int). **/
    return (void*)(sizeof(int) + ptr);
    #endif
    }


/*** Free a memory buffer allocated using `nmSysMalloc()` (or similar).
 *** 
 *** @param ptr A pointer to the memory to be freed.
 ***/
void
nmSysFree(void* ptr)
    {
    /** Fallback if sysMalloc() is disabled. **/
    #ifndef NM_USE_SYSMALLOC
    nmDebugFree(ptr);
    #else
    
    /** Count sized blocks, if enabled. **/
    #ifdef SIZED_BLK_COUNTING
    int size;
    size = *(int*)(((char*)ptr)-sizeof(int));
    if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
    #endif
    
    /** Free the initial size int, as well as the rest of the allocated memory. **/
    nmDebugFree(((char*)ptr) - sizeof(int));
    #endif
    return;
    }


/*** Reallocates a memory block from `nmSysMalloc()` (or similar) to a new
 *** size, maintaining as much data as possible.  Data loss only occurs if the
 *** new size is smaller, in which case bits are lost starting at the end of
 *** the buffer.
 *** 
 *** @param ptr A pointer to the current buffer (deallocated by this call).
 *** @param new_size The size that the buffer should be after this call.
 *** @returns The new buffer, or NULL if an error occurs.
 ***/
void*
nmSysRealloc(void* ptr, int new_size)
    {
    /** Fallback if sysMalloc() is disabled. **/
    #ifndef NM_USE_SYSMALLOC
    return (void*)nmDebugRealloc(ptr, new_size);
    #else
    
    /** Behaves as nmSysMalloc() if there is no target pointer. **/
    if (ptr == NULL) return nmSysMalloc(new_size);
    
    /** If no work needs to be done, do nothing. **/
    void* buffer_ptr = ((char*)ptr) - sizeof(int);
    const int size = *(int*)buffer_ptr;
    if (size == new_size) return ptr;
    
    /** Realloc the given memory, taking the initial size int into account into account. **/
    char* new_ptr = (char*)nmDebugRealloc(buffer_ptr, sizeof(int) + new_size);
    if (new_ptr == NULL) return NULL;
    
    /** Update the initial size int. **/
    *(int*)new_ptr = new_size;
    
    /** Handle counting. **/
    #ifdef SIZED_BLK_COUNTING
    if (0 < size && size <= MAX_SIZE) nmsys_outcnt[size]--;
    if (0 < new_size && new_size <= MAX_SIZE) nmsys_outcnt[new_size]++;
    #endif
    
    /** Return the pointer to the new memory. **/
    return (void*)(sizeof(int) + new_ptr);
    #endif
    }


/*** Duplicate a string into a new memory buffer, which is allocated by using
 *** `nmSysMalloc()` (and thus, it should be freed with `nmSysFree()` to avoid
 *** causing a memory leak).
 ***
 *** Note: The string is not deallocated by this function call. Thus, it can
 *** 	be stack allocated, heap allocated, or even a string literal without
 *** 	causing an error.
 *** 
 *** @attention The str pointer is _assumed_ to point to a null-terminated
 *** 	string.  If this is not the case, the behavior is undefined, as with
 *** 	the C standard `strdup()` function.
 ***
 *** @param str The string to be duplicated.
 *** @returns A pointer to the new string buffer containing the string, or
 ***          NULL if an error occurs.
 ***/
char*
nmSysStrdup(const char* str)
    {
    /** Fallback if sysMalloc() is disabled. **/
    #ifndef NM_USE_SYSMALLOC
    return strdup(str);
    #else
    
    /** Allocate space for the string. **/
    size_t n = strlen(str) + 1u; /* Length, including the null terminator. */
    char* new_str = (char*)nmSysMalloc(n);
    if (new_str == NULL) return NULL;
    
    /** Copy the string into the new memory. **/
    memcpy(new_str, str, n);
    
    return new_str;
    #endif
    }
