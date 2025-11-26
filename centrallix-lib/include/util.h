#ifndef UTILITY_H
#define	UTILITY_H

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      util.c, util.h                                          */
/* Author:      Micah Shennum and Israel Fuller                         */
/* Date:        May 26, 2011                                            */
/* Description:	Collection of utilities including:                      */
/*              - Utilities for parsing numbers.                        */
/*              - The timer utility for benchmarking code.              */
/*              - snprint_bytes() for formatting a byte count.          */
/*              - snprint_llu() for formatting large numbers.           */
/*              - fprint_mem() for printing memory stats.               */
/*              - min() and max() for handling numbers.                 */
/*              - The check functions for reliably printing debug data. */
/************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

    int strtoi(const char *nptr, char **endptr, int base);
    unsigned int strtoui(const char *nptr, char **endptr, int base);

    char* snprint_bytes(char* buf, const size_t buf_size, unsigned int bytes);
    char* snprint_llu(char* buf, size_t buflen, unsigned long long value);
    void fprint_mem(FILE* out);
    
    typedef struct
	{
	double start, total;
	}
	Timer, *pTimer;
    
    pTimer timer_init(pTimer timer);
    pTimer timer_new(void);
    pTimer timer_start(pTimer timer);
    pTimer timer_stop(pTimer timer);
    double timer_get(pTimer timer);
    pTimer timer_reset(pTimer timer);
    void timer_de_init(pTimer timer);
    void timer_free(pTimer timer);

#ifdef	__cplusplus
}
#endif

#ifndef __cplusplus
#include <errno.h>

/*** TODO: Greg - Can we assume this code will always be compiled with GCC?
 *** If not, then the __typeof__, __LINE__, and __FILE__ syntaxes might be a
 *** portability concern.
 ***/

/*** @brief Returns the smaller of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The smaller of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define min(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a < _b) ? _a : _b; \
    })

/*** @brief Returns the larger of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The larger of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define max(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a > _b) ? _a : _b; \
    })

/** File name macro, expanding functionality like __FILE__ and __LINE__. **/
#define __FILENAME__ \
    ({ \
    const char* last_directory = strrchr(__FILE__, '/'); \
    ((last_directory != NULL) ? last_directory + 1 : __FILE__); \
    })

/** Error Handling. **/
void print_err(int code, const char* function_name, const char* file_name, const int line_number);

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is not zero. Not intended for user errors.
 ***
 *** @param result The result of the function we're checking.
 *** @returns Whether the passed function succeeded.
 ***/
#define check(result) \
    ({ \
    errno = 0; /* Reset errno to prevent confusion. */ \
    __typeof__ (result) _r = (result); \
    const bool success = (_r == 0); \
    if (!success) print_err(_r, #result, __FILE__, __LINE__); \
    success; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is negative. Not intended for user errors.
 ***
 *** @param result The result of the function we're checking.
 *** @returns Whether the passed function succeeded.
 ***/
#define check_neg(result) \
    ({ \
    errno = 0; /* Reset errno to prevent confusion. */ \
    __typeof__ (result) _r = (result); \
    const bool success = (_r >= 0); \
    if (!success) print_err(_r, #result, __FILE__, __LINE__); \
    success; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is -1. Not intended for user errors.
 ***
 *** @param result The result of the function we're checking.
 *** @returns Whether the passed function succeeded.
 ***/
#define check_weak(result) \
    ({ \
    errno = 0; /* Reset errno to prevent confusion. */ \
    __typeof__ (result) _r = (result); \
    const bool success = (_r != -1); \
    if (!success) print_err(_r, #result, __FILE__, __LINE__); \
    success; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is a NAN double. Not intended for user errors.
 ***
 *** @param result The result of the function we're checking.
 *** @returns result
 ***/
#define check_double(result) \
    ({ \
    errno = 0; /* Reset errno to prevent confusion. */ \
    __typeof__ (result) _r = (result); \
    if (isnan(_r)) print_err(0, #result, __FILE__, __LINE__); \
    _r; \
    })

/*** Ensures that developer diagnostics are printed if the result of the
 *** passed function call is a NULL pointer. Not intended for user errors.
 ***
 *** @param result The result of the function we're checking.
 *** @returns result
 ***/
#define check_ptr(result) \
    ({ \
    errno = 0; /* Reset errno to prevent confusion. */ \
    __typeof__ (result) _r = (result); \
    if (_r == NULL) print_err(0, #result, __FILE__, __LINE__); \
    _r; \
    })

#endif  /* __cplusplus */

#endif	/* UTILITY_H */
