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
/* Module:	cxss (Centrallix Security Subsystem) AuthStack          */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 2, 2013                                           */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/*									*/
/*		The AuthStack module provides authentication and	*/
/*		authorization contexts for the system.			*/
/************************************************************************/


/*** cxss_internal_FreeAuthStack() - deinitalize and free memory used by
 *** an auth stack item.
 ***/
int
cxss_internal_FreeAuthStack(pCxssAuthStack item)
    {
    int i;

	/** Release the list of endorsements **/
	for(i=0;i<item->Endorsements.nItems;i++)
	    nmFree(item->Endorsements.Items[i], sizeof(CxssEndorsement));
	xaDeInit(&item->Endorsements);

	/** Release memory for the stack item **/
	nmFree(item, sizeof(CxssAuthStack));

    return 0;
    }



/*** cxss_internal_AllocAuthStack() - create a new, initialized auth stack
 *** item with an empty set of endorsements.
 ***/
pCxssAuthStack
cxss_internal_AllocAuthStack()
    {
    pCxssAuthStack item;

	/** Allocate memory for it **/
	item = (pCxssAuthStack)nmMalloc(sizeof(CxssAuthStack));
	if (!item)
	    return NULL;
	
	/** Set up endorsements list **/
	xaInit(&item->Endorsements, 16);

	/** Basic setup **/
	item->CopyCnt = 0;
	item->Prev = NULL;

    return item;
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
    pCxssAuthStack src_ctx = (pCxssAuthStack)src_ctx_v;
    pCxssAuthStack* dst_ctx = (pCxssAuthStack*)dst_ctx_v;
    pCxssAuthStack new_ctx;
    pCxssEndorsement e;
    int i;

	/** Get a new authstack structure **/
	new_ctx = cxss_internal_AllocAuthStack();
	if (!new_ctx)
	    {
	    *dst_ctx = NULL;
	    return -1;
	    }

	/** Copy over the current endorsements **/
	if (src_ctx)
	    {
	    for(i=0;i<src_ctx->Endorsements.nItems;i++)
		{
		e = (pCxssEndorsement)nmMalloc(sizeof(CxssEndorsement));
		if (e)
		    {
		    memcpy(e, src_ctx->Endorsements.Items[i], sizeof(CxssEndorsement));
		    xaAddItem(&new_ctx->Endorsements, (void*)e);
		    }
		}
	    }

	*dst_ctx = new_ctx;

    return 0;
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
    pCxssAuthStack ctx = (pCxssAuthStack)ctx_v;
    pCxssAuthStack del;

	/** Loop through the stack and free each item **/
	while (ctx)
	    {
	    del = ctx;
	    ctx = ctx->Prev;
	    cxss_internal_FreeAuthStack(del);
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
    char buf1[64];
    char buf2[64];
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
	    if (strcmp(ptr1, ptr2) != 0 && *ptr2)
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
    pCxssAuthStack sptr, del;

	/** Get auth stack pointer **/
	sptr = (pCxssAuthStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    mssError(1,"CXSS","Attempt to cxssPopContext() beyond end of auth stack");
	    return -1;
	    }

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
	cxss_internal_FreeAuthStack(del);

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
    pCxssAuthStack sptr;

	/** Get auth stack pointer **/
	sptr = (pCxssAuthStack)thGetSecParam(NULL);

	/** Create first stack item? **/
	if (!sptr)
	    {
	    sptr = cxss_internal_AllocAuthStack();
	    thSetSecParam(NULL, (void*)sptr, cxss_internal_CopyContext, cxss_internal_FreeContext);
	    }
	else
	    {
	    /** Increment copy count **/
	    sptr->CopyCnt++;
	    }

    return 0;
    }



/*** cxssAddEndorsement() - add an endorsement to the current auth
 *** context.  An endorsement is a "privilege" in an abstract sense.
 ***/
int
cxssAddEndorsement(char* endorsement, char* context)
    {
    pCxssAuthStack sptr, new;
    pCxssEndorsement e;
    int i;

	/** Don't add it if we already have it **/
	if (cxssHasEndorsement(endorsement, context) == 1)
	    return 0;

	/** Get auth stack pointer **/
	sptr = (pCxssAuthStack)thGetSecParam(NULL);
	if (!sptr)
	    {
	    mssError(1,"CXSS","Attempt to add endorsement '%s' outside of an auth context", endorsement);
	    return -1;
	    }

	/** Need to duplicate stack head (copy-on-write)? **/
	if (sptr->CopyCnt > 0)
	    {
	    new = cxss_internal_AllocAuthStack();
	    if (!new)
		return -1;
	    new->Prev = sptr;
	    for(i=0;i<sptr->Endorsements.nItems;i++)
		{
		e = (pCxssEndorsement)nmMalloc(sizeof(CxssEndorsement));
		if (!e)
		    return -1;
		memcpy(e, sptr->Endorsements.Items[i], sizeof(CxssEndorsement));
		xaAddItem(&new->Endorsements, (void*)e);
		}
	    sptr->CopyCnt--;
	    thSetSecParam(NULL, (void*)new, cxss_internal_CopyContext, cxss_internal_FreeContext);
	    sptr = new;
	    }

	/** Add endorsement to the stack head **/
	e = (pCxssEndorsement)nmMalloc(sizeof(CxssEndorsement));
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
    pCxssAuthStack sptr;
    pCxssEndorsement e;
    int i;

	/** Get auth stack pointer **/
	sptr = (pCxssAuthStack)thGetSecParam(NULL);
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

