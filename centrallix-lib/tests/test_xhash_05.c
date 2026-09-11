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
/* Module:	test_xhash_05.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test xhInitialize() and the hash computation itself.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"

/** Tested module. **/
#include "xhash.h"

/*** These are internal functions, so we use forward forward declarations to
 *** make them available for testing.
 ***/
int xhInitialize();
int xh_internal_ComputeHash(char* key, int keylen, int rows);

#define KEY_LEN		8
#define ROW_COUNT	5

/** Row counts to hash against, including the degenerate single row table. **/
static int rows[ROW_COUNT] = {1, 2, 17, 64, 1000003};

/*** Enough rows that two keys landing on the same row would mean the hash
 *** ignored the difference between them rather than that the rows ran out.
 ***/
#define WIDE_ROWS	1000003

static char key[KEY_LEN] = {'h', 'a', 's', 'h', 'k', 'e', 'y', '\xf0'};

static bool doTest(void)
    {
    bool success = true;

	/*** The hash never leaves the rows it was given, and survives
	 *** xhInitialize(), which once rebuilt the table the hash reads from.
	 ***/
	for (int r = 0; r < ROW_COUNT; r++)
	    {
	    int hash = xh_internal_ComputeHash(key, KEY_LEN, rows[r]);
	    success &= EXPECT_RANGE(hash, 0, rows[r] - 1, "%d");
	    success &= EXPECT_EQL(check(xhInitialize()), 0, "%d");
	    success &= EXPECT_EQL(xh_internal_ComputeHash(key, KEY_LEN, rows[r]), hash, "%d");

	    /** An empty key hashes to the first row of any table. **/
	    success &= EXPECT_EQL(xh_internal_ComputeHash(key, 0, rows[r]), 0, "%d");
	    }

	/*** The hash is in memory only, so it may be changed, but only on
	 *** purpose: update this value when the hash or the key above changes.
	 ***/
	success &= EXPECT_EQL(xh_internal_ComputeHash(key, KEY_LEN, WIDE_ROWS), 609235, "%d");

	/** Where a byte sits matters, not just which bytes are present. **/
	success &= EXPECT_EQL(xh_internal_ComputeHash("ab", 2, WIDE_ROWS)
	    != xh_internal_ComputeHash("ba", 2, WIDE_ROWS), true, "%d");

	/** Only the first keylen bytes of the key are hashed. **/
	char altered[KEY_LEN];
	memcpy(altered, key, KEY_LEN);
	altered[KEY_LEN - 1] ^= 0x5A;
	success &= EXPECT_EQL(
	    xh_internal_ComputeHash(altered, KEY_LEN - 1, WIDE_ROWS),
	    xh_internal_ComputeHash(key, KEY_LEN - 1, WIDE_ROWS), "%d");

	/** Every byte of the key changes the hash. **/
	const int base = xh_internal_ComputeHash(key, KEY_LEN, WIDE_ROWS);
	for (int i = 0; i < KEY_LEN; i++)
	    {
	    memcpy(altered, key, KEY_LEN);
	    altered[i] ^= 0x5A;
	    bool differs = EXPECT_EQL(
		xh_internal_ComputeHash(altered, KEY_LEN, WIDE_ROWS) != base, true, "%d");
	    if (!differs) fprintf(stderr, "  > Byte %d of the key did not affect the hash.\n", i);
	    success &= differs;
	    }

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-05 Hash Computation";
    return loopTest(doTest) * (3ll + 4ll * ROW_COUNT + KEY_LEN);
    }

/** Scope cleanup. **/
#undef KEY_LEN
#undef ROW_COUNT
#undef WIDE_ROWS
