/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2025-2026 LightSys Technology Services, Inc.		*/
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
#include "check.h"
#include "newmalloc.h"
#include "test_utils.h"

/** Tested module. **/
#include "clusters.h"


static int cmp_Pairs(const void* v1, const void* v2)
    {
    const Pair* Pair1 = v1;
    const Pair* Pair2 = v2;
    return Pair1->i - Pair2->i;
    }

static void freePairs(pXArray xPairs)
    {
	/** The search functions hand ownership of each pair to the caller. **/
	while (xPairs->nItems > 0)
	    nmFree(xPairs->Items[--xPairs->nItems], sizeof(Pair));
	check(xaDeInit(xPairs));
    }

#define EXPECT_PAIR(Pair, k1, k2, sim_min, sim_max) \
    ({ \
	bool success = true; \
	pPair d = (Pair); \
	success &= EXPECT_EQL(d->i, k1, "%u"); \
	success &= EXPECT_EQL(d->j, k2, "%u"); \
	success &= EXPECT_RANGE(d->similarity, sim_min, sim_max, "%g"); \
	success; \
    })
    

static bool doTest(void)
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
	
	/** Check error cases. **/
	success &= EXPECT_EQL(caCompleteSearch(NULL, 6, caLevCompare,  0.8,      NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 0, caLevCompare,  0.8,      NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, NULL,            0.8,      NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare,  1.1,      NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare, -0.1,      NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare,  INFINITY, NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare, -INFINITY, NULL), NULL, "%p");
	success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare,  NAN,      NULL), NULL, "%p");
	
	/** Test complete search. **/
	{
	    XArray xPairs;
	    if (check(xaInit(&xPairs, 4)) != 0) return false;
	    success &= EXPECT_EQL(caCompleteSearch(data, 6, caLevCompare, 0.8, &xPairs), &xPairs, "%p");
	    pPair* Pairs = (pPair*)xPairs.Items;
	    for (unsigned int i = 0u; i < xPairs.nItems; i++)
		{
		pPair cur = Pairs[i];
		if (cur->i > cur->j)
		    {
		    unsigned int temp = cur->i;
		    cur->i = cur->j;
		    cur->j = temp;
		    }
		}
	    qsort(Pairs, xPairs.nItems, sizeof(pPair), cmp_Pairs);
	    success &= EXPECT_EQL(xPairs.nItems, 3, "%d");
	    success &= EXPECT_PAIR(Pairs[0], 0, 1, 0.8, 1.0);
	    success &= EXPECT_PAIR(Pairs[1], 0, 5, 0.8, 1.0);
	    success &= EXPECT_PAIR(Pairs[2], 1, 5, 0.8, 1.0);
	    freePairs(&xPairs);
	}
	
	/** Test sliding search: Large window. **/
	{
	    XArray xPairs;
	    if (check(xaInit(&xPairs, 4)) != 0) return false;
	    success &= EXPECT_EQL(caSlidingSearch(data, 6, 5, caLevCompare, 0.8, &xPairs), &xPairs, "%p");
	    pPair* Pairs = (pPair*)xPairs.Items;
	    for (unsigned int i = 0u; i < xPairs.nItems; i++)
		{
		pPair cur = Pairs[i];
		if (cur->i > cur->j)
		    {
		    unsigned int temp = cur->i;
		    cur->i = cur->j;
		    cur->j = temp;
		    }
		}
	    qsort(Pairs, xPairs.nItems, sizeof(pPair), cmp_Pairs);
	    success &= EXPECT_EQL(xPairs.nItems, 2, "%d");
	    success &= EXPECT_PAIR(Pairs[0], 0, 1, 0.8, 1.0);
	    // success &= EXPECT_PAIR(Pairs[1], 0, 5, 0.8, 1.0); /* Sliding search misses this pair. */
	    success &= EXPECT_PAIR(Pairs[1], 1, 5, 0.8, 1.0);
	    freePairs(&xPairs);
	}
	
	/** Test sliding search: Small window. **/
	{
	    XArray xPairs;
	    if (check(xaInit(&xPairs, 4)) != 0) return false;
	    success &= EXPECT_EQL(caSlidingSearch(data, 6, 2, caLevCompare, 0.8, &xPairs), &xPairs, "%p");
	    pPair* Pairs = (pPair*)xPairs.Items;
	    for (unsigned int i = 0u; i < xPairs.nItems; i++)
		{
		pPair cur = Pairs[i];
		if (cur->i > cur->j)
		    {
		    unsigned int temp = cur->i;
		    cur->i = cur->j;
		    cur->j = temp;
		    }
		}
	    qsort(Pairs, xPairs.nItems, sizeof(pPair), cmp_Pairs);
	    success &= EXPECT_EQL(xPairs.nItems, 1, "%d");
	    success &= EXPECT_PAIR(Pairs[0], 0, 1, 0.8, 1.0);
	    // success &= EXPECT_PAIR(Pairs[1], 0, 5, 0.8, 1.0); /* Sliding search misses this pair. */
	    // success &= EXPECT_PAIR(Pairs[2], 1, 5, 0.8, 1.0); /* Sliding search misses this pair. */
	    freePairs(&xPairs);
	}
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-06 Searching";
    return loopTest(doTest) * 3;
    }
