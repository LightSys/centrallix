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
/* Module:	test_base64_04.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test line wrapping with b64Wrap() and b64Encode().	*/
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
#define AREA	256
#define RAW	(GUARD + AREA + GUARD)

/** The MIME line length (RFC 2045). **/
#define MIME_LINE	76

typedef struct
    {
    const char*	    Src;	/* data to encode */
    size_t	    LineLen;	/* line length handed to b64Encode() */
    int		    Flags;	/* flags handed to b64Encode() */
    const char*	    Exp;	/* expected encoding, or NULL for an error */
    }
    EncodeCase;

static EncodeCase encode_cases[] =
    {
    /**	  Src		LineLen	Flags		Exp **/

    /** Every line ends in a line ending, including a short final line. **/
	{ "foobar",	4,	B64_F_NONE,	"Zm9v\nYmFy\n" },
	{ "fooba",	4,	B64_F_NONE,	"Zm9v\nYmE=\n" },
	{ "f",		4,	B64_F_NONE,	"Zg==\n" },
	{ "foob",	8,	B64_F_NONE,	"Zm9vYg==\n" },

    /** Data that ends on a line boundary gets no empty line. **/
	{ "foobar",	8,	B64_F_NONE,	"Zm9vYmFy\n" },

    /** CRLF line endings. **/
	{ "foobar",	4,	B64_F_CRLF,	"Zm9v\r\nYmFy\r\n" },
	{ "fooba",	8,	B64_F_CRLF,	"Zm9vYmE=\r\n" },

    /** Empty data has no lines. **/
	{ "",		4,	B64_F_NONE,	"" },
	{ "",		4,	B64_F_CRLF,	"" },

    /** Without wrapping, B64_F_CRLF changes nothing. **/
	{ "foobar",	0,	B64_F_CRLF,	"Zm9vYmFy" },

    /** Lines must hold whole 4-character groups. **/
	{ "foobar",	6,	B64_F_NONE,	NULL },
	{ "foobar",	1,	B64_F_NONE,	NULL },
	{ "foobar",	77,	B64_F_NONE,	NULL },

    /** Flags that b64Encode() does not take. **/
	{ "foobar",	4,	B64_F_SKIP_INVALID, NULL },
	{ "foobar",	4,	1<<8,		NULL },
    };

/** Number of encode cases run per call to doTest(). **/
#define N_ENCODE_CASES	((int)(sizeof(encode_cases) / sizeof(EncodeCase)))

typedef struct
    {
    const char*	    Src;	/* text to wrap */
    size_t	    LineLen;	/* line length handed to b64Wrap() */
    int		    Flags;	/* flags handed to b64Wrap() */
    const char*	    Exp;	/* expected wrapped text, or NULL for an error */
    }
    WrapCase;

static WrapCase wrap_cases[] =
    {
    /**	  Src		LineLen	Flags		Exp **/

    /** Any positive line length works. **/
	{ "abcdefg",	3,	B64_F_NONE,	"abc\ndef\ng\n" },
	{ "abcdef",	3,	B64_F_NONE,	"abc\ndef\n" },
	{ "abcdefg",	1,	B64_F_NONE,	"a\nb\nc\nd\ne\nf\ng\n" },
	{ "abcdefg",	7,	B64_F_NONE,	"abcdefg\n" },
	{ "abcdefg",	100,	B64_F_NONE,	"abcdefg\n" },
	{ "abcdefg",	3,	B64_F_CRLF,	"abc\r\ndef\r\ng\r\n" },
	{ "",		3,	B64_F_NONE,	"" },

    /** A line length of 0 has nothing to wrap to. **/
	{ "abcdefg",	0,	B64_F_NONE,	NULL },

    /** Flags that b64Wrap() does not take. **/
	{ "abcdefg",	3,	B64_F_SKIP_INVALID, NULL },
    };

/** Number of wrap cases run per call to doTest(). **/
#define N_WRAP_CASES	((int)(sizeof(wrap_cases) / sizeof(WrapCase)))

