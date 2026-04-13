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
<!-- File:        newmalloc.md                                            -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes the centrallix memory manager, implemented in -->
<!--              newmalloc.c, including how to allocate and free memory, -->
<!--              as well as features like caching and some debug tools.  -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# Memory Management in Centrallix

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [Memory Management in Centrallix](#objectsystem-driver-interface)
  - [Introduction](#introduction)
  - [nmMalloc()](#nmmalloc)
  - [nmFree()](#nmfree)
  - [nmStats()](#nmstats)
  - [nmRegister()](#nmregister)
  - [nmDebug()](#nmdebug)
  - [nmDeltas()](#nmdeltas)
  - [nmSysMalloc()](#nmsysmalloc)
  - [nmSysRealloc()](#nmsysrealloc)
  - [nmSysStrdup()](#nmsysstrdup)
  - [nmSysFree()](#nmsysfree)


## Introduction
Centrallix has its own memory management wrapper that caches deallocated blocks of memory by size for faster reuse.  This wrapper also detects double-freeing of blocks (sometimes), making debugging of memory problems just a little bit easier.

In addition, the memory manager provides statistics on the hit ratio of allocated blocks coming from the lists vs. `malloc()`, and on how many blocks of each size/type are `malloc()`ed and cached.  This information can be helpful for tracking down memory leaks.  Empirical testing has shown an increase of performance of around 50% or more in programs that use newmalloc.

One caveat is that this memory manager does not provide `nmRealloc()` function, only `nmMalloc()` and `nmFree()`.  Thus, either `malloc()`, `free()`, and `realloc()` or [`nmSysMalloc()`](#nmsysmalloc), [`nmSysFree()`](#nmsysfree), and [`nmSysRealloc()`](#nmsysrealloc) should be used for blocks of memory that might vary in size.

The newmalloc module can be accessed by adding `#include "cxlib/newmalloc.h"` to the include section of a .c file in centrallix, or `#include "newmalloc.h"` in centrallix-lib.

- 📖 **Note**: This memory manager is usually the wrong choice for blocks of memory of arbitrary, inconsistent sizes.  It is intended for allocating structures quickly that are of a specific size.  For example, allocated space for a struct that is always the same size.

- 🥱 **tl;dr**: Use `nmMalloc()` for structs, not for strings.

- ⚠️ **Warning**: Do not mix and match, even though calling `free()` on a block obtained from `nmMalloc()` or calling `nmFree()` on a block obtained from `malloc()` might not crash the program immediately.  However, it may result in either inefficient use of the memory manager, or a significant memory leak, respectively.  These practices will also lead to incorrect results from the statistics and block count mechanisms.

The newmalloc module provides the following functions:


## nmMalloc()
```c
void* nmMalloc(int size);
```
This function allocates a block of the given `size`.  It returns `NULL` if the memory could not be allocated.


## nmFree()
```c
void nmFree(void* ptr, int size);
```
This function frees the block of memory.

- ⚠️ **Warning**: The caller **must know the size of the block.**  Getting this wrong is very bad!!  For structures, this is trivial, simply use `sizeof()`, exactly the same as with `nmMalloc()`.


## nmStats()
```c
void nmStats(void);
```
Prints statistics about the memory manager, for debugging and optimizing.

For example:
```
NewMalloc subsystem statistics:
   nmMalloc: 20244967 calls, 19908369 hits (98.337%)
   nmFree: 20233966 calls
   bigblks: 49370 too big, 32768 largest size
```

- ⚠️ **Warning**: Centrallix-lib must be built with the configure option `--enable-debugging` for this function to work.  Otherwise, all the stats will be zeros.


## nmRegister()
```c
void nmRegister(int size, char* name);
```
Registers an inteligent name for block of the specified size.  This allows the memory manager to give more information when reporting block allocation counts.  A given size can have more than one name.  This function is optional and not required for any production usecases, but using it can make tracking down memory leaks easier.

This function is usually called in a module's `Initialize()` function on each of the structures the module uses internally.


## nmDebug()
```c
void nmDebug(void);
```
Prints a listing of block allocation counts, giving (by size):
- The number of blocks allocated but not yet freed.
- The number of blocks in the cache.
- The total allocations for this block size.
- A list of names (from [`nmRegister()`](#nmregister)) for that block size.


## nmDeltas()
```c
void nmDeltas(void);
```
Prints a listing of all blocks whose allocation count has changed, and by how much, since the last `nmDeltas()` call.  This function is VERY USEFUL FOR MEMORY LEAK DETECTIVE WORK.


## nmSysMalloc()
```c
void* nmSysMalloc(int size);
```
Allocates memory without using the block-caching algorithm.  This is roughly equivalent to `malloc()`, but pointers returned by malloc and this function are not compatible - i.e., you cannot `free()` something that was [`nmSysMalloc()`](#nmsysmalloc)'ed, nor can you [`nmSysFree()`](#nmsysfree) something that was `malloc()`'ed.

- 📖 **Note**: This function is much better to use on variable-sized blocks of memory.  `nmMalloc()` is better for fixed-size blocks, such as for structs.


## nmSysRealloc()
```c
void* nmSysRealloc(void* ptr, int newsize);
```
Changes the size of an allocated block of memory that was obtained from [`nmSysMalloc()`](#nmsysmalloc), [`nmSysRealloc()`](#nmsysrealloc), or [`nmSysStrdup()`](#nmsysstrdup).  The new pointer may be different if the block needs to be moved.  This is the rough equivalent of `realloc()`.

- 📖 **Note**: If you are `realloc()`'ing a block of memory and need to store pointers to data somewhere inside the block, it is often better to store an offset rather than a full pointer.  This is because a full pointer becomes invalid if a [`nmSysRealloc()`](#nmsysrealloc) causes the block to move.


## nmSysStrdup()
```c
char* nmSysStrdup(const char* str);
```
Allocates memory using the [`nmSysMalloc()`](#nmsysmalloc) function and copies the string `str` into this memory.  It is a rough equivalent of `strdup()`.  The resulting pointer can be free'd using [`nmSysFree()`](#nmsysfree).


## nmSysFree()
```c
void nmSysFree(void* ptr);
```
Frees a block of memory allocated by [`nmSysMalloc()`](#nmsysmalloc), [`nmSysRealloc()`](#nmsysrealloc), or [`nmSysStrdup()`](#nmsysstrdup).
