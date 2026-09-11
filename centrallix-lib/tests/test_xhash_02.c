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
#include "check.h"

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

/*** Walk the chain of a row, checking that it holds the expected keys in
 *** order.  xhAdd() appends to the end of the chain, so the keys come back
 *** in the order they were added.
 ***
 *** @param hash The hash table, which must have exactly one row.
 *** @param expect_keys The keys expected in the chain, in order.
 *** @param expect_count The number of expected keys.
 *** @returns true if the chain matched, false otherwise.
 ***/
static bool checkChain(pXHashTable hash, char** expect_keys, int expect_count)
    {
    bool success = true;
    pXHashEntry entry = XHE(hash->Rows.Items[0]);

	for (int i = 0; i < expect_count; i++)
	    {
	    if (!EXPECT_NOT_NULL(entry)) return false;
	    success &= EXPECT_EQL(entry->Key, expect_keys[i], "%p");
	    entry = entry->Next;
	    }
	success &= EXPECT_EQL(entry, NULL, "%p");

    return success;
    }

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;
    char* remaining[CHAIN_COUNT + 1];
    int remaining_count = 0;

	/** One row means every key collides. **/
	success &= EXPECT_EQL(check(xhInit(&hash, 1, 0)), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** Build the chain. **/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    {
	    success &= EXPECT_EQL(check(xhAdd(&hash, keys[i], data[i])), 0, "%d");
	    success &= EXPECT_EQL(hash.nItems, i + 1, "%d");
	    }
	success &= checkChain(&hash, keys, CHAIN_COUNT);

	/** Every key in the chain is reachable. **/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");

	/** A key absent from a populated chain is neither found nor removed. **/
	success &= EXPECT_EQL(xhLookup(&hash, "absent"), NULL, "%p");
	success &= EXPECT_EQL(xhRemove(&hash, "absent"), -1, "%d");
	success &= EXPECT_EQL(hash.nItems, CHAIN_COUNT, "%d");

	/** A duplicate is caught no matter how deep in the chain it sits. **/
	success &= EXPECT_EQL(xhAdd(&hash, keys[CHAIN_COUNT / 2], "duplicate"), -1, "%d");
	success &= EXPECT_EQL(hash.nItems, CHAIN_COUNT, "%d");

	/** Unlink the head, the middle, and the tail of the chain. **/
	int removed_count = 0;
	for (int i = 0; i < CHAIN_COUNT; i++)
	    {
	    if (isRemoved(i))
		{
		success &= EXPECT_EQL(check(xhRemove(&hash, keys[i])), 0, "%d");
		removed_count++;
		}
	    else
		remaining[remaining_count++] = keys[i];
	    }
	success &= EXPECT_EQL(removed_count, REMOVED_COUNT, "%d");

	/*** The chain kept the other entries, in order, and lost the removed
	 *** ones.
	 ***/
	for (int i = 0; i < CHAIN_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]),
		isRemoved(i) ? NULL : data[i], "%p");
	success &= EXPECT_EQL(hash.nItems, CHAIN_COUNT - REMOVED_COUNT, "%d");
	success &= checkChain(&hash, remaining, remaining_count);

	/** A removed key can be added back, landing at the end of the chain. **/
	success &= EXPECT_EQL(check(xhAdd(&hash, keys[0], data[0])), 0, "%d");
	success &= EXPECT_EQL(xhLookup(&hash, keys[0]), data[0], "%p");
	success &= EXPECT_EQL(hash.nItems, CHAIN_COUNT - REMOVED_COUNT + 1, "%d");
	remaining[remaining_count++] = keys[0];
	success &= checkChain(&hash, remaining, remaining_count);

	/** Clean up. **/
	success &= EXPECT_EQL(check(xhClear(&hash, NULL, NULL)), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");
	success &= EXPECT_EQL(hash.Rows.Items[0], NULL, "%p");
	success &= EXPECT_EQL(check(xhDeInit(&hash)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-02 Collisions";

    /*** The three chain walks check two things per entry plus the end of the
     *** chain, over chains of CHAIN_COUNT, CHAIN_COUNT - REMOVED_COUNT, and
     *** one more than that.
     ***/
    return loopTest(doTest) * (21ll - 3ll * REMOVED_COUNT + 10ll * CHAIN_COUNT);
    }

/** Scope cleanup. **/
#undef CHAIN_COUNT
#undef REMOVED_COUNT
