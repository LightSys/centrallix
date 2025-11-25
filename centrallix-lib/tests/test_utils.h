#ifndef TEST_UTILITY_H
#define	TEST_UTILITY_H

/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_uitls.h						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Useful utils to improve the developer experience when   */
/* 		testing centrallix-lib.					*/
/************************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"

/*** Expect two values to be equal.
 *** 
 *** @param v1 The first value.
 *** @param v2 The second value.
 *** @param sp The specifier to print the values if there is an error. This
 ***    MUST be a string known to the compiler at compile time, such as a
 ***    sting literal or a macro that expands to one.
 ***/
#define EXPECT_EQL(v1, v2, sp) \
    ({ \
    __typeof__ (v1) _v1 = (v1); \
    __typeof__ (v2) _v2 = (v2); \
    int success = (_v1 == _v2); \
    if (!success) fprintf(stderr, "  > Expected %s ("sp") to equal %s ("sp") at %s:%d\n", #v1, _v1, #v2, _v2, __FILE__, __LINE__); \
    success; \
    })
    
/*** Expect two strings to be equal.
 *** 
 *** @param v1 The first string.
 *** @param v2 The second string.
 ***/
#define EXPECT_STR_EQL(v1, v2) \
    ({ \
    char* _v1 = (v1); \
    char* _v2 = (v2); \
    int success = (strcmp(_v1, _v2) == 0); \
    if (!success) fprintf(stderr, "  > Expected %s (\"%s\") to equal %s (\"%s\") at %s:%d\n", #v1, _v1, #v2, _v2, __FILE__, __LINE__); \
    success; \
    })

/** Repeat the test as many times as possible within a set time window. **/
#define loop_tests(test_fn)                                                 \
    ({                                                                      \
    long long result = 0ll;                                                 \
    Timer iter_timer_, *iter_timer = timer_start(timer_init(&iter_timer_)); \
    while (timer_get(iter_timer) < 0.02)                                    \
	{                                                                   \
	result++;                                                           \
	if (!test_fn())                                                     \
	    {                                                               \
	    result = -1;                                                    \
	    break;                                                          \
	    }                                                               \
	}                                                                   \
    timer_de_init(iter_timer);                                              \
    result;                                                                 \
    })

#endif
