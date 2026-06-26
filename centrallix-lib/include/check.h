#ifndef CHECK_H
#define	CHECK_H

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
/* Module:      check.c, check.h                                        */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: A utility to help with error checking on function       */
/*              return values, especially for library functions.        */
/************************************************************************/

#include <errno.h>

#include "expect.h"

/** File name macro, expanding functionality like __FILE__ and __LINE__. **/
#define __FILENAME__ \
    ({ \
    const char* last_directory = strrchr(__FILE__, '/'); \
    ((last_directory != NULL) ? last_directory + 1 : __FILE__); \
    })

/** Internal error printer (forward declaration). **/
void print_err_internal(const int error_code, const char* c_str, const char* file_name, const int line_number);

#define print_err(error_code, c_str) print_err_internal(error_code, (c_str), __FILE__, __LINE__)
#define print_fail(c_str) errno = 0; print_err(-1, (c_str))

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is not zero.  Not intended for user errors.
 *** 
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the error message if an error occurs.
 *** @returns The result of the checked expression.
 ***/
#define check(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	const int _r = (result); \
	if (UNLIKELY(_r != 0)) print_err(_r, #result); \
	_r; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is negative. Not intended for user errors.
 ***
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the error message if an error occurs.
 *** @returns The result of the checked expression.
 ***/
#define check_neg(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	const int _r = (result); \
	if (UNLIKELY(_r < 0)) print_err(_r, #result); \
	_r; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is a NAN double. Not intended for user errors.
 ***
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the error message if an error occurs.
 *** @returns The result of the checked expression.
 ***/
#define check_double(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	const double _r = (result); \
	if (UNLIKELY(isnan(_r))) print_err(0, #result); \
	_r; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is a NULL pointer. Not intended for user errors.
 ***
 *** @param result The expression to check.  The text of this expression is
 *** 	included in the error message if an error occurs.
 *** @returns The result of the checked expression.
 ***/
#define check_ptr(result) \
    ({ \
	errno = 0; /* Reset errno to prevent confusion. */ \
	const void* _r = (result); \
	if (UNLIKELY(_r == NULL)) print_err(0, #result); \
	_r; \
    })

#endif	/* CHECK_H */
