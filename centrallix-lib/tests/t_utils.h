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
/* Module:	t_utils.h						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Useful utils to improve the developer experience when   */
/* 		testing centrallix-lib.					*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "expect.h"
#include "range.h"
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
#define MIN_TEST_SECONDS 0.2

/** Assertion functions, defined in t_utils.c. **/
bool t_i_assertTrue(bool success, const char* expr, const char* file, int line);
bool t_i_assertFmt(bool success, const char* fmt, ...) __attribute__ ((format(printf, 2, 3)));
bool t_i_assertStrEql(const char* str1, const char* str2, const char* expr1, const char* expr2, const char* file, int line);
bool t_i_assertStrEqlN(const char* str1, const char* str2, size_t len, const char* expr1, const char* expr2, const char* file, int line);
bool t_i_assertNotNull(const void* ptr, const char* expr, const char* file, int line);

/*** Assert a value to be true.
 *** 
 *** @param v The value.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_TRUE(v) \
    t_i_assertTrue(!!(v), #v, __FILE__, __LINE__)

/*** Assert that a value is NULL. While ASSERT_EQL(ptr, NULL) also does this,
 *** ASSERT_NOT_NULL() is clearer to read and gives a distinct error message.
 *** 
 *** @param ptr The pointer.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_NOT_NULL(ptr) \
    t_i_assertNotNull((ptr), #ptr, __FILE__, __LINE__)

/*** Assert two values to be equal.
 *** 
 *** @param v1 The first value.
 *** @param v2 The second value.
 *** @param sp The specifier to print the values if there is an error. This
 ***    MUST be a string known to the compiler at compile time, such as a
 ***    string literal or a macro that expands to one.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_EQL(v1, v2, sp) \
    ({ \
    __typeof__ (v1) _v1 = (v1); \
    __typeof__ (v2) _v2 = (v2); \
    t_i_assertFmt((_v1 == _v2), \
	"  > Expected %s ("sp") to equal %s ("sp") at %s:%d\n", \
	#v1, _v1, #v2, _v2, __FILE__, __LINE__ \
    ); \
    })

/*** Assert two strings to be equal.
 *** 
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_STR_EQL(str1, str2) \
    t_i_assertStrEql((str1), (str2), #str1, #str2, __FILE__, __LINE__)

/*** Assert two strings to be equal, including null characters.
 *** Warning: Can read off the end of strings.
 *** 
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @param len The length of strings to check (aka. N).
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_STR_EQL_N(str1, str2, len) \
    t_i_assertStrEqlN((str1), (str2), (size_t)(len), #str1, #str2, __FILE__, __LINE__)

/*** Assert a value to fall within a range.
 *** 
 *** @param v The value.
 *** @param min_v The minimum acceptable value.
 *** @param max_v The maximum acceptable value.
 *** @param sp The specifier to print the values if there is an error. This
 ***    MUST be a string known to the compiler at compile time, such as a
 ***    string literal or a macro that expands to one.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_RANGE(v, min_v, max_v, sp) \
    ({ \
    __typeof__ (v) _v = (v); \
    __typeof__ (min_v) _min = (min_v); \
    __typeof__ (max_v) _max = (max_v); \
    t_i_assertFmt((_min <= _v && _v <= _max), \
	"  > Expected %s ("sp") to be in the range %s ("sp") - %s ("sp") at %s:%d\n", \
	#v, _v, #min_v, _min, #max_v, _max, __FILE__, __LINE__ \
    ); \
    success; \
    })

/*** Assert a string to contain another string.
 *** 
 *** @param str The string to search.
 *** @param sub The string to find within it.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_STR_HAS(str, sub) \
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

/*** Assert a string to not contain another string.
 *** 
 *** @param str The string to search.
 *** @param sub The string that must not appear within it.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_STR_LACKS(str, sub) \
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

/*** Assert a string to contain each of the given strings, in the order given,
 *** making no assumptions about what surrounds or separates them.
 *** 
 *** @param str The string to search.
 *** @param ... The strings to find within it, in order.
 *** @returns true if successful, false otherwise.
 ***/
#define ASSERT_STR_HAS_IN_ORDER(str, ...) \
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

/*** Copy n characters from src to dest, including null-terminators, which are
 *** replaced with the '_' character.
 *** 
 *** @param dest The destination string buffer (must be at least `n+1` bytes).
 *** @param src The source string (must be at least `n` bytes), which can
 *** 	include null-terminator characters ('\0').
 *** @param n The number of characters to copy.
 ***/
void copyReplaceNulls(char* dest, const char* src, size_t n);

/** Repeat the test as many times as possible within a set time window. **/
long long loopTest(bool (*test_fn)(void));

#endif
