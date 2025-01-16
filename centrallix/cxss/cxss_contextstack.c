#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	cxss (Centrallix Security Subsystem) CtxStack          */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 2, 2013                                           */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/*									*/
/*		The Context Stack module provides authentication and	*/
/*		authorization contexts for the system.			*/
/************************************************************************/


/*** cxss_internal_FreeCtxStack() - deinitalize and free memory used by
 *** an auth stack item.
 ***/
int
cxss_internal_FreeCtxStack(pCxssCtxStack item)
    {
    int i;
    pCxssVariable vbl;

	/** Release the list of endorsements **/
	for(i=0;i<item->Endorsements.nItems;i++)
	    nmFree(item->Endorsements.Items[i], sizeof(CxssEndorsement));
	xaDeInit(&item->Endorsements);

	/** Release list of session variables **/
	for(i=0;i<item->SessionVariables.nItems;i++)
	    {
	    vbl = (pCxssVariable)item->SessionVariables.Items[i];
	    if (vbl->Value) nmSysFree(vbl->Value);
	    nmFree(vbl, sizeof(CxssVariable));
	    }
	xaDeInit(&item->SessionVariables);

	/** Release memory for the stack item **/
	nmFree(item, sizeof(CxssCtxStack));

    return 0;
    }



/*** cxss_internal_AllocCtxStack() - create a new, initialized auth stack
 *** item with an empty set of endorsements.
 ***/
pCxssCtxStack
cxss_internal_AllocCtxStack()
    {
    pCxssCtxStack item = NULL;

	/** Allocate memory for it **/
	item = (pCxssCtxStack)nmMalloc(sizeof(CxssCtxStack));
	if (!item)
	    goto error;
	
	/** Set up endorsements list **/
	if (xaInit(&item->Endorsements, 16) < 0)
	    goto error;

	/** Set up the session variables list **/
	if (xaInit(&item->SessionVariables, 16) < 0)
	    {
	    xaDeInit(&item->Endorsements);
	    goto error;
	    }

	/** Basic setup **/
	item->CopyCnt = 0;
	item->Prev = NULL;

	return item;

    error:
	if (item)
	    nmFree(item, sizeof(CxssCtxStack));
	return NULL;
    }



/*** cxss_internal_CopyContext() - make a copy of an authstack context, by
 *** initializing it just to the most recent authstack item (we don't allow
 *** a copied context to be popped beyond that item).
 ***
 *** This function is a callback that gets called by the MTASK threading
 *** layer when a new thread is being created, or when the application
 *** saves a copy of a thread's security context, and is used as a copy
 *** constructor.
 ***/
int
cxss_internal_CopyContext(void* src_ctx_v, void** dst_ctx_v)
    {
    pCxssCtxStack src_ctx = (pCxssCtxStack)src_ctx_v;
    pCxssCtxStack* dst_ctx = (pCxssCtxStack*)dst_ctx_v;
    pCxssCtxStack new_ctx = NULL;
    pCxssEndorsement e;
    pCxssVariable vbl, oldvbl;
    int i;

	/** Get a new authstack structure **/
	new_ctx = cxss_internal_AllocCtxStack();
	if (!new_ctx)
	    goto error;

	/** Copy over the current endorsements **/
	if (src_ctx)
	    {
	    for(i=0;i<src_ctx->Endorsements.nItems;i++)
		{
		e = (pCxssEndorsement)nmMalloc(sizeof(CxssEndorsement));
		if (!e)
		    goto error;
		memcpy(e, src_ctx->Endorsements.Items[i], sizeof(CxssEndorsement));
		xaAddItem(&new_ctx->Endorsements, (void*)e);
		}
	    }

	/** Copy over the session variables **/
	if (src_ctx)
	    {
	    for(i=0;i<src_ctx->SessionVariables.nItems;i++)
		{
		oldvbl = (pCxssVariable)src_ctx->SessionVariables.Items[i];
		vbl = (pCxssVariable)nmMalloc(sizeof(CxssVariable));
		if (!vbl)
		    goto error;
		memcpy(vbl, oldvbl, sizeof(CxssVariable));
		if (oldvbl->Value)
		    {
		    vbl->Value = nmSysStrdup(oldvbl->Value);
		    if (!vbl->Value)
			goto error;
		    }
		xaAddItem(&new_ctx->SessionVariables, (void*)vbl);
		}
	    }

#if CXSS_DEBUG_CONTEXTSTACK
	new_ctx->CallerReturnAddr = NULL;
#endif

	*dst_ctx = new_ctx;

	return 0;

    error:
	/** Clean up and exit **/
	if (new_ctx)
	    cxss_internal_FreeCtxStack(new_ctx);
	*dst_ctx = NULL;

	return -1;
    }



