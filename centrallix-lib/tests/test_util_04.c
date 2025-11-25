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
/* Module:	test_util_04.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Test the util.h max() and min() functions.		*/
/************************************************************************/

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"

/** Tested module. **/
#include "util.h"

static bool do_tests(void)
    {
    bool success = true;
    
    /** Max with doubles. **/
    success &= EXPECT_EQL(max(0.0, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(max(1.0, 0.0), 1.0, "%g");
    success &= EXPECT_EQL(max(-1.0, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(max(-1.1, 0.1), 0.1, "%g");
    success &= EXPECT_EQL(max(0.0001, 0.00011), 0.00011, "%g");
    success &= EXPECT_EQL(max(DBL_MAX, 0.0), DBL_MAX, "%g");
    success &= EXPECT_EQL(max(DBL_MIN, 0.0), DBL_MIN, "%g");
    success &= EXPECT_EQL(max(-DBL_MIN, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(max(-DBL_MAX, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(max(pow(10, DBL_DIG), pow(10, FLT_DIG)), pow(10, DBL_DIG), "%g");
    
    /** Min with doubles. **/
    success &= EXPECT_EQL(min(0.0, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(min(1.0, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(min(-1.0, 0.0), -1.0, "%g");
    success &= EXPECT_EQL(min(-1.1, 0.1), -1.1, "%g");
    success &= EXPECT_EQL(min(0.0001, 0.00011), 0.0001, "%g");
    success &= EXPECT_EQL(min(DBL_MAX, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(min(DBL_MIN, 0.0), 0.0, "%g");
    success &= EXPECT_EQL(min(-DBL_MIN, 0.0), -DBL_MIN, "%g");
    success &= EXPECT_EQL(min(-DBL_MAX, 0.0), -DBL_MAX, "%g");
    success &= EXPECT_EQL(min(pow(10, DBL_DIG), pow(10, FLT_DIG)), pow(10, FLT_DIG), "%g");
    
    /** Max with ints. **/
    success &= EXPECT_EQL(max(0, 0), 0, "%d");
    success &= EXPECT_EQL(max(1, 0), 1, "%d");
    success &= EXPECT_EQL(max(-1, 0), 0, "%d");
    success &= EXPECT_EQL(max(INT_MAX, 0), INT_MAX, "%d");
    success &= EXPECT_EQL(max(INT_MIN, 0), 0, "%d");
    
    /** Min with ints. **/
    success &= EXPECT_EQL(min(0, 0), 0, "%d");
    success &= EXPECT_EQL(min(1, 0), 0, "%d");
    success &= EXPECT_EQL(min(-1, 0), -1, "%d");
    success &= EXPECT_EQL(min(INT_MAX, 0), 0, "%d");
    success &= EXPECT_EQL(min(INT_MIN, 0), INT_MIN, "%d");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "util-04 min() & max()";
    return loop_tests(do_tests);
    }
