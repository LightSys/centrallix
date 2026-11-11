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
/* Module:	test_base64_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test b64Encode() for text and binary data, without line	*/
/* 		wrapping.						*/
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
    const char*	    Src;	/* data to encode */
    ssize_t	    SrcLen;	/* length handed to b64Encode() */
    const char*	    Exp;	/* expected encoding */
    }
    Case;

static Case cases[] =
    {
    /**	  Src			SrcLen	Exp **/

    /** RFC 4648 test vectors. **/
	{ "",			0,	"" },
	{ "f",			1,	"Zg==" },
	{ "fo",			2,	"Zm8=" },
	{ "foo",		3,	"Zm9v" },
	{ "foob",		4,	"Zm9vYg==" },
	{ "fooba",		5,	"Zm9vYmE=" },
	{ "foobar",		6,	"Zm9vYmFy" },

    /** A length of -1 means strlen(). **/
	{ "",			-1,	"" },
	{ "Aladdin:open sesame",-1,	"QWxhZGRpbjpvcGVuIHNlc2FtZQ==" },

    /** A shorter length encodes only the start of the data. **/
	{ "foobar",		3,	"Zm9v" },

    /** Binary data, including nulls and both non-alphanumeric characters. **/
	{ "\x00",		1,	"AA==" },
	{ "\x00\x00",		2,	"AAA=" },
	{ "\x00\xFF\x00",	3,	"AP8A" },
	{ "a\0b",		3,	"YQBi" },
	{ "\xFB\xFF",		2,	"+/8=" },
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

	    /** Encode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    const ssize_t rval = b64Encode(dst, AREA, tc->Src, tc->SrcLen, 0, B64_F_NONE);

	    /** The result is the encoding, null-terminated, and its length is returned. **/
	    success &= ASSERT_EQL(rval, exp_len, "%zd");
	    success &= ASSERT_STR_EQL(dst, tc->Exp);

	    /** Nothing outside the area is touched. **/
	    for (int i = 0; i < GUARD; i++)
		{
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
		success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
		}
	    }

    return success;
    }

/*** This test verifies that b64Encode() produces standard padded base64 for
 *** text and binary data, honors an explicit length or -1 for strlen(), and
 *** returns the length of the null-terminated encoding it writes.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-00 b64Encode() without line wrapping";
    return loopTest(doTest) * NCASES;
    }
