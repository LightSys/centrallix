#include "config.h"
#include "cxlib/mtask.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#ifdef HAVE_NCURSES
#include <curses.h>
#include <term.h>
#else
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW	3
#endif

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005-2026 LightSys Technology Services, Inc.	*/
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

/** Exit statuses, counted by the test target in Makefile.in. **/
#define T_EXIT_PASS	0
#define T_EXIT_FAIL	1
#define T_EXIT_SKIPPED	2
#define T_EXIT_CRASH	3
#define T_EXIT_LOCKUP	4

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
    #ifdef HAVE_NCURSES
    if (use_curses) {
        // Clear blue color from stderr
        tputs(tparm(tigetstr("sgr0")), 1, (int(*)(int))puterr);
    }
    #endif

    printf("%-62.62s  ", test_name);

    #ifdef HAVE_NCURSES
    if (use_curses) {
        tputs(tparm(tigetstr("setaf"), color), 1, putchar); //set stdout text color
    }
    #endif

    printf("%s\n", result);

    #ifdef HAVE_NCURSES
    if (use_curses) {
        tputs(tparm(tigetstr("sgr0")), 1, putchar); //clear stdout text attributes
    }
    #endif
    }

void
segv_handler(int v)
    {
    print_result("CRASH", COLOR_RED);
    exit(T_EXIT_CRASH);
    }
void
abort_handler(int v)
    {
    print_result("FAIL", COLOR_RED);
    exit(T_EXIT_FAIL);
    }
void
alarm_handler(int v)
    {
    print_result("LOCKUP", COLOR_RED);
    exit(T_EXIT_LOCKUP);
    }

void
start(void* v)
    {
    long long rval;

    signal(SIGSEGV, segv_handler);
    signal(SIGABRT, abort_handler);
    signal(SIGALRM, alarm_handler);
    alarm(4);

    #ifdef HAVE_NCURSES
    int result = setupterm(0, 1, 0);
    if (result != 0) {
        use_curses = false;
    }

    if (use_curses) {
        // Set any error output to be blue
        tputs(tparm(tigetstr("setaf"), COLOR_BLUE), 1, (int(*)(int))puterr);
    }
    #endif

    rval = test(&test_name);

    if (rval < 0)
        {
        print_result("FAIL", COLOR_RED);
        exit(T_EXIT_FAIL);
        }
    if (rval == 1)
        {
        print_result("Skipped", COLOR_YELLOW);
        exit(T_EXIT_SKIPPED);
        }
    print_result("Pass", COLOR_GREEN);
    exit(T_EXIT_PASS);
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }
