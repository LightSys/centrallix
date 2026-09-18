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
/* Module:	test_xhash_01.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test a hash table keyed by fixed length binary keys,	*/
/* 		which may contain NUL bytes and need not be		*/
/* 		NUL terminated.						*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"

/** Tested module. **/
#include "xhash.h"

#define HASH_ROWS	7
#define KEY_LEN		4
#define KEY_COUNT	8

/*** Keys deliberately include embedded NUL bytes, high bytes, and pairs that
 *** a NUL terminated comparison would consider equal.
 ***/
static char keys[KEY_COUNT][KEY_LEN] =
    {
	{'k', 'e', 'y', '0'},
	{'k', 'e', 'y', '1'},
	{'\0', '\0', '\0', '\0'},
	{'\0', '\0', '\0', '\1'},
	{'a', '\0', 'b', '\0'},
	{'a', '\0', 'b', '\1'},
	{'\xff', '\x80', '\xfe', '\x7f'},
	{'\xff', '\x80', '\xfe', '\xff'},
    };

static char* data[KEY_COUNT] =
    {"zero", "one", "two", "three", "four", "five", "six", "seven"};

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;

	/** A nonzero key length selects fixed length binary keys. **/
	success &= EXPECT_EQL(check(xhInit(&hash, HASH_ROWS, KEY_LEN)), 0, "%d");
	success &= EXPECT_EQL(hash.KeyLen, KEY_LEN, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** Add each pair. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    {
	    success &= EXPECT_EQL(check(xhAdd(&hash, keys[i], data[i])), 0, "%d");
	    success &= EXPECT_EQL(hash.nItems, i + 1, "%d");
	    }

	/** Keys that differ only past a NUL byte are separate entries. **/
	for (int i = 0; i < KEY_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");

	/** Only the first KEY_LEN bytes of a key are examined. **/
	char long_key[KEY_LEN * 2];
	memcpy(long_key, keys[0], KEY_LEN);
	memset(long_key + KEY_LEN, 'x', KEY_LEN);
	success &= EXPECT_EQL(xhLookup(&hash, long_key), data[0], "%p");
	success &= EXPECT_EQL(xhAdd(&hash, long_key, "duplicate"), -1, "%d");
	success &= EXPECT_EQL(hash.nItems, KEY_COUNT, "%d");

	/** A key differing in its last examined byte is a different key. **/
	char other_key[KEY_LEN];
	memcpy(other_key, keys[0], KEY_LEN);
	other_key[KEY_LEN - 1] = 'Z';
	success &= EXPECT_EQL(xhLookup(&hash, other_key), NULL, "%p");
	success &= EXPECT_EQL(xhRemove(&hash, other_key), -1, "%d");

	/** Removing by an equal copy of a key removes the original entry. **/
	success &= EXPECT_EQL(check(xhRemove(&hash, long_key)), 0, "%d");
	success &= EXPECT_EQL(xhLookup(&hash, keys[0]), NULL, "%p");
	success &= EXPECT_EQL(hash.nItems, KEY_COUNT - 1, "%d");

	/** The other entries are untouched by that removal. **/
	for (int i = 1; i < KEY_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]), data[i], "%p");

	/** Clean up. **/
	success &= EXPECT_EQL(check(xhClear(&hash, NULL, NULL)), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");
	success &= EXPECT_EQL(check(xhDeInit(&hash)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-01 Fixed Length Keys";
    return loopTest(doTest) * (13ll + 4ll * KEY_COUNT);
    }

/** Scope cleanup. **/
#undef HASH_ROWS
#undef KEY_LEN
#undef KEY_COUNT
