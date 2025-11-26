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
/* Module:	test_clusters_01.c					*/
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
    
    /** Many, many replace edits. **/
    const static unsigned short num_edits = 254;//15827;
    char AAA[num_edits + 1], BBB[num_edits + 1];
    memset(AAA, 'A', num_edits);
    memset(BBB, 'B', num_edits);
    AAA[num_edits] = BBB[num_edits] = '\0';
    success &= EXPECT_EQL(ca_edit_dist(AAA, "", 0, 0), num_edits, "%d");
    success &= EXPECT_EQL(ca_edit_dist("", BBB, 0, 0), num_edits, "%d");
    success &= EXPECT_EQL(ca_edit_dist(AAA, BBB, 0, 0), num_edits, "%d");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "cluster-01 ca_edit_dist(): Stress test";
    return loop_tests(do_tests);
    }
