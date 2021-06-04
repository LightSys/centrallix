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
	//alarm(10);
	times(&t);
	start = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	rval = test(&tname);
	times(&t);
	end = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	if (rval < 0)
	    printf("%-62.62s  FAIL\n", tname);
	else
	    printf("%-62.62s  PASS %lld\n", tname, rval*100/(long long)(end - start));

    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }

