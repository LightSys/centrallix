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
/* Description:	Test the .h timer1 functionality.			*/
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

/** Assumes success is in scope. **/
#define TEST_SNPRINT_LLU(buf, buf_size, value, expect) \
    EXPECT_STR_EQL(snprint_llu(buf, buf_size, value), expect) && \
    EXPECT_EQL(snprint_llu(buf, buf_size, value), &buf[0], "%p")
    
#define TEST_SNPRINT_BYTES(buf, buf_size, value, expect) \
    EXPECT_STR_EQL(snprint_bytes(buf, buf_size, value), expect) && \
    EXPECT_EQL(snprint_bytes(buf, buf_size, value), &buf[0], "%p")

static bool do_tests(void)
    {
    bool success = true;
    
    /** Detect if metric or CS units are intended. **/
    bool cs = true;
    #ifdef UTIL_USE_METRIC
    if (UTIL_USE_METRIC) cs = false;
    #endif
    
    /** Allocate space for the string buffer. **/
    char buf[32];
    const size_t buf_size = sizeof(buf) / sizeof(char);
    memset(buf, UINT_MAX, sizeof(buf)); /* Use unexpected data to catch uninitialized reads. */
    
    /** Test snprint_bytes(). **/
    success &= TEST_SNPRINT_BYTES(buf, buf_size, 0, "0 bytes");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, 10, "10 bytes");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, 100, "100 bytes");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 1) - pow(1000, 0), (cs) ? "999 bytes"  : "999 bytes");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 1),                (cs) ? "1000 bytes" : "1 KB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 2) - pow(1000, 1), (cs) ? "975.59 KiB"  : "999 KB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 2),                (cs) ? "976.56 KiB" : "1 MB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 3) - pow(1000, 2), (cs) ? "952.72 MiB" : "999 MB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1000, 3),                (cs) ? "953.67 MiB" : "1 GB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 1) - pow(1024, 0), (cs) ? "1023 bytes" : "1.02 KB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 1),                (cs) ? "1 KiB"      : "1.02 KB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 2) - pow(1024, 1), (cs) ? "1023 KiB"   : "1.05 MB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 2),                (cs) ? "1 MiB"      : "1.05 MB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 3) - pow(1024, 2), (cs) ? "1023 MiB"   : "1.07 GB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, pow(1024, 3),                (cs) ? "1 GiB"      : "1.07 GB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, INT_MAX,                     (cs) ? "2 GiB"      : "2.15 GB");
    success &= TEST_SNPRINT_BYTES(buf, buf_size, UINT_MAX,                    (cs) ? "4 GiB"      : "4.29 GB");
    
    /** Test snprint_llu(). Note: 10^16 would fail due to the double precision limit. **/
    success &= TEST_SNPRINT_LLU(buf, buf_size, 0, "0");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 1) - 1,   "9");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 1),       "10");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 2) - 1,   "99");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 2),       "100");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 3) - 1,   "999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 3),       "1,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 4) - 1,   "9,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 4),       "10,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 5) - 1,   "99,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 5),       "100,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 6) - 1,   "999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 6),       "1,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 7) - 1,   "9,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 7),       "10,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 8) - 1,   "99,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 8),       "100,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 9) - 1,   "999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 9),       "1,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 10) - 1,  "9,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 10),      "10,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 11) - 1,  "99,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 11),      "100,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 12) - 1,  "999,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 12),      "1,000,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 13) - 1,  "9,999,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 13),      "10,000,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 14) - 1,  "99,999,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 14),      "100,000,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 15) - 1,  "999,999,999,999,999");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(10, 15),      "1,000,000,000,000,000");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 1) - 1, "1,023");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 1),     "1,024");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 2) - 1, "1,048,575");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 2),     "1,048,576");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 3) - 1, "1,073,741,823");
    success &= TEST_SNPRINT_LLU(buf, buf_size, pow(1024, 3),     "1,073,741,824");
    success &= TEST_SNPRINT_LLU(buf, buf_size, SHRT_MAX,         "32,767");
    success &= TEST_SNPRINT_LLU(buf, buf_size, USHRT_MAX,        "65,535");
    success &= TEST_SNPRINT_LLU(buf, buf_size, INT_MAX,          "2,147,483,647");
    success &= TEST_SNPRINT_LLU(buf, buf_size, UINT_MAX,         "4,294,967,295");
    success &= TEST_SNPRINT_LLU(buf, buf_size, LLONG_MAX,        "9,223,372,036,854,775,807");
    success &= TEST_SNPRINT_LLU(buf, buf_size, ULLONG_MAX,       "18,446,744,073,709,551,615");
    
    /** Test __FILENAME__. **/
    success &= EXPECT_STR_EQL(__FILENAME__, "test_util_05.c");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "util-05 Printing";
    return loop_tests(do_tests);
    }