/** Check that nothing outside the area was touched. **/
static bool
checkGuards(const unsigned char* raw)
    {
    bool success = true;

	for (int i = 0; i < GUARD; i++)
	    {
	    success &= ASSERT_EQL(raw[i], 0xAA, "%d");
	    success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
	    }

    return success;
    }

/** Check one result against an expected string, or NULL for an error. **/
static bool
checkResult(int c, ssize_t rval, const char* dst, const char* exp)
    {
    bool success = true;

	const ssize_t exp_rval = (exp == NULL) ? -1 : (ssize_t)strlen(exp);
	if (!ASSERT_EQL(rval, exp_rval, "%zd"))
	    {
	    fprintf(stderr, "  > Case %d\n", c);
	    success = false;
	    }
	if (exp != NULL) success &= ASSERT_STR_EQL(dst, exp);

    return success;
    }

static bool
testEncode(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < N_ENCODE_CASES; c++)
	    {
	    const EncodeCase* tc = &encode_cases[c];

	    memset(raw, 0xAA, RAW);
	    const ssize_t rval = b64Encode(dst, AREA, tc->Src, -1, tc->LineLen, tc->Flags);
	    success &= checkResult(c, rval, dst, tc->Exp);
	    success &= checkGuards(raw);
	    }

    return success;
    }

static bool
testWrap(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < N_WRAP_CASES; c++)
	    {
	    const WrapCase* tc = &wrap_cases[c];

	    /** Wrap into a separate buffer. **/
	    memset(raw, 0xAA, RAW);
	    ssize_t rval = b64Wrap(dst, AREA, tc->Src, -1, tc->LineLen, tc->Flags);
	    success &= checkResult(c, rval, dst, tc->Exp);
	    success &= checkGuards(raw);

	    /** Wrapping in place gives the same result. **/
	    memset(raw, 0xAA, RAW);
	    memcpy(dst, tc->Src, strlen(tc->Src) + 1);
	    rval = b64Wrap(dst, AREA, dst, -1, tc->LineLen, tc->Flags);
	    success &= checkResult(c, rval, dst, tc->Exp);
	    success &= checkGuards(raw);
	    }

    return success;
    }

/** MIME-length lines from b64Encode() match wrapping its unwrapped output. **/
static bool
testMimeLines(void)
    {
    bool success = true;
    char data[MIME_LINE * 2];
    char unwrapped[AREA];
    char expected[AREA];
    char wrapped[AREA];

	memset(data, 'x', sizeof(data));

	/** 57 bytes fill a 76-character line exactly; one more starts a second. **/
	const size_t lens[] = { 56, 57, 58, 114, 115 };
	const size_t lines[] = { 1, 1, 2, 2, 3 };
	for (size_t i = 0; i < sizeof(lens) / sizeof(lens[0]); i++)
	    {
	    const ssize_t enc_len = b64Encode(unwrapped, sizeof(unwrapped), data, (ssize_t)lens[i], 0, B64_F_NONE);
	    const ssize_t exp_len = b64Wrap(expected, sizeof(expected), unwrapped, enc_len, MIME_LINE, B64_F_CRLF);
	    const ssize_t rval = b64Encode(wrapped, sizeof(wrapped), data, (ssize_t)lens[i], MIME_LINE, B64_F_CRLF);
	    success &= ASSERT_EQL(rval, exp_len, "%zd");
	    success &= ASSERT_STR_EQL(wrapped, expected);
	    success &= ASSERT_EQL(rval, enc_len + (ssize_t)lines[i] * 2, "%zd");
	    }

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;

	success &= testEncode();
	success &= testWrap();
	success &= testMimeLines();

    return success;
    }

/*** This test verifies that b64Encode() and b64Wrap() end every line,
 *** including a short final one, with "\n" or "\r\n", never add an empty
 *** line, wrap in place, and reject line lengths and flags they cannot use.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-04 line wrapping";
    return loopTest(doTest) * (N_ENCODE_CASES + N_WRAP_CASES * 2 + 5);
    }
