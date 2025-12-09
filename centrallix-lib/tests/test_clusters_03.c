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
/* Module:	test_clusters_03.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 26th, 2025					*/
/* Description:	Test the ca_cos_compare() function from clusters.h.	*/
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


static bool do_tests(void)
    {
    bool success = true;
    
    /** Make an array to STORE() pointers to vectors so we can free them. **/
    const unsigned int max_index = 32u;
    unsigned int index = 0u;
    pVector free_list[max_index];
    #define STORE(v) (free_list[index++] = (v))
    
    /** ca_cos_compare() shortcut macro. **/
    #define cos_cmp(str1, str2) ca_cos_compare(STORE(ca_build_vector(str1)), STORE(ca_build_vector(str2)))
    
    /** Basic tests of cosine similarity. **/
    success &= EXPECT_RANGE(cos_cmp("hello", "hello"), 0.999, 1.0, "%g");
    success &= EXPECT_RANGE(cos_cmp("hello", "zephora"), 0.0, 0.001, "%g");
    success &= EXPECT_RANGE(cos_cmp("hello", "hello world"), 0.6, 0.7, "%g");
    success &= EXPECT_RANGE(cos_cmp("hello there", "hellow there"), 0.9, 1.0, "%g");
    
    /** Tests on fabricated contact information. */
    /*** All email addresses and phone numbers are imaginary and were
     *** fabricated for the purposes of this test.
     ***/
    success &= EXPECT_RANGE(cos_cmp("Cynthia Adams; cynthiaadams@gmail.com; 720-769-1293", "Timothy Adams; thetbear@gmail.com; 720-891-1470"), 0.49, 0.54, "%g");
    success &= EXPECT_RANGE(cos_cmp("Timothy Adams; thetbear@gmail.com; 720-891-1470", "Lance Freson; lancetheturtle@gmail.com; 720-111-8189"), 0.45, 0.50, "%g");
    success &= EXPECT_RANGE(cos_cmp("Lance Freson; lancetheturtle@gmail.com; 720-111-8189", "Gregory Freson; greatgregory@gmail.com; 720-198-5791"), 0.425, 0.475, "%g");
    success &= EXPECT_RANGE(cos_cmp("Gregory Freson; greatgregory@gmail.com; 720-198-5791", "Gregory Freson; greatgregory@gmail.co; 720-198-5791"), 0.94, 0.99, "%g");
    success &= EXPECT_RANGE(cos_cmp("Nathan Mayor; nmmayor@yahoo.com; +1-800-192-9128", "Mindy Mayor; nmmayor@yahoo.com; 720-981-9149"), 0.575, 0.625, "%g");
    success &= EXPECT_RANGE(cos_cmp("This is an identical case", "This is an identical case"), 0.975, 1.00, "%g");
    success &= EXPECT_RANGE(cos_cmp("Samuel", "Alex"), 0.00, 0.025, "%g");
    
    /** Clean up scope. **/
    #undef STORE
    #undef cos_cmp
    
    /** Clean up using the free list. **/
    if (index >= max_index)
	{
	printf("  > MEMORY ERROR!!\n");
	printf("  > Allocated %u vectors, overflowing the free list of size %u.\n", index + 1, max_index);
	printf("  > Increase the size of the free list (aka. max_index) to %u or more.\n", index + 1);
	return false;
	}
    while (index > 0u)
	{
	pVector cur_vector = free_list[--index];
	if (cur_vector == NULL) continue;
	else ca_free_vector(cur_vector);
	}
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-03 ca_cos_compare()";
    return loop_tests(do_tests);
    }
