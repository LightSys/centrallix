#ifndef _XHANDLE_H
#define _XHANDLE_H

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
/* Module: 	xhandle.c, xhandle.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 25, 2001   					*/
/* Description:	Handle (unique id) support library.  This module allows	*/
/*		for the simple association of incrementing numeric IDs	*/
/*		with structure pointers.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xhandle.h,v 1.4 2005/02/26 04:32:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xhandle.h,v $

    $Log: xhandle.h,v $
    Revision 1.4  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.3  2004/06/12 04:09:37  gbeeley
    - supporting logic to allow saving of an MTask security context for later
      use in a new thread.  This is needed for the asynchronous event delivery
      mechanism for object-updates being sent to the client.

    Revision 1.2  2002/05/03 03:46:29  gbeeley
    Modifications to xhandle to support clearing the handle list.  Added
    a param to xhClear to provide support for xhnClearHandles.  Added a
    function in mtask.c to allow the retrieval of ticks-since-boot without
    making a syscall.  Fixed an MTASK bug in the scheduler relating to
    waiting on timers and some modulus arithmetic.

    Revision 1.1  2002/04/25 17:56:54  gbeeley
    Added Handle support (xhandle module, XHN).  This is used to provide a
    more flexible abstraction between API return values (handles vs. ptrs)
    and the underlying structures they actually reference.  Handles are
    64bit on glibc2 ia32 platforms (unsigned long long int).


 **END-CVSDATA***********************************************************/

#ifdef CXLIB_INTERNAL
#include "xhash.h"
#else
#include "cxlib/xhash.h"
#endif


/*** Handle type - 64bit integer on most platforms should work. ***/
#ifndef __NO_LONGLONG
typedef unsigned long long int handle_t;
#define XHN_HANDLE_PRT "%16.16llX"
#else
typedef unsigned long int handle_t;
#define XHN_HANDLE_PRT "%8.8X"
#endif

#define XHN_INVALID_HANDLE ((handle_t)(0))

/*** Handle structure ***/
typedef struct _HD
    {
    int			Magic;		/* MGK_HANDLE */
    handle_t		HandleID;
    void*		Ptr;
    }
    HandleData, *pHandleData;


/*** Handle context ***/
typedef struct _HC
    {
    XHashTable		HandleTable;
    XHashTable		HandleTableByPtr;
    handle_t		NextHandleID;
    }
    HandleContext, *pHandleContext;


/*** Handle context creation/destruction functions ***/
int xhnInitContext(pHandleContext this);
int xhnDeInitContext(pHandleContext this);

/*** Handle management ***/
handle_t xhnAllocHandle(pHandleContext ctx, void* ptr);
void* xhnHandlePtr(pHandleContext ctx, handle_t handle_id);
handle_t xhnHandle(pHandleContext ctx, void* ptr);
int xhnUpdateHandle(pHandleContext ctx, handle_t handle_id, void* ptr);
int xhnUpdateHandleByPtr(pHandleContext ctx, void* old_ptr, void* ptr);
int xhnFreeHandle(pHandleContext ctx, handle_t handle_id);
int xhnClearHandles(pHandleContext ctx, int (*iter_fn)());

handle_t xhnStringToHandle(char* str, char** endptr, int base);

#endif /* _XHANDLE_H */


