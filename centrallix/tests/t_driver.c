#include "cxlib/mtask.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

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


#define RED    "\033[1m\x1B[31m"
#define GREEN  "\033[1m\x1B[32m"
#define RESET  "\x1B[0m"

long long test(char**);

char * test_name = "?";

void
segv_handler(int v)
    {
    printf(RESET "%-62.62s  " RED "CRASH\n" RESET, test_name);
    exit(0);
    }
void
abort_handler(int v)
    {
    printf(RESET "%-62.62s  " RED "ABORT\n" RESET, test_name);
    exit(0);
    }
void
alarm_handler(int v)
    {
    printf(RESET "%-62.62s  " RED "LOCKUP\n" RESET, test_name);
    exit(0);
    }

void
start(void* v)
    {
    long long rval;

    signal(SIGSEGV, segv_handler);
    signal(SIGABRT, abort_handler);
    signal(SIGALRM, alarm_handler);
    alarm(4);
    
    rval = test(&test_name);
    
    if (rval < 0)
        printf(RESET "%-62.62s  " RED "FAIL\n" RESET, test_name);
    else
        printf(RESET "%-62.62s  " GREEN "Pass\n" RESET, test_name);

    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }
