/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2014-2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_memstr_00.c					*/
/* Author:	Brady Steed						*/
/* Creation:	July 10, 2014						*/
/* Description:	Test the memstr function.				*/
/************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "memstr.h"
#include "strtcpy.h"
#include <assert.h>
#include <stdbool.h>
#include "t_utils.h"

static bool
doTest(void)
    {
    char buffer1[40];
    char buffer2[20];
    char * ptr;

	strtcpy(buffer1, "this tests the boundary conditions.", sizeof(buffer1));
	strtcpy(buffer2, "tests the", sizeof(buffer2));
	ptr = memstr(buffer1, buffer2, sizeof buffer1);
	assert(ptr);
	strtcpy(buffer2, "this t", sizeof(buffer2));
	ptr = memstr(buffer1, buffer2, sizeof buffer1);
	assert(ptr);
	strtcpy(buffer2, "cond", sizeof(buffer2));
	ptr = memstr(buffer1, buffer2, sizeof buffer1);
	assert(ptr);
	strtcpy(buffer2, "s.", sizeof(buffer2));
	ptr = memstr(buffer1, buffer2, sizeof buffer1);
	assert(ptr);
	strtcpy(buffer2, "random", sizeof(buffer2));
	ptr = memstr(buffer1, buffer2, sizeof buffer1);
	assert(!ptr);
	ptr = memstr(buffer2, buffer1, sizeof buffer2);
	assert(!ptr);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "memstr-00 correct null ptr";
    return loopTest(doTest) * 6;
    }
