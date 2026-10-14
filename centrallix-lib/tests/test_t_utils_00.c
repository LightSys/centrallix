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
/* Module:	test_t_utils_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	October 14th, 2026					*/
/* Description:	Test the assertion macros in t_utils.h.  These tests	*/
/*		are a little fragile, but I don't expect error messages	*/
/*		to change often and devs running tests should probably	*/
/*		know if they do.					*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Tested module. **/
#include "t_utils.h"

#define MSG_SIZE	512
#define COPY_SIZE	8

/** What the last captured assertion printed, and the line it ran on. **/
static char msg[MSG_SIZE];
static int msg_line;

/*** Run an assertion with stderr redirected into msg.  Keep each use on one
 *** line so that msg_line matches the line the assertion reports.
 ***
 *** @param result Set to what the assertion returned.
 *** @param assertion The assertion to run.
 ***/
#define CAPTURE(result, assertion) \
    { \
    FILE* _saved = stderr; \
    msg[0] = '\0'; \
    FILE* _stream = fmemopen(msg, sizeof(msg), "w"); \
    if (!ASSERT_NOT_NULL(_stream)) return false; \
    stderr = _stream; \
    result = (assertion); \
    fclose(stderr); \
    stderr = _saved; \
    msg_line = __LINE__; \
    }

/*** Check the last captured message.
 ***
 *** @param body The message between "Expected " and " at file:line".
 *** @returns true if the message matches, false otherwise.
 ***/
static bool expectMsg(const char* body)
    {
    char expected[MSG_SIZE];

	snprintf(expected, sizeof(expected), "  > Expected %s at %s:%d\n", body, __FILE__, msg_line);

    return ASSERT_STR_EQL(msg, expected);
    }

static bool doTest(void)
    {
    bool success = true;
    bool result;
    int zero = 0, one = 1, two = 2, three = 3;
    char* null_str = NULL;
    char* ab = "ab";
    char ab_copy[] = "ab";
    char* ac = "ac";
    char ab_c[] = {'a', 'b', '\0', 'c'};
    char ab_d[] = {'a', 'b', '\0', 'd'};
    char copy[COPY_SIZE];

	/*** ASSERT_TRUE() can't vouch for itself, so its results are
	 *** checked directly.  The checks below rely on it.
	 ***/
	CAPTURE(result, ASSERT_TRUE(one));
	success &= (result == true);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_TRUE(zero));
	success &= (result == false);
	success &= expectMsg("zero to be true");

	/** ASSERT_EQL() **/
	CAPTURE(result, ASSERT_EQL(two, two, "%d"));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_EQL(two, three, "%d"));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("two (2) to equal three (3)");

	/** ASSERT_STR_EQL() compares contents, not pointers. **/
	CAPTURE(result, ASSERT_STR_EQL(ab, ab_copy));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_STR_EQL(null_str, null_str));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_STR_EQL(ab, ac));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("ab (\"ab\") to equal ac (\"ac\")");

	/** ASSERT_STR_EQL() fails when only one string is null. **/
	CAPTURE(result, ASSERT_STR_EQL(null_str, ab));
	success &= ASSERT_TRUE(!result);
	success &= ASSERT_TRUE(msg[0] != '\0');
	CAPTURE(result, ASSERT_STR_EQL(ab, null_str));
	success &= ASSERT_TRUE(!result);
	success &= ASSERT_TRUE(msg[0] != '\0');

	/** ASSERT_STR_EQL_N() compares exactly len bytes, nulls included. **/
	CAPTURE(result, ASSERT_STR_EQL_N(ab_c, ab_d, 3));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_STR_EQL_N(null_str, null_str, 4));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_STR_EQL_N(ab_c, ab_d, 4));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("ab_c (\"ab_c\") to equal ab_d (\"ab_d\") where '_' is a null-terminator,");
	CAPTURE(result, ASSERT_STR_EQL_N(null_str, ab, 2));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("null_str (\"(null)\") to equal ab (\"ab\") where '_' is a null-terminator,");
	CAPTURE(result, ASSERT_STR_EQL_N(ab, null_str, 2));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("ab (\"ab\") to equal null_str (\"(null)\") where '_' is a null-terminator,");

	/** ASSERT_RANGE() includes both ends. **/
	CAPTURE(result, ASSERT_RANGE(one, one, three, "%d"));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_RANGE(three, one, three, "%d"));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_RANGE(zero, one, three, "%d"));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("zero (0) to be in the range one (1) - three (3)");
	CAPTURE(result, ASSERT_RANGE(three, zero, two, "%d"));
	success &= ASSERT_TRUE(!result);
	success &= expectMsg("three (3) to be in the range zero (0) - two (2)");

	/** ASSERT_NOT_NULL() **/
	CAPTURE(result, ASSERT_NOT_NULL(ab));
	success &= ASSERT_TRUE(result);
	success &= ASSERT_STR_EQL(msg, "");
	CAPTURE(result, ASSERT_NOT_NULL(null_str));
	success &= ASSERT_TRUE(!result);
	success &= ASSERT_TRUE(msg[0] != '\0');

	/** STR_COPY_REPLACE_NULLS() writes n bytes and a terminator. **/
	memset(copy, 'x', sizeof(copy));
	STR_COPY_REPLACE_NULLS(copy, ab_c, 4);
	success &= ASSERT_STR_EQL_N(copy, "ab_c\0xxx", COPY_SIZE);
	memset(copy, 'x', sizeof(copy));
	STR_COPY_REPLACE_NULLS(copy, ab_c, 0);
	success &= ASSERT_STR_EQL_N(copy, "\0xxxxxxx", COPY_SIZE);

    return success;
    }

long long test(char** tname)
    {
    *tname = "t_utils-00 Assertions";
    return loopTest(doTest) * 22ll;
    }

/** Scope cleanup. **/
#undef MSG_SIZE
#undef COPY_SIZE
#undef CAPTURE
