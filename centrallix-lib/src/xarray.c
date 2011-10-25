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


/*** xaSetItem - set a specific item in the xarray to a given value
 ***/
int
xaSetItem(pXArray this, int index, void* item)
    {
    int new_alloc;
    void* ptr;
    int i, oldend;

	if (index < 0) return -1;
	oldend = this->nItems;

	/** Need more memory? **/
	if (index >= this->nAlloc)
	    {
	    new_alloc = this->nAlloc;
	    while (new_alloc <= index) new_alloc += BLK_INCR;
	    ptr = (void**)nmSysRealloc(this->Items, new_alloc*sizeof(void*));
	    if (!ptr) return -1;
	    this->nAlloc = new_alloc;
	    this->Items = ptr;
	    }
	if (this->nItems < index+1) this->nItems = index+1;

	/** Plop the item in its spot. **/
	this->Items[index] = item;
	if (oldend != this->nItems) for(i=oldend;i<this->nItems-1;i++) this->Items[i] = NULL;

    return index;
    }

/*** xaInsertBefore - insert item before index
 ***/
int
xaInsertBefore(pXArray this, int index, void* item)
    {
    void* ptr;
    int i;

	/** sanity check on args **/
	if (index < 0 || index >= this->nItems) return -1;

	/** Need more memory? **/
	if (this->nItems + 1 > this->nAlloc)
	    {
	    this->nAlloc += BLK_INCR;
	    ptr = (void**)nmSysRealloc(this->Items, this->nAlloc*sizeof(void*));
	    if (!ptr) return -1;
	    this->Items = ptr;
	    memset(ptr+this->nItems, 0, BLK_INCR*sizeof(void*));
	    }
	/** Move from index on inclusive ahead one slot **/
	for (i=this->nItems-1;i>=index;i--) this->Items[i+1] = this->Items[i];

	/** Insert item in the spot that opened up **/
	this->Items[index] = item;
	this->nItems++;

	return index;
    }

/*** xaInsertAfter - insert after index
 ***/
int xaInsertAfter(pXArray this, int index, void* item)
    {
    void* ptr;
    int i;

	/** sanity check on args **/
	if (index < 0 || index >= this->nItems) return -1;

	/** Need more memory? **/
	if (this->nItems + 1 > this->nAlloc)
	    {
	    this->nAlloc += BLK_INCR;
	    ptr = (void**)nmSysRealloc(this->Items, this->nAlloc*sizeof(void*));
	    if (!ptr) return -1;
	    this->Items = ptr;
	    memset(ptr+this->nItems, 0, BLK_INCR*sizeof(void*));
	    }
	/** Move from index on inclusive ahead one slot **/
	for (i=this->nItems-1;i>index;i--) this->Items[i+1] = this->Items[i];

	/** Insert item in the spot that opened up **/
	this->Items[index+1] = item;
	this->nItems++;

	return index+1;
    }


