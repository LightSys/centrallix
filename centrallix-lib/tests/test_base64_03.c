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
/* Module:	test_base64_03.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test b64Decode() with B64_F_SKIP_INVALID, which ignores	*/
/*		characters outside the base64 alphabet.			*/
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
    const char*	    Src;	/* base64 to decode */
    const char*	    Exp;	/* expected decoded data, or NULL for an error */
    }
    Case;

static Case cases[] =
    {
    /**	  Src					Exp **/

    /** Line endings from wrapped base64. **/
	{ "Zm9v\nYmFy\n",			"foobar" },
	{ "Zm9v\r\nYmFy\r\n",			"foobar" },
	{ "Zm9v\r\nYmE=\r\n",			"fooba" },

    /** Other characters, anywhere in the input. **/
	{ " Z m 9 v ",				"foo" },
	{ "#Zm9v#",				"foo" },
	{ "\tZm\x80\xFF" "9v",			"foo" },
	{ "\n\n",				"" },

    /** Skipped characters may follow or split the padding. **/
	{ "Zg==\r\n",				"f" },
	{ "Zg=\n=",				"f" },
	{ "Zm8\n=",				"fo" },

    /** The data itself must still be valid. **/
	{ "Zg==\r\nZg==\r\n",			NULL },
	{ "Z\n",				NULL },
	{ "Zg=\n",				NULL },
	{ "Z\n=g=",				NULL },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

static bool
doTest(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];
	    const ssize_t exp_rval = (tc->Exp == NULL) ? -1 : (ssize_t)strlen(tc->Exp);

	    /** Decode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    const ssize_t rval = b64Decode(dst, AREA, tc->Src, -1, B64_F_SKIP_INVALID);
	    if (!ASSERT_EQL(rval, exp_rval, "%zd"))
		{
		fprintf(stderr, "  > Case %d\n", c);
		success = false;
		}
	    if (tc->Exp != NULL) success &= ASSERT_STR_EQL(dst, tc->Exp);

	    /** Nothing outside the area is touched. **/
	    for (int i = 0; i < GUARD; i++)
		{
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
		success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
		}
	    }

    return success;
    }

/*** This test verifies that B64_F_SKIP_INVALID lets b64Decode() ignore line
 *** endings and other characters outside the alphabet, anywhere in the
 *** input, while every rule about the alphabet characters and padding
 *** themselves still applies.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-03 b64Decode() skipping invalid characters";
    return loopTest(doTest) * NCASES;
    }
