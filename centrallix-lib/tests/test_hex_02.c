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
/* Module:	test_hex_02.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 17th, 2026					*/
/* Description:	Test that hexDecode() rejects malformed input, bad	*/
/*		arguments, and buffers that are too small.		*/
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
    size_t	    DstSize;	/* size handed to hexDecode() */
    int		    Flags;	/* flags handed to hexDecode() */
    ssize_t	    ExpRval;	/* expected return value */
    }
    Case;

static Case cases[] =
    {
    /**	  Src		SrcLen	DstSize	Flags			ExpRval **/

    /** Every byte takes two digits. **/
	{ "6",		-1,	AREA,	HEX_F_NONE,		-1 },
	{ "666",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "666f6f6",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "666f",	3,	AREA,	HEX_F_NONE,		-1 },

    /** Characters that are not hex digits. **/
	{ "6g",		-1,	AREA,	HEX_F_NONE,		-1 },
	{ "G6",		-1,	AREA,	HEX_F_NONE,		-1 },
	{ "0x66",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "-1",		-1,	AREA,	HEX_F_NONE,		-1 },
	{ "66 6f",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ " 666",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "66\r\n",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "6\x80",	-1,	AREA,	HEX_F_NONE,		-1 },
	{ "6\0",	2,	AREA,	HEX_F_NONE,		-1 },

    /** The decoded data and its null must fit in dst. **/
	{ "666f6f",	-1,	4,	HEX_F_NONE,		 3 },
	{ "666f6f",	-1,	3,	HEX_F_NONE,		-1 },
	{ "66",		-1,	2,	HEX_F_NONE,		 1 },
	{ "66",		-1,	1,	HEX_F_NONE,		-1 },
	{ "",		-1,	1,	HEX_F_NONE,		 0 },
	{ "",		-1,	0,	HEX_F_NONE,		-1 },

    /** Bad arguments. **/
	{ "666f",	-2,	AREA,	HEX_F_NONE,		-1 },
	{ "666f",	-1,	AREA,	HEX_F_TRUNCATE,		-1 },
	{ "666f",	-1,	AREA,	1<<8,			-1 },
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
	    const ssize_t rval = hexDecode(dst, tc->DstSize, tc->Src, tc->SrcLen, tc->Flags);
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

/*** This test verifies that hexDecode() returns -1 for an odd number of
 *** digits, characters that are not hex digits, bad arguments, and a dst
 *** too small for the data and its null, while never writing outside the
 *** dst_size it was given.
 ***/
long long
test(char** tname)
    {
    *tname = "hex-02 hexDecode() errors";
    return loopTest(doTest) * NCASES;
    }
