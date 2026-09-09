#ifndef TEST_UTILITY_H
#define	TEST_UTILITY_H

/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2025-2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_utils.h						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Useful utils to improve the developer experience when   */
/* 		testing centrallix-lib.					*/
/************************************************************************/

#include <stdio.h>
#include <string.h>

#include "timer.h"

/*** Define lockup times.  Valgrind instruments every memory access, so tests
 *** may need longer to finish when running under valgrind.
 ***/
#define NORMAL_LOCKUP_SECONDS 5u
#define VALGRIND_LOCKUP_SECONDS 10u

/** Detect valgrind. **/
#ifdef USING_VALGRIND
#include "valgrind/valgrind.h"
#define LOCKUP_SECONDS	((RUNNING_ON_VALGRIND) ? VALGRIND_LOCKUP_SECONDS : NORMAL_LOCKUP_SECONDS)
#else
#define LOCKUP_SECONDS	NORMAL_LOCKUP_SECONDS
#endif

/*** The minimum number of seconds that a test can take.  Tests that take less
 *** time than this might not use enough cpu cycles, confusing the test suite
 *** performance tracking.
 ***/
#define MIN_TEST_SECONDS 0.1

/*** Expect two values to be equal.
 *** 
 *** @param v1 The first value.
 *** @param v2 The second value.
 *** @param sp The specifier to print the values if there is an error. This
 ***    MUST be a string known to the compiler at compile time, such as a
 ***    sting literal or a macro that expands to one.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_EQL(v1, v2, sp) \
    ({ \
    __typeof__ (v1) _v1 = (v1); \
    __typeof__ (v2) _v2 = (v2); \
    int success = (_v1 == _v2); \
    if (!success) fprintf(stderr, \
	"  > Expected %s ("sp") to equal %s ("sp") at %s:%d\n", \
	#v1, _v1, #v2, _v2, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Expect two strings to be equal.
 *** 
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_STR_EQL(str1, str2) \
    ({ \
    char* _str1 = (str1); \
    char* _str2 = (str2); \
    int success = (_str1 == _str2) || (_str1 != NULL && _str2 != NULL && strcmp(_str1, _str2) == 0); \
    if (!success) fprintf(stderr, \
	"  > Expected %s (\"%s\") to equal %s (\"%s\") at %s:%d\n", \
	#str1, _str1, #str2, _str2, __FILE__, __LINE__ \
    ); \
    success; \
    })
    
/*** Expect two cosine vectors from `cluster.c` to be equal.
 *** 
 *** @param v1 The first vector.
 *** @param v2 The second vector.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_VEC_EQL(v1, v2) \
    ({ \
	pVector _v1 = (v1); \
	pVector _v2 = (v2); \
	int success = ca_eql(_v1, _v2); \
	if (!success) \
	    { \
	    printf("  > Expected %s (1.) to equal %s (2.) at %s:%d, but got:\n", #v1, #v2, __FILE__, __LINE__); \
	    printf("  > 1. (%p) ", _v1); ca_print_vector(_v1); printf("\n"); \
	    printf("  > 2. (%p) ", _v2); ca_print_vector(_v2); printf("\n"); \
	    fflush(stdout); \
	    } \
	success; \
    })

/*** Expect a value to fall within a range.
 *** 
 *** @param v The value.
 *** @param min_v The minimum acceptable value.
 *** @param max_v The maximum acceptable value.
 *** @param sp The specifier to print the values if there is an error. This
 ***    MUST be a string known to the compiler at compile time, such as a
 ***    sting literal or a macro that expands to one.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_RANGE(v, min_v, max_v, sp) \
    ({ \
    __typeof__ (v) _v = (v); \
    __typeof__ (min_v) _min = (min_v); \
    __typeof__ (max_v) _max = (max_v); \
    int success = (_min <= _v && _v <= _max); \
    if (!success) fprintf(stderr, \
	"  > Expected %s ("sp") to be in the range %s ("sp") - %s ("sp") at %s:%d\n", \
	#v, _v, #min_v, _min, #max_v, _max, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Expect a string to contain another string.
 *** 
 *** @param str The string to search.
 *** @param sub The string to find within it.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_STR_HAS(str, sub) \
    ({ \
    char* _str = (str); \
    char* _sub = (sub); \
    int success = (_str != NULL && _sub != NULL && strstr(_str, _sub) != NULL); \
    if (!success) fprintf(stderr, \
	"  > Expected %s (\"%s\") to contain %s (\"%s\") at %s:%d\n", \
	#str, _str, #sub, _sub, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Expect a string to not contain another string.
 *** 
 *** @param str The string to search.
 *** @param sub The string that must not appear within it.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_STR_LACKS(str, sub) \
    ({ \
    char* _str = (str); \
    char* _sub = (sub); \
    int success = (_str != NULL && _sub != NULL && strstr(_str, _sub) == NULL); \
    if (!success) fprintf(stderr, \
	"  > Expected %s (\"%s\") to not contain %s (\"%s\") at %s:%d\n", \
	#str, _str, #sub, _sub, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Find the first of the given texts that does not appear in the subject
 *** after every text listed before it.
 *** 
 *** @param subject The text to search.
 *** @param texts The texts to look for, in order, ending with a NULL.
 *** @returns The first text that is missing or out of order, or NULL if they
 ***	all appear in the order given.
 ***/
static inline char* strFindOutOfOrder(char* subject, char** texts)
    {
    char* pos = subject;
    char* found;
    int i;

	if (subject == NULL) return texts[0];
	for(i=0; texts[i]; i++)
	    {
	    found = strstr(pos, texts[i]);
	    if (found == NULL) return texts[i];
	    pos = found + strlen(texts[i]);
	    }

    return NULL;
    }

/*** Expect a string to contain each of the given strings, in the order given,
 *** making no assumptions about what surrounds or separates them.
 *** 
 *** @param str The string to search.
 *** @param ... The strings to find within it, in order.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_STR_HAS_IN_ORDER(str, ...) \
    ({ \
    char* _str = (str); \
    char* _missing = strFindOutOfOrder(_str, (char*[]){__VA_ARGS__, NULL}); \
    int success = (_missing == NULL); \
    if (!success) fprintf(stderr, \
	"  > Expected %s (\"%s\") to contain \"%s\" after the strings before it at %s:%d\n", \
	#str, _str, _missing, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Syntactic sugar to expect a pointer to be non null in a clearer, more
 *** concise way.
 *** 
 *** @param ptr The pointer.
 *** @returns true if successful, false otherwise.
 ***/
#define EXPECT_NOT_NULL(ptr) \
    ({ \
    __typeof__ (ptr) _ptr = (ptr); \
    int success = (_ptr != NULL); \
    if (!success) fprintf(stderr, \
	"  > Expected %s (%p) to be non null at %s:%d\n", \
	#ptr, _ptr, __FILE__, __LINE__ \
    ); \
    success; \
    })

/** Repeat the test as many times as possible within a set time window. **/
#define loopTest(test_fn) \
    ({ \
    long long result = 0ll; \
    Timer iter_timer_buf, *iter_timer = timerStart(timerInit(&iter_timer_buf)); \
    while (timerGet(iter_timer) < MIN_TEST_SECONDS) \
	{ \
	result++; \
	if (!test_fn()) \
	    { \
	    result = -1; \
	    break; \
	    } \
	} \
    timerDeInit(iter_timer); \
    result; \
    })

#endif
