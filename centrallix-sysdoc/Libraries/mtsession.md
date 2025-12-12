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
<!-- File:        mtsession.md                                            -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes the mtsession module in centrallix-lib.       -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# The MTSession Library

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [The MTSession Library](#the-mtsession-library)
  - [Introduction](#introduction)
  - [mssUserName()](#mssusername)
  - [mssPassword()](#msspassword)
  - [mssSetParam()](#msssetparam)
  - [mssGetParam()](#mssgetparam)
  - [mssError()](#msserror)
  - [mssErrorErrno()](#msserrorerrno)


## Introduction
The mtsession (MSS) module is used for session authentication, error reporting, and for storing session-wide variables such as the current date format, username, and password (used when issuing a login request to a remote server).  Care should be taken in the use of Centrallix that its coredump files are NOT in a world-readable location, as the password will be visible in the coredump file (or just ulimit the core file size to 0).
<!-- TODO: Greg - This feels like important security info that should maybe be documented somewhere more obvious, but I don't know. -->

- ⚠️ **Warning**: This documentation is incomplete, as several relevant functions are not explained here.  You can help by expanding it.


## mssInitialize()
```c
int mssInitialize(char* authmethod, char* authfile, char* logmethod, int logall, char* log_progname);
```
This function initializes the session manager and sets global variables used in this module.  It returns 0 if successful and -1 if an error occurs.


## mssUserName()
```c
char* mssUserName();
```
This function returns the current user name, or `NULL` an error occurs.


## mssPassword()
```c
char* mssPassword();
```
This function returns the current user's password that they used to log into Centrallix, or `NULL` an error occurs.


## mssSetParam()
```c
int mssSetParam(char* paramname, char* param);
```
This function sets the session parameter of the provided name (`paramname`) to the provided value (`param`).  The parameter MUST be a string value.  This function returns 0 if successful, or -1 an error occurs.


## mssGetParam()
```c
char* mssGetParam(char* paramname);
```
Returns the value of a session parameter of the provided name (`paramname`), or `NULL` if an error occurs.  Common session parameters include:
- `dfmt`: The current date format.
- `mfmt`: The current money format.
- `textsize`: The current max text size from a read of an object's content via `objGetAttrValue(obj, "objcontent", POD(&str))`


## mssError()
```c
int mssError(int clr, char* module, char* message, ...);
```
Formats and caches an error message for return to the user.  This function returns 0 if successful, or -1 if an error occurred.

| Parameter | Type          | Description
| --------- | ------------- | ------------
| crl       | int           | If set to 1, all previous error messages are cleared. Set this when the error is initially discovered and no other module is likely to have made a relevant `mssError()` call for the current error.
| module    | char*         | A two-to-five letter abbreviation of the module reporting the error.  This is typically the module or driver's abbreviation prefix in full uppercase letters (although that is not required).  This is intended to help the developer find the source of the error faster.
| message   | char*         | A string error message, accepting format specifiers like `%d` and `%s` which are supplied by the argument list, similar to `printf()`.
| ...       | ...           | Parameters for the formatting.

Errors that occur inside a session context are normally stored up and not printed until other MSS module routines are called to fetch the errors.  Errors occurring outside a session context (such as in Centrallix's network listener) are printed to Centrallix's standard output immediately.

The `mssError()` function is not required to be called at every function nesting level when an error occurs.  For example, if the expression compiler returns -1 indicating that a compilation error occurred, it has probably already added one or more error messages to the error list.  The calling function should only call `mssError()` if doing so would provide additional context or other useful information (e.g. _What_ expression failed compilation? _Why_ as an expression being compiled? etc.).  However, it is far easier to give too little information that too much, so it can often be best to air on the side of calling `mssError()` with information that might be irrelevant, rather than skipping it and leaving the developer confused.

- 📖 **Note**: The `mssError()` routines do not cause the calling function to return or exit.  The function must still clean up after itself and return an appropriate value (such as `-1` or `NULL`) to indicate failure.

- ⚠️ **Warning**: Even if `-1` is returned, the error message may still be sent to the user in some scenarios.  This is not guaranteed, though.

- ⚠️ **Warning**: `%d` and `%s` are the ONLY supported format specifier for this function.  **DO NOT** use any other format specifiers like `%lf`, `%u`, `%lu`, `%c` etc.  **DO NOT** attempt to include `%%` for a percent symbol in your error message, as misplaced percent symbols often break this function. If you wish to use these features of printf, it is recommended to print the error message to a buffer and pass that buffer to `mssError()`, as follows:
    ```c
    char err_buf[256];
    snprintf(err_buf, sizeof(err_buf),
	"Incorrect values detected: %u, %g (%lf), '%c'",
	unsigned_int_value, double_value, char_value
    );
    if (mssError(1, "EXMPL", "%s", err_buf) != 0)
	{
	fprintf(stderr, "ERROR! %s\n", err_buf);
	}
    return -1;
    ```



## mssErrorErrno()
```c
int mssErrorErrno(int clr, char* module, char* message, ...);
```
This function works the same way as [`mssError`](#mssError), except checks the current value of `errno` and includes a description of any error stored there.  This is useful if a system call or other library function is responsible for this error.
