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
#include <assert.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    char buffer1[40];
    char buffer2[20];
    char * ptr;

	*tname = "memstr-00 correct null ptr";
	iter = 64000;
	for(i=0;i<iter;i++)
	    {
	    strcpy(buffer1, "this tests the boundary conditions.");
	    strcpy(buffer2, "tests the");
        ptr = memstr(buffer1, buffer2, sizeof buffer1);
        assert(ptr);
        strcpy(buffer2, "this t");
        ptr = memstr(buffer1, buffer2, sizeof buffer1);
        assert(ptr);
        strcpy(buffer2, "cond");
        ptr = memstr(buffer1, buffer2, sizeof buffer1);
        assert(ptr);
        strcpy(buffer2, "s.");
        ptr = memstr(buffer1, buffer2, sizeof buffer1);
        assert(ptr);
        strcpy(buffer2, "random");
        ptr = memstr(buffer1, buffer2, sizeof buffer1);
        assert(!ptr);
        ptr = memstr(buffer2, buffer1, sizeof buffer2);
        assert(!ptr);
	    }

    return iter*5;
    }
