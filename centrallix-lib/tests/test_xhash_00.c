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
/* Module:	test_xhash_00.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test xhInit(), xhAdd(), xhLookup(), xhRemove(), and	*/
/*		xhDeInit() on a table keyed by NUL terminated strings.	*/
/*									*/
/*		Unsupported cases skipped by the test suite:		*/
/*		- a table with zero rows: divides by zero.		*/
/*		- a key shorter than a nonzero key length: over reads.  */
/*		- xhDeInit() on a table with entries: leaks them.	*/
/*		- xhAdd() allocation failure: needs an inject to test.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"

/** Tested module. **/
#include "xhash.h"

#define HASH_ROWS	17
#define KEY_COUNT	5

static char* keys[KEY_COUNT] = {"alpha", "beta", "gamma", "delta", "epsilon"};
static char* data[KEY_COUNT] = {"one", "two", "three", "four", "five"};

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;

	/** A key length of zero selects NUL-terminated string keys. **/
	success &= ASSERT_EQL(xhInit(&hash, HASH_ROWS, 0), 0, "%d");
	success &= ASSERT_EQL(hash.nRows, HASH_ROWS, "%d");
	success &= ASSERT_EQL(hash.KeyLen, 0, "%d");
	success &= ASSERT_EQL(hash.nItems, 0, "%d");

	/** Every row starts out empty. **/
	success &= ASSERT_EQL(xaCount(&hash.Rows), HASH_ROWS, "%d");
	int used_rows = 0;
	for (int i = 0; i < HASH_ROWS; i++)
	    if (hash.Rows.Items[i] != NULL) used_rows++;
	success &= ASSERT_EQL(used_rows, 0, "%d");

	/** An empty table has nothing to find or remove. **/
	success &= ASSERT_EQL(xhLookup(&hash, keys[0]), NULL, "%p");
	success &= ASSERT_EQL(xhRemove(&hash, keys[0]), -1, "%d");

	/** Add each pair.  The table stores the pointers it is given. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    {
	    success &= ASSERT_EQL(xhAdd(&hash, keys[i], data[i]), 0, "%d");
	    success &= ASSERT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");
	    success &= ASSERT_EQL(hash.nItems, i + 1, "%d");
	    }

	/** Every entry survives the additions that follow it. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    success &= ASSERT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");

	/** Keys are matched by content rather than by pointer. **/
	char key_copy[16];
	strcpy(key_copy, keys[0]);
	success &= ASSERT_EQL(xhLookup(&hash, key_copy), data[0], "%p");

	/** Neither a prefix nor an extension of a key matches it. **/
	success &= ASSERT_EQL(xhLookup(&hash, "alph"), NULL, "%p");
	success &= ASSERT_EQL(xhLookup(&hash, "alphabet"), NULL, "%p");

	/** A duplicate key is rejected and leaves the existing entry alone. **/
	success &= ASSERT_EQL(xhAdd(&hash, key_copy, "duplicate"), -1, "%d");
	success &= ASSERT_EQL(hash.nItems, KEY_COUNT, "%d");
	success &= ASSERT_EQL(xhLookup(&hash, keys[0]), data[0], "%p");

	/** Removing a key that was never added does not disturb the table. **/
	success &= ASSERT_EQL(xhRemove(&hash, "alphabet"), -1, "%d");
	success &= ASSERT_EQL(hash.nItems, KEY_COUNT, "%d");

	/** The empty string is a usable key. **/
	success &= ASSERT_EQL(xhAdd(&hash, "", data[0]), 0, "%d");
	success &= ASSERT_EQL(xhLookup(&hash, ""), data[0], "%p");
	success &= ASSERT_EQL(xhRemove(&hash, ""), 0, "%d");
	success &= ASSERT_EQL(hash.nItems, KEY_COUNT, "%d");

	/** Remove each key.  A removed key cannot be found or removed again. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    {
	    success &= ASSERT_EQL(xhRemove(&hash, keys[i]), 0, "%d");
	    success &= ASSERT_EQL(xhLookup(&hash, keys[i]), NULL, "%p");
	    success &= ASSERT_EQL(xhRemove(&hash, keys[i]), -1, "%d");
	    success &= ASSERT_EQL(hash.nItems, KEY_COUNT - i - 1, "%d");
	    }

	/** Two tables are independent, even when given the same keys. **/
	XHashTable other;
	success &= ASSERT_EQL(xhInit(&other, HASH_ROWS, 0), 0, "%d");
	success &= ASSERT_EQL(xhAdd(&hash, keys[0], data[0]), 0, "%d");
	success &= ASSERT_EQL(xhAdd(&other, keys[0], data[1]), 0, "%d");
	success &= ASSERT_EQL(xhLookup(&hash, keys[0]), data[0], "%p");
	success &= ASSERT_EQL(xhLookup(&other, keys[0]), data[1], "%p");
	success &= ASSERT_EQL(xhRemove(&other, keys[0]), 0, "%d");
	success &= ASSERT_EQL(xhLookup(&hash, keys[0]), data[0], "%p");
	success &= ASSERT_EQL(hash.nItems, 1, "%d");
	success &= ASSERT_EQL(other.nItems, 0, "%d");
	success &= ASSERT_EQL(xhDeInit(&other), 0, "%d");

	/** Clean up. **/
	success &= ASSERT_EQL(xhRemove(&hash, keys[0]), 0, "%d");
	success &= ASSERT_EQL(xhDeInit(&hash), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-00 String Keys";
    return loopTest(doTest) * (32ll + 8ll * KEY_COUNT);
    }

/** Scope cleanup. **/
#undef HASH_ROWS
#undef KEY_COUNT
