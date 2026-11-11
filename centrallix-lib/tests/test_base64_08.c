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
/* Module:	test_base64_08.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 11th, 2026					*/
/* Description:	Test that b64Decode() reverses b64Encode() on every	*/
/*		byte value and on random data.				*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "t_utils.h"

/** Tested module. **/
#include "base64.h"

/** Largest round trip input, and a buffer that holds its encoding. **/
#define MAX_DATA	300
#define MAX_ENC		(MAX_DATA / 3 * 4 + 4 + 1)

static unsigned int seed_counter = 0;

/** Round trip every byte value, then random data of a random length. **/
static bool
doTest(void)
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

/*** This test verifies that decoding the output of b64Encode() gives back
 *** the original data, for every byte value and for random data of a random
 *** length.
 ***/
long long
test(char** tname)
    {
    *tname = "base64-08 round trip";
    return loopTest(doTest) * 2;
    }
