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
/* Module:	test_ascii85_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 13th, 2026					*/
/* Description:	Test a85Encode() against known encodings.		*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "ascii85.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	32
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Src;	/* data to encode */
    size_t	    SrcLen;	/* length handed to a85Encode() */
    const char*	    Exp;	/* expected encoding */
    }
    Case;

static Case cases[] =
    {
    /**	  Src				SrcLen	Exp **/

    /** Nothing to encode. **/
	{ "",				0,	"" },

    /** Full groups. **/
	{ "Man ",			4,	"9jqo^" },
	{ "\xFF\xFF\xFF\xFF",		4,	"s8W-!" },
	{ "\0\0\0\1",			4,	"!!!!\"" },
	{ "Man is distinguished",	20,	"9jqo^BlbD-BleB1DJ+*+F(f,q" },

    /** A final 1-3 bytes give one more digit than bytes. **/
	{ "\xFF",			1,	"rr" },
	{ "\0",				1,	"!!" },
	{ "\0\0",			2,	"!!!" },
	{ "\0\0\0",			3,	"!!!!" },
	{ "sure.",			5,	"F*2M7/c" },

    /** Full groups of zeros become "z", but a final short group does not. **/
	{ "\0\0\0\0",			4,	"z" },
	{ "\0\0\0\0\0",			5,	"z!!" },
	{ "\0\0\0\0Man \0\0\0\0",	12,	"z9jqo^z" },
	{ "\1\0\0\0\0\0\0\0",		8,	"!<<*\"z" },
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
	    const ssize_t exp_len = (ssize_t)strlen(tc->Exp);

	    /** Encode into a guarded area sized by a85EncodedSizeMax(). **/
	    memset(raw, 0xAA, RAW);
	    const size_t dst_size = a85EncodedSizeMax(tc->SrcLen);
	    if (!ASSERT_RANGE(dst_size, (size_t)1, (size_t)AREA, "%zu")) return false;
	    const ssize_t rval = a85Encode(dst, dst_size, tc->Src, tc->SrcLen);

	    /** The result is the encoding, null-terminated, and its length is returned. **/
	    success &= ASSERT_EQL(rval, exp_len, "%zd");
	    success &= ASSERT_STR_EQL(dst, tc->Exp);

	    /** Nothing outside dst_size bytes is touched. **/
	    for (int i = 0; i < GUARD; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    for (size_t i = GUARD + dst_size; i < RAW; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    }

    return success;
    }

/*** This test verifies that a85Encode() produces known ASCII85 encodings,
 *** including "z" for zero groups and short final groups, and returns the
 *** length of the null-terminated encoding it writes within dst_size.
 ***/
long long
test(char** tname)
    {
    *tname = "ascii85-00 a85Encode() known encodings";
    return loopTest(doTest) * NCASES;
    }
