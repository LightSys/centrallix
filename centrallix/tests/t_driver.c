#include "cxlib/mtask.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <curses.h>
#include <term.h>

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
/* Module: 	t_driver.c     					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	3 Aug 2020 					*/
/* Description: Driver for straight C Centrallix tests. To use it, write
    a C file implementing the "test" function (asserting your test
    conditions or returning a nonzero value for failures), then link in
    t_driver.o. */
/************************************************************************/

long long test(char**);

char * test_name = "?";
bool use_curses = true;

int
puterr(char character)
{
    return putc(character, stderr);
}

void
print_result(char * result, int color)
    {
    if (use_curses) {
        // Clear blue color from stderr
        tputs(tparm(tigetstr("sgr0")), 1, puterr);
    }
    printf("%-62.62s  ", test_name);
    if (use_curses) {
        tputs(tparm(tigetstr("setaf"), color), 1, putchar); //set stdout text color
        tputs(tparm(tigetstr("bold")), 1, putchar); //set stdout text to bold
    }
    printf("%s\n", result);
    if (use_curses) {
        tputs(tparm(tigetstr("sgr0")), 1, putchar); //clear stdout text attributes
    }
    }

void
segv_handler(int v)
    {
    print_result("CRASH", COLOR_RED);
    exit(0);
    }
void
abort_handler(int v)
    {
    print_result("FAIL", COLOR_RED);
    exit(0);
    }
void
alarm_handler(int v)
    {
    print_result("LOCKUP", COLOR_RED);
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

    int result = setupterm(0, 1, 0);
    if (result != 0) {
        use_curses = false;
    }
    
    if (use_curses) {
        // Set any error output to be blue
        tputs(tparm(tigetstr("setaf"), COLOR_BLUE), 1, puterr);
    }

    rval = test(&test_name);

    if (rval < 0)
        print_result("FAIL", COLOR_RED);
    else
        print_result("Pass", COLOR_GREEN);

    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }

