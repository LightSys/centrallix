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
/* Module:	test_xhash_04.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test that a hash table holding far more keys than it	*/
/*		has rows still finds and removes every one of them.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"

/** Tested module. **/
#include "xhash.h"

#define HASH_ROWS	64
#define KEY_COUNT	256
#define KEY_SIZE	16

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;
    char keys[KEY_COUNT][KEY_SIZE];
    int used_rows = 0;
    int i;

	success &= EXPECT_EQL(check(xhInit(&hash, HASH_ROWS, 0)), 0, "%d");

	/*** Each key doubles as its own data, giving every entry a unique
	 *** pointer to look up.  The keys share the stack of an mtask thread,
	 *** which is far smaller than a process stack.
	 ***/
	for (int i = 0; i < KEY_COUNT; i++)
	    {
	    snprintf(keys[i], sizeof(keys[i]), "spread-key-%d", i);
	    success &= EXPECT_EQL(check(xhAdd(&hash, keys[i], keys[i])), 0, "%d");
	    }
	success &= EXPECT_EQL(hash.nItems, KEY_COUNT, "%d");

	/** Nothing was lost or confused with another key. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]), (char*)keys[i], "%p");

	/** Removing every key empties every row. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    success &= EXPECT_EQL(check(xhRemove(&hash, keys[i])), 0, "%d");
	for (int i = 0; i < HASH_ROWS; i++)
	    if (hash.Rows.Items[i] != NULL) used_rows++;
	success &= EXPECT_EQL(used_rows, 0, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** Clean up. **/
	success &= EXPECT_EQL(check(xhDeInit(&hash)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-04 Many Keys";
    return loopTest(doTest) * (5ll + 3ll * KEY_COUNT);
    }

/** Scope cleanup. **/
#undef HASH_ROWS
#undef KEY_COUNT
#undef KEY_SIZE