/*** cxss_internal_FreeContext() - release an entire authstack context.
 ***
 *** This function is a callback that gets called by the MTASK threading
 *** layer as a destructor when a thread is being destroyed or when a new
 *** security context is being applied to a thread (overwriting the old
 *** one).
 ***/
int
cxss_internal_FreeContext(void* ctx_v)
    {
    pCxssCtxStack ctx = (pCxssCtxStack)ctx_v;
    pCxssCtxStack del;

	/** Loop through the stack and free each item **/
	while (ctx)
	    {
	    del = ctx;
	    ctx = ctx->Prev;
	    cxss_internal_FreeCtxStack(del);
	    }

    return 0;
    }



/*** cxss_internal_CompareContexts() - compare two contexts (objects/classes)
 *** to determine if the first is the same as or more specific than the second
 *** one.  Returns -1 if not, 0 if OK.
 ***/
int
cxss_internal_CompareContexts(char* ctx1, char* ctx2)
    {
    char buf1[CXSS_IDENTIFIER_LENGTH];
    char buf2[CXSS_IDENTIFIER_LENGTH];
    char* ptr1;
    char* ptr2;
    char* end1;
    char* end2;
    int i;

	/** Special case * context **/
	if (!strcmp(ctx2, "*"))
	    return 0;
	if (!strcmp(ctx1, "*"))
	    ctx1 = ":::";

	/** Check each piece **/
	if (strlen(ctx1) >= sizeof(buf1) || strlen(ctx2) >= sizeof(buf2))
	    return -1;
	strtcpy(buf1, ctx1, sizeof(buf1));
	strtcpy(buf2, ctx2, sizeof(buf2));
	end1 = buf1;
	end2 = buf2;
	for(i=0;i<4;i++)
	    {
	    ptr1 = strsep(&end1, ":");
	    ptr2 = strsep(&end2, ":");
	    if (!ptr1) ptr1="";
	    if (!ptr2) ptr2="";
	    if (strcmp(ptr1, ptr2) != 0 && *ptr2 && strcmp(ptr1, "*"))
		return -1;
	    }

    return 0;
    }



/*** cxssPopContext() - this function is called when a part of the Centrallix
 *** server is LEAVING a context in which security settings and decisions may
 *** have been made.  This returns the security state to what it was before
 *** the corresponding cxssPushContext was called.
 ***/
int
cxssPopContext()
    {
    pCxssCtxStack sptr, del;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    mssError(1,"CXSS","Attempt to cxssPopContext() beyond end of auth stack");
	    return -1;
	    }

#if CXSS_DEBUG_CONTEXTSTACK
	if (sptr->CallerReturnAddr)
	    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
	    if (sptr->CallerReturnAddr != __builtin_return_address(1))
#pragma GCC diagnostic pop
		printf("WARNING - unbalanced cxssPopContext / cxssPushContext\n");
	    sptr->CallerReturnAddr = NULL;
	    }
