#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "xhash.h"
#include "xhandle.h"
#include "newmalloc.h"
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
/* Module: 	xhandle.c, xhandle.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 25, 2001   					*/
/* Description:	Handle (unique id) support library.  This module allows	*/
/*		for the simple association of incrementing numeric IDs	*/
/*		with structure pointers.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xhandle.c,v 1.4 2004/06/12 04:09:37 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xhandle.c,v $

    $Log: xhandle.c,v $
    Revision 1.4  2004/06/12 04:09:37  gbeeley
    - supporting logic to allow saving of an MTask security context for later
      use in a new thread.  This is needed for the asynchronous event delivery
      mechanism for object-updates being sent to the client.

    Revision 1.3  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

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



/*** xhnInitContext() - initialize a context for the creation of handles.  The
 *** context will be different for different applications or domains of 
 *** handles needed.  Handle id's are started at a random id for each 
 *** context - be sure to seed the lrand48() generator!
 ***/
int 
xhnInitContext(pHandleContext this)
    {

	/** Init the hash tables **/
	xhInit(&(this->HandleTable), 63, sizeof(handle_t));
	xhInit(&(this->HandleTableByPtr), 63, sizeof(void*));

	/** Set a random initial handle **/
	this->NextHandleID = lrand48();
	if (this->NextHandleID == XHN_INVALID_HANDLE) this->NextHandleID++;

    return 0;
    }


/*** xhnDeInitContext() - deinitializes a context for handle creation.
 ***/
int 
xhnDeInitContext(pHandleContext this)
    {

	/** We probably need to xhClear() one hash table first. **/

	/** Deinit the hash tables **/
	xhDeInit(&(this->HandleTable));
	xhDeInit(&(this->HandleTableByPtr));

    return 0;
    }


/*** xhnAllocHandle() - generate a new handle id for a given pointer.
 ***/
handle_t
xhnAllocHandle(pHandleContext ctx, void* ptr)
    {
    pHandleData h;

	/** Alloc the structure **/
	h = (pHandleData)nmMalloc(sizeof(HandleData));
	if (!h) return XHN_INVALID_HANDLE;
	SETMAGIC(h, MGK_HANDLE);

	/** Grab a handle id **/
	h->HandleID = ctx->NextHandleID++;
	if (ctx->NextHandleID == XHN_INVALID_HANDLE) ctx->NextHandleID++;
	if (h->HandleID == ((handle_t)1)<<(sizeof(handle_t)*8-2)) ctx->NextHandleID = 0;
	if (ctx->NextHandleID == XHN_INVALID_HANDLE) ctx->NextHandleID++;

	/** Fill in the pointer. **/
	h->Ptr = ptr;

	/** Add to the table **/
	xhAdd(&(ctx->HandleTable), (void*)&(h->HandleID), (void*)h);
	xhAdd(&(ctx->HandleTableByPtr), (void*)&(h->Ptr), (void*)h);

    return h->HandleID;
    }


/*** xhnHandlePtr() - return the pointer for a given handle id
 ***/
void*
xhnHandlePtr(pHandleContext ctx, handle_t handle_id)
    {
    pHandleData h;

	/** Look it up **/
	h = (pHandleData)xhLookup(&(ctx->HandleTable), (void*)&handle_id);
	if (!h) return NULL;
	ASSERTMAGIC(h, MGK_HANDLE);

    return h->Ptr;
    }


/*** xhnHandle() - return the handle for a given pointer.
 ***/
handle_t
xhnHandle(pHandleContext ctx, void* ptr)
    {
    pHandleData h;

	/** Look it up **/
	h = (pHandleData)xhLookup(&(ctx->HandleTableByPtr), (void*)&ptr);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_HANDLE);

    return h->HandleID;
    }


/*** xhnUpdateHandle() - update the pointer for a given handle so that future
 *** requests for xhnHandlePtr() return the new value.
 ***/
