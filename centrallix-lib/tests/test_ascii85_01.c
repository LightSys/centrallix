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
/* Module:	test_ascii85_01.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 13th, 2026					*/
/* Description:	Test a85EncodedSizeMax(), encoding in pieces, and	*/
/*		buffers that are too small.				*/
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
#include "ascii85.h"

/** Largest input length swept, and buffers large enough for any output. **/
#define MAX_DATA	64
#define BUF_SIZE	(MAX_DATA * 2)

static unsigned int seed_counter = 0;

/** Check the size helper and encoder for one input. **/
static bool
checkData(const unsigned char* data, size_t len, size_t piece_len)
    {
    bool success = true;
    char whole[BUF_SIZE];
    char pieces[BUF_SIZE];
    unsigned char buf[BUF_SIZE];

	/** Encode in one call. **/
	const size_t max_size = a85EncodedSizeMax(len);
	const ssize_t enc_len = a85Encode(whole, sizeof(whole), data, len);
	if (!ASSERT_RANGE(enc_len, (ssize_t)0, (ssize_t)BUF_SIZE - 1, "%zd")) return false;

	/** a85EncodedSizeMax() is exact, less 4 for each zero group. **/
	size_t n_zero = 0;
	for (size_t i = 0; i + 4 <= len; i += 4)
	    if (memcmp(data + i, "\0\0\0\0", 4) == 0) n_zero++;
	success &= ASSERT_EQL((size_t)enc_len + 1 + n_zero * 4, max_size, "%zu");

	/** Encoding in pieces of a multiple of 4 bytes gives the same text. **/
	size_t pos = 0;
	for (size_t i = 0; i < len; i += piece_len)
	    {
	    const size_t this_len = (len - i < piece_len) ? len - i : piece_len;
	    const ssize_t rval = a85Encode(pieces + pos, sizeof(pieces) - pos, data + i, this_len);
	    if (!ASSERT_RANGE(rval, (ssize_t)0, (ssize_t)(sizeof(pieces) - pos - 1), "%zd")) return false;
	    pos += (size_t)rval;
	    }
	pieces[pos] = '\0';
	success &= ASSERT_STR_EQL(pieces, whole);

	/** A dst one byte smaller than a85EncodedSizeMax() fails and is untouched. **/
	memset(buf, 0xAA, sizeof(buf));
	success &= ASSERT_EQL(a85Encode((char*)buf, max_size - 1, data, len), (ssize_t)-1, "%zd");
	for (size_t i = 0; i < sizeof(buf); i++)
	    success &= ASSERT_EQL(buf[i], 0xAA, "%d");

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;
    unsigned char data[MAX_DATA];

	/** Random nonzero bytes, with about every third 4-byte group all zeros. **/
	srand(seed_counter++);
	for (size_t i = 0; i < MAX_DATA; i++) data[i] = (unsigned char)(1 + rand() % 255);
	for (size_t i = 0; i + 4 <= MAX_DATA; i += 4)
	    if (rand() % 3 == 0) memset(data + i, 0, 4);

	/** Sweep every length, in pieces of 4, 8, or 12 bytes. **/
	for (size_t len = 0; len <= MAX_DATA; len++)
	    success &= checkData(data, len, 4 * (1 + len % 3));

	/** An encoding of exactly SSIZE_MAX characters fits, since SSIZE_MAX % 5 == 2. **/
	const size_t g = (size_t)SSIZE_MAX / 5;
	success &= ASSERT_EQL(a85EncodedSizeMax(g * 4), g * 5 + 1, "%zu");
	success &= ASSERT_EQL(a85EncodedSizeMax(g * 4 + 1), (size_t)SSIZE_MAX + 1, "%zu");

	/** Sizes that do not fit in a ssize_t give a size of 0. **/
	success &= ASSERT_EQL(a85EncodedSizeMax(g * 4 + 2), (size_t)0, "%zu");
	success &= ASSERT_EQL(a85EncodedSizeMax(SIZE_MAX), (size_t)0, "%zu");

    return success;
    }

/*** This test verifies that a85EncodedSizeMax() bounds the encoding and is
 *** exact without zero groups, that encoding in pieces of a multiple of 4
 *** bytes matches one call, that a smaller dst fails without being touched,
 *** and that sizes too large for a ssize_t give 0.
 ***/
long long
test(char** tname)
    {
    *tname = "ascii85-01 a85EncodedSizeMax() and encoding in pieces";
    return loopTest(doTest) * (MAX_DATA + 1 + 4);
    }
