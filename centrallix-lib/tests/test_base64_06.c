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
/* Module:	test_base64_06.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test b64Purify(), which removes characters outside the	*/
/*		base64 alphabet in place.				*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "base64.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	64
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Src;	/* string to purify */
    ssize_t	    Len;	/* length handed to b64Purify() */
    const char*	    Exp;	/* expected result, or NULL for an error */
    }
    Case;

static Case cases[] =
    {
    /**	  Src				Len	Exp **/

    /** Line endings from wrapped base64. **/
	{ "Zm9v\r\nYmFy\r\n",		-1,	"Zm9vYmFy" },
	{ "Zm9v\nYmE=\n",		-1,	"Zm9vYmE=" },

    /** Padding and the whole alphabet are kept. **/
	{ "Zg==",			-1,	"Zg==" },
	{ "ABCDEFGHIJKLMNOPQRSTUVWXYZ",	-1,	"ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
	{ "abcdefghijklmnopqrstuvwxyz",	-1,	"abcdefghijklmnopqrstuvwxyz" },
	{ "0123456789+/=",		-1,	"0123456789+/=" },

    /** Everything else is removed, wherever it is. **/
	{ " Zm 9v ",			-1,	"Zm9v" },
	{ "-_.,;:#!\t",			-1,	"" },
	{ "Zm\xC3\xA9" "9v\x80\xFF",	-1,	"Zm9v" },
	{ "",				-1,	"" },

    /** An explicit length purifies only the start, nulls included. **/
	{ "Zm9v\nYmFy",			5,	"Zm9v" },
	{ "Zm\0" "9v",			5,	"Zm9v" },
	{ "Zm9v",			0,	"" },

    /** Bad arguments. **/
	{ "Zm9v",			-2,	NULL },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

static bool
doTest(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* str = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];
	    const ssize_t exp_rval = (tc->Exp == NULL) ? -1 : (ssize_t)strlen(tc->Exp);
	    const size_t src_size = (tc->Len >= 0) ? (size_t)tc->Len : strlen(tc->Src);

	    /** Purify a copy in a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    memcpy(str, tc->Src, src_size);
	    if (tc->Len == -1) str[src_size] = '\0';
	    const ssize_t rval = b64Purify(str, tc->Len);
	    if (!ASSERT_EQL(rval, exp_rval, "%zd"))
		{
		fprintf(stderr, "  > Case %d\n", c);
		success = false;
		}
	    if (tc->Exp != NULL) success &= ASSERT_STR_EQL(str, tc->Exp);

	    /** Nothing past the null at str[len] is touched. **/
	    for (int i = 0; i < GUARD; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    for (size_t i = GUARD + src_size + 1; i < RAW; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    }

    return success;
    }

/*** This test verifies that b64Purify() keeps the base64 alphabet and '=',
 *** removes everything else in place, honors an explicit length or -1 for
 *** strlen(), and returns the null-terminated result's length.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-06 b64Purify()";
    return loopTest(doTest) * NCASES;
    }
