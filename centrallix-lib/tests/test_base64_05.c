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
/* Module:	test_base64_05.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 10th, 2026					*/
/* Description:	Test the size helpers b64EncodedSize(),			*/
/*		b64WrappedSize(), and b64DecodedSizeMax() against the	*/
/*		output of the functions they size for.			*/
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
#include "base64.h"

/** Largest input length swept, and buffers large enough for any output. **/
#define MAX_DATA	100
#define BUF_SIZE	(MAX_DATA * 4)

/** Line lengths and flags swept, 0 meaning no wrapping. **/
static const size_t line_lens[] = { 0, 4, 8, 76 };
static const int flag_sets[] = { B64_F_NONE, B64_F_CRLF };
#define N_LINE_LENS	(sizeof(line_lens) / sizeof(line_lens[0]))
#define N_FLAG_SETS	(sizeof(flag_sets) / sizeof(flag_sets[0]))

static unsigned int seed_counter = 0;

/** Encode exactly into dst_size bytes of a guarded buffer, which must fail. **/
static bool
checkTooSmall(const unsigned char* data, size_t len, size_t line_len, int flags, size_t dst_size)
    {
    bool success = true;
    unsigned char buf[BUF_SIZE];

	memset(buf, 0xAA, sizeof(buf));
	success &= ASSERT_EQL(b64Encode((char*)buf, dst_size, data, (ssize_t)len, line_len, flags), (ssize_t)-1, "%zd");

	/** A failed encode leaves dst untouched. **/
	for (size_t i = 0; i < sizeof(buf); i++)
	    success &= ASSERT_EQL(buf[i], 0xAA, "%d");

    return success;
    }

/** Check every helper for one input length, line length, and flag set. **/
static bool
checkSizes(const unsigned char* data, size_t len, size_t line_len, int flags)
    {
    bool success = true;
    char encoded[BUF_SIZE];
    char unwrapped[BUF_SIZE];
    unsigned char decoded[BUF_SIZE];

	/** b64EncodedSize() is exact, including the null. **/
	const size_t enc_size = b64EncodedSize(len, line_len, flags);
	const ssize_t enc_len = b64Encode(encoded, sizeof(encoded), data, (ssize_t)len, line_len, flags);
	if (!ASSERT_RANGE(enc_len, (ssize_t)0, (ssize_t)BUF_SIZE, "%zd")) return false;
	success &= ASSERT_EQL(enc_size, (size_t)enc_len + 1, "%zu");

	/** An exact-size dst works, and one byte less fails. **/
	char exact[BUF_SIZE];
	success &= ASSERT_EQL(b64Encode(exact, enc_size, data, (ssize_t)len, line_len, flags), enc_len, "%zd");
	success &= ASSERT_STR_EQL(exact, encoded);
	success &= checkTooSmall(data, len, line_len, flags, enc_size - 1);

	/** b64WrappedSize() is exact for wrapping the unwrapped encoding. **/
	const ssize_t unwrapped_len = b64Encode(unwrapped, sizeof(unwrapped), data, (ssize_t)len, 0, B64_F_NONE);
	if (line_len > 0)
	    {
	    success &= ASSERT_EQL(b64WrappedSize((size_t)unwrapped_len, line_len, flags), enc_size, "%zu");
	    char wrapped[BUF_SIZE];
	    success &= ASSERT_EQL(b64Wrap(wrapped, enc_size, unwrapped, unwrapped_len, line_len, flags), enc_len, "%zd");
	    success &= ASSERT_EQL(b64Wrap(wrapped, enc_size - 1, unwrapped, unwrapped_len, line_len, flags), (ssize_t)-1, "%zd");
	    }

	/** b64DecodedSizeMax() is enough to decode, with or without line endings. **/
	const ssize_t dec_len = b64Decode(decoded, b64DecodedSizeMax((size_t)enc_len), encoded, enc_len, B64_F_SKIP_INVALID);
	success &= ASSERT_EQL(dec_len, (ssize_t)len, "%zd");
	success &= ASSERT_TRUE(memcmp(decoded, data, len) == 0);

	/** Without padding too. **/
	size_t unpadded_len = (size_t)unwrapped_len;
	while (unpadded_len > 0 && unwrapped[unpadded_len - 1] == '=') unpadded_len--;
	success &= ASSERT_EQL(b64Decode(decoded, b64DecodedSizeMax(unpadded_len), unwrapped, (ssize_t)unpadded_len, B64_F_NONE), (ssize_t)len, "%zd");

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

	/** Sweep every length, line length, and flag set. **/
	for (size_t len = 0; len <= MAX_DATA; len++)
	    for (size_t l = 0; l < N_LINE_LENS; l++)
		for (size_t f = 0; f < N_FLAG_SETS; f++)
		    success &= checkSizes(data, len, line_lens[l], flag_sets[f]);

	/** Arguments the functions would reject give a size of 0. **/
	success &= ASSERT_EQL(b64EncodedSize(6, 6, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64EncodedSize(6, 4, B64_F_SKIP_INVALID), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64WrappedSize(6, 0, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64WrappedSize(6, 4, B64_F_SKIP_INVALID), (size_t)0, "%zu");

	/** Sizes that do not fit in a ssize_t give a size of 0, even if they fit in a size_t. **/
	success &= ASSERT_EQL(b64EncodedSize((size_t)SSIZE_MAX / 4 * 3 + 3, 0, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64WrappedSize((size_t)SSIZE_MAX, 1, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64EncodedSize(SIZE_MAX, 0, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64EncodedSize(SIZE_MAX / 4 * 3, 4, B64_F_CRLF), (size_t)0, "%zu");
	success &= ASSERT_EQL(b64WrappedSize(SIZE_MAX, 1, B64_F_NONE), (size_t)0, "%zu");
	success &= ASSERT_RANGE(b64DecodedSizeMax(SIZE_MAX), (size_t)1, SIZE_MAX, "%zu");

    return success;
    }

/*** This test verifies that b64EncodedSize() and b64WrappedSize() give the
 *** exact buffer size their functions need, that a buffer one byte smaller
 *** fails without being touched, that b64DecodedSizeMax() is always enough,
 *** and that the exact helpers report 0 for arguments that cannot be sized.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-05 size helpers";
    return loopTest(doTest) * ((MAX_DATA + 1) * N_LINE_LENS * N_FLAG_SETS + 10);
    }
