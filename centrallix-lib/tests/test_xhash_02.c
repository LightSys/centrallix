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
/* Module:	test_xhash_02.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test how a hash table handles keys that land in the	*/
/*		same row, using a single row table to force every key	*/
/*		into one chain.						*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"

/** Tested module. **/
#include "xhash.h"

#define CHAIN_COUNT	8

static char* keys[CHAIN_COUNT] = {"k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7"};
static char* data[CHAIN_COUNT] = {"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"};

/*** The keys removed below: the head, the middle, and the tail.  Keep
 *** REMOVED_COUNT equal to the number of indices isRemoved() accepts; the
 *** test checks that it is.
 ***/
#define REMOVED_COUNT	3
static bool isRemoved(int i)
    {
    return (i == 0 || i == CHAIN_COUNT / 2 || i == CHAIN_COUNT - 1);
    }

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;

	/** One row means every key collides. **/
	success &= ASSERT_EQL(xhInit(&hash, 1, 0), 0, "%d");
	success &= ASSERT_EQL(hash.nItems, 0, "%d");

	/** Build the chain. **/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    {
	    success &= ASSERT_EQL(xhAdd(&hash, keys[i], data[i]), 0, "%d");
	    success &= ASSERT_EQL(hash.nItems, i + 1, "%d");
	    }

	/** Every key in the chain is reachable. **/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    success &= ASSERT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");

	/** A key absent from a populated chain is neither found nor removed. **/
	success &= ASSERT_EQL(xhLookup(&hash, "absent"), NULL, "%p");
	success &= ASSERT_EQL(xhRemove(&hash, "absent"), -1, "%d");
	success &= ASSERT_EQL(hash.nItems, CHAIN_COUNT, "%d");

	/** A duplicate is caught no matter how deep in the chain it sits. **/
	success &= ASSERT_EQL(xhAdd(&hash, keys[CHAIN_COUNT / 2], "duplicate"), -1, "%d");
	success &= ASSERT_EQL(hash.nItems, CHAIN_COUNT, "%d");

	/** Unlink the head, the middle, and the tail of the chain. **/
	int removed_count = 0;
	for (int i = 0; i < CHAIN_COUNT; i++)
	    if (isRemoved(i))
		{
		success &= ASSERT_EQL(xhRemove(&hash, keys[i]), 0, "%d");
		removed_count++;
		}
	success &= ASSERT_EQL(removed_count, REMOVED_COUNT, "%d");

	/** The other entries survive and the removed ones are gone. **/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    success &= ASSERT_EQL(xhLookup(&hash, keys[i]),
		isRemoved(i) ? NULL : data[i], "%p");
	success &= ASSERT_EQL(hash.nItems, CHAIN_COUNT - REMOVED_COUNT, "%d");

	/** A removed key can be added back. **/
	success &= ASSERT_EQL(xhAdd(&hash, keys[0], data[0]), 0, "%d");
	success &= ASSERT_EQL(xhLookup(&hash, keys[0]), data[0], "%p");
	success &= ASSERT_EQL(hash.nItems, CHAIN_COUNT - REMOVED_COUNT + 1, "%d");

	/** Clean up. **/
	success &= ASSERT_EQL(xhClear(&hash, NULL, NULL), 0, "%d");
	success &= ASSERT_EQL(hash.nItems, 0, "%d");
	success &= ASSERT_EQL(hash.Rows.Items[0], NULL, "%p");
	success &= ASSERT_EQL(xhDeInit(&hash), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-02 Collisions";
    return loopTest(doTest) * (16ll + REMOVED_COUNT + 4ll * CHAIN_COUNT);
    }

/** Scope cleanup. **/
#undef CHAIN_COUNT
#undef REMOVED_COUNT
