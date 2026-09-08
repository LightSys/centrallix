/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005-2026 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	test_00baseline.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 11th, 2005					*/
/* Description: Test suite driver for centrallix-lib tests.		*/
/************************************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <unistd.h>

#include "cxlibconfig-internal.h"
#include "mtask.h"
#include "util.h"

/*** Define lockup times.  Valgrind instruments every memory access, so tests
 *** may need longer to finish when running under valgrind.
 ***/
#define NORMAL_LOCKUP_SECONDS 5u
#define VALGRIND_LOCKUP_SECONDS 10u

/** Detect valgrind. **/
#ifdef USING_VALGRIND
#include "valgrind/valgrind.h"
#define LOCKUP_SECONDS	((RUNNING_ON_VALGRIND) ? VALGRIND_LOCKUP_SECONDS : NORMAL_LOCKUP_SECONDS)
#else
#define LOCKUP_SECONDS	NORMAL_LOCKUP_SECONDS
#endif


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

	/** Setup handlers for signals that may occur during a test. **/
	signal(SIGSEGV, segv_handler);
	signal(SIGABRT, abort_handler);
	signal(SIGALRM, alarm_handler);
	alarm(LOCKUP_SECONDS);

	/** Run the test while tracking CPU time. **/
	times(&t);
	start = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	rval = test(&tname);
	times(&t);
	end = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;

	/** Print test results. **/
	if (rval < 0)
	    printf("%-62.62s  FAIL\n", tname);
	else
	    {
	    long long duration = end - start;
	    if (duration == 0)
		{
		printf("%-62.62s  PASS ???\n", tname);
		printf("Warning: Test ran too fast! Ops/sec could not be measured. Please run tests in a loop or use loopTests() from test_utils.h.\n");
		return;
		}
	    double ops_per_second = rval * (100.0 / duration);

	    /** Round to four significant figures. **/
	    int precision = 3;
	    unsigned long long factor = 1;
	    double scaled = ops_per_second;
	    while (scaled >= 10.0)
		{
		scaled /= 10.0;
		if (precision > 0)
		    precision--;
		else
		    factor *= 10;
		}
	    while (scaled > 0.0 && scaled < 1.0)
		{
		scaled *= 10.0;
		precision++;
		}

	    /** Print whole numbers with commas. **/
	    if (precision == 0)
		{
		char buf[32];
		unsigned long long rounded = (unsigned long long)(ops_per_second / factor + 0.5) * factor;
		printf("%-62.62s  PASS %s\n", tname, snprintCommasLlu(buf, sizeof(buf), rounded));
		}
	    else
		printf("%-62.62s  PASS %.*f\n", tname, precision, ops_per_second);
	    }

    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }
