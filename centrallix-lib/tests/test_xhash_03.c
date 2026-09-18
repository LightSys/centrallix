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
/* Module:	test_xhash_03.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test xhClear(), including the free function it calls	*/
/*		for the data of each entry, and how a hash table	*/
/*		handles entries whose data is NULL.			*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "newmalloc.h"
#include "check.h"

/** Tested module. **/
#include "xhash.h"

#define HASH_ROWS	11
#define ITEM_COUNT	6
#define DATA_SIZE	((int)sizeof(int))

static char* keys[ITEM_COUNT] = {"one", "two", "three", "four", "five", "six"};

/** Free the data of an entry and count the entries xhClear() handed over. **/
static int test_free(void* data, void* arg)
    {
	if (data != NULL) nmFree(data, DATA_SIZE);
	if (arg != NULL) (*(unsigned int*)arg)++;

    return 0;
    }

static unsigned int seed_counter = 0;

static bool doTest(void)
    {
    bool success = true;
    XHashTable hash;
    unsigned int freed = 0;

	/** Use a distinct seed each pass so the data varies. **/
	srand(seed_counter++);

	success &= EXPECT_EQL(check(xhInit(&hash, HASH_ROWS, 0)), 0, "%d");

	/** Fill the table with data that xhClear() has to free. **/
	for (int i = 0; i < ITEM_COUNT; i++)
	    {
	    int* value = checkPtr(nmMalloc(DATA_SIZE));
	    if (value == NULL) return false;
	    *value = rand();
	    success &= EXPECT_EQL(check(xhAdd(&hash, keys[i], (char*)value)), 0, "%d");
	    char* found = xhLookup(&hash, keys[i]);
	    success &= EXPECT_EQL(found, (char*)value, "%p");
	    if (found != NULL) success &= EXPECT_EQL(*(int*)found, *value, "%d");
	    }
	success &= EXPECT_EQL(hash.nItems, ITEM_COUNT, "%d");

	/** An entry may carry NULL data; only its key marks its presence. **/
	success &= EXPECT_EQL(check(xhAdd(&hash, "null-data", NULL)), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, ITEM_COUNT + 1, "%d");
	success &= EXPECT_EQL(xhLookup(&hash, "null-data"), NULL, "%p");

	/** Such an entry is still found by a removal, and by a duplicate add. **/
	success &= EXPECT_EQL(xhAdd(&hash, "null-data", NULL), -1, "%d");
	success &= EXPECT_EQL(check(xhRemove(&hash, "null-data")), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, ITEM_COUNT, "%d");

	/*** Clear the table.  Every entry is handed to the free function,
	 *** including the one whose data is NULL.
	 ***/
	success &= EXPECT_EQL(check(xhAdd(&hash, "null-data", NULL)), 0, "%d");
	success &= EXPECT_EQL(check(xhClear(&hash, test_free, &freed)), 0, "%d");
	success &= EXPECT_EQL(freed, (unsigned int)ITEM_COUNT + 1, "%u");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** The cleared table holds nothing and every row is empty. **/
	for (int i = 0; i < ITEM_COUNT; i++)
	    success &= EXPECT_EQL(xhLookup(&hash, keys[i]), NULL, "%p");
	int used_rows = 0;
	for (int i = 0; i < HASH_ROWS; i++)
	    if (hash.Rows.Items[i] != NULL) used_rows++;
	success &= EXPECT_EQL(used_rows, 0, "%d");

	/** Clearing an already cleared table frees nothing. **/
	success &= EXPECT_EQL(check(xhClear(&hash, test_free, &freed)), 0, "%d");
	success &= EXPECT_EQL(freed, (unsigned int)ITEM_COUNT + 1, "%u");

	/** The table is usable again after being cleared. **/
	success &= EXPECT_EQL(check(xhAdd(&hash, keys[0], "reused")), 0, "%d");
	success &= EXPECT_STR_EQL(xhLookup(&hash, keys[0]), "reused");
	success &= EXPECT_EQL(hash.nItems, 1, "%d");

	/** A NULL free function leaves the data alone. **/
	success &= EXPECT_EQL(check(xhClear(&hash, NULL, NULL)), 0, "%d");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** A free function may be given a NULL argument. **/
	int* value = checkPtr(nmMalloc(DATA_SIZE));
	if (value == NULL) return false;
	success &= EXPECT_EQL(check(xhAdd(&hash, keys[0], (char*)value)), 0, "%d");
	success &= EXPECT_EQL(check(xhClear(&hash, test_free, NULL)), 0, "%d");
	success &= EXPECT_EQL(freed, (unsigned int)ITEM_COUNT + 1, "%u");
	success &= EXPECT_EQL(hash.nItems, 0, "%d");

	/** Clean up. **/
	success &= EXPECT_EQL(check(xhDeInit(&hash)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    *tname = "xhash-03 Clearing";
    return loopTest(doTest) * (25ll + 4ll * ITEM_COUNT);
    }

/** Scope cleanup. **/
#undef HASH_ROWS
#undef ITEM_COUNT
#undef DATA_SIZE
