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
/* Module: 	test_util_01.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	May 26th, 2011 					        */
/* Description: Test strtoui                                             */
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
	    assert(strtoui(text,NULL,0)==(unsigned int)i);
	    }
	sweep = end;

	/** Long too big for uint. **/
	snprintf(text,TXT_SIZE,"%lld",UINT_MAX+7LL);
	assert(strtoui(text,NULL,0)==UINT_MAX);

	/** Too big for long? **/
	assert(strtoui("121340193481047193741092347298347291391741",NULL,0)==UINT_MAX);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "util-01 convertion from strings to unsigned integers";
    return loopTests(doTests) * (CHUNK+2);
    }
