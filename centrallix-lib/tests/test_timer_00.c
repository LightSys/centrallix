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
#include "range.h"

/** Tested module. **/
#include "timer.h"

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
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	double t1_inter = roundTo(timerGet(timer1), 3);
	double t2_inter = roundTo(timerGet(timer2), 3);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	timerStop(timer1);
	timerStop(timer2);
	
	/** Extract values with rounding to give margin for error. **/
	double t1_val = roundTo(timerGet(timer1), 3);
	double t2_val = roundTo(timerGet(timer2), 3);
	
	/** Check for incorrect values. **/
	if (!EXPECT_EQL(t1_inter, 0.1, "%g")) goto fail;
	if (!EXPECT_EQL(t2_inter, 0.1, "%g")) goto fail;
	if (!EXPECT_EQL(t1_inter, t2_inter, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val, 0.2, "%g")) goto fail;
	if (!EXPECT_EQL(t2_val, 0.2, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val, t2_val, "%g")) goto fail;
	
	/** Test that timer can resume properly. **/
	timerStart(timer1);
	timerStart(timer2);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	double t1_inter2 = roundTo(timerGet(timer1), 3);
	double t2_inter2 = roundTo(timerGet(timer2), 3);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	timerStop(timer1);
	timerStop(timer2);
	
	/** Extract values with rounding to give margin for error. **/
	double t1_val2 = roundTo(timerGet(timer1), 3);
	double t2_val2 = roundTo(timerGet(timer2), 3);
	
	/** Check for incorrect values. **/
	if (!EXPECT_EQL(t1_inter2, 0.3, "%g")) goto fail;
	if (!EXPECT_EQL(t2_inter2, 0.3, "%g")) goto fail;
	if (!EXPECT_EQL(t1_inter2, t2_inter2, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val2, 0.4, "%g")) goto fail;
	if (!EXPECT_EQL(t2_val2, 0.4, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val2, t2_val2, "%g")) goto fail;
	
	/** Clean up. **/
	timerDeInit(timer1);
	timerFree(timer2);
	    
	/*** This test takes a lot of real time (calling usleep()) without
	 *** using very many CPU cycles.  This means we need to waste some
	 *** CPU cycles so that the test runner doesn't crash because the
	 *** CPU clock time was too low.
	 ***/
	long long i = loop_tests(doNothing);
	
	/** Return success. **/
	return i;
	
	/** Return failure. **/
	fail:
	return -1;
    }
