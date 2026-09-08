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
/* Module: 	test_util_00.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	May 26th, 2011 					        */
/* Description: Test strtoi                                             */
/************************************************************************/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "test_utils.h"

#include "util.h"

#define TXT_SIZE 1024
#define RANGE    500000

/** Values from the sweep converted per call to doTests(). **/
#define CHUNK	 10000

/** Next value in the sweep, advanced by CHUNK and wrapped at the end. **/
static int sweep = -RANGE;

static bool
doTests(void)
    {
    int i;
    int end;
    char text[TXT_SIZE]="";

	if (sweep >= RANGE) sweep = -RANGE;
	end = sweep + CHUNK;
	if (end > RANGE) end = RANGE;

	for(i=sweep; i<end; i++)
	    {
	    snprintf(text,TXT_SIZE,"%d",i);
	    assert(strtoi(text,NULL,0)==i);
	    }
	sweep = end;

	/** Long too big for int. **/
	snprintf(text,TXT_SIZE,"%lld",INT_MAX+7LL);
	assert(strtoi(text,NULL,0)==INT_MAX);
	snprintf(text,TXT_SIZE,"%lld",INT_MIN-7LL);
	assert(strtoi(text,NULL,0)==INT_MIN);

	/** Too big for long? **/
	assert(strtoi("121340193481047193741092347298347291391741",NULL,0)==INT_MAX);
	assert(strtoi("-19874238479349128374239483948347834913498",NULL,0)==INT_MIN);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "util-00 convertion from strings to integers";
    return loopTests(doTests) * (CHUNK+4);
    }
