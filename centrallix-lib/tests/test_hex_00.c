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
/* Module:	test_hex_00.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 17th, 2026					*/
/* Description:	Test hexEncode() for text and binary data, including	*/
/*		encoding in place.					*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "hex.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	64
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Src;	/* data to encode */
    ssize_t	    SrcLen;	/* length handed to hexEncode() */
    const char*	    Exp;	/* expected encoding */
    }
    Case;

static Case cases[] =
    {
    /**	  Src					SrcLen	Exp **/

    /** Text. **/
	{ "",					0,	"" },
	{ "f",					1,	"66" },
	{ "foobar",				6,	"666f6f626172" },

    /** A length of -1 means strlen(). **/
	{ "",					-1,	"" },
	{ "Aladdin",				-1,	"416c616464696e" },

    /** A shorter length encodes only the start of the data. **/
	{ "foobar",				3,	"666f6f" },

    /** Binary data, including nulls and every digit. **/
	{ "\x00",				1,	"00" },
	{ "\x00\xFF\x00",			3,	"00ff00" },
	{ "a\0b",				3,	"610062" },
	{ "\x01\x23\x45\x67\x89\xAB\xCD\xEF",	8,	"0123456789abcdef" },
	{ "\xFE\xDC\xBA\x98\x76\x54\x32\x10",	8,	"fedcba9876543210" },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

/** Encode each case into a guarded area, and again in place. **/
static bool
doTest(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];
	    const ssize_t exp_len = (ssize_t)strlen(tc->Exp);

	    /** Encode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    ssize_t rval = hexEncode(dst, AREA, tc->Src, tc->SrcLen, HEX_F_NONE);

	    /** The result is the encoding, null-terminated, and its length is returned. **/
	    success &= ASSERT_EQL(rval, exp_len, "%zd");
	    success &= ASSERT_STR_EQL(dst, tc->Exp);

	    /** Nothing outside the area is touched. **/
	    for (int i = 0; i < GUARD; i++)
		{
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
		success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
		}

	    /** Encoding in place gives the same result. **/
	    memset(raw, 0xAA, RAW);
	    memcpy(dst, tc->Src, (tc->SrcLen == -1) ? strlen(tc->Src) + 1 : (size_t)tc->SrcLen);
	    rval = hexEncode(dst, AREA, dst, tc->SrcLen, HEX_F_NONE);
	    success &= ASSERT_EQL(rval, exp_len, "%zd");
	    success &= ASSERT_STR_EQL(dst, tc->Exp);
	    }

    return success;
    }

/*** This test verifies that hexEncode() produces lowercase hex, high digit
 *** first, for text and binary data, honors an explicit length or -1 for
 *** strlen(), encodes in place, and returns the length of the
 *** null-terminated encoding it writes.
 ***/
long long
test(char** tname)
    {
    *tname = "hex-00 hexEncode()";
    return loopTest(doTest) * (NCASES * 2);
    }
