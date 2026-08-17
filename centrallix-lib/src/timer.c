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

#include "check.h"
#include "expect.h"
#include "newmalloc.h"

#include "timer.h"

/*** Get the current monotonic time in seconds.
 *** 
 *** @returns The current monotonic time as a fractional number of seconds.
 ***/
static double
getTime(void)
    {
    struct timespec ts;
    
	if (check(clock_gettime(CLOCK_MONOTONIC, &ts)) != 0)
	    return NAN;
    
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9f;
    }

/*** Initialize a timer struct.  The initial timer is not yet started and has
 *** no total time saved.
 *** 
 *** @param timer The timer to initialize.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timerInit(pTimer timer)
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
timerNew(void)
    {
    return timerInit(checkPtr(nmMalloc(sizeof(Timer))));
    }

/*** Start timing.  If the timer was already timing, does nothing.
 *** 
 *** @param timer The timer to start.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timerStart(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NULL;
	if (isnan(timer->start)) timer->start = getTime();
    
    return timer;
    }

/*** Stop timing and add the elapsed time to the timer total.  If the timer
 *** isn't running, does nothing.
 *** 
 *** @param timer The timer to stop.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timerStop(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NULL;
	if (isnan(timer->start)) return timer;
	timer->total += getTime() - timer->start;
	timer->start = NAN;
    
    return timer;
    }

/*** Get the total time that elapsed while the timer was running.
 *** 
 *** @param timer The timer to read.
 *** @returns The total time in seconds, or NAN if `timer` is NULL.
 ***/
double
timerGet(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return NAN;

	const double current_time = (isnan(timer->start)) ? 0.0 : (getTime() - timer->start);
    
    return current_time + timer->total;
    }

/*** Reset a timer to its initial state, where it isn't started and no time
 *** has elapsed yet.
 *** 
 *** @param timer The timer to reset.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timerReset(pTimer timer)
    {
    return timerInit(timer);
    }

/*** De-initialize a timer allocated by timerInit().
 *** 
 *** @param timer The timer to de-initialize.
 ***/
void
timerDeInit(pTimer timer) {}

/*** De-initialize and free a timer allocated by timerNew().
 *** 
 *** @param timer The timer to free.
 ***/
void
timerFree(pTimer timer)
    {
	if (UNLIKELY(timer == NULL)) return;
	
	timerDeInit(timer);
	nmFree(timer, sizeof(Timer));
    
    return;
    }
