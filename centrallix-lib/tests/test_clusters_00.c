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
/* Module:	test_clusters_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 25th, 2025					*/
/* Description:	Test the ca_edit_dist() function from clusters.h.	*/
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
    
	/** Kitten tests. **/
	success &= EXPECT_EQL(ca_edit_dist("kitten", "kitten",  0, 0), 0, "%d"); /* 0 edits. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "skitten", 0, 0), 1, "%d"); /* 1 insert. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "itten",   0, 0), 1, "%d"); /* 1 delete. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "mitten",  0, 0), 1, "%d"); /* 1 replace. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "smitten", 0, 0), 2, "%d"); /* 1 insert and 1 replace. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "iktten",  0, 0), 1, "%d"); /* 1 transpose. */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "kittens", 0, 0), 1, "%d"); /* 1 insert (end). */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "kitte",   0, 0), 1, "%d"); /* 1 delete (end). */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "kittem",  0, 0), 1, "%d"); /* 1 replace (end). */
	success &= EXPECT_EQL(ca_edit_dist("kitten", "kittne",  0, 0), 1, "%d"); /* 1 transpose (end). */
	
	/** Alternate words. **/
	success &= EXPECT_EQL(ca_edit_dist("lawn",   "flown",   0, 0), 2, "%d"); /* 1 insert and one replace. */
	success &= EXPECT_EQL(ca_edit_dist("hello",  "hello!",  0, 0), 1, "%d"); /* 1 insert (end). */
	success &= EXPECT_EQL(ca_edit_dist("zert",   "zerf",    0, 0), 1, "%d"); /* 1 replace (end). */
	success &= EXPECT_EQL(ca_edit_dist("llearr", "lear",    0, 0), 2, "%d"); /* 2 deletes (start & end). */
	
	/** Long strings for testing edge cases. **/
	char* str1 = "This is a very long string!! I do not expect this function to need to process a string longer than this, because this string is a full 254 characters. That is pretty long. The object system limits strings to this size so we cannot make a longer string...";
	const size_t str_size = (strlen(str1) + 1) * sizeof(char);
	char* str2 = memcpy(malloc(str_size), str1, str_size);
	char* str3 = "This is quite a lengthy string. I do not expect the function to compute any longer string since this one is a full 254 characters. That is plenty, even if someone adds many contact details to their record!! Thus, this test should cover most cases we see.";
	
	/** Test edge cases. **/
	success &= EXPECT_EQL(ca_edit_dist("", "", 0, 0), 0, "%d"); /* Identical, empty string: 0 edits. */
	success &= EXPECT_EQL(ca_edit_dist(str1, str1, 0, 0), 0, "%d"); /* Identical, very long strings. */
	success &= EXPECT_EQL(ca_edit_dist(str1, str2, 0, 0), 0, "%d"); /* Identical, very long strings (different pointers). */
	success &= EXPECT_EQL(ca_edit_dist(str2, str3, 0, 0), 133, "%d"); /* 133 edits. */
	
	/** Empty string comparsions. **/
	success &= EXPECT_EQL(ca_edit_dist(str1, "", 0, 0), (int)strlen(str1), "%d");
	success &= EXPECT_EQL(ca_edit_dist(str2, "", 0, 0), (int)strlen(str2), "%d");
	success &= EXPECT_EQL(ca_edit_dist(str3, "", 0, 0), (int)strlen(str3), "%d");
	
	/** Specifying lengths with overflows. **/
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 13, 13), 0, "%d");
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 13, 0), 9, "%d");
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 0, 13), 6, "%d");
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 0, 0), 3, "%d");
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 19, 0), 3, "%d");
	success &= EXPECT_EQL(ca_edit_dist("A string with edits", "A string without edits", 0, 19), 6, "%d");
	
	/** Test for errors in identical pointer optimizations with specified lengths. **/
	char identical_string[] = "Identical String!";
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 0, 0), 0, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 0, 17), 0, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 17, 0), 0, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 0, 16), 1, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 0, 8), 9, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 5, 6), 1, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 16, 13), 3, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 0, 1), 16, "%d");
	success &= EXPECT_EQL(ca_edit_dist(identical_string, identical_string, 1, 0), 16, "%d");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-00 ca_edit_dist()";
    return loop_tests(do_tests);
    }