#endif

	/** No changes since corresponding push? **/
	if (sptr->CopyCnt > 0)
	    {
	    sptr->CopyCnt--;
	    return 0;
	    }

	/** Delete this stack item and pop the stack **/
	del = sptr;
	sptr = sptr->Prev;
	thSetSecParam(NULL, (void*)sptr, cxss_internal_CopyContext, cxss_internal_FreeContext);
	cxss_internal_FreeCtxStack(del);

    return 0;
    }



/*** cxssPushContext() - start a new context for security settings and
 *** decisions.  This doesn't actually duplicate the context item at the
 *** head of the stack, but follows copy-on-write semantics so that the
 *** item is not duplicated until settings on it are changed.
 ***/
int
cxssPushContext()
    {
    pCxssCtxStack sptr;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);

	/** Create first stack item? **/
	if (!sptr)
	    {
	    sptr = cxss_internal_AllocCtxStack();
	    thSetSecParam(NULL, (void*)sptr, cxss_internal_CopyContext, cxss_internal_FreeContext);
	    }
	else
	    {
	    /** Increment copy count **/
	    sptr->CopyCnt++;
	    }

#if CXSS_DEBUG_CONTEXTSTACK
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
	sptr->CallerReturnAddr = __builtin_return_address(1);
#pragma GCC diagnostic pop
#endif

    return 0;
    }



/*** cxss_internal_CheckCOW() - check to see if we need to copy-on-write
 *** and so duplicate the current context stack item.
 ***
 *** Returns the top-of-context-stack after the COW check is done.
 ***/
pCxssCtxStack
cxss_internal_CheckCOW()
    {
    pCxssCtxStack sptr, new;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    mssError(1,"CXSS","Attempt to access context stack without a security context");
	    return NULL;
	    }

	/** Need to duplicate stack head (copy-on-write)? **/
	if (sptr->CopyCnt > 0)
	    {
	    if (cxss_internal_CopyContext((void*)sptr, (void**)&new) < 0)
		{
		mssError(1,"CXSS","Copy context failed");
		return NULL;
		}
	    new->Prev = sptr;
#if CXSS_DEBUG_CONTEXTSTACK
	    new->CallerReturnAddr = sptr->CallerReturnAddr;
	    sptr->CallerReturnAddr = NULL;
#endif
	    sptr->CopyCnt--;
	    thSetSecParam(NULL, (void*)new, cxss_internal_CopyContext, cxss_internal_FreeContext);
	    sptr = new;
	    }

    return sptr;
    }



/*** cxssAddEndorsement() - add an endorsement to the current auth
 *** context.  An endorsement is a "privilege" in an abstract sense.
 ***/
int
cxssAddEndorsement(char* endorsement, char* context)
    {
    pCxssCtxStack sptr;
    pCxssEndorsement e;

	/** Don't add it if we already have it **/
	if (cxssHasEndorsement(endorsement, context) == 1)
	    return 0;

	/** Get auth stack pointer **/
	sptr = cxss_internal_CheckCOW();
	if (!sptr)
	    {
	    mssError(0,"CXSS","Could not add endorsement '%s'", endorsement);
	    return -1;
	    }

	/** Add endorsement to the stack head **/
	e = (pCxssEndorsement)nmMalloc(sizeof(CxssEndorsement));
	if (!e)
	    {
	    mssError(1,"CXSS","Could not add endorsement '%s': out of memory", endorsement);
	    return -1;
	    }
	strtcpy(e->Endorsement, endorsement, sizeof(e->Endorsement));
	strtcpy(e->Context, context, sizeof(e->Context));
	xaAddItem(&sptr->Endorsements, (void*)e);

    return 0;
    }



/*** cxssHasEndorsement() - indicate whether or not a given endorsement is
 *** present in the current context.  Returns 0 if the endorsement is not
 *** present, or 1 if it is.  Returns -1 on error.
 ***/
