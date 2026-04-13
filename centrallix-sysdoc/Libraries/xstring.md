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
<!-- File:        xstring.md                                              -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes the xstring module in centrallix-lib.         -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# The XString Library

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [The XString Library](#the-xstring-library)
  - [Introduction](#introduction)
  - [xsNew()](#xsnew)
  - [xsFree()](#xsfree)
  - [xsInit()](#xsinit)
  - [xsDeInit()](#xsdeinit)
  - [xsCheckAlloc()](#xscheckalloc)
  - [xsConcatenate()](#xsconcatenate)
  - [xsCopy()](#xscopy)
  - [xsStringEnd()](#xsstringend)
  - [xsConcatPrintf()](#xsconcatprintf)
  - [xsPrintf()](#xsprintf)
  - [xsWrite()](#xswrite)
  - [xsRTrim()](#xsrtrim)
  - [xsLTrim()](#xsltrim)
  - [xsTrim()](#xstrim)
  - [xsFind()](#xsfind)
  - [xsFindRev()](#xsfindrev)
  - [xsSubst()](#xssubst)
  - [xsReplace()](#xsreplace)
  - [xsInsertAfter()](#xsinsertafter)
  - [xsGenPrintf_va()](#xsgenprintf_va)
  - [xsGenPrintf()](#xsgenprintf)
  - [xsString()](#xsstring)
  - [xsLength()](#xslength)
  - [xsQPrintf_va(), xsQPrintf(), & xsConcatQPrintf()](#xsqprintf_va-xsqprintf--xsconcatqprintf)


## Introduction
The xstring (xs) module is used for managing growable strings.  It is based on a structure containing a small initial string buffer to avoid string allocations for small strings.  However, it can also perform `realloc()` operations to extend the string space for storing incrementally larger strings.  This module allows for strings to contain arbitrary data, even NULL (`'\0'`) characters mid-string.  Thus, it can also be used as an extensible buffer for arbitrary binary data.

- 📖 **Note**: The contents of the XString can be easily referenced with the `xstring->String` field in the xstring struct.

- ⚠️ **Warning**: Do not mix calls to [`xsNew()`](#xsnew)/[`xsFree()`](#xsfree) with calls to [`xsInit()`](#xsinit)/[`xsDeInit()`](#xsdeinit).  Every struct allocated using new must be freed, and ever struct allocated using init must be deinitted.  Mixing these calls can lead to memory leaks, bad frees, and crashes.


## xsNew()
```c
pXString xsNew()
```
This function allocates a new XString structure to contain a new, empty string.  It uses [`nmMalloc()`](#nmmalloc) because the XString struct is always a consistant size.  This function returns a pointer to the new string if successful, or `NULL` if an error occurs.


## xsFree()
```c
void xsFree(pXString this);
```
This function frees an XString structure allocated with [`xsNew()`](#xsnew), freeing all associated memory.


## xsInit()
```c
int xsInit(pXString this);
```
This function initializes an XString structure to contain a new, empty string.  This function returns 0 if successful, or -1 if an error occurs.


## xsDeInit()
```c
int xsDeInit(pXString this);
```
This function deinitializes an XString structure allocated with [`xsInit()`](#xsinit), freeing all associated memory.  This function returns 0 if successful, or -1 if an error occurs.


## xsCheckAlloc()
```c
int xsCheckAlloc(pXString this, int addl_needed);
```
This function will optionally allocate more memory, if needed, given the currently occupied data area and the additional space required (specified with `addl_needed`).  This function returns 0 if successful, or -1 if an error occurs.


## xsConcatenate()
```c
int xsConcatenate(pXString this, char* text, int len);
```
This function concatenates the `text` string onto the end of the XString's value.  If `len` is set, that number of characters are copied, including possible null characters (`'\0'`).  If `len` is -1, all data up to the null-terminater is copied.  This function returns 0 if successful, or -1 if an error occurs.

- ⚠️ **Warning**: Do not store pointers to values within the string while adding text to the end of the string.  The string may be reallocated to increase space, causing such pointers to break.  Instead, use offset indexes into the string and calculate pointers on demand with `xs->String + offset`.
    
    For example, **DO NOT**:
    ```c
    XString xs;
    if (xsInit(&xs) != 0) goto handle_error;
    
    if (xsConcatenate(&xs, "This is the first sentence. ", -1) != 0) goto handle_error;
    char* ptr = xsStringEnd(&xs); /* Stores string pointer! */
    if (xsConcatenate(&xs, "This is the second sentence.", -1) != 0) goto handle_error;
    
    /** Print will probably read invalid memory. **/
    printf("A pointer to the second sentence is '%s'\n", ptr);
    
    ...
    
    if (xsDeInit(&xs) != 0) goto handle_error;
    ```
    
    Instead, use indexes and pointer arithmetic like this:
    ```c
    XString xs;
    if (xsInit(&xs) != 0) goto handle_error;
    
    if (xsConcatenate(&xs, "This is the first sentence. ", -1) != 0) goto handle_error;
    int offset = xsStringEnd(&xs) - xs->String; /* Stores index offset. */
    if (xsConcatenate(&xs, "This is the second sentence.", -1) != 0) goto handle_error;
    
    /** Print will probably work fine. **/
    printf("A pointer to the second sentence is '%s'\n", xs->String + offset);
    
    ...
    
    if (xsDeInit(&xs) != 0) goto handle_error;
    ```


## xsCopy()
```c
int xsCopy(pXString this, char* text, int len);
```
This function copies the string `text` into the XString, overwriting any previous contents.  This function returns 0 if successful, or -1 if an error occurs.


## xsStringEnd()
```c
char* xsStringEnd(pXString this);
```
This function returns a pointer to the end of the string.  This function is more efficient than searching for a null-terminator using `strlen()` because the xs module already knows the string length.  Furthermore, since some string may contain nulls, using `strlen()` may produce an incorrect result.


## xsConcatPrintf()
```c
int xsConcatPrintf(pXString this, char* fmt, ...);
```
This function prints additional data onto the end of the string.  It is similar to printf, however, only the following features are supported:
- `%s`: Add a string (`char*`).
- `%d`: Add a number (`int`).
- `%X`: Add something?
- `%%`: Add a `'%'` character.
Attempting to use other features of printf (such as `%lf`, `%c`, `%u`, etc.) will cause unexpected results.

This function returns 0 if successful, or -1 if an error occurs.


## xsPrintf()
```c
int xsPrintf(pXString this, char* fmt, ...);
```
This function works the same as [`xsConcatPrintf()`](#xsconcatprintf), except that it overwrites the previous string instead of appending to it.  This function returns 0 if successful, or -1 if an error occurs.


## xsWrite()
```c
int xsWrite(pXString this, char* buf, int len, int offset, int flags);
```
This function writes data into the xstring, similar to using the standard fdWrite or objWrite API.  This function can thus be used as a value for `write_fn`, for those functions that require this (such as the `expGenerateText()` function).  This function returns `len` if successful, or -1 if an error occurs.


## xsRTrim()
```c
int xsRTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from the right side of the xstring.  This function returns 0 if successful, or -1 if an error occurs.


## xsLTrim()
```c
int xsLTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from the left side of the xstring.  This function returns 0 if successful, or -1 if an error occurs.


## xsTrim()
```c
int xsTrim(pXString this);
```
This function trims whitespace characters (spaces, tabs, newlines, and line feeds) from both sides of the xstring.  This function returns 0 if successful, or -1 if an error occurs.


## xsFind()
```c
int xsFind(pXString this, char* find, int findlen, int offset)
```
This function searches for a specific string (`find`) in the xstring, starting at the provided `offset`.  `findlen` is the length of the provided string, allowing it to include null characters (pass -1 to have the length calculated using `strlen(find)`).  This function returns the index where the string was found if successful, or -1 if an error occurs (including the string not being found).


## xsFind()
```c
int xsFindRev(pXString this, char* find, int findlen, int offset)
```
This function works the same as [`xsFind()`](#xsfind) except that it searches from the end of the string, resulting in better performance if the value is closer to the end of the string.  This function returns the index where the string was found if successful, or -1 if an error occurs (including the string not being found).


## xsSubst()
```c
int xsSubst(pXString this, int offset, int len, char* rep, int replen)
```
This function substitutes a string into a given position in an xstring.  This does not search for matches as with [`xsReplace()`](#xsrepalce), instead the position (`offset`) and length (`len`) must be specified.  Additionally, the length of the replacement string (`replen`) can be specified handle null characters.  Both `len` and `replen` can be left blank to generate them using `strlen()`.  This function returns 0 if successful, or -1 if an error occurs.


## xsReplace()
```c 
int xsReplace(pXString this, char* find, int findlen, int offset, char* rep, int replen);
```
This function searches an xString for the specified string (`find`) and replaces that string with another specified string (`rep`).  Both strings can have their length specified (`findlen` and `replen` respectively), or left as -1 to generate it using `strlen()`.  This function returns the starting offset of the replace if successful, or -1 if an error occurs (including the string not being found).


## xsInsertAfter()
```c
int xsInsertAfter(pXString this, char* ins, int inslen, int offset);
```
This function inserts the specified string (`ins`) at offset (`offset`).  The length of the string can be specified (`inslen`), or left as -1 to generate it using `strlen()`.  This function returns the new offset after the insertion (i.e. `offset + inslen`), or -1 if an error occurs.


## xsGenPrintf_va()
```c
int xsGenPrintf_va(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, va_list va);
```
This function performs a `printf()` operation to an `xxxWrite()` style function.

In the wise words of Greg Beeley from 2002:
> This routine isn't really all that closely tied to the XString module, but this seemed to be the best place for it.  If a `buf` and `buf_size` are supplied (`NULL` otherwise), then `buf` MUST be allocated with the `nmSysMalloc()` routine.  Otherwise, **kaboom!**  This routine will grow `buf` if it is too small, and will update `buf_size` accordingly.

This function returns the printed length (>= 0) on success, or -(errno) if an error occurs.


## xsGenPrintf()
```c
int xsGenPrintf(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, ...);
```
This function works the same as [`xsGenPrintf_va()`](#xsgenprintf_va), but with a more convenient signature for the developer.


## xsString()
```c
char* xsString(pXString this);
```
This function returns the stored string after checking for various errors, or returns `NULL` if an error occurs.


## xsLength()
```c
xsLength(pXString this);
```
This function returns the length of the string in constant time (since this value is stored in `this->Length`) checking for various errors, or returns `NULL` if an error occurs.

<!-- TODO: Greg - So why do we need xsStringEnd() again when `this->String + this->Length` or `this->String + xsLength()` appears to also solve all the same problems? Is it just to support legacy code? -->


## xsQPrintf_va(), xsQPrintf(), & xsConcatQPrintf()
```c
int xsQPrintf_va(pXString this, char* fmt, va_list va);
int xsQPrintf(pXString this, char* fmt, ...);
int xsConcatQPrintf(pXString this, char* fmt, ...);
```
These functions use the `QPrintf` to add data to an xstring.  They return 0 on success, or some other value on failure.
