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
/* Module:	test_util_02.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Test the util.h round_to() function.			*/
/************************************************************************/

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
    
	/** Rounding to 0 decimals does not change whole numbers. **/
	success &= EXPECT_EQL(round_to(0.0, 0), 0.0, "%g");
	success &= EXPECT_EQL(round_to(123, 0), 123.0, "%g");
	success &= EXPECT_EQL(round_to(-123, 0), -123.0, "%g");
	    
	/** Rounding to 0 decimals rounds decimals. **/
	success &= EXPECT_EQL(round_to(1.23, 0), 1.0, "%g");
	success &= EXPECT_EQL(round_to(24.43, 0), 24.0, "%g");
	success &= EXPECT_EQL(round_to(1234567890.499, 0), 1234567890.0, "%g");
	success &= EXPECT_EQL(round_to(-1.82, 0), -2.0, "%g");
	success &= EXPECT_EQL(round_to(-987654321.499, 0), -987654321.0, "%g");
	
	/** Test rounding to various numbers of decimals. **/
	success &= EXPECT_EQL(round_to(1.23, 1), 1.2, "%g");
	success &= EXPECT_EQL(round_to(1.23, 2), 1.23, "%g");
	success &= EXPECT_EQL(round_to(1.23, 3), 1.23, "%g");
	success &= EXPECT_EQL(round_to(1.23, 8), 1.23, "%g");
	success &= EXPECT_EQL(round_to(5824113.8, 8), 5824113.8, "%g");
	
	/** Test rounding to negative numbers of decimals. **/
	success &= EXPECT_EQL(round_to(1.23, -1), 0.0, "%g");
	success &= EXPECT_EQL(round_to(123.4, -1), 120.0, "%g");
	success &= EXPECT_EQL(round_to(586241.7, -4), 590000.0, "%g");
	
	/** Rounding infinity. */
	success &= EXPECT_EQL(round_to( INFINITY, 0), INFINITY, "%g");
	success &= EXPECT_EQL(round_to(-INFINITY, 0), -INFINITY, "%g");
	success &= EXPECT_EQL(round_to( INFINITY, 16), INFINITY, "%g");
	success &= EXPECT_EQL(round_to(-INFINITY, 16), -INFINITY, "%g");
	success &= EXPECT_EQL(round_to( INFINITY, -16), INFINITY, "%g");
	success &= EXPECT_EQL(round_to(-INFINITY, -16), -INFINITY, "%g");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "util-02 round_to()";
    return loop_tests(do_tests);
    }
