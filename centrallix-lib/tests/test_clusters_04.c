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
/* Module:	test_clusters_04.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 26th, 2025					*/
/* Description:	Test the ca_lev_compare() function from clusters.h.	*/
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
        
    /** Basic tests of Levenshtein edit distance similarity. **/
    success &= EXPECT_RANGE(ca_lev_compare("hello", "hello"), 0.99, 1.0, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "hello!"), 0.8, 1.0, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "asdfkh"), 0.0, 0.1, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "aaaaaaaaaaaaaaaaa"), 0.0, 0.1, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "nope"), 0.0, 0.2, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "noepo"), 0.15, 0.25, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("below", "hello!"), 0.4, 0.6, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "smitten"), 0.65, 0.85, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", "bobbobbobbob"), 0.0, 0.1, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hello", ""), 0.0, 0.05, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("", ""), 0.99, 1.0, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("blooooop", "blob"), 0.3, 0.5, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("", "!"), 0.0, 0.01, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("h", "h"), 0.99, 1.0, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("hi", "hi"), 0.99, 1.0, "%g");
    
    /** Kitten tests with specific edit operations. **/
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "kitten"), 0.99, 1.0, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "skitten"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "itten"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "mitten"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "smitten"), 0.7, 0.8, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "iktten"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "kittens"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "kitte"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "kittem"), 0.8, 0.9, "%g");
    success &= EXPECT_RANGE(ca_lev_compare("kitten", "kittne"), 0.8, 0.9, "%g");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-04 ca_lev_compare()";
    return loop_tests(do_tests);
    }
