<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 1998-2012 LightSys Technology Services, Inc.           -->
<!--                                                                      -->
<!-- This program is free software; you can redistribute it and/or modify -->
<!-- it under the terms of the GNU General Public License as published by -->
<!-- the Free Software Foundation; either version 2 of the License, or    -->
<!-- (at your option) any later version.                                  -->
<!--                                                                      -->
<!-- This program is distributed in the hope that it will be useful,      -->
<!-- but WITHOUT ANY WARRANTY; without even the implied warranty of       -->
<!-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        -->
<!-- GNU General Public License for more details.                         -->
<!--                                                                      -->
<!-- You should have received a copy of the GNU General Public License    -->
<!-- along with this program; if not, write to the Free Software          -->
<!-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             -->
<!-- 02111-1307  USA                                                      -->
<!--                                                                      -->
<!-- A copy of the GNU General Public License has been included in this   -->
<!-- distribution in the file "COPYING".                                  -->
<!--                                                                      -->
<!-- File:        xarray.md                                               -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes the xarray module in centrallix-lib.          -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# The XArray Library

**Author**:   Greg Beeley

**Date**:     January 13, 1999

**Updated**:  December 11, 2025

**License**:  Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [The XArray Library](#the-xarray-library)
  - [Introduction](#introduction)
  - [xaNew()](#xanew)
  - [xaFree()](#xafree)
  - [xaInit()](#xainit)
  - [xaDeInit()](#xadeinit)
  - [xaAddItem()](#xaadditem)
  - [xaAddItemSorted()](#xaadditemsorted)
  - [xaAddItemSortedInt32()](#xaadditemsortedint32)
  - [xaGetItem()](#xagetitem)
  - [xaFindItem()](#xafinditem)
  - [xaFindItemR()](#xafinditemr)
  - [xaRemoveItem()](#xaremoveitem)
  - [xaClear()](#xaclear)
  - [xaClearR()](#xaclearr)
  - [xaCount()](#xacount)
  - [xaInsertBefore()](#xainsertbefore)
  - [xaInsertAfter()](#xainsertafter)


## Introduction
The xarray (xa) module is intended to manage sized growable arrays, similar to a light-weight arraylist implementation.  It includes the `XArray`, which has the following fields:
- `nItems : int`: The number of items in the array.
- `nAlloc : int`: Internal variable to store the size of the allocated memory.
- `Items : void**`: The allocated array of items.

- 📖 **Note**: Some code occasionally sets `nAlloc` to 0 after an XArray struct has been deinitialized to indicate that the relevant data is no longer allocated.  Other than this, it is only used internally by the library.

- ⚠️ **Warning**: Do not mix calls to [`xaNew()`](#xanew)/[`xaFree()`](#xafree) with calls to [`xaInit()`](#xainit)/[`xaDeInit()`](#xadeinit).  Every struct allocated using new must be freed, and ever struct allocated using init must be deinitted.  Mixing these calls can lead to memory leaks, bad frees, and crashes.


## xaNew()
```c
pXArray xaNew(int init_size);
```
Allocates a new `XArray` struct on the heap (using [`nmMalloc()`](#nmmalloc) for caching) and returns a pointer to it, or returns `NULL` if an error occurs.


## xaFree()
```c
int xaFree(pXArray this);
```
Frees a `pXArray` allocated using [`xaNew`](#xanew), returning 0 if successful or -1 if an error occurs.


## xaInit()
```c
int xaInit(pXArray this, int init_size);
```
This function initializes an allocated (but uninitialized) xarray. It makes room for `init_size` items initially, but this is only an optimization.  A typical value for `init_size` is 16.  Remember to [`xaDeInit`](#xadeinit) this xarray, do **not** [`xaFree`](#xafree) it.

This function returns 0 on success, or -1 if an error occurs.


## xaDeInit()
```c
int xaDeInit(pXArray this);
```
This function de-initializes an xarray, but does not free the XArray structure itself.  This is useful if the structure is a local variable allocated using [`xaInit()`](#xainit).

This function returns 0 on success, or -1 if an error occurs.

For example:
```c
XArray arr;
if (xaInit(&arr, 16) != 0) goto handle_error;

/** Use the xarray. **/

if (arr.nAlloc != 0 && xaDeInit(&arr) != 0) goto handle_error;
arr.nAlloc = 0;
```


## xaAddItem()
```c
int xaAddItem(pXArray this, void* item);
```
This function adds an item to the end of the xarray.  The item is assumed to be a `void*`, but this function will _not_ follow pointeres stored in the array.  Thus, other types can be typecast and stored into that location (such as an `int`).

This function returns 0 on success, or -1 if an error occurs.


## xaAddItemSorted()
```c
int xaAddItemSorted(pXArray this, void* item, int keyoffset, int keylen);
```
This function adds an item to a sorted xarray while maintaining the sorted property.  The value for sorting is expected to begin at the offset given by `keyoffset` and continue for `keylen` bytes.  This function _will_ follow pointers are stored in the array so casting other types to store them is not allowed (as it is with [`xaAddItem()`](#xaadditem)).


## xaAddItemSortedInt32()
```c
int xaAddItemSortedInt32(pXArray this, void* item, int keyoffset)
```
<!-- TODO: Greg - How does this work? Does it assume that the item pointers are typecasted 32-bit ints or that they point to 32-bit ints? -->


## xaGetItem()
```c
void* xaGetItem(pXArray this, int index)
```
This function returns an item given a specific index into the xarray, or `NULL` if the index is out of bounds.  If the bounds check needs to be omitted for performance and the caller can otherwise verify that no out of bounds read is possible (e.g. because they are iterating from 0 to `xarray->nItems`), the caller should access `xarray->Items` directly.  Either way, the result may need to be typecasted or stored in a variable of a specific type for it to be useable, and error checking for `NULL` values should be used.


## xaFindItem()
```c
int xaFindItem(pXArray this, void* item);
```
This function returns array index for the provided item in the array, or -1 if the item could not be found.  Requires an exact match, so two `void*` pointing to different memory with identical contents are not considered equal by this function.  If the data is actually another datatype typecasted as a `void*`, all 8 bytes must be identical for a match.

For example:
```c
void* data = &some_data;

XArray xa;
xaInit(&xa, 16);

...

xaAddItem(&xa, data);

...

int item_id = xaFindItem(&xa, data);
assert(data == xa.Items[item_id]);
```


## xaFindItemR()
```c
int xaFindItemR(pXArray this, void* item);
```
This function works the same as [`xaFindItem()`](#xafinditem), however it iterates in reverse, giving a slight performance boost, especially for finding items near the end of the array.


## xaRemoveItem()
```c
int xaRemoveItem(pXArray this, int index)
```
This function removes an item from the xarray at the given the index, then shifts all following items back to fill the gap created by the removal.  XArray is not optimized for removing multiple items efficiently.  This function returns 0 on success, or -1 if an error occurs.


## xaClear()
```c
int xaClear(pXArray this, int (*free_fn)(), void* free_arg);
```
This function removes all elements from the xarray, leaving it empty.  `free_fn()` is invoked on each element with a `void*` to the element to be freed as the first argument and `free_arg` as the second argument (the return value of `free_fn()` is always ignored).  This function returns 0 on success (even if the `free_fn()` returns an error), or -1 if an error is detected.


## xaClearR()
```c
int xaClearR(pXArray this, int (*free_fn)(), void* free_arg);
```
This function works the same as [`xaClear()`](#xaclear), except that it is slightly faster because the free function is evaluated on items in reverse order.


## xaCount()
```c
int xaCount(pXArray this);
```
This function returns the number of items in the xarray, or -1 on error.  It is equivalent to accessing `xarray->nItems` (although the latter expression will not return an error).


## xaInsertBefore()
```c
int xaInsertBefore(pXArray this, int index, void* item)
```
This function inserts an item before the specified index, moving all following items forward to make space.  The new item cannot be inserted past the end of the array.  This function returns the index on success, or -1 if an error occurs.


## xaInsertAfter()
```c
int xaInsertAfter(pXArray this, int index, void* item)
```
This function inserts an item after the specified index, moving all following items forward to make space.  The new item cannot be inserted past the end of the array.  This function returns the index on success, or -1 if an error occurs.
