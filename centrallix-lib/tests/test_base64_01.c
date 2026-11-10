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
/* Module:	test_base64_01.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test b64Decode() on valid input, with and without	*/
/*		padding, including round trips and decoding in place.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "base64.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	64
#define RAW	(GUARD + AREA + GUARD)

/** Largest round trip input, and a buffer that holds its encoding. **/
#define MAX_DATA	300
#define MAX_ENC		(MAX_DATA / 3 * 4 + 4 + 1)

typedef struct
    {
    const char*	    Src;	/* base64 to decode */
    ssize_t	    SrcLen;	/* length handed to b64Decode() */
    const char*	    Exp;	/* expected decoded data */
    ssize_t	    ExpLen;	/* expected decoded length */
    }
    Case;

static Case cases[] =
    {
    /**	  Src			SrcLen	Exp			ExpLen **/

    /** RFC 4648 test vectors. **/
	{ "",			-1,	"",			0 },
	{ "Zg==",		-1,	"f",			1 },
	{ "Zm8=",		-1,	"fo",			2 },
	{ "Zm9v",		-1,	"foo",			3 },
	{ "Zm9vYg==",		-1,	"foob",			4 },
	{ "Zm9vYmE=",		-1,	"fooba",		5 },
	{ "Zm9vYmFy",		-1,	"foobar",		6 },

    /** Padding is optional. **/
	{ "Zg",			-1,	"f",			1 },
	{ "Zm8",		-1,	"fo",			2 },
	{ "Zm9vYg",		-1,	"foob",			4 },
	{ "Zm9vYmE",		-1,	"fooba",		5 },

    /** Leftover bits in the final group are ignored. **/
	{ "QQ==",		-1,	"A",			1 },
	{ "QR==",		-1,	"A",			1 },
	{ "QR",			-1,	"A",			1 },
	{ "QUJ=",		-1,	"AB",			2 },

    /** An explicit length decodes only the start of the input. **/
	{ "Zm9vYmFy",		4,	"foo",			3 },
	{ "Zm9vYmFy",		0,	"",			0 },
	{ "Zm9vYmFy!!!",	8,	"foobar",		6 },

    /** Binary data, including nulls and both non-alphanumeric characters. **/
	{ "AA==",		-1,	"\x00",			1 },
	{ "AP8A",		-1,	"\x00\xFF\x00",		3 },
	{ "YQBi",		-1,	"a\0b",			3 },
	{ "+/8=",		-1,	"\xFB\xFF",		2 },

    /** An HTTP Basic credential. **/
	{ "QWxhZGRpbjpvcGVuIHNlc2FtZQ==", -1, "Aladdin:open sesame", 19 },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

static unsigned int seed_counter = 0;

/** Decode each case into a guarded area, and again in place. **/
static bool
testCases(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];

	    /** Decode into a guarded area. **/
	    memset(raw, 0xAA, RAW);
	    ssize_t rval = b64Decode(dst, AREA, tc->Src, tc->SrcLen, B64_F_NONE);

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
	    rval = b64Decode(dst, AREA, dst, tc->SrcLen, B64_F_NONE);
	    success &= ASSERT_EQL(rval, tc->ExpLen, "%zd");
	    success &= ASSERT_STR_EQL_N(dst, tc->Exp, tc->ExpLen);
	    success &= ASSERT_EQL(dst[tc->ExpLen], '\0', "%d");
	    }

    return success;
    }

/** Round trip every byte value, then random data of a random length. **/
static bool
testRoundTrip(void)
    {
    bool success = true;
    unsigned char data[MAX_DATA];
    unsigned char decoded[MAX_DATA + 1];
    char encoded[MAX_ENC];

	/** Use a distinct seed each pass so the data varies. **/
	srand(seed_counter++);

	for (int pass = 0; pass < 2; pass++)
	    {
	    /** Every byte value in order, then random bytes. **/
	    size_t len = 256;
	    for (size_t i = 0; i < len; i++) data[i] = (unsigned char)i;
	    if (pass == 1)
		{
		len = (size_t)rand() % (MAX_DATA + 1);
		for (size_t i = 0; i < len; i++) data[i] = (unsigned char)rand();
		}

	    /** Encode, then decode the result. **/
	    const ssize_t enc_len = b64Encode(encoded, sizeof(encoded), data, (ssize_t)len, 0, B64_F_NONE);
	    success &= ASSERT_EQL(enc_len, (ssize_t)((len + 2) / 3 * 4), "%zd");
	    if (enc_len < 0) return false;
	    const ssize_t dec_len = b64Decode(decoded, sizeof(decoded), encoded, enc_len, B64_F_NONE);
	    success &= ASSERT_EQL(dec_len, (ssize_t)len, "%zd");
	    success &= ASSERT_TRUE(memcmp(decoded, data, len) == 0);
	    }

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;

	success &= testCases();
	success &= testRoundTrip();

    return success;
    }

/*** This test verifies that b64Decode() accepts standard base64 with or
 *** without padding, ignores leftover bits, honors an explicit length or -1
 *** for strlen(), decodes in place, and round trips with b64Encode().
 ***/
long long
test(char** tname)
    {
    *tname = "base64-01 b64Decode() on valid input";
    return loopTest(doTest) * (NCASES * 2 + 2);
    }