int
cxssHasEndorsement(char* endorsement, char* context)
    {
    pCxssCtxStack sptr;
    pCxssEndorsement e;
    int i;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    mssError(1,"CXSS","Attempt to verify endorsement '%s' outside of an auth context", endorsement);
	    return -1;
	    }

	/** Look for the endorsement **/
	for(i=0;i<sptr->Endorsements.nItems;i++)
	    {
	    e = (pCxssEndorsement)sptr->Endorsements.Items[i];
	    if (!strcmp(endorsement, e->Endorsement) && cxss_internal_CompareContexts(context, e->Context) == 0)
		return 1;
	    }

    return 0;
    }


/*** cxssGetEndorsementList() - return a list of the currently allowed
 *** endorsements and contexts.  The caller is responsible for freeing the
 *** memory used (each string is allocated using nmSysMalloc() or similar)
 ***/
int
cxssGetEndorsementList(pXArray endorsements, pXArray contexts)
    {
    int i;
    pCxssCtxStack sptr;
    pCxssEndorsement e;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);
	if (!sptr)
	    return 0;

	/** Copy the list over **/
	for(i=0;i<sptr->Endorsements.nItems;i++)
	    {
	    e = (pCxssEndorsement)sptr->Endorsements.Items[i];
	    xaAddItem(endorsements, nmSysStrdup(e->Endorsement));
	    if (contexts)
		xaAddItem(contexts, nmSysStrdup(e->Context));
	    }

    return sptr->Endorsements.nItems;
    }


/*** cxssSetVariable() - Set a session variable.
 ***/
int
cxssSetVariable(char* name, char* value, int valuealloc)
    {
    pCxssCtxStack sptr;
    pCxssVariable vbl;
    int i, found;

	/** Get auth stack pointer **/
	sptr = cxss_internal_CheckCOW();
	if (!sptr)
	    {
	    mssError(0,"CXSS","Could not set variable '%s'", name);
	    return -1;
	    }

	/** Allocate value if need be **/
	if (!valuealloc && value)
	    {
	    value = nmSysStrdup(value);
	    if (!value)
		{
		mssError(1,"CXSS","Could not set variable '%s': out of memory", name);
		return -1;
		}
	    }

	/** Find variable, if already exists **/
	for(found=i=0;i<sptr->SessionVariables.nItems;i++)
	    {
	    vbl = sptr->SessionVariables.Items[i];
	    if (!strcmp(vbl->Name, name))
		{
		/** Found existing variable to modify **/
		found = 1;
		if (vbl->Value)
		    nmSysFree(vbl->Value);
		vbl->Value = value;
		break;
		}
	    }

	/** Did not find an existing variable - add a new one **/
	if (!found)
	    {
	    vbl = (pCxssVariable)nmMalloc(sizeof(CxssVariable));
	    if (!vbl)
		{
		mssError(1,"CXSS","Could not set variable '%s': out of memory", name);
		return -1;
		}
	    strtcpy(vbl->Name, name, sizeof(vbl->Name));
	    vbl->Value = value;
	    xaAddItem(&sptr->SessionVariables, (void*)vbl);
	    }

    return 0;
    }



/*** cxssGetVariable() - get a session variable.  Returns -1  on an error 
 *** condition, or 0 on success.  Always sets *value to either the
 *** variable's value or to NULL, if the variable is unset and a default is
 *** not given.
 ***/
int
cxssGetVariable(char* vblname, char** value, char* default_value)
    {
    pCxssCtxStack sptr;
    pCxssVariable vbl;
    int i;

	/** Get auth stack pointer **/
	sptr = (pCxssCtxStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    *value = default_value;
	    mssError(1,"CXSS","Attempt to get variable '%s' outside of an auth context", vblname);
	    return -1;
	    }

	/** Look for the variable **/
	for(i=0;i<sptr->SessionVariables.nItems;i++)
	    {
	    vbl = (pCxssVariable)sptr->SessionVariables.Items[i];
	    if (!strcmp(vbl->Name, vblname))
		{
		*value = (vbl->Value)?(vbl->Value):default_value;
		return 0;
		}
	    }

	/** Not found - go with the default **/
	*value = default_value;

    return 0;
    }

