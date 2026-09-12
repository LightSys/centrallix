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
/* Module:	test_clusters_04.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 26th, 2025					*/
/* Description:	Test the caLevCompare() function from clusters.h.	*/
/************************************************************************/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"

/** Tested module. **/
#include "clusters.h"


static bool doTest(void)
    {
    bool success = true;
	
	/** Basic tests of Levenshtein edit distance similarity. **/
	success &= EXPECT_RANGE(caLevCompare("hello", "hello"), 0.99, 1.0, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "hello!"), 0.8, 1.0, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "asdfkh"), 0.0, 0.1, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "aaaaaaaaaaaaaaaaa"), 0.0, 0.1, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "nope"), 0.0, 0.2, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "noepo"), 0.15, 0.25, "%g");
	success &= EXPECT_RANGE(caLevCompare("below", "hello!"), 0.4, 0.6, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "smitten"), 0.65, 0.85, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", "bobbobbobbob"), 0.0, 0.1, "%g");
	success &= EXPECT_RANGE(caLevCompare("hello", ""), 0.0, 0.05, "%g");
	success &= EXPECT_RANGE(caLevCompare("", ""), 0.99, 1.0, "%g");
	success &= EXPECT_RANGE(caLevCompare("blooooop", "blob"), 0.3, 0.5, "%g");
	success &= EXPECT_RANGE(caLevCompare("", "!"), 0.0, 0.01, "%g");
	success &= EXPECT_RANGE(caLevCompare("h", "h"), 0.99, 1.0, "%g");
	success &= EXPECT_RANGE(caLevCompare("hi", "hi"), 0.99, 1.0, "%g");
	
	/** Kitten tests with specific edit operations. **/
	success &= EXPECT_RANGE(caLevCompare("kitten", "kitten"), 0.99, 1.0, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "skitten"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "itten"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "mitten"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "smitten"), 0.7, 0.8, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "iktten"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "kittens"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "kitte"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "kittem"), 0.8, 0.9, "%g");
	success &= EXPECT_RANGE(caLevCompare("kitten", "kittne"), 0.8, 0.9, "%g");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-04 caLevCompare()";
    return loopTest(doTest) * 25;
    }
