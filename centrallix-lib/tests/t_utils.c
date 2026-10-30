/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	t_utils.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	October 16th, 2026					*/
/* Description:	Implementations of the functions in t_utils.h.		*/
/************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "expect.h"
#include "range.h"
#include "t_utils.h"

/*** Implement ASSERT_TRUE().
 ***
 *** @param success Whether the value was true.
 *** @param expr The source text of the value.
 *** @param file The file of the assertion.
 *** @param line The line of the assertion.
 *** @returns success.
 ***/
bool
t_i_assertTrue(bool success, const char* expr, const char* file, int line)
    {
	if (UNLIKELY(!success))
	    fprintf(stderr,
		"  > Expected %s to be true at %s:%d\n",
		expr, file, line
	    );

    return success;
    }

/*** Implement ASSERT_EQL() and ASSERT_RANGE(), which compare in the macro
 *** to keep the types of their values.
 ***
 *** @param success Whether the comparison passed.
 *** @param fmt The message to print on failure.
 *** @param ... The arguments for fmt.
 *** @returns success.
 ***/
bool
t_i_assertFmt(bool success, const char* fmt, ...)
    {
    va_list args;

	if (UNLIKELY(!success))
	    {
	    va_start(args, fmt);
	    vfprintf(stderr, fmt, args);
	    va_end(args);
	    }

    return success;
    }

/*** Implement ASSERT_STR_EQL().
 ***
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @param expr1 The source text of the first string.
 *** @param expr2 The source text of the second string.
 *** @param file The file of the assertion.
 *** @param line The line of the assertion.
 *** @returns true if successful, false otherwise.
 ***/
bool
t_i_assertStrEql(const char* str1, const char* str2, const char* expr1, const char* expr2, const char* file, int line)
    {
    const bool both_null = (str1 == NULL && str2 == NULL);
    const bool both_eql = (str1 != NULL && str2 != NULL && strcmp(str1, str2) == 0);
    const bool success = (both_null || both_eql);

	if (UNLIKELY(!success))
	    fprintf(stderr,
		"  > Expected %s (\"%s\") to equal %s (\"%s\") at %s:%d\n",
		expr1, str1, expr2, str2, file, line
	    );

    return success;
    }

/*** Implement ASSERT_STR_EQL_N().
 ***
 *** @param str1 The first string.
 *** @param str2 The second string.
 *** @param len The length of strings to check (aka. N).
 *** @param expr1 The source text of the first string.
 *** @param expr2 The source text of the second string.
 *** @param file The file of the assertion.
 *** @param line The line of the assertion.
 *** @returns true if successful, false otherwise.
 ***/
bool
t_i_assertStrEqlN(const char* str1, const char* str2, size_t len, const char* expr1, const char* expr2, const char* file, int line)
    {
    const bool both_null = (str1 == NULL && str2 == NULL);
    const bool both_eql = (str1 != NULL && str2 != NULL && memcmp(str1, str2, len) == 0);
    const bool success = (both_null || both_eql);

	if (UNLIKELY(!success))
	    {
	    const size_t buf_len = max(7, len + 1);
	    char tmp1[buf_len];
	    if (str1 == NULL) strcpy(tmp1, "(null)");
	    else copyReplaceNulls(tmp1, str1, len);

	    char tmp2[buf_len];
	    if (str2 == NULL) strcpy(tmp2, "(null)");
	    else copyReplaceNulls(tmp2, str2, len);

	    fprintf(stderr,
		"  > Expected %s (\"%s\") to equal %s (\"%s\") where '_' is a null-terminator, at %s:%d\n",
		expr1, tmp1, expr2, tmp2, file, line
	    );
	    }

    return success;
    }

/*** Implement ASSERT_NOT_NULL().
 ***
 *** @param ptr The pointer.
 *** @param expr The source text of the pointer.
 *** @param file The file of the assertion.
 *** @param line The line of the assertion.
 *** @returns true if successful, false otherwise.
 ***/
bool
t_i_assertNotNull(const void* ptr, const char* expr, const char* file, int line)
    {
    const bool success = (ptr != NULL);

	if (UNLIKELY(!success))
	    fprintf(stderr,
		"  > Expected %s (%p) to be non null at %s:%d\n",
		expr, ptr, file, line
	    );

    return success;
    }

void
copyReplaceNulls(char* dest, const char* src, size_t n)
    {
	for (size_t i = 0lu; i < n; i++)
	    dest[i] = (src[i] == '\0') ? '_' : src[i];
	dest[n] = '\0';

    return;
    }

long long
loopTest(bool (*test_fn)(void))
    {
    long long result = 0ll;
    Timer iter_timer_buf;
    Timer *iter_timer = timerStart(timerInit(&iter_timer_buf));

	while (timerGet(iter_timer) < MIN_TEST_SECONDS)
	    {
	    result++;
	    if (!test_fn())
		{
		result = -1;
		break;
		}
	    }
	timerDeInit(iter_timer);

    return result;
    }
