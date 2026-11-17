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
/* Module:	test_base64_07.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test encoding and decoding data in chunks, the way a	*/
/*		caller will to stream data using stateless functions.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "base64.h"

/** Largest input, and a buffer large enough for any of its encodings. **/
#define MAX_DATA	500
#define BUF_SIZE	(MAX_DATA * 3)

/** The MIME line length (RFC 2045), and the bytes that fill one line. **/
#define MIME_LINE	76
#define MIME_LINE_BYTES	(MIME_LINE / 4 * 3)

static unsigned int seed_counter = 0;

/*** Encode data in chunks of a random multiple of chunk_unit bytes, with only
 *** the final chunk shorter, and check that the pieces join into the same
 *** text as encoding it all at once.
 ***/
static bool
checkChunkedEncode(const unsigned char* data, size_t len, size_t chunk_unit, size_t line_len, int flags)
    {
    bool success = true;
    char whole[BUF_SIZE];
    char joined[BUF_SIZE];
    size_t joined_len = 0;

	const ssize_t whole_len = b64Encode(whole, sizeof(whole), data, (ssize_t)len, line_len, flags);
	if (!ASSERT_RANGE(whole_len, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;

	for (size_t off = 0; off < len; )
	    {
	    const size_t chunk = min(chunk_unit * (1 + (size_t)rand() % 4), len - off);
	    const ssize_t n = b64Encode(joined + joined_len, sizeof(joined) - joined_len, data + off, (ssize_t)chunk, line_len, flags);
	    if (!ASSERT_RANGE(n, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;
	    joined_len += (size_t)n;
	    off += chunk;
	    }
	joined[joined_len] = '\0';

	success &= ASSERT_EQL(joined_len, (size_t)whole_len, "%zu");
	success &= ASSERT_STR_EQL(joined, whole);

    return success;
    }

/*** Decode an encoding in chunks of a random multiple of 4 characters, and
 *** check that the pieces join into the original data.
 ***/
static bool
checkChunkedDecode(const unsigned char* data, size_t len)
    {
    bool success = true;
    char encoded[BUF_SIZE];
    unsigned char joined[BUF_SIZE];
    size_t joined_len = 0;

	const ssize_t enc_len = b64Encode(encoded, sizeof(encoded), data, (ssize_t)len, 0, B64_F_NONE);
	if (!ASSERT_RANGE(enc_len, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;

	for (size_t off = 0; off < (size_t)enc_len; )
	    {
	    const size_t chunk = min((size_t)(4 * (1 + rand() % 20)), (size_t)enc_len - off);
	    const ssize_t n = b64Decode(joined + joined_len, sizeof(joined) - joined_len, encoded + off, (ssize_t)chunk, B64_F_NONE);
	    if (!ASSERT_RANGE(n, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;
	    joined_len += (size_t)n;
	    off += chunk;
	    }

	success &= ASSERT_EQL(joined_len, len, "%zu");
	success &= ASSERT_TRUE(memcmp(joined, data, len) == 0);

    return success;
    }

static bool
doTest(void)
    {
    bool success = true;
    unsigned char data[MAX_DATA];

	/** Use a distinct seed each pass so the data and chunks vary. **/
	srand(seed_counter++);
	const size_t len = (size_t)rand() % (MAX_DATA + 1);
	for (size_t i = 0; i < len; i++) data[i] = (unsigned char)rand();

	/** Unwrapped encoding takes chunks of whole 3-byte groups. **/
	success &= checkChunkedEncode(data, len, 3, 0, B64_F_NONE);

	/** Wrapped encoding takes chunks of whole lines. **/
	success &= checkChunkedEncode(data, len, MIME_LINE_BYTES, MIME_LINE, B64_F_NONE);
	success &= checkChunkedEncode(data, len, MIME_LINE_BYTES, MIME_LINE, B64_F_CRLF);
	success &= checkChunkedEncode(data, len, 3, 4, B64_F_CRLF);

	/** Decoding takes chunks of whole 4-character groups. **/
	success &= checkChunkedDecode(data, len);

    return success;
    }

/*** This test verifies the chunking rule for streaming: encoding chunks of
 *** whole 3-byte groups (or whole lines when wrapping) and decoding chunks of
 *** whole 4-character groups gives the same result as one call on all of
 *** the data.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-07 encoding and decoding in chunks";
    return loopTest(doTest) * 5;
    }
