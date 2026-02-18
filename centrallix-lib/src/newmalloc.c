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
/*		and debugging services to help find memory leaks and	*/
/*		other memory errors.  It also uses magic numbers to	*/
/*		ensure memory allocation consistency.  This module is	*/
/*		slower than standard libc allocation when in debug	*/
/*		mode, but it is drastically faster in production mode.	*/
/*									*/
/*		Although blocks returned from the nm* routines can be	*/
/*		mixed-and-matched with the normal libc functions, this	*/
/*		leads to inaccuracies in the debugging information and	*/
/*		thus defeats the purpose of having a custom library.	*/
/*		The returned blocks from the nmSys* functions CANNOT	*/
/*		be intermixed with normal malloc/free.  In short, if	*/
/*		you use this library, do NOT use the normal libc	*/
/*		malloc/free at all.					*/
/************************************************************************/

#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "magic.h"
#include "newmalloc.h"
#include "strtcpy.h"

/** Temporary implementations until the ones from the dups branch is available. **/
/** TODO: Israel - Remove once the dups branch is merged. **/
#define min(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a < _b) ? _a : _b; \
    })
#define max(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a > _b) ? _a : _b; \
    })

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
    size_t size;
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

/** List of overlay structs, used for caching allocated memory. **/
/** TODO: Greg - This is over 327 KB of variables, is that a problem? (On the stack, this would be a seg fault!!) **/
pOverlay lists[MAX_SIZE+1];
unsigned long long int listcnt[MAX_SIZE+1];
unsigned long long int outcnt[MAX_SIZE+1];
unsigned long long int outcnt_delta[MAX_SIZE+1];
unsigned long long int usagecnt[MAX_SIZE+1];
unsigned long long int nmFreeCnt;
unsigned long long int nmMallocCnt;
unsigned long long int nmMallocHits;
unsigned long long int nmMallocTooBig;
size_t nmMallocLargest;

#ifdef BLK_LEAK_CHECK
void* blks[MAX_BLOCKS];
int blksiz[MAX_BLOCKS];
#endif

bool is_init = false;
int (*err_fn)() = NULL;

unsigned long long int nmsys_outcnt[MAX_SIZE+1];
unsigned long long int nmsys_outcnt_delta[MAX_SIZE+1];

/*** The registration data that associates a name with a specific block size
 *** of data stored in memory. Used to create a linked list for each possible
 *** block size.
 *** 
 *** Memory Stats:
 ***   - Padding: 0 bytes
 ***   - Total size: 80 bytes
 *** 
 *** @param Size The size of the memory block.
 *** @param Name The name associated with the memory block.
 *** @param Next The next block of this size in the linked list.
 *** @param Magic A magic value for detecting corrupted memory.
 ***/
typedef struct _RB
    {
    int		Magic;
    int		Size;
    struct _RB*	Next;
    char	Name[64];
    }
    RegisteredBlockType, *pRegisteredBlockType;

/** Heads of registered-block linked lists, where index = block size. **/
pRegisteredBlockType blknames[MAX_SIZE+1];


