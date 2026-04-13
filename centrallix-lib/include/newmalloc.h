#ifndef _NEWMALLOC_H
#define _NEWMALLOC_H

#ifdef HAVE_CONFIG_H
#ifdef CXLIB_INTERNAL
#include "cxlibconfig-internal.h"
#else
#include "cxlib/cxlibconfig.h"
#endif /* defined CXLIB_INTERNAL */
#endif /* defined HAVE_CONFIG_H */

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
/* Module:	NewMalloc memory management module                      */
/* Author:	Greg Beeley (GRB)                                       */
/*									*/
/* Description:	Module that implements block caching and debugging	*/
/*		facilities for effective memory management.		*/
/************************************************************************/


typedef struct _ov
    {
    int		Magic;
    struct _ov 	*Next;
    }
    Overlay, *pOverlay;

#ifdef NMMALLOC_DEBUG
#define	BLK_LEAK_CHECK	1
#define	DUP_FREE_CHECK	1
#endif

#ifdef NMMALLOC_PROFILING
#define GLOBAL_BLK_COUNTING	1
#define SIZED_BLK_COUNTING	1
#endif

/*** nmMalloc() block caching reuses memory blocks, causing Valgrind to loose
 *** track of where the developer actually allocated them.  Thus, we disable
 *** it if Valgrind is in use.  Also, we disable the nmSysXyz() calls, causing
 *** them to be replaced with pass-through wrappers.
 ***/
#ifdef USING_VALGRIND
#define NO_BLK_CACHE	1
#undef NM_USE_SYSMALLOC
#endif

#define OVERLAY(x)	((pOverlay)(x))
#define MAX_SIZE	(8192lu)
#define MIN_SIZE	(sizeof(Overlay))

#ifdef BLK_LEAK_CHECK
#define MAX_BLOCKS	65536
extern void* blks[MAX_BLOCKS];
extern int blksiz[MAX_BLOCKS];
#endif
extern pOverlay lists[MAX_SIZE+1];

void nmInitialize();
void nmSetErrFunction(int (*error_fn)());
void nmClear();
void nmCheckAll(); // checks for buffer overflows
void* nmMalloc(size_t size);
void nmFree(void* ptr, size_t size);
void nmStats();
void nmRegister(size_t size, char* name);
void nmPrintNames(size_t size);
void nmDebug();
void nmDeltas();
void* nmSysMalloc(size_t size);
void nmSysFree(void* ptr);
void* nmSysRealloc(void* ptr, size_t new_size);
char* nmSysStrdup(const char* str);
size_t nmSysGetSize(void* ptr);

/** Tagging system (not implemented). **/
void nmEnableTagging();
void nmRegisterTagID(int tag_id, char* name);
void nmSetTag(void* ptr, int tag_id, void* tag);
void* nmGetTag(void* ptr, int tag_id);
void nmRemoveTag(void* ptr, int tag_id);

#endif
