#ifndef UTILITY_H
#define	UTILITY_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	(util.c,.h)                                             */
/* Author:	Micah Shennum                                           */
/* Date:	May 26, 2011                                            */
/* Description:	Collection of utilities                                 */
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

/** TODO: Greg, is the __typeof__ syntax from GCC a portability concern? **/

/*** @brief Returns the smaller of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The smaller of the two values.
 *** 
 *** @note This macro uses GCC extensions to enusre type safety.
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
 *** @note This macro uses GCC extensions to enusre type safety.
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
void print_diagnostics(int code, const char* function_name, const char* file_name, const int line_number);

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
    if (!success) print_diagnostics(_r, #result, __FILE__, __LINE__); \
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
    if (!success) print_diagnostics(_r, #result, __FILE__, __LINE__); \
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
    if (!success) print_diagnostics(_r, #result, __FILE__, __LINE__); \
    success; \
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
    if (_r == NULL) print_diagnostics(0, #result, __FILE__, __LINE__); \
    _r; \
    })

/** Pattern for printing a binary int using printf(). **/
#define INT_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

/*** Converts an int to the values that should be passed to printf() for the
 *** INT_TO_BINARY_PATTERN pattern.
 *** 
 *** @attention - Double evaluation is NOT HANDLED so int_val will be evaluted
 ***    32 times when this macro is used. Ensure that evaluation of the value
 ***    passed for int_val does not have important side effects!
 *** 
 *** @param int_val The int to be printed.
 *** @returns Values for printf().
 ***/
#define INT_TO_BINARY(int_val) \
    ((int_val) & 0b10000000000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b01000000000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00100000000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00010000000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00001000000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000100000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000010000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000001000000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000100000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000010000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000001000000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000100000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000010000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000001000000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000100000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000010000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000001000000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000100000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000010000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000001000000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000100000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000010000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000001000000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000100000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000010000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000001000000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000100000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000010000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000001000 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000000100 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000000010 ? '1' : '0'), \
    ((int_val) & 0b00000000000000000000000000000001 ? '1' : '0')

#endif  /* __cplusplus */

#endif	/* UTILITY_H */
