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
<!-- File:        xhash.md                                                -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes the xhash module in centrallix-lib.           -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# The XHash Library

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [The XHash Library](#the-xhash-library)
  - [Introduction](#introduction)
  - [xhInitialize()](#xhinitialize)
  - [xhInit()](#xhinit)
  - [xhDeInit()](#xhdeinit)
  - [xhAdd()](#xhadd)
  - [xhRemove()](#xhremove)
  - [xhLookup()](#xhlookup)
  - [xhClear()](#xhclear)
  - [xhForEach()](#xhforeach)
  - [xhClearKeySafe()](#xhclearkeysafe)


## Introduction
The xhash (xh) module provides an extensible hash table interface.  The hash table is a table of linked lists of items, so collisions and overflows are handled by this data structure (although excessive collisions still cause a performance loss).  This implementation also supports variable-length keys for more flexible usecases.

- ⚠️ **Warning**: All `xhXYZ()` function calls assume that the `pXHashTable this` arg points to a valid hashtable struct.  All non-init functions assume that this struct has been validly initialized and has not yet been freed.  If these conditions are not met, the resulting behavior is undefined.


## xhInitialize()
```c
int xhInitialize();
```
Initialize the random number table for hash computation, returning 0 on success or -1 if an error occurs.  Normally, you can assume someone else has already called this during program startup.


## xhInit()
```c
int xhInit(pXHashTable this, int rows, int keylen);
```
This function initializes a hash table, setting the number of rows and the key length.  Specify a `keylen` of 0 for for variable length keys (aka. null-terminated strings).  The `rows` should be an odd number, preferably prime (although that isn't required).  `rows` **SHOULD NOT** be a power of 2.  Providing this value allows the caller to optimize it based on how much data they expect to be stored in the hash table.  If this value is set to 1, the hash search degenerates to a linear array search with extra overhead.  Thus, the value should be large enough to comfortably accommodate the elements with minimal collisions.  Typical values include 31, 251, or 255 (though 255 is not prime).


## xhDeInit()
```c
int xhDeInit(pXHashTable this);
```
This function deinitializes a hash table struct, freeing all rows.  Note that the stored data is not freed and neither are the keys as this data is assumed to be the responsibility of the caller.  Returns 0 on success, or -1 if an error occurs.


## xhAdd()
```c
int xhAdd(pXHashTable this, char* key, char* data);
```
Adds an item to the hash table, with a given key value and data pointer.  Both data and key pointers must have a lifetime that exceeds the time that they item is hashed, as they are assumed to be the responsibility of the caller.  This function returns 0 on success, or -1 if an error occurs.


## xhRemove()
```c
int xhRemove(pXHashTable this, char* key);
```
This function removes an item with the given key value from the hash table.  It returns 0 if the item was successfully removed, or -1 if an error occurs (including failing to find the item).


## xhLookup()
```c
char* xhLookup(pXHashTable this, char* key);
```
This function returns a pointer to the data associated with the given key, or `NULL` if an error occurs (including failing to find the key).


## xhClear()
```c
int xhClear(pXHashTable this, int (*free_fn)(), void* free_arg);
```
Clears all items from a hash table.  If a `free_fn()` is provided, it will be invoked with each data pointer as the first argument and `free_arg` as the second argument as items are removed.  The return value of the `free_fn()` is ignored.  This function returns 0 on success (even if the `free_fn()` returns an error), or -1 if an error is detected.


## xhForEach()
```c
int xhForEach(pXHashTable this, int (*callback_fn)(pXHashEntry, void*), void* each_arg);
```
This function executes an operation on each entry of the hash table entry.  The provided callback function will be called with each entry (in an arbitrary order).  This function is provided 2 parameters: the current hash table entry, and a `void*` argument specified using `each_arg`.  If any invocation of the callback function returns a value other than 0, the `xhForEach()` will immediately fail, returning that value as the error code.

This function returns 0 if the function executes successfully, 1 if the callback function is `NULL`, or n (where n != 0) if the callback function returns n.  It does not return any error code other than 1 or any error codes returned by `callback_fn()`.


## xhClearKeySafe()
```c
int xhClearKeySafe(pXHashTable this, void (*free_fn)(pXHashEntry, void*), void* free_arg);
```
This function clears all contents from the hash table.  The free function is passed each hash entry struct and `free_arg`, allowing it to free both the value and key, if needed, and the free function is not allowed to return an error code.  This function returns 0 for success as long as `free_fn()` is nonnull, otherwise it returns -1.
