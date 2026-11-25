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
/* Module:	test_t_utils_01.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	October 14th, 2026					*/
/* Description:	Test loopTest() from t_utils.h.			*/
/************************************************************************/

#include <stdbool.h>

/** Test dependencies. **/
#include "timer.h"

/** Tested module. **/
#include "t_utils.h"

/** Calls to countCalls(), and the call that fails (0 for none). **/
static long long calls;
static long long fail_at;

static bool countCalls(void)
    {
    calls++;
    return (calls != fail_at);
    }

long long test(char** tname)
    {
    bool success = true;
    long long iterations;
    Timer timer_buf, *timer;

	*tname = "t_utils-01 loopTest()";

	/** A passing test repeats until the time window has passed. **/
	calls = 0ll;
	fail_at = 0ll;
	timer = timerStart(timerInit(&timer_buf));
	iterations = loopTest(countCalls);
	success &= ASSERT_TRUE(timerGet(timer) >= MIN_TEST_SECONDS);
	timerDeInit(timer);
	success &= ASSERT_TRUE(iterations > 0ll);
	success &= ASSERT_EQL(iterations, calls, "%lld");

	/** A failing test stops at the first failure. **/
	calls = 0ll;
	fail_at = 1ll;
	success &= ASSERT_EQL(loopTest(countCalls), -1ll, "%lld");
	success &= ASSERT_EQL(calls, 1ll, "%lld");
	calls = 0ll;
	fail_at = 3ll;
	success &= ASSERT_EQL(loopTest(countCalls), -1ll, "%lld");
	success &= ASSERT_EQL(calls, 3ll, "%lld");

    return (success) ? iterations : -1ll;
    }
