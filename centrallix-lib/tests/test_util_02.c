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
/* Module:	test_util_02.c						*/
/* Author:	Israel Fuller						*/
/* Creation:	November 24th, 2025					*/
/* Description:	Test the util.h printing functionality.			*/
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

/*** Call an snprint-style function and check both what it wrote and what it
 *** returned.
 *** `expect` is the untruncated result so that we can expect the return to be
 *** the correct length while requring the function to write exactly as much as
 *** `buf_size` allows.  Nothing is written when `buf_size` is zero.
 ***/
#define TEST_SNPRINT(fn, buf, buf_size, value, expect) \
    ({ \
    char* _expect = (expect); \
    const size_t _buf_size = (size_t)(buf_size); \
    const int _len = fn((buf), _buf_size, (value)); \
    char _trunc[strlen(_expect) + 1]; \
    strcpy(_trunc, _expect); \
    if (_buf_size > 0 && (size_t)_len >= _buf_size) _trunc[_buf_size - 1] = '\0'; \
    EXPECT_EQL(_len, (int)strlen(_expect), "%d") && \
	(_buf_size == 0 || EXPECT_STR_EQL((buf), _trunc)); \
    })

#define TEST_SNPRINT_COMMAS_LLU(buf, buf_size, value, expect) \
	TEST_SNPRINT(snprintCommasLlu, buf, buf_size, value, expect)

#define TEST_SNPRINT_BYTES(buf, buf_size, value, expect) \
	TEST_SNPRINT(snprintBytes, buf, buf_size, value, expect)

static bool doTest(void)
    {
    bool success = true;

	/** Detect if metric or CS units are intended. **/
	bool cs = true;
	#ifdef USE_METRIC
	if (USE_METRIC) cs = false;
	#endif

	/** Allocate space for the string buffer. **/
	char buf[32];
	const size_t buf_size = sizeof(buf) / sizeof(char);
	memset(buf, UINT_MAX, sizeof(buf)); /* Use unexpected data to catch uninitialized reads. */

	/** Test snprintBytes(). **/
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

	/** Truncation: the full length is returned, but only part is written. **/
	success &= TEST_SNPRINT_BYTES(buf, 0, pow(1024, 1), (cs) ? "1 KiB" : "1.02 KB");
	success &= TEST_SNPRINT_BYTES(buf, 1, pow(1024, 1), (cs) ? "1 KiB" : "1.02 KB");
	success &= TEST_SNPRINT_BYTES(buf, 4, pow(1024, 1), (cs) ? "1 KiB" : "1.02 KB");
	success &= TEST_SNPRINT_BYTES(buf, 5, pow(1024, 1), (cs) ? "1 KiB" : "1.02 KB");
	success &= TEST_SNPRINT_BYTES(buf, 6, pow(1024, 1), (cs) ? "1 KiB" : "1.02 KB");

	/** Test snprintCommasLlu(). Note: 10^16 would fail due to the double precision limit. **/
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, 0, "0");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 1) - 1,   "9");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 1),       "10");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 2) - 1,   "99");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 2),       "100");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 3) - 1,   "999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 3),       "1,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 4) - 1,   "9,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 4),       "10,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 5) - 1,   "99,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 5),       "100,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 6) - 1,   "999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 6),       "1,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 7) - 1,   "9,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 7),       "10,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 8) - 1,   "99,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 8),       "100,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 9) - 1,   "999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 9),       "1,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 10) - 1,  "9,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 10),      "10,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 11) - 1,  "99,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 11),      "100,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 12) - 1,  "999,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 12),      "1,000,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 13) - 1,  "9,999,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 13),      "10,000,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 14) - 1,  "99,999,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 14),      "100,000,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 15) - 1,  "999,999,999,999,999");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(10, 15),      "1,000,000,000,000,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 1) - 1, "1,023");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 1),     "1,024");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 2) - 1, "1,048,575");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 2),     "1,048,576");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 3) - 1, "1,073,741,823");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, pow(1024, 3),     "1,073,741,824");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, SHRT_MAX,         "32,767");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, USHRT_MAX,        "65,535");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, INT_MAX,          "2,147,483,647");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, UINT_MAX,         "4,294,967,295");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, LLONG_MAX,        "9,223,372,036,854,775,807");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, buf_size, ULLONG_MAX,       "18,446,744,073,709,551,615");

	/** Truncation: the full length is returned, but only part is written. **/
	success &= TEST_SNPRINT_COMMAS_LLU(buf, 0, pow(10, 3), "1,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, 1, pow(10, 3), "1,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, 3, pow(10, 3), "1,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, 5, pow(10, 3), "1,000");
	success &= TEST_SNPRINT_COMMAS_LLU(buf, 6, pow(10, 3), "1,000");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "util-02 Printing";
    return loopTest(doTest) * (22 + 48);
    }
