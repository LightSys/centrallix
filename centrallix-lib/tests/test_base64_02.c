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
/* Module:	test_base64_02.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test that b64Decode() rejects malformed input, bad	*/
/*		arguments, and buffers that are too small.		*/
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
    ssize_t	    SrcLen;	/* length handed to b64Decode() */
    size_t	    DstSize;	/* size handed to b64Decode() */
    int		    Flags;	/* flags handed to b64Decode() */
    ssize_t	    ExpRval;	/* expected return value */
    }
    Case;

static Case cases[] =
    {
    /**	  Src		SrcLen	DstSize	Flags			ExpRval **/

    /** A final group of 1 character holds too few bits for a byte. **/
	{ "Q",		-1,	AREA,	B64_F_NONE,		-1 },
	{ "Q=",		-1,	AREA,	B64_F_NONE,		-1 },
	{ "Q==",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Q===",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm9vY",	-1,	AREA,	B64_F_NONE,		-1 },

    /** Padding is all or nothing, and never more than two characters. **/
	{ "Zg=",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zg===",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm8==",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm9v=",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "====",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "=",		-1,	AREA,	B64_F_NONE,		-1 },

    /** Padding ends the data. **/
	{ "Zg==Zm9v",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm8=Zg==",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Z=g=",	-1,	AREA,	B64_F_NONE,		-1 },

    /** Characters outside the alphabet. **/
	{ "Zm9v\n",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm 9v",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm9v#",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "PD8_",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "PD8-",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm\x80v",	-1,	AREA,	B64_F_NONE,		-1 },
	{ "Zm\0v",	 4,	AREA,	B64_F_NONE,		-1 },

    /** The decoded data and its null must fit in dst. **/
	{ "Zm9vYmFy",	-1,	7,	B64_F_NONE,		 6 },
	{ "Zm9vYmFy",	-1,	6,	B64_F_NONE,		-1 },
	{ "Zm9vYmE=",	-1,	6,	B64_F_NONE,		 5 },
	{ "Zm9vYmE=",	-1,	5,	B64_F_NONE,		-1 },
	{ "Zg",		-1,	2,	B64_F_NONE,		 1 },
	{ "Zg",		-1,	1,	B64_F_NONE,		-1 },
	{ "",		-1,	1,	B64_F_NONE,		 0 },
	{ "",		-1,	0,	B64_F_NONE,		-1 },

    /** Bad arguments. **/
	{ "Zm9v",	-2,	AREA,	B64_F_NONE,		-1 },
	{ "Zm9v",	-1,	AREA,	B64_F_CRLF,		-1 },
	{ "Zm9v",	-1,	AREA,	1<<8,			-1 },
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

	    /** Decode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    const ssize_t rval = b64Decode(dst, tc->DstSize, tc->Src, tc->SrcLen, tc->Flags);
	    if (!ASSERT_EQL(rval, tc->ExpRval, "%zd"))
		{
		fprintf(stderr, "  > Case %d: \"%s\"\n", c, tc->Src);
		success = false;
		}

	    /** Nothing outside of DstSize is touched, even on failure. **/
	    for (int i = 0; i < GUARD; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    for (size_t i = GUARD + tc->DstSize; i < RAW; i++)
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    }

    return success;
    }

/*** This test verifies that b64Decode() returns -1 for a 1-character final
 *** group, partial or excess padding, data after padding, characters outside
 *** the alphabet, bad arguments, and a dst too small for the data and its
 *** null, while never writing outside the dst_size it was given.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-02 b64Decode() errors";
    return loopTest(doTest) * NCASES;
    }
