#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "xringqueue.h"
#include "newmalloc.h"

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
/* Module: 	xringqueue.c, xringqueue.h				*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	March 13, 2003						*/
/* Description:	Extensible ring queue system for Centrallix.  Provides	*/
/*		for variable-length 32-bit value ring queues.  On Alpha */
/*		systems, values are 64-bit, but can still store 32-bit	*/
/*		values.							*/
/************************************************************************/


/*** xrqInit - initialize an xringqueue structure
 ***/
int 
xrqInit(pXRingQueue this, int initSize)
    {

	/** Clear the thing and alloc the initial size **/
	this->nAlloc = initSize;
	/** sensible default if caller forgot to fill it in **/
	if (this->nAlloc==0) this->nAlloc = 16;
	this->Items = (void**)nmSysMalloc(this->nAlloc*sizeof(void*));
	this->nextIn=0;
	this->nextOut=0;

    return 0;
    }

/*** xrqDeInit - deinitialize an xringqueue structure
 ***/
int 
xrqDeInit(pXRingQueue this)
    {

	/** Free the items **/
	nmSysFree(this->Items);
	memset(this,0,sizeof(XRingQueue));

    return 0;
    }


/*** xrqEnqueue - adds an item to the end of an xringqueue structure's queue
 *** listing.  Returns the 0 on success, or -1 on failure.
 ***/
int
xrqEnqueue(pXRingQueue this, void* item)
    {
    void** ptr;

	/** Need more memory? **/
	if (xrqCount(this)>=this->nAlloc-1)
	    {
	    /** get the new memory **/
	    ptr = (void**)nmSysRealloc(this->Items, (this->nAlloc*2)*sizeof(void*));
	    if (!ptr) return -1;

	    /** only need to move data around if nextIn isn't pointing at the end of the list **/
	    if(this->nextIn != this->nAlloc-1)
		{
		/** move the data up to its new location **/
		memcpy(ptr+this->nAlloc,ptr,(this->nextIn)*sizeof(void*));

		/** zero out it's old location to be safe **/
		memset(ptr,0,sizeof(void*)*this->nextIn);

		/** update the location of nextIn **/
		this->nextIn+=this->nAlloc;
		}

	    /** update the memory we've allocated **/
	    this->nAlloc *= 2;
	    this->Items = ptr;
	    }

	/** Plop the item at the end of the line **/
	this->Items[this->nextIn++] = item;
	this->nextIn%=this->nAlloc;

    return 0;
    }


/*** xrqGetItem - lookup an item by its index, and return the item itself
 ***/
void* 
xrqDequeue(pXRingQueue this)
    {
    void *ret;
    if(xrqCount(this)>0)
	{
	ret = this->Items[this->nextOut];
	this->Items[this->nextOut]=NULL;
	this->nextOut++;
	this->nextOut%=this->nAlloc;
	return ret;
	}
    else
	return NULL;
    }


/*** xrqClear - remove all items from the queue.
 ***/
int 
xrqClear(pXRingQueue this)
    {
    this->nextIn=0;
    this->nextOut=0;
    memset(this->Items,0,sizeof(void*)*this->nAlloc);
    return 0;
    }


/*** xrqCount - return number of items in the queue
 ***/
int 
xrqCount(pXRingQueue this)
    {
    return ((this->nextIn-this->nextOut)+this->nAlloc)%this->nAlloc;
    }

