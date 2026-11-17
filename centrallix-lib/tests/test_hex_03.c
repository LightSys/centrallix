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
/* Module:	test_hex_03.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 17th, 2026					*/
/* Description:	Test hexEncode() errors, and truncation with		*/
/*		HEX_F_TRUNCATE.						*/
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
    size_t	    DstSize;	/* size handed to hexEncode() */
    int		    Flags;	/* flags handed to hexEncode() */
    ssize_t	    ExpRval;	/* expected return value */
    const char*	    Exp;	/* expected encoding, if ExpRval is not -1 */
    }
    Case;

static Case cases[] =
    {
    /**	  Src		SrcLen	DstSize	Flags		ExpRval	Exp **/

    /** Without HEX_F_TRUNCATE, the encoding and its null must fit in dst. **/
	{ "foo",	3,	7,	HEX_F_NONE,	 6,	"666f6f" },
	{ "foo",	3,	6,	HEX_F_NONE,	-1,	NULL },
	{ "foo",	3,	1,	HEX_F_NONE,	-1,	NULL },
	{ "",		0,	1,	HEX_F_NONE,	 0,	"" },
	{ "",		0,	0,	HEX_F_NONE,	-1,	NULL },

    /** HEX_F_TRUNCATE writes as many digits as fit, even an odd number. **/
	{ "foo",	3,	AREA,	HEX_F_TRUNCATE,	 6,	"666f6f" },
	{ "foo",	3,	7,	HEX_F_TRUNCATE,	 6,	"666f6f" },
	{ "foo",	3,	6,	HEX_F_TRUNCATE,	 5,	"666f6" },
	{ "foo",	3,	5,	HEX_F_TRUNCATE,	 4,	"666f" },
	{ "foo",	3,	2,	HEX_F_TRUNCATE,	 1,	"6" },
	{ "foo",	3,	1,	HEX_F_TRUNCATE,	 0,	"" },
	{ "",		0,	1,	HEX_F_TRUNCATE,	 0,	"" },

    /** HEX_F_TRUNCATE still needs room for the null. **/
	{ "foo",	3,	0,	HEX_F_TRUNCATE,	-1,	NULL },

    /** Bad arguments. **/
	{ "foo",	-2,	AREA,	HEX_F_NONE,	-1,	NULL },
	{ "foo",	3,	AREA,	1<<8,		-1,	NULL },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

/** Largest digit count checked when truncating in place. **/
#define MAX_DIGITS	20

/** Encode each case into a guarded area. **/
static bool
checkCases(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];

	    memset(raw, 0xAA, RAW);
	    const ssize_t rval = hexEncode(dst, tc->DstSize, tc->Src, tc->SrcLen, tc->Flags);
	    if (!ASSERT_EQL(rval, tc->ExpRval, "%zd"))
		{
		fprintf(stderr, "  > Case %d: \"%s\" into %zu bytes\n", c, tc->Src, tc->DstSize);
		success = false;
		}

	    /** A success writes the expected encoding, and a failure writes nothing. **/
	    size_t written = 0;
	    if (tc->Exp != NULL)
		{
		success &= ASSERT_STR_EQL(dst, tc->Exp);
		written = strlen(tc->Exp) + 1;
		}
	    for (int i = 0; i < GUARD; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    for (size_t i = GUARD + written; i < RAW; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    }

    return success;
    }

/** Truncate in place to every digit count, encoding one more byte than fits. **/
static bool
checkInPlace(void)
    {
    bool success = true;
    const unsigned char data[MAX_DIGITS / 2 + 1] =
	{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA };
    const char* full = "0123456789abcdeffedcba";
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (size_t digits = 0; digits <= MAX_DIGITS; digits++)
	    {
	    const size_t n_bytes = digits / 2 + 1;

	    memset(raw, 0xAA, RAW);
	    memcpy(dst, data, n_bytes);
	    const ssize_t rval = hexEncode(dst, digits + 1, dst, (ssize_t)n_bytes, HEX_F_TRUNCATE);
	    success &= ASSERT_EQL(rval, (ssize_t)digits, "%zd");
	    success &= ASSERT_STR_EQL_N(dst, full, digits);
	    success &= ASSERT_EQL(dst[digits], '\0', "%d");

	    /** Nothing past dst_size is touched. **/
	    for (size_t i = GUARD + digits + 1; i < RAW; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    }

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;

	success &= checkCases();
	success &= checkInPlace();

    return success;
    }

/*** This test verifies that hexEncode() fails without touching dst when the
 *** encoding does not fit or the arguments are bad, and that
 *** HEX_F_TRUNCATE writes as many digits as fit (an odd number included),
 *** in place or not, without writing past dst_size.
 ***/
long long
test(char** tname)
    {
    *tname = "hex-03 hexEncode() errors and truncation";
    return loopTest(doTest) * (NCASES + MAX_DIGITS + 1);
    }
