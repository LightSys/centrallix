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

/**CVSDATA***************************************************************

    $Id: t_driver.c,v 1.4 2010/05/12 18:21:21 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/tests/t_driver.c,v $

    $Log: t_driver.c,v $
    Revision 1.4  2010/05/12 18:21:21  gbeeley
    - (rewrite) This is a mostly-rewrite of the mtlexer module for correctness
      and for security.  Adding many test suite items for mtlexer, a good
      fraction of which fail on the old mtlexer module.  The new module is
      currently mildly slower than the old one, but is more correct.

    Revision 1.3  2006/06/21 21:17:02  gbeeley
    - Updating timings on smmalloc tests to speed things up a bit.
    - Updating test driver to detect crashes, lockups, and assertion failures.

    Revision 1.2  2005/10/08 23:59:57  gbeeley
    - (change) modify timing in test driver to reflect actual cpu and child
      process cpu utilization

    Revision 1.1  2005/03/14 20:41:25  gbeeley
    - changed configuration to allow different levels of hardening (mainly, so
      asserts can be enabled without enabling the ds checksum stuff).
    - initial working version of the smmalloc (shared memory malloc) module.
    - test suite for smmalloc "make test".
    - results from test suite run on 1.4GHz Athlon, GCC 2.96, RH73
    - smmalloc not actually tested between two processes yet.
    - TO-FIX: smmalloc interprocess locking needs to be reworked to prefer
      spinlocks where doable instead of using sysv semaphores which are SLOW.

 **END-CVSDATA***********************************************************/

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
	alarm(10);
	times(&t);
	start = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	rval = test(&tname);
	times(&t);
	end = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
	if (rval < 0)
	    printf("%-62.62s  FAIL\n", tname);
	else{
            if(!(end-start))printf("%-62.62s finished too fast to time\n",tname);
	    printf("%-62.62s  PASS %lld\n", tname, rval*100/(long long)((end-start)?(end-start):1));
        }
    return;
    }

int
main(int argc, char* argv[])
    {
    mtInitialize(0, start);
    return 0;
    }

