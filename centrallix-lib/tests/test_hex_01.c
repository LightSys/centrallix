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
/* Module:	test_hex_01.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 17th, 2026					*/
/* Description:	Test hexDecode() on valid input, including decoding in	*/
/*		place.							*/
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
    const char*	    Src;	/* hex to decode */
    ssize_t	    SrcLen;	/* length handed to hexDecode() */
    const char*	    Exp;	/* expected decoded data */
    ssize_t	    ExpLen;	/* expected decoded length */
    }
    Case;

static Case cases[] =
    {
    /**	  Src			SrcLen	Exp					ExpLen **/

    /** Text. **/
	{ "",			-1,	"",					0 },
	{ "66",			-1,	"f",					1 },
	{ "666f6f626172",	-1,	"foobar",				6 },

    /** Either case, or a mix of both. **/
	{ "416C616464696E",	-1,	"Aladdin",				7 },
	{ "aBcD",		-1,	"\xAB\xCD",				2 },
	{ "0123456789abcdef",	-1,	"\x01\x23\x45\x67\x89\xAB\xCD\xEF",	8 },
	{ "FEDCBA9876543210",	-1,	"\xFE\xDC\xBA\x98\x76\x54\x32\x10",	8 },

    /** An explicit length decodes only the start of the input. **/
	{ "666f6f626172",	4,	"fo",					2 },
	{ "666f6f626172",	0,	"",					0 },
	{ "666f6f!!",		6,	"foo",					3 },

    /** Binary data, including nulls. **/
	{ "00",			-1,	"\x00",					1 },
	{ "00ff00",		-1,	"\x00\xFF\x00",				3 },
	{ "610062",		-1,	"a\0b",					3 },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

/** Decode each case into a guarded area, and again in place. **/
static bool
doTest(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];

	    /** Decode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    ssize_t rval = hexDecode(dst, AREA, tc->Src, tc->SrcLen, HEX_F_NONE);

	    /** The result is the decoded data, null-terminated. **/
	    success &= ASSERT_EQL(rval, tc->ExpLen, "%zd");
	    success &= ASSERT_STR_EQL_N(dst, tc->Exp, tc->ExpLen);
	    success &= ASSERT_EQL(dst[tc->ExpLen], '\0', "%d");

	    /** Nothing outside the area is touched. **/
	    for (int i = 0; i < GUARD; i++)
		{
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
		success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
		}

	    /** Decoding in place gives the same result. **/
	    memset(raw, 0xAA, RAW);
	    memcpy(dst, tc->Src, strlen(tc->Src) + 1);
	    rval = hexDecode(dst, AREA, dst, tc->SrcLen, HEX_F_NONE);
	    success &= ASSERT_EQL(rval, tc->ExpLen, "%zd");
	    success &= ASSERT_STR_EQL_N(dst, tc->Exp, tc->ExpLen);
	    success &= ASSERT_EQL(dst[tc->ExpLen], '\0', "%d");
	    }

    return success;
    }

/*** This test verifies that hexDecode() accepts hex digits in either case,
 *** honors an explicit length or -1 for strlen(), decodes in place, and
 *** returns the length of the null-terminated data it writes.
 ***/
long long
test(char** tname)
    {
    *tname = "hex-01 hexDecode()";
    return loopTest(doTest) * (NCASES * 2);
    }
