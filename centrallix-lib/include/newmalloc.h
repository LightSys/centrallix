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
    Overlay,*pOverlay;

#ifdef NMMALLOC_DEBUG
#define	BLK_LEAK_CHECK	1
#define	DUP_FREE_CHECK	1
#endif

#ifdef NMMALLOC_PROFILING
#define GLOBAL_BLK_COUNTING	1
#define SIZED_BLK_COUNTING	1
#endif

/** nmMalloc block caching causes Valgrind to lose track of what call
 ** stack actually allocated the block to begin with.  So if we're using
 ** valgrind, turn off block caching altogether.
 **/
#ifdef USING_VALGRIND
#define NO_BLK_CACHE	1
#endif

#define OVERLAY(x)	((pOverlay)(x))
#define MAX_SIZE	(8192)
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
void* nmMalloc(int size);
void nmFree(void* ptr,int size);
void nmStats();
void nmRegister(int size,char* name);
void nmDebug();
void nmDeltas();
void* nmSysMalloc(int size);
void nmSysFree(void* ptr);
void* nmSysRealloc(void* ptr, int newsize);
char* nmSysStrdup(const char* ptr);

void nmEnableTagging();
void nmRegisterTagID(int tag_id, char* name);
void nmSetTag(void* ptr, int tag_id, void* tag);
void* nmGetTag(void* ptr, int tag_id);
void nmRemoveTag(void* ptr, int tag_id);

#endif