/** Initialize the NewMalloc subsystem. **/
void
nmInitialize(void)
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
	
	/** Done. **/
	is_init = true;
    
    return;
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
		"ERROR: Bad magic_start at %p (%p) -- 0x%08x != 0x%08x\n",
		MEMDATA(mem), mem, mem->magic_start, MGK_MEMSTART
	    );
	    ret = -1;
	    }
	
	/** Check the ending magic value. **/
	if (ENDMAGIC(mem) != MGK_MEMEND)
	    {
	    fprintf(stderr,
		"ERROR: Bad magic_end at %p (%p) -- 0x%08x != 0x%08x\n",
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
nmCheckAll(void)
    {
#ifdef BUFFER_OVERFLOW_CHECKING
    int ret = 0;
    
	/** Traverse the memory list and check each item. **/
	for (pMemStruct mem = startMemList; mem != NULL; mem = mem->next)
	    if (nmCheckItem(mem) == -1) ret = -1;
	
	/** "Handle" error. **/
	if (ret == -1)
	    {
	    fprintf(stderr, "causing segfault to halt.......\n");
	    *(int*)NULL = 0;
	    }
#endif
    
    return;
    }


#ifdef BUFFER_OVERFLOW_CHECKING
/*** Allocate new memory, using before and after magic values.
 *** 
 *** @param size The size of the memory buffer to be allocated.
 *** @returns A pointer to the start of the allocated memory buffer.
 ***/
void*
nmDebugMalloc(size_t size)
    {
	/** Allocate space for the data. **/
	pMemStruct tmp = (pMemStruct)malloc(size + EXTRA_MEM);
	if (tmp == NULL) return NULL;
	
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
    
    return;
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
nmDebugRealloc(void* ptr, size_t new_size)
    {
	/** Behaves as nmDebugMalloc() if there is no target pointer. **/
	if (ptr == NULL) return nmDebugMalloc(new_size);
	
	/** Allocate new data. **/
	void* new_ptr = (void*)nmDebugMalloc(new_size);
	if (new_ptr == NULL) return NULL;	
	
	/** Move the old data. **/
	size_t old_size = MEMDATATOSTRUCT(ptr)->size;
	memmove(new_ptr, ptr, min(new_size, old_size));
	
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
    
    return;
    }


/*** Clear the allocated memory block cache.  This deallocates all memory
 *** blocks that were marked as unused by a call to `nmFree()`, but were
 *** moved to the cache instead of being freed to the OS memory pool.
 ***/
void
nmClear(void)
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
    
    return;
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
nmMalloc(size_t size)
    {
    void* tmp = NULL;
    
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
		if (listcnt[size] == 0u)
		    {
		    fprintf(stderr,
			"ERROR: Removed block from cache when stats show that "
			"it should be empty!! Expect broken statistics.\n"
		    );
		    }
		listcnt[size]--;
#endif
		}
	    }
	else
	    {
/** Handle counting. **/
#ifdef GLOBAL_BLK_COUNTING
	    nmMallocTooBig++; /* TODO: Greg - Couldn't the memory also be too small to cache? */
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
nmFree(void* ptr, size_t size)
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
		    fprintf(stderr, "ERROR: Duplicate nmFree()!!!  Size = %d, Address = %p\n", size, ptr);
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
	    if (outcnt[size] == 0u)
		{
		fprintf(stderr,
		    "ERROR: Call to nmFree() memory of a size that has no "
		    "valid allocated memory!! Expect broken statistics.\n"
		);
		}
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
    
    return;
    }


/** Print stats about the NewMalloc subsystem, for debugging. **/
void
nmStats(void)
    {
	if (!is_init) nmInitialize();
	
	/** Warn if statistics are not being tracked. **/
	#ifndef GLOBAL_BLK_COUNTING
	fprintf(stderr, "Warning: GLOBAL_BLK_COUNTING is disabled.\n");
	#endif
	
	/** Print subsystem stats. **/
	printf(
	    "NewMalloc subsystem statistics:\n"
	    "   nmMalloc: %llu calls, %llu hits (%3.3lf%%)\n"
	    "   nmFree: %llu calls\n"
	    "   bigblks: %llu too big, %zu largest size\n\n",
	    nmMallocCnt, nmMallocHits, (double)nmMallocHits / (double)nmMallocCnt * 100.0,
	    nmFreeCnt,
	    nmMallocTooBig, nmMallocLargest
	    );
    
    return;
    }


/** Register a new memory size with a name, for debugging. **/
void
nmRegister(size_t size, char* name)
    {
    pRegisteredBlockType blk = NULL;
    
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
	strtcpy(blk->Name, name, sizeof(blk->Name) / sizeof(char));
    
    return;
    }

/*** Print the registered names for a block of data of a given size to stdout.
 *** 
 *** @param size The size of block to query.
 ***/
void
nmPrintNames(size_t size)
    {
	/** Traverse the linked list of registered structs to print each name. **/
	for (pRegisteredBlockType blk = blknames[size]; blk != NULL; blk = blk->Next)
	    {
	    ASSERTMAGIC(blk, MGK_REGISBLK);
	    printf("%s ", blk->Name);
	    }
    
    return;
    }


/** Print debug information about the newmalloc system. **/
void
nmDebug(void)
    {
	/** Print the header for the block sizes table. **/
	printf("size\tout\tcache\tusage\tnames\n");
	
	/** Iterate through each possible block size. **/
	for (size_t size = MIN_SIZE; size < MAX_SIZE; size++)
	    {
	    /** Skip unused block sizes. **/
	    if (usagecnt[size] == 0llu) continue;
	    
	    /** Print stats about this block size. **/
	    printf(
		"%zu\t%llu\t%lld\t%llu\t",
		size, outcnt[size], listcnt[size], usagecnt[size]
	    );
	    
	    /** Print each name for this block size. **/
	    nmPrintNames(size);
	    
	    printf("\n");
	    }
	
	/** Print the header for the nmSysXYZ() info table. **/
	printf(
	    "\n-----\n"
	    "size\toutcnt\n"
	    "-------\t-------\n"
	);
	
	/** Iterate through each possible block size. **/
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    /** Skip unused block sizes. **/
	    if (nmsys_outcnt[size] == 0llu) continue;
	    
	    /** Print the nmSysXYZ() block information. **/
	    printf("%zu\t%llu\n", size, nmsys_outcnt[size]);
	    }
	printf("\n");
    
    return;
    }


/** Print debug information about the newmalloc system. **/
void
nmDeltas(void)
    {
	/** Print the header for the block size deltas table. **/
	printf(
	    "size\tdelta\tnames\n"
	    "-------\t-------\t-------\n"
	);
	
	/** Iterate through each possible block size. **/
	unsigned long long int total_delta = 0llu;
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    /** Skip entries where there is no change. **/
	    if (outcnt[size] == outcnt_delta[size]) continue;
	    
	    /** Print the change and add it to the total_delta. **/
	    printf("%zu\t%lld\t", size, outcnt[size] - outcnt_delta[size]);
	    total_delta += (size * (outcnt[size] - outcnt_delta[size]));
	    
	    /** Print each name for this block size from the linked list. **/
	    nmPrintNames(size);
	    
	    /** End of line. **/
	    printf("\n");
	    
	    /** Reset the delta. **/
	    outcnt_delta[size] = outcnt[size];
	    }
	
	/** Print the header for the nmSysXYZ() info table. **/
	printf(
	    "\nsize\tdelta\n"
	    "-------\t-------\n"
	);
	for (size_t size = MIN_SIZE; size <= MAX_SIZE; size++)
	    {
	    /** Skip sizes where no change in memory occurred. **/
	    if (nmsys_outcnt[size] == nmsys_outcnt_delta[size]) continue;
	    
	    /** Print the results. **/
	    printf("%zu\t%llu\n", size, nmsys_outcnt[size] - nmsys_outcnt_delta[size]);
	    total_delta += (size * (nmsys_outcnt[size] - nmsys_outcnt_delta[size]));
	    nmsys_outcnt_delta[size] = nmsys_outcnt[size];
	    }
	printf("\n");
	
	/** Print the total delta. **/
	/** TODO: Israel - Change this to use snprint_bytes() once that function is available. **/
	printf("delta %llu total bytes\n", total_delta);
    
    return;
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
nmSysMalloc(size_t size)
    {
/** Fallback if sysMalloc() is disabled. **/
#ifndef NM_USE_SYSMALLOC
	return (void*)nmDebugMalloc(size);
#else
	
	/** Allocate the requested space, plus the initial size int. **/
	char* ptr = (char*)nmDebugMalloc(sizeof(unsigned int) + size);
	if (ptr == NULL) return NULL;
	
	/** Convert the provided size to an unsigned int. **/
	/** TODO: Greg - Can we modify nmSys blocks to start with a size_t to avoid this? **/
	if (size > UINT_MAX)
	    {
	    fprintf(stderr,
		"ERROR: Requested buffer size (%zu) > UINT MAX (%u).\n",
		size, UINT_MAX
	    );
	    return NULL;
	    }
	
	/** Set the size uint. **/
	*(unsigned int*)(ptr) = (unsigned int)size;
	
 /** Update sized block counting, if necessary. **/
 #ifdef SIZED_BLK_COUNTING
	if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]++;
 #endif
	
	/** Return the allocated memory (starting after the size uint). **/
	return (void*)(sizeof(unsigned int) + ptr);
#endif
    
    return NULL; /** Unreachable. **/
    }