int
xhnUpdateHandle(pHandleContext ctx, handle_t handle_id, void* ptr)
    {
    pHandleData h;

	/** Look it up **/
	h = (pHandleData)xhLookup(&(ctx->HandleTable), (void*)&handle_id);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_HANDLE);

	/** Update the pointer **/
	xhRemove(&(ctx->HandleTableByPtr), (void*)&(h->Ptr));
	h->Ptr = ptr;
	xhAdd(&(ctx->HandleTableByPtr), (void*)&(h->Ptr), (void*)h);

    return 0;
    }


/*** xhnUpdateHandleByPtr() - update the pointer for a given handle so that future
 *** requests for xhnHandlePtr() return the new value.
 ***/
int
xhnUpdateHandleByPtr(pHandleContext ctx, void* old_ptr, void* ptr)
    {
    pHandleData h;

	/** Look it up **/
	h = (pHandleData)xhLookup(&(ctx->HandleTableByPtr), (void*)&old_ptr);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_HANDLE);

	/** Update the pointer **/
	xhRemove(&(ctx->HandleTableByPtr), (void*)&(h->Ptr));
	h->Ptr = ptr;
	xhAdd(&(ctx->HandleTableByPtr), (void*)&(h->Ptr), (void*)h);

    return 0;
    }


/*** xhnFreeHandle() - release the handle and memory used by it.  The handle
 *** id will then become invalid and will not be reused until the id's wrap
 *** around (at 2^62 id's).
 ***/
int
xhnFreeHandle(pHandleContext ctx, handle_t handle_id)
    {
    pHandleData h;

	/** Look it up **/
	h = (pHandleData)xhLookup(&(ctx->HandleTable), (void*)&handle_id);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_HANDLE);

	/** Remove from table and release it. **/
	xhRemove(&(ctx->HandleTable), (void*)&handle_id);
	xhRemove(&(ctx->HandleTableByPtr), (void*)&(h->Ptr));
	nmFree(h, sizeof(HandleData));

    return 0;
    }


/*** xhnStringToHandle - converts a character string segment into a handle,
 *** in much the same way as strtoul() does for (potentially) shorter integers.
 ***/
handle_t
xhnStringToHandle(char* str, char** endptr, int base)
    {
    handle_t this = (handle_t)0;
    int digit;

	/** Determine base? **/
	if (base == 0 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	    base = 16;
	else if (base == 0 && str[0] == '0')
	    base = 8;
	else if (base == 0)
	    base = 10;

	/** Skip initial 0x **/
	if (base == 16 && str[0] == 0 && (str[1] == 'x' || str[1] == 'X'))
	    str += 2;

	/** Build integer while chars in the base range **/
	while(1)
	    {
	    if (str[0] >= '0' && str[0] <= '9')
		digit = str[0] - '0';
	    else if (str[0] >= 'a' && str[0] <= 'z')
		digit = str[0] - 'a' + 10;
	    else if (str[0] >= 'A' && str[0] <= 'Z')
		digit = str[0] - 'A' + 10;
	    else
		break;

	    if (digit >= base)
		break;

	    this = this*base + digit;
	    str++;
	    }

	if (endptr) *endptr = str;

    return this;
    }


/*** xhn_internal_FreeHandle() - this routine is used as a callback from
 *** ClearHandles, below, to free the handle and possibly call another
 *** free function.
 ***/
int
xhn_internal_FreeHandle(void* v1, void* v2)
    {
    pHandleData h = (pHandleData)v1;
    int (*free_fn)() = (int (*)())v2;

	free_fn(h->Ptr);
	nmFree(h, sizeof(HandleData));

    return 0;
    }


/*** xhnIterateHandles() - goes through all of the handles in a context
 *** and calls the given function on them.
 ***/
int
xhnClearHandles(pHandleContext ctx, int (*iter_fn)())
    {

	xhClear(&(ctx->HandleTable), xhn_internal_FreeHandle, iter_fn);
	xhClear(&(ctx->HandleTableByPtr), NULL, NULL);

    return 0;
    }


