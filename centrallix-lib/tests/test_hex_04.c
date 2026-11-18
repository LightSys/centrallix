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
/* Module:	test_hex_04.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 17th, 2026					*/
/* Description:	Test the size helpers hexEncodedSize() and		*/
/*		hexDecodedSize() against the output of the functions	*/
/*		they predict.						*/
/************************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "hex.h"

/** Largest input length swept, and buffers large enough for any output. **/
#define MAX_DATA	100
#define BUF_SIZE	(MAX_DATA * 2 + 1)

static unsigned int seed_counter = 0;

/** Check both helpers for one input length. **/
static bool
checkSizes(const unsigned char* data, size_t len)
    {
    bool success = true;
    char encoded[BUF_SIZE];
    char buf[BUF_SIZE];
    unsigned char decoded[BUF_SIZE];

	/** hexEncodedSize() is exact, including the null. **/
	const size_t enc_size = hexEncodedSize(len);
	const ssize_t enc_len = hexEncode(encoded, sizeof(encoded), data, (ssize_t)len, HEX_F_NONE);
	if (!ASSERT_RANGE(enc_len, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;
	success &= ASSERT_EQL(enc_size, (size_t)enc_len + 1, "%zu");

	/** An exact-size dst works, and one byte less fails without touching dst. **/
	success &= ASSERT_EQL(hexEncode(buf, enc_size, data, (ssize_t)len, HEX_F_NONE), enc_len, "%zd");
	success &= ASSERT_STR_EQL(buf, encoded);
	memset(buf, 0xAA, sizeof(buf));
	success &= ASSERT_EQL(hexEncode(buf, enc_size - 1, data, (ssize_t)len, HEX_F_NONE), (ssize_t)-1, "%zd");
	for (size_t i = 0; i < sizeof(buf); i++)
	    success &= ASSERT_EQL((unsigned char)buf[i], 0xAA, "%d");

	/** hexDecodedSize() is exact, including the null. **/
	const size_t dec_size = hexDecodedSize((size_t)enc_len);
	success &= ASSERT_EQL(dec_size, len + 1, "%zu");
	success &= ASSERT_EQL(hexDecode(decoded, dec_size, encoded, enc_len, HEX_F_NONE), (ssize_t)len, "%zd");
	success &= ASSERT_TRUE(memcmp(decoded, data, len) == 0);
	success &= ASSERT_EQL(hexDecode(decoded, dec_size - 1, encoded, enc_len, HEX_F_NONE), (ssize_t)-1, "%zd");

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;
    unsigned char data[MAX_DATA];

	/** Use a distinct seed each pass so the data varies. **/
	srand(seed_counter++);
	for (size_t i = 0; i < MAX_DATA; i++) data[i] = (unsigned char)rand();

	/** Sweep every length. **/
	for (size_t len = 0; len <= MAX_DATA; len++)
	    success &= checkSizes(data, len);

	/** Sizes that do not fit in a ssize_t give a size of 0, even if they fit in a size_t. **/
	success &= ASSERT_EQL(hexEncodedSize(((size_t)SSIZE_MAX - 1) / 2), (size_t)SSIZE_MAX, "%zu");
	success &= ASSERT_EQL(hexEncodedSize(((size_t)SSIZE_MAX - 1) / 2 + 1), (size_t)0, "%zu");
	success &= ASSERT_EQL(hexEncodedSize(SIZE_MAX), (size_t)0, "%zu");
	success &= ASSERT_EQL(hexDecodedSize(SIZE_MAX), SIZE_MAX / 2 + 1, "%zu");

    return success;
    }

/*** This test verifies that hexEncodedSize() and hexDecodedSize() give the
 *** exact buffer size their functions need, that a buffer one byte smaller
 *** fails, and that hexEncodedSize() reports 0 for sizes that do not fit in a
 *** ssize_t.
 ***/
long long
test(char** tname)
    {
    *tname = "hex-04 size helpers";
    return loopTest(doTest) * (MAX_DATA + 1 + 4);
    }
