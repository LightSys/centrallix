#ifndef WARN_H
#define	WARN_H

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      warn.c, warn.h                                          */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: A utility wrapper to print warnings when a function     */
/*              call misbehaves.  Not for printing errors.              */
/************************************************************************/

#include <errno.h>
#include <math.h>
#include <string.h>

#include "expect.h"

/** File name macro, expanding functionality like __FILE__ and __LINE__. **/
#define __FILENAME__ \
    ({ \
    const char* last_directory = strrchr(__FILE__, '/'); \
    ((last_directory != NULL) ? last_directory + 1 : __FILE__); \
    })

/** Internal warning printer (forward declaration). **/
void printWarningInternal(const int error_code, const char* c_str, const char* file_name, const int line_number);
#define printWarning(error_code, c_str) printWarningInternal(error_code, (c_str), __FILE__, __LINE__)

/*** Prints a warning if the result of the passed function call is an error
 *** code, aka. any not zero int.
 *** 
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the warning message if a warning occurs.
 *** @returns The result of the checked expression.
 ***/
#define warnFail(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	int _r = (result); \
	if (UNLIKELY(_r != 0)) printWarning(_r, #result" failed"); \
	_r; \
    })

/*** Prints a warning if the result of the passed function call is negative.
 *** 
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the warning message if a warning occurs.
 *** @returns The result of the checked expression.
 ***/
#define warnNeg(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	int _r = (result); \
	if (UNLIKELY(_r < 0)) printWarning(_r, #result" failed"); \
	_r; \
    })

/*** Prints a warning if the result of the passed function call is NAN.
 *** 
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the warning message if a warning occurs.
 *** @returns The result of the checked expression.
 ***/
#define warnDouble(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	double _r = (result); \
	if (UNLIKELY(isnan(_r))) printWarning(-1, #result" failed"); \
	_r; \
    })

/*** Prints a warning if the result of the passed function call is NULL.
 *** 
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the warning message if a warning occurs.
 *** @returns The result of the checked expression.
 ***/
#define warnNull(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	void* _r = (result); \
	if (UNLIKELY(_r == NULL)) printWarning(-1, #result" failed"); \
	_r; \
    })

#endif	/* WARN_H */
