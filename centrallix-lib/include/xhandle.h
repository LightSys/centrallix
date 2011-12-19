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


