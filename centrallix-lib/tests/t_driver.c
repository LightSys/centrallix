#include "cxlibconfig-internal.h"
#include "mtask.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/times.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	test_00baseline.c     					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 11th, 2005 					*/
/* Description: Test suite entry to generate a baseline comparison value*/
/************************************************************************/


long long test(char**);

char * tname = "?";

void
segv_handler(int v)
    {
    printf("%-62.62s  CRASH\n", tname);
    exit(0);
    }
void
abort_handler(int v)
    {
    printf("%-62.62s  ABORT\n", tname);
    exit(0);
    }
void
alarm_handler(int v)
    {
    printf("%-62.62s  LOCKUP\n", tname);
    exit(0);
    }

void
start(void* v)
    {
    struct tms t;
    clock_t start,end;
    long long rval;

	signal(SIGSEGV, segv_handler);
	signal(SIGABRT, abort_handler);
	signal(SIGALRM, alarm_handler);
	
	/*** Set a timer before Lockup is triggered, using a significantly
	 *** larger value if Valgrind appears to be enabled.
	 ***/
	#ifndef NM_USE_SYSMALLOC
	alarm(90); /* Valgrind detected. */
	#else
	alarm(5); /* Normal timeout. */
	#endif
	
	
	times(&t);
	start = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	rval = test(&tname);
	times(&t);
	end = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	
	if (rval < 0)
	    printf("%-62.62s  FAIL\n", tname);
	else
	    {
	    long long duration = end - start;
	    if (duration == 0)
		{
		printf("%-62.62s  PASS ???\n", tname);
		printf("Warning: Test ran too fast! Ops/sec could not be measured. Please run tests in a loop or use loop_tests() from test_utils.h.\n");
		return;
		}
	    long long ops_per_second = rval * (100 / duration);
	    if (ops_per_second > 0) printf("%-62.62s  PASS %lld\n", tname, ops_per_second);
	    else printf("%-62.62s  PASS %.4lf\n", tname, rval * (100.0 / duration));
	    }

    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }
