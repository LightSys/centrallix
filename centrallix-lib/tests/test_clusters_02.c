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
/* Module:	test_clusters_02.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 25th, 2025					*/
/* Description:	Test the ca_build_vector() function from clusters.h.	*/
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
	const unsigned int max_index = 16u;
	unsigned int index = 0u;
	pVector free_list[max_index];
	#define STORE(v) (free_list[index++] = (v))
	#define vec(s) STORE(ca_build_vector(s))
	
	/** Edge case: Null string. **/
	success &= EXPECT_EQL(ca_build_vector(NULL), NULL, "%p");
	
	/** Edge case: Empty string. **/
	success &= EXPECT_VEC_EQL(vec(""), ((int[]){-172, 11, -78}));
	
	/** Single letter cases. **/
	success &= EXPECT_VEC_EQL(vec("a"), ((int[]){-204, 12, -25, 12, -20}));
	success &= EXPECT_VEC_EQL(vec("b"), ((int[]){-151, 13, -11, 13, -87}));
	success &= EXPECT_VEC_EQL(vec("v"), ((int[]){-221, 7, -19, 7, -9}));
	
	/** Multi-letter cases. **/
	success &= EXPECT_VEC_EQL(vec("def"), ((int[]){-79, 4, -51, 2, -4, 7, -64, 9, -49}));
	success &= EXPECT_VEC_EQL(vec("vec"), ((int[]){-37, 1, -175, 12, -18, 6, -8, 7, -9}));
	
	/** White space and punctuation should be ignored. **/
	success &= EXPECT_VEC_EQL(vec("Yippee!!!"), vec(">>->y  i!&P^^_pe$/\n?e"));
	
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
    *tname = "cluster-02 ca_build_vector()";
    return loop_tests(do_tests);
    }

/** Clean up scope. **/
#undef STORE
#undef vec
