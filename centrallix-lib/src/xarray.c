#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "xarray.h"
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
/* Module: 	xarray.c, xarray.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 12, 1998					*/
/* Description:	Extensible array system for Centrallix.  Provides	*/
/*		for variable-length 32-bit value arrays.  On Alpha 	*/
/*		systems, values are 64-bit, but can still store 32-bit	*/
/*		values.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xarray.c,v 1.3 2003/04/03 04:32:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xarray.c,v $

    $Log: xarray.c,v $
    Revision 1.3  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

    Revision 1.2  2002/11/14 03:44:27  gbeeley
    Added a new function to the XArray module to do sorted array adds
    based on an integer field, which is portable between LSB and MSB
    platforms.  Fixed the normal sorted add routine which was not
    operating correctly anyhow.

    Revision 1.1.1.1  2001/08/13 18:04:22  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:02:57  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#define BLK_INCR	16


/*** xaInit - initialize an xarray structure
 ***/
int 
xaInit(pXArray this, int init_size)
    {

	/** Clear the thing and alloc the initial size **/
	this->nAlloc = BLK_INCR;
	if (this->nAlloc < init_size) this->nAlloc = init_size;
	this->Items = (void**)nmSysMalloc(this->nAlloc*sizeof(void*));
	this->nItems = 0;

    return 0;
    }


/*** xaDeInit - deinitialize an xarray structure
 ***/
int 
xaDeInit(pXArray this)
    {

	/** Free the items **/
	nmSysFree(this->Items);
	this->nItems = 0;

    return 0;
    }


/*** xaAddItem - adds an item to the end of an xarray structure's array
 *** listing.  Returns the index of the added item, or -1 if failed.
 ***/
int 
xaAddItem(pXArray this, void* item)
    {
    void** ptr;

	/** Need more memory? **/
	if (this->nItems >= this->nAlloc)
	    {
	    ptr = (void**)nmSysRealloc(this->Items, (this->nAlloc+BLK_INCR)*sizeof(void*));
	    if (!ptr) return -1;
	    this->nAlloc += BLK_INCR;
	    this->Items = ptr;
	    }

	/** Plop the item on the end **/
	this->Items[this->nItems++] = item;

    return this->nItems-1;
    }


/*** xaAddItemSorted - inserts an item into the appropriate place in the
 *** xarray in order to maintain a sorted array, based on an offset and
 *** length into the structure being filed.
 ***/
int
xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen)
    {
    void** ptr;
    int i;

	/** Need more memory? **/
	if (this->nItems >= this->nAlloc)
	    {
	    ptr = (void**)nmSysRealloc(this->Items, (this->nAlloc+BLK_INCR)*sizeof(void*));
	    if (!ptr) return -1;
	    this->nAlloc += BLK_INCR;
	    this->Items = ptr;
	    }

	/** Find an appropriate location **/
	for(i=0;i<=this->nItems;i++)
	    {
	    if (i == this->nItems || 
	        memcmp(((char*)item)+keyoffset, ((char*)this->Items[i])+keyoffset, keylen) < 0)
		{
		if (i < this->nItems) 
		    memmove(this->Items+i+1, this->Items+i, (this->nItems-i)*sizeof(void*));
		this->Items[i] = item;
		this->nItems++;
		break;
		}
	    }

    return this->nItems-1;
    }


/*** xaAddItemSortedInt32 - inserts an item into the appropriate place in the
 *** xarray in order to maintain a sorted array, based on an offset and
 *** a 32-bit host-byte-order integer value.
 ***/
int
xaAddItemSortedInt32(pXArray this, void* item, int keyoffset)
    {
    void** ptr;
    int i;

	/** Need more memory? **/
	if (this->nItems >= this->nAlloc)
	    {
	    ptr = (void**)nmSysRealloc(this->Items, (this->nAlloc+BLK_INCR)*sizeof(void*));
	    if (!ptr) return -1;
	    this->nAlloc += BLK_INCR;
	    this->Items = ptr;
	    }

	/** Find an appropriate location **/
	for(i=0;i<=this->nItems;i++)
	    {
	    if (i == this->nItems || 
	        *(int*)(((char*)item)+keyoffset) < *(int*)(((char*)this->Items[i])+keyoffset))
		{
		if (i < this->nItems) 
		    memmove(this->Items+i+1, this->Items+i, (this->nItems-i)*sizeof(void*));
		this->Items[i] = item;
		this->nItems++;
		break;
		}
	    }

    return this->nItems-1;
    }


/*** xaGetItem - lookup an item by its index, and return the item itself
 ***/
void* 
xaGetItem(pXArray this, int index)
    {
    return (index>=this->nItems || index==-1)?(NULL):(this->Items[index]);
    }


/*** xaFindItem - lookup an item by its value, and return the index if
 *** found, or -1 if not found.
 ***/
int 
xaFindItem(pXArray this, void* item)
    {
    int i,k=-1;

	/** Search for it **/
	for(i=0;i<this->nItems;i++) if (item==this->Items[i])
	    {
	    k=i;
	    break;
	    }

    return k;
    }


/*** xaRemoveItem - removes an item at a specific index from the array.
 *** To remove an item of a specific value, use this function in 
 *** conjunction with xaFindItem.
 ***/
int 
xaRemoveItem(pXArray this, int index)
    {
    int i;

	if (index>=this->nItems || index==-1) return -1;

	/** Move everything back to remove it **/
	this->nItems--;
	for(i=index;i<this->nItems;i++)
	    {
	    this->Items[i] = this->Items[i+1];
	    }

    return 0;
    }


/*** xaClear - remove all items from the array.
 ***/
int 
xaClear(pXArray this)
    {
    this->nItems=0;
    return 0;
    }


/*** xaCount - return number of items in the array
 ***/
int 
xaCount(pXArray this)
    {
    return this->nItems;
    }

