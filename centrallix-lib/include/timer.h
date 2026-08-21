#ifndef TIMER_H
#define TIMER_H

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

typedef struct
    {
    double start, total;
    }
    Timer, *pTimer;

pTimer timerInit(pTimer timer);
pTimer timerNew(void);
pTimer timerStart(pTimer timer);
pTimer timerStop(pTimer timer);
double timerGet(pTimer timer);
pTimer timerReset(pTimer timer);
void timerDeInit(pTimer timer);
void timerFree(pTimer timer);

/*** Debug function for quickly benchmarking the speed of C code.  Do not use
 *** this function in production code:  It breaks compiler and mssError() line
 *** numbers and is generally bad style.
 ***/
#define timerBenchmark(timer, c_code) \
    { \
    pTimer _timer = (timer); \
    timerStart(_timer); \
    { c_code }; \
    timerStop(_timer); \
    }

#endif /* TIMER_H */
