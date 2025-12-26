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
/* Module:	test_clusters_05.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 26th, 2025					*/
/* Description:	Test the ca_most_similar() function from clusters.h.	*/
/************************************************************************/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "util.h"
#include "xhash.h"

/** Tested module. **/
#include "clusters.h"

static const unsigned int key_length = 64u;
static pXHashTable mock_sims = NULL;
static bool* success_ptr = NULL;
static double get_mock_sim(void* v1, void* v2)
    {
    char key[key_length];
    char* str1 = v1;
    char* str2 = v2;
    
	/** Try key1, key2. **/
	snprintf(key, sizeof(key), "%s|%s", str1, str2);
	double* sim = (double*)xhLookup(mock_sims, key);
	if (sim != NULL) goto found;
	
	/** Try key2, key1. **/
	snprintf(key, sizeof(key), "%s|%s", str2, str1);
	sim = (double*)xhLookup(mock_sims, key);
	if (sim != NULL) goto found;
	
	/** Key not found. **/
	fprintf(stderr, "  > get_mock_sim(\"%s\", \"%s\"): No sim provided!\n", str1, str2);
	*success_ptr = false;
	return NAN;
    
    found:
    /** Key found. **/
    return *sim;
    }

static int do_nothing() { return 0; }

static bool do_tests(void)
    {
    bool success = true;
    
	/** Check error cases. **/
	success &= EXPECT_STR_EQL(ca_most_similar(NULL, (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, 0.0), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", NULL, 2, ca_lev_compare, 0.0), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 0, ca_lev_compare, 0.0), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, NULL, 0.0), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, 1.1), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, -0.1), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, INFINITY), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, -INFINITY), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, NAN), NULL);
	
	/** Simple test cases. **/
	success &= EXPECT_STR_EQL(ca_most_similar("str1", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, 0.0), "str1");
	success &= EXPECT_STR_EQL(ca_most_similar("str", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, 0.0), "str1");
	success &= EXPECT_STR_EQL(ca_most_similar("kitten", (void*[]){"str_abc", "str1"}, 2, ca_lev_compare, 0.0), "str1");
	success &= EXPECT_STR_EQL(ca_most_similar("str1", (void*[]){"str2", "str", "eight"}, 3, ca_lev_compare, 0.0), "str2");
	
	/** Many, identically similar options. */
	success &= EXPECT_STR_EQL(ca_most_similar("kitten",
	    (void*[]){"skitten", "itten", "mitten", "iktten", "kittens", "kitte", "kittem", "kittne"}, 8,
	ca_lev_compare, 0.0), "skitten");
	
	/** Pointer-perfect handling. **/
	char* target = "string";
	success &= EXPECT_EQL((char*)ca_most_similar(target, (void*[]){"str", target}, 2, ca_lev_compare, 0.0), target, "%s");
	
	/** List overflow. **/
	success &= EXPECT_STR_EQL(ca_most_similar("target", (void*[]){"str1", "targets", "target", "walmart"}, 2, ca_lev_compare, 0.0), "targets");
	
	/** Threshold exceeded. **/
	success &= EXPECT_STR_EQL(ca_most_similar("blob", (void*[]){"blooooop", "targets", "string"}, 3, ca_lev_compare, 0.0), "blooooop");
	success &= EXPECT_STR_EQL(ca_most_similar("blob", (void*[]){"blooooop", "targets", "string"}, 3, ca_lev_compare, 0.5), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("hello", (void*[]){"bane", "noepo", "stars"}, 3, ca_lev_compare, 0.0), "noepo");
	success &= EXPECT_STR_EQL(ca_most_similar("hello", (void*[]){"bane", "noepo", "stars"}, 3, ca_lev_compare, 0.25), NULL);
	success &= EXPECT_STR_EQL(ca_most_similar("kitten", (void*[]){"skitten", "fit"}, 2, ca_lev_compare, 0.0), "skitten");
	success &= EXPECT_STR_EQL(ca_most_similar("kitten", (void*[]){"skitten", "fit"}, 2, ca_lev_compare, 0.9), NULL);
	
	/** Make an array to STORE() pointers to vectors so we can free them. **/
	const unsigned int max_index = 32u;
	unsigned int index = 0u;
	pVector free_list[max_index];
	#define STORE(v) (free_list[index++] = (v))
	#define str(s) STORE(ca_build_vector(s))
	
	/** Alternative similarity function. **/
	pVector hello = str("hello"), fellow = str("fellow"), felon = str("felon");
	pVector held = str("held"), zephora = str("zephora"), hexza = str("hexza");
	pVector hello_there = str("hello there"), hello_world = str("hello world");
	pVector hellow_there = str("hellow there");
	success &= EXPECT_STR_EQL(ca_most_similar(hello, (void*[]){fellow, felon, hello, held}, 4, ca_cos_compare, 0.0), hello);
	success &= EXPECT_STR_EQL(ca_most_similar(str("hello"), (void*[]){fellow, felon, hello, held}, 4, ca_cos_compare, 0.0), hello);
	success &= EXPECT_STR_EQL(ca_most_similar(hello, (void*[]){zephora, hello_world, hexza}, 3, ca_cos_compare, 0.0), hello_world);
	success &= EXPECT_STR_EQL(ca_most_similar(hello, (void*[]){zephora, hello_world}, 1, ca_cos_compare, 0.0), zephora);
	success &= EXPECT_STR_EQL(ca_most_similar(hello, (void*[]){zephora}, 1, ca_cos_compare, 0.0), zephora);
	success &= EXPECT_STR_EQL(ca_most_similar(hello_there, (void*[]){hello_world, zephora, hellow_there, hexza}, 4, ca_cos_compare, 0.0), hellow_there);
	success &= EXPECT_STR_EQL(ca_most_similar(hello_there, (void*[]){hello_world, zephora, hellow_there}, 2, ca_cos_compare, 0.0), hello_world);
	success &= EXPECT_STR_EQL(ca_most_similar(hello_there, (void*[]){hello_world, zephora, hellow_there}, 2, ca_cos_compare, 0.8), NULL);
	
	/** Special characters (ignored by the similarity function). **/
	pVector yip = str("Yippee!!!");
	pVector str1 = str("@*#((%^!&@*-+!"), str2 = str(">>->y  i!&P^^_pe$/\n?e"), str3 = str("yip");
	success &= EXPECT_STR_EQL(ca_most_similar(yip, (void*[]){str1, str2, str3}, 3, ca_cos_compare, 0.0), str2);
	success &= EXPECT_STR_EQL(ca_most_similar(yip, (void*[]){str1, str2, str3}, 3, ca_cos_compare, 1.0), str2);
	
	/** Clean up scope. **/
	#undef STORE
	#undef str
	
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
	
	/** Set up the mock similarity function. **/
	XHashTable sim_table;
	if (!check(xhInit(&sim_table, 64, 0))) return false;
	mock_sims = &sim_table;
	success_ptr = &success;
	
	/** Completely different strings are similar. **/
	double str1_str = 0.2, str1_str2 = 0.1, str1_eight = 0.8;
	if (!check(xhAdd(&sim_table, "str1|str",   (void*)&str1_str))) return false;
	if (!check(xhAdd(&sim_table, "str1|str2",  (void*)&str1_str2))) return false;
	if (!check(xhAdd(&sim_table, "str1|eight", (void*)&str1_eight))) return false;
	success &= EXPECT_STR_EQL(ca_most_similar("str1", (void*[]){"str2", "str", "eight"}, 3, get_mock_sim, 0.0), "eight");
	success &= EXPECT_STR_EQL(ca_most_similar("str1", (void*[]){"str2", "str", "eight"}, 3, get_mock_sim, 0.9), NULL);
	if (!check(xhClear(&sim_table, do_nothing, NULL))) return false;
	
	/** Nans are skipped. **/
	double val_nan = 0.8, val_vals = NAN, val_val = 0.2;
	if (!check(xhAdd(&sim_table, "val|nan",  (void*)&val_nan))) return false;
	if (!check(xhAdd(&sim_table, "val|vals", (void*)&val_vals))) return false;
	if (!check(xhAdd(&sim_table, "val|val",  (void*)&val_val))) return false;
	success &= EXPECT_STR_EQL(ca_most_similar("val", (void*[]){"val", "vals", "nan"}, 3, get_mock_sim, 0.0), "nan");
	success &= EXPECT_STR_EQL(ca_most_similar("val", (void*[]){"val", "vals", "nan"}, 3, get_mock_sim, 0.9), NULL);
	if (!check(xhClear(&sim_table, do_nothing, NULL))) return false;
	
	/** Clean up. **/
	if (!check(xhDeInit(&sim_table))) return false;
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-05 ca_most_similar()";
    return loop_tests(do_tests);
    }
