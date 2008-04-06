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

/**CVSDATA***************************************************************

    $Id: newmalloc.h,v 1.5 2008/04/06 21:34:36 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/newmalloc.h,v $

    $Log: newmalloc.h,v $
    Revision 1.5  2008/04/06 21:34:36  gbeeley
    - (bugfix) nmSysStrdup() should use const qualifier.

    Revision 1.4  2007/12/13 23:10:25  gbeeley
    - (change) adding --enable-debugging to the configure script, and without
      debug turned on, disable a lot of the nmMalloc() / nmFree() instrument-
      ation that was using at least 95% of the cpu time in Centrallix.
    - (bugfix) fixed a few int vs. size_t warnings in MTASK.

    Revision 1.3  2007/04/08 03:43:06  gbeeley
    - (bugfix) some code quality fixes
    - (feature) MTASK integration with the Valgrind debugger.  Still some
      problems to be sorted out, but this does help.  Left to themselves,
      MTASK and Valgrind do not get along, due to the approach to threading.

    Revision 1.2  2003/03/04 06:28:22  jorupp
     * added buffer overflow checking to newmalloc
    	-- define BUFFER_OVERFLOW_CHECKING in newmalloc.c to enable

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

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
