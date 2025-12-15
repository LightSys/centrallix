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

/** Useful testing macro. **/
#define EXPECT_VEC_EQL(v1, v2) \
    ({ \
	pVector _v1 = (v1); \
	pVector _v2 = (v2); \
	int success = ca_eql(_v1, _v2); \
	if (!success) \
	    { \
	    printf("  > Expected %s to equal %s at %s:%d, but got:\n", #v1, #v2, __FILE__, __LINE__); \
	    printf("  > 1. "); ca_print_vector(_v1); printf("\n"); \
	    printf("  > 2. "); ca_print_vector(_v2); printf("\n"); \
	    fflush(stdout); \
	    } \
	success; \
    })

static bool do_tests(void)
    {
    bool success = true;
    
	/** Make an array to STORE() pointers to vectors so we can free them. **/
	const unsigned int max_index = 16u;
	unsigned int index = 0u;
	pVector free_list[max_index];
	#define STORE(v) (free_list[index++] = (v))
	
	/** Edge case: Null string. **/
	success &= EXPECT_EQL(ca_build_vector(NULL), NULL, "%p");
	
	/** Edge case: Empty string. **/
	success &= EXPECT_VEC_EQL(STORE(ca_build_vector("")), ((int[]){-172, 11, -78}));
	
	/** Single letter cases. **/
	success &= EXPECT_VEC_EQL(STORE(ca_build_vector("a")), ((int[]){-204, 12, -25, 12, -20}));
	success &= EXPECT_VEC_EQL(STORE(ca_build_vector("b")), ((int[]){-151, 13, -11, 13, -87}));
	success &= EXPECT_VEC_EQL(STORE(ca_build_vector("v")), ((int[]){-221, 7, -19, 7, -9}));
	
	/** Multi-letter cases. **/
	success &= EXPECT_VEC_EQL(STORE(ca_build_vector("def")), ((int[]){-79, 4, -51, 2, -4, 7, -64, 9, -49}));
	
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
#undef EXPECT_VEC_EQL
