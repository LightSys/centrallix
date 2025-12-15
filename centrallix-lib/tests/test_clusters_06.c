/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_clusters_06.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 26th, 2025					*/
/* Description:	Test the searching functions from clusters.h.		*/
/************************************************************************/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "util.h"

/** Tested module. **/
#include "clusters.h"


static int cmp_dups(const void* v1, const void* v2)
    {
    const Dup* dup1 = v1;
    const Dup* dup2 = v2;
    const int r = strcmp(dup1->key1, dup2->key1);
    return (r != 0) ? r : strcmp(dup1->key2, dup2->key2);
    }

#define EXPECT_DUP(dup, k1, k2, sim_min, sim_max) \
    ({ \
	bool success = true; \
	pDup d = (dup); \
	success &= EXPECT_STR_EQL(d->key1, k1); \
	success &= EXPECT_STR_EQL(d->key2, k2); \
	success &= EXPECT_RANGE(d->similarity, sim_min, sim_max, "%g"); \
	success; \
    })
    

static bool do_tests(void)
    {
    bool success = true;
    
	/** Allocate some test data. **/
	void* data[] = {
	    "string",
	    "string2",
	    "str",
	    "hello world",
	    "data",
	    "string3",
	};
	void* keys[] = { "1", "2", "3", "4", "5", "6" };
	
	/** Check error cases. **/
	success &= EXPECT_EQL(ca_complete_search(NULL, 6, ca_lev_compare, 0.8, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 0, ca_lev_compare, 0.8, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, NULL, 0.8, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, 1.1, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, -0.1, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, INFINITY, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, -INFINITY, NULL, NULL), NULL, "%p");
	success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, NAN, NULL, NULL), NULL, "%p");
	
	/** Test complete search. **/
	{
	    XArray xdups;
	    if (!check(xaInit(&xdups, 4))) return false;
	    success &= EXPECT_EQL(ca_complete_search(data, 6, ca_lev_compare, 0.8, keys, &xdups), &xdups, "%p");
	    pDup* dups = (pDup*)xdups.Items;
	    for (unsigned int i = 0u; i < xdups.nItems; i++)
		{
		pDup cur = dups[i];
		if (cur->key1 > cur->key2)
		    {
		    char* temp = cur->key1;
		    cur->key1 = cur->key2;
		    cur->key2 = temp;
		    }
		}
	    qsort(dups, xdups.nItems, sizeof(pDup), cmp_dups);
	    success &= EXPECT_EQL(xdups.nItems, 3, "%d");
	    success &= EXPECT_DUP(dups[0], "1", "2", 0.8, 1.0);
	    success &= EXPECT_DUP(dups[1], "1", "6", 0.8, 1.0);
	    success &= EXPECT_DUP(dups[2], "2", "6", 0.8, 1.0);
	}
	
	/** Test sliding search: Large window. **/
	{
	    XArray xdups;
	    if (!check(xaInit(&xdups, 4))) return false;
	    success &= EXPECT_EQL(ca_sliding_search(data, 6, 5, ca_lev_compare, 0.8, keys, &xdups), &xdups, "%p");
	    pDup* dups = (pDup*)xdups.Items;
	    for (unsigned int i = 0u; i < xdups.nItems; i++)
		{
		pDup cur = dups[i];
		if (cur->key1 > cur->key2)
		    {
		    char* temp = cur->key1;
		    cur->key1 = cur->key2;
		    cur->key2 = temp;
		    }
		}
	    qsort(dups, xdups.nItems, sizeof(pDup), cmp_dups);
	    success &= EXPECT_EQL(xdups.nItems, 2, "%d");
	    success &= EXPECT_DUP(dups[0], "1", "2", 0.8, 1.0);
	    // success &= EXPECT_DUP(dups[1], "1", "6", 0.8, 1.0); /* Missed. */
	    success &= EXPECT_DUP(dups[1], "2", "6", 0.8, 1.0);
	}
	
	/** Test sliding search: Small window. **/
	{
	    XArray xdups;
	    if (!check(xaInit(&xdups, 4))) return false;
	    success &= EXPECT_EQL(ca_sliding_search(data, 6, 2, ca_lev_compare, 0.8, keys, &xdups), &xdups, "%p");
	    pDup* dups = (pDup*)xdups.Items;
	    for (unsigned int i = 0u; i < xdups.nItems; i++)
		{
		pDup cur = dups[i];
		if (cur->key1 > cur->key2)
		    {
		    char* temp = cur->key1;
		    cur->key1 = cur->key2;
		    cur->key2 = temp;
		    }
		}
	    qsort(dups, xdups.nItems, sizeof(pDup), cmp_dups);
	    success &= EXPECT_EQL(xdups.nItems, 1, "%d");
	    success &= EXPECT_DUP(dups[0], "1", "2", 0.8, 1.0);
	    // success &= EXPECT_DUP(dups[1], "1", "6", 0.8, 1.0); /* Missed. */
	    // success &= EXPECT_DUP(dups[2], "2", "6", 0.8, 1.0); /* Missed. */
	}
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-06 Searching";
    return loop_tests(do_tests);
    }
