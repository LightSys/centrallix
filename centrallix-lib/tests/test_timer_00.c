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
/* Module:	test_timer_00.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Test the util.h timer1 functionality.			*/
/************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/** Test dependencies. **/
#include "test_utils.h"

/** Tested module. **/
#include "timer.h"

/** The length of one wait, in microseconds and in seconds. **/
#define SLEEP_USEC 100000
#define SLEEP_SEC (SLEEP_USEC / 1000000.0)

/** Scheduler and Valgrind overhead allowed per wait, in seconds. **/
#define SLACK_SEC 0.1

/*** usleep() guarantees a minimum delay, not an exact one, so elapsed time is
 *** checked against a range rather than a single value.  After n waits the
 *** timer must read at least the time requested, plus at most n waits' slack.
 ***/
#define ELAPSED_MIN(n) ((n) * SLEEP_SEC)
#define ELAPSED_MAX(n) ((n) * (SLEEP_SEC + SLACK_SEC))

/** The most the two timers may disagree, having been started microseconds apart. **/
#define SKEW 0.005

/** A function for wasting cpu cycles. **/
static bool doNothing(void)
    {
    return true;
    }

long long test(char** tname)
    {
    *tname = "timer-00 Timer";
    
	/** Allocate a stack and a heap timer. **/
	Timer t;
	Timer *timer1 = timerInit(&t);
	pTimer timer2 = timerNew();
	
	/** 0.1 second wait. **/
	timerStart(timer1);
	timerStart(timer2);
	usleep(SLEEP_USEC);
	double t1_inter = timerGet(timer1);
	double t2_inter = timerGet(timer2);
	usleep(SLEEP_USEC);
	timerStop(timer1);
	timerStop(timer2);
	
	double t1_val = timerGet(timer1);
	double t2_val = timerGet(timer2);
	
	/** Check for incorrect values. **/
	if (!EXPECT_RANGE(t1_inter, ELAPSED_MIN(1), ELAPSED_MAX(1), "%g")) goto fail;
	if (!EXPECT_RANGE(t2_inter, ELAPSED_MIN(1), ELAPSED_MAX(1), "%g")) goto fail;
	if (!EXPECT_RANGE(fabs(t1_inter - t2_inter), 0.0, SKEW, "%g")) goto fail;
	if (!EXPECT_RANGE(t1_val, ELAPSED_MIN(2), ELAPSED_MAX(2), "%g")) goto fail;
	if (!EXPECT_RANGE(t2_val, ELAPSED_MIN(2), ELAPSED_MAX(2), "%g")) goto fail;
	if (!EXPECT_RANGE(fabs(t1_val - t2_val), 0.0, SKEW, "%g")) goto fail;
	
	/** Test that timer can resume properly. **/
	timerStart(timer1);
	timerStart(timer2);
	usleep(SLEEP_USEC);
	double t1_inter2 = timerGet(timer1);
	double t2_inter2 = timerGet(timer2);
	usleep(SLEEP_USEC);
	timerStop(timer1);
	timerStop(timer2);
	
	double t1_val2 = timerGet(timer1);
	double t2_val2 = timerGet(timer2);
	
	/** Check for incorrect values. **/
	if (!EXPECT_RANGE(t1_inter2, ELAPSED_MIN(3), ELAPSED_MAX(3), "%g")) goto fail;
	if (!EXPECT_RANGE(t2_inter2, ELAPSED_MIN(3), ELAPSED_MAX(3), "%g")) goto fail;
	if (!EXPECT_RANGE(fabs(t1_inter2 - t2_inter2), 0.0, SKEW, "%g")) goto fail;
	if (!EXPECT_RANGE(t1_val2, ELAPSED_MIN(4), ELAPSED_MAX(4), "%g")) goto fail;
	if (!EXPECT_RANGE(t2_val2, ELAPSED_MIN(4), ELAPSED_MAX(4), "%g")) goto fail;
	if (!EXPECT_RANGE(fabs(t1_val2 - t2_val2), 0.0, SKEW, "%g")) goto fail;
	
	/** Clean up. **/
	timerDeInit(timer1);
	timerFree(timer2);
	    
	/*** This test takes a lot of real time (calling usleep()) without
	 *** using very many CPU cycles.  This means we need to waste some
	 *** CPU cycles so that the test runner doesn't crash because the
	 *** CPU clock time was too low.
	 ***/
	long long i = loopTests(doNothing);
	
	/** Return success. **/
	return i;
	
	/** Return failure. **/
	fail:
	return -1;
    }
