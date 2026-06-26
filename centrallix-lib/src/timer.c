/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      timer.c, timer.h                                        */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: A simple timer utility, intended for benchmarking code  */
/*              performance in wall time.                               */
/************************************************************************/

#include <math.h>
#include <time.h>

#include "util.h"
#include "expect.h"
#include "newmalloc.h"
#include "timer.h"

/*** Get the current monotonic time in seconds.
 ***
 *** @returns The current monotonic time as a fractional number of seconds.
 ***/
static double
get_time(void)
    {
    struct timespec ts;
    
	clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9f;
    }

/*** Initialize a timer struct.
 ***
 *** @param timer The timer to initialize.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_init(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NULL;
	timer->start = NAN;
	timer->total = 0.0;
    
    return timer;
    }

/*** Allocate and initialize a new timer.
 ***
 *** @returns A newly allocated timer, or NULL if allocation fails.
 ***/
pTimer
timer_new(void)
    {
    return timer_init(check_ptr(nmMalloc(sizeof(Timer))));
    }

/*** Start timing.
 ***
 *** @param timer The timer to start.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_start(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NULL;
	timer->start = get_time();
    
    return timer;
    }

/*** Stop timing and add the elapsed time to the timer total.
 ***
 *** @param timer The timer to stop.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_stop(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NULL;
	timer->total += get_time() - timer->start;
    
    return timer;
    }

/*** Get the total accumulated time for a timer.
 ***
 *** @param timer The timer to read.
 *** @returns The total accumulated time in seconds, or NAN if `timer`
 *** 	is NULL.
 ***/
double
timer_get(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NAN;
    
    return timer->total;
    }

/*** Reset a timer to its initial state so that it can be reused to time
 *** something else.
 ***
 *** @param timer The timer to reset.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_reset(pTimer timer)
    {
    return timer_init(timer);
    }

/*** De-initialize a timer before it is freed.
 ***
 *** @param timer The timer to de-initialize.
 ***/
void
timer_de_init(pTimer timer) {}

/*** De-initialize and free a timer allocated by timer_new().
 ***
 *** @param timer The timer to free.
 ***/
void
timer_free(pTimer timer)
    {
	timer_de_init(timer);
	nmFree(timer, sizeof(Timer));
    
    return;
    }
