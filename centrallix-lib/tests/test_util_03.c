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
/* Module:	test_util_03.c						*/
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
#include "util.h"

static bool do_nothing(void)
    {
    return true;
    }

long long test(char** tname)
    {
    *tname = "util-03 Timer";
    
	/** Allocate a stack and a heap timer. **/
	Timer t;
	Timer *timer1 = timer_init(&t);
	pTimer timer2 = timer_new();
	
	/** 0.1 second wait. **/
	timer_start(timer1);
	timer_start(timer2);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	double t1_inter = round_to(timer_get(timer1), 3);
	double t2_inter = round_to(timer_get(timer2), 3);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	timer_stop(timer1);
	timer_stop(timer2);
	
	/** Extract values with rounding to give margin for error. **/
	double t1_val = round_to(timer_get(timer1), 3);
	double t2_val = round_to(timer_get(timer2), 3);
	
	/** Check for incorrect values. **/
	if (!EXPECT_EQL(t1_inter, 0.1, "%g")) goto fail;
	if (!EXPECT_EQL(t2_inter, 0.1, "%g")) goto fail;
	if (!EXPECT_EQL(t1_inter, t2_inter, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val, 0.2, "%g")) goto fail;
	if (!EXPECT_EQL(t2_val, 0.2, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val, t2_val, "%g")) goto fail;
	
	/** Test that timer can resume properly. **/
	timer_start(timer1);
	timer_start(timer2);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	double t1_inter2 = round_to(timer_get(timer1), 3);
	double t2_inter2 = round_to(timer_get(timer2), 3);
	usleep(99900); /* 0.0999 seconds (leave room for overhead). */
	timer_stop(timer1);
	timer_stop(timer2);
	
	/** Extract values with rounding to give margin for error. **/
	double t1_val2 = round_to(timer_get(timer1), 3);
	double t2_val2 = round_to(timer_get(timer2), 3);
	
	/** Check for incorrect values. **/
	if (!EXPECT_EQL(t1_inter2, 0.3, "%g")) goto fail;
	if (!EXPECT_EQL(t2_inter2, 0.3, "%g")) goto fail;
	if (!EXPECT_EQL(t1_inter2, t2_inter2, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val2, 0.4, "%g")) goto fail;
	if (!EXPECT_EQL(t2_val2, 0.4, "%g")) goto fail;
	if (!EXPECT_EQL(t1_val2, t2_val2, "%g")) goto fail;
	
	/** Clean up. **/
	timer_de_init(timer1);
	timer_free(timer2);
	    
	/*** This test takes a lot of real time (calling usleep()) without
	 *** using very many CPU cycles.  This means we need to waste some
	 *** CPU cycles so that the test runner doesn't crash because the
	 *** CPU clock time was too low.
	 ***/
	long long i = loop_tests(do_nothing);
	
	/** Return success. **/
	return i;
	
	/** Return failure. **/
	fail:
	return -1;
    }
