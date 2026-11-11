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
/* Module:	test_qprintf_69.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test the qprintf &B64 and &DB64 filters, which use the	*/
/*		base64 library, including text around the filtered	*/
/*		value and output into a growing XString.		*/
/************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"
#include "base64.h"
#include "xstring.h"

/** Tested module. **/
#include "qprintf.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	64
#define RAW	(GUARD + AREA + GUARD)

/** Enough data that its encoding outgrows a new XString. **/
#define LONG_DATA	300

typedef struct
    {
    const char*	    Format;	/* qpfPrintf() format, with a %STR or %*STR */
    int		    Len;	/* length for a %*STR, otherwise unused */
    const char*	    Arg;	/* string argument */
    const char*	    Exp;	/* expected output, or NULL for an error */
    }
    Case;

static Case cases[] =
    {
    /**	  Format		Len	Arg			Exp **/

    /** Encoding, with text around the value. **/
	{ "%STR&B64",		0,	"foobar",		"Zm9vYmFy" },
	{ "Basic %STR&B64",	0,	"Aladdin:open sesame",	"Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==" },
	{ "[%STR&B64]",		0,	"fo",			"[Zm8=]" },
	{ "%*STR&B64",		3,	"foobar",		"Zm9v" },

    /** Decoding, with text around the value. **/
	{ "%STR&DB64",		0,	"Zm9vYmFy",		"foobar" },
	{ "user=%STR&DB64.",	0,	"Zm9v",			"user=foo." },
	{ "%*STR&DB64",		4,	"Zm9vYmFy",		"foo" },

    /** A decoding error fails the print. **/
	{ "%STR&DB64",		0,	"Zg=",			NULL },

    /** The output and its null must fit. **/
	{ "%STR&B64",		0,	"0123456789012345678901234567890123456789012345", NULL },
	{ "%STR&DB64",		0,	"MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMw==", NULL },
    };

/** Number of cases run per call to doTest(). **/
#define NCASES	((int)(sizeof(cases) / sizeof(Case)))

/** Print each case into a guarded area. **/
static bool
testCases(void)
    {
    bool success = true;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;

	for (int c = 0; c < NCASES; c++)
	    {
	    const Case* tc = &cases[c];
	    const int exp_rval = (tc->Exp == NULL) ? -1 : (int)strlen(tc->Exp);

	    memset(raw, 0xAA, RAW);
	    const int rval = (strstr(tc->Format, "%*") != NULL)
		? qpfPrintf(NULL, dst, AREA, tc->Format, tc->Len, tc->Arg)
		: qpfPrintf(NULL, dst, AREA, tc->Format, tc->Arg);
	    bool ok = (tc->Exp == NULL) ? ASSERT_TRUE(rval < 0) : ASSERT_EQL(rval, exp_rval, "%d");
	    if (tc->Exp != NULL) ok &= ASSERT_STR_EQL(dst, tc->Exp);
	    if (!ok)
		{
		fprintf(stderr, "  > Case %d: \"%s\"\n", c, tc->Format);
		success = false;
		}

	    /** Nothing outside the area is touched. **/
	    for (int i = 0; i < GUARD; i++)
		{
		success &= ASSERT_EQL(raw[i], 0xAA, "%d");
		success &= ASSERT_EQL(raw[GUARD + AREA + i], 0xAA, "%d");
		}
	    }

    return success;
    }

/** Encode and decode with text before the value, into a growing XString. **/
static bool
testXString(void)
    {
    bool success = true;
    char data[LONG_DATA + 1];
    char encoded[LONG_DATA * 2];
    char expected[LONG_DATA * 2 + 16];
    XString xs;

	memset(data, 'x', LONG_DATA);
	data[LONG_DATA] = '\0';
	const ssize_t enc_len = b64Encode(encoded, sizeof(encoded), data, LONG_DATA, 0, B64_F_NONE);
	if (!ASSERT_RANGE(enc_len, (ssize_t)0, (ssize_t)sizeof(encoded), "%zd")) return false;

	/** Encode. **/
	snprintf(expected, sizeof(expected), "Basic %s", encoded);
	if (!ASSERT_EQL(xsInit(&xs), 0, "%d")) return false;
	success &= ASSERT_EQL(xsQPrintf(&xs, "Basic %STR&B64", data), (int)strlen(expected), "%d");
	success &= ASSERT_STR_EQL(xsString(&xs), expected);
	xsDeInit(&xs);

	/** Decode. **/
	snprintf(expected, sizeof(expected), "data:%s", data);
	if (!ASSERT_EQL(xsInit(&xs), 0, "%d")) return false;
	success &= ASSERT_EQL(xsQPrintf(&xs, "data:%STR&DB64", encoded), (int)strlen(expected), "%d");
	success &= ASSERT_STR_EQL(xsString(&xs), expected);
	xsDeInit(&xs);

    return success;
    }

/** Grow to exactly req, keeping only the data before offset. **/
static int
growExact(char** str, size_t* size, size_t offset, void* args, size_t req)
    {

	if (req <= *size) return 1;
	char* new_str = malloc(req);
	if (new_str == NULL) return 0;
	memset(new_str, 0xAA, req);
	if (*str != NULL) memcpy(new_str, *str, min(offset, *size));
	free(*str);
	*str = new_str;
	*size = req;

    return 1;
    }

/** qpfPrintf() into a buffer grown by growExact(). **/
static int
growPrintf(char** str, size_t* size, const char* format, ...)
    {
    va_list ap;
    int rval;

	va_start(ap, format);
	rval = qpfPrintf_va_internal(NULL, str, size, growExact, NULL, format, ap);
	va_end(ap);

    return rval;
    }

/** Print into a buffer that growExact() grows from 1 byte. **/
static bool
checkGrow(const char* format, const char* arg, const char* exp)
    {
    bool success = true;
    size_t size = 1;
    char* str = malloc(size);

	if (!ASSERT_NOT_NULL(str)) return false;
	const int rval = growPrintf(&str, &size, format, arg);
	success &= ASSERT_EQL(rval, (int)strlen(exp), "%d");
	success &= ASSERT_STR_EQL(str, exp);
	free(str);

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;

	success &= testCases();
	success &= testXString();
	success &= checkGrow("Basic %STR&B64", "Aladdin:open sesame", "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==");
	success &= checkGrow("user=%STR&DB64.", "Zm9vYmFy", "user=foobar.");

    return success;
    }

/*** This test verifies that the qprintf &B64 and &DB64 filters encode and
 *** decode exactly like the base64 library, honor a %*STR length, keep the
 *** text around the value intact, grow the buffer without losing that text,
 *** and fail when the output cannot fit.
 ***/
long long
test(char** tname)
    {
    *tname = "qprintf-69 &B64 and &DB64 filters";
    return loopTest(doTest) * (NCASES + 4);
    }