/*** TODO: Greg - I believe that the above code is the best way to satisfy my interpretation of
 *** requirement 2 of the coding style: "Every function must have a return
 *** line at the end, even if the function never reaches that return."
 *** Without the goofy unreachable return, the last line will be an #endif.
 *** Am I following the style correctly?
 ***/


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
	const size_t size = nmSysGetSize(ptr);
	if (size > 0 && size <= MAX_SIZE) nmsys_outcnt[size]--;
 #endif
	
	/** Free the initial unsigned int, as well as the rest of the allocated memory. **/
	nmDebugFree(((char*)ptr) - sizeof(unsigned int));
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
nmSysRealloc(void* ptr, size_t new_size)
    {
/** Fallback if sysMalloc() is disabled. **/
#ifndef NM_USE_SYSMALLOC
	return (void*)nmDebugRealloc(ptr, new_size);
#else
	
	/** Behaves as nmSysMalloc() if there is no target pointer. **/
	if (ptr == NULL) return nmSysMalloc(new_size);
	
	/** If the memory block size does not change, do nothing. **/
	if (nmSysGetSize(ptr) == new_size) return ptr;
	
	/** Realloc the given memory with space for the initial uint. **/
	void* buffer_ptr = ((char*)ptr) - sizeof(unsigned int);
	char* new_ptr = (char*)nmDebugRealloc(buffer_ptr, sizeof(unsigned int) + new_size);
	if (new_ptr == NULL) return NULL;
	
	/** Update the initial size uint. **/
	*(unsigned int*)new_ptr = new_size;
	
 /** Handle counting. **/
 #ifdef SIZED_BLK_COUNTING
	if (0 < size && size <= MAX_SIZE) nmsys_outcnt[size]--;
	if (0 < new_size && new_size <= MAX_SIZE) nmsys_outcnt[new_size]++;
 #endif
	
	/** Return the pointer to the new memory. **/
	return (void*)(sizeof(unsigned int) + new_ptr);
#endif
	
    return NULL; /** Unreachable. **/
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
    
    return NULL; /** Unreachable. **/
    }


/*** Gets the size of memory allocated in the buffer using an `nmSysXYZ()`
 *** function, if possible (depending on compiler #define flags, this size
 *** might not be stored).
 *** 
 *** @param ptr A pointer to the memory buffer to be checked.  The function's
 *** 	behavior is undefined if this pointer is a nonNULL value which does
 *** 	NOT point to the start of a valid memory block from an `nmSysXYZ()`.
 *** @returns The size of the memory buffer, if it can be found, or
 *** 	0 if the size of the buffer was not stored, or
 *** 	0 if `ptr` is NULL.
 ***/
size_t
nmSysGetSize(void* ptr)
    {
#ifndef NM_USE_SYSMALLOC
    return 0lu; /* Value not stored. */
#else
    if (ptr == NULL) return 0lu;
    
    /*** Create a pointer to the start of the allocated buffer and read the
     *** allocation size stored there.
     ***/
    const void* buffer_ptr = ((char*)ptr) - sizeof(unsigned int);
    const unsigned int raw_size_value = *(unsigned int*)buffer_ptr;
    return (size_t)raw_size_value;
#endif
    }
