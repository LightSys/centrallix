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

pTimer timer_init(pTimer timer);
pTimer timer_new(void);
pTimer timer_start(pTimer timer);
pTimer timer_stop(pTimer timer);
double timer_get(pTimer timer);
pTimer timer_reset(pTimer timer);
void timer_de_init(pTimer timer);
void timer_free(pTimer timer);

/*** Debug function for quickly benchmarking the speed of C code. Do not use
 *** this function in production code.
 ***/
#define timer_benchmark(timer, c_code) \
    { \
    pTimer _timer = (timer); \
    timer_start(_timer); \
    { c_code }; \
    timer_stop(_timer); \
    }

#endif /* TIMER_H */
