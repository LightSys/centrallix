/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_newmalloc_02.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test the nmStats() function from the NewMalloc library.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"

/** Tested module. **/
#include "newmalloc.h"

#define BLOCK_COUNT	64lu
#define BLOCK_SIZE	128

static bool doTests(void)
    {
    bool success = true;

	/** Give the allocator some activity to report on. **/
	void* blocks[BLOCK_COUNT];
	for (size_t i = 0lu; i < BLOCK_COUNT; i++)
	    success &= EXPECT_NOT_NULL(blocks[i] = nmMalloc(BLOCK_SIZE));
	for (size_t i = 0lu; i < BLOCK_COUNT; i++)
	    nmFree(blocks[i], BLOCK_SIZE);

	/*** Debug info, captured to verify that nmStats() prints the stats.
	 *** nmStats() prints via the library's own stdout, so capturing it
	 *** requires us to redirect that file descriptor into a pipe that
	 *** we flush into a buffer.  This deadlocks if stats prints over
	 *** 64kb of data and fills the pipe, but that shouldn't happen.
	 ***/
	char stats_buf[2048];
	int stats_pipe[2];
	success &= EXPECT_EQL(pipe(stats_pipe), 0, "%d");
	fflush(stdout);
	int saved_stdout = dup(STDOUT_FILENO);
	dup2(stats_pipe[1], STDOUT_FILENO);
	close(stats_pipe[1]);
	nmStats(); /** Run target code. **/
	fflush(stdout);
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);
	ssize_t stats_len = read(stats_pipe[0], stats_buf, sizeof(stats_buf) - 1lu);
	close(stats_pipe[0]);
	stats_buf[(stats_len > 0) ? stats_len : 0] = '\0';

	/*** Every counter line is reported.  The counts themselves are only
	 *** tracked when the library is built with NMMALLOC_PROFILING, so
	 *** their values aren't checked here.
	 ***/
	success &= EXPECT_RANGE(strlen(stats_buf), (size_t)32, sizeof(stats_buf) - 1lu, "%zu");
	success &= EXPECT_NOT_NULL(strstr(stats_buf, "NewMalloc subsystem statistics:"));
	success &= EXPECT_NOT_NULL(strstr(stats_buf, "nmMalloc:"));
	success &= EXPECT_NOT_NULL(strstr(stats_buf, "nmFree:"));
	success &= EXPECT_NOT_NULL(strstr(stats_buf, "bigblks:"));

	/** Clear cache. **/
	nmClear();

    return success;
    }

long long test(char** tname)
    {
    *tname = "newmalloc-02 nmStats()";
    return loopTests(doTests);
    }

/** Scope cleanup. **/
#undef BLOCK_COUNT
#undef BLOCK_SIZE
