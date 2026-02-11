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
/* Module:	test_xstring_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 25th, 2025					*/
/* Description:	Test all the functions in the xstring library, except:	*/
/* 		xsInit(), xsDeInit(), xsWrite(), xsGenPrintf_va(),	*/
/* 		xsQPrintf(), xsQPrintf_va(), xsConcatQPrintf()		*/
/************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "util.h"

/** Tested module. **/
#include "xstring.h"

#define EXPECT_XSTR_EQL(xs, expect) \
	EXPECT_EQL((size_t)xsLength(xs), strlen(expect), "%ld") & \
	EXPECT_STR_EQL(xsString(xs), expect)

static bool do_tests(void)
    {
    bool success = true;
    
	/** Create and init xstring. **/
	pXString xs = xsNew();
	success &= EXPECT_EQL(xs != NULL, true, "%d");
	if (xs == NULL) return false;
	
	/** Verify initial state. **/
	success &= EXPECT_EQL(xsLength(xs), 0, "%d");
	success &= EXPECT_STR_EQL(xsString(xs), "");
	
	/** Set content with xsCopy(). **/
	success &= EXPECT_EQL(xsCopy(xs, "Hello", -1), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Hello");
	
	/** Append content with xsConcatenate(). **/
	success &= EXPECT_EQL(xsConcatenate(xs, " World", -1), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Hello World");
	
	/** Test xsStringEnd() with a reverse traversal. **/
	char* end_ptr = xsStringEnd(xs);
	success &= EXPECT_EQL(end_ptr != NULL, true, "%d");
	success &= EXPECT_EQL(end_ptr[0], '\0', "%d");
	success &= EXPECT_EQL(end_ptr[-1], 'd', "%d");
	success &= EXPECT_EQL(end_ptr[-2], 'l', "%d");
	success &= EXPECT_EQL(end_ptr[-3], 'r', "%d");
	success &= EXPECT_EQL(end_ptr[-4], 'o', "%d");
	success &= EXPECT_EQL(end_ptr[-5], 'W', "%d");
	success &= EXPECT_EQL(end_ptr[-6], ' ', "%d");
	success &= EXPECT_EQL(end_ptr[-7], 'o', "%d");
	success &= EXPECT_EQL(end_ptr[-8], 'l', "%d");
	success &= EXPECT_EQL(end_ptr[-9], 'l', "%d");
	success &= EXPECT_EQL(end_ptr[-10], 'e', "%d");
	success &= EXPECT_EQL(end_ptr[-11], 'H', "%d");
	
	/** Overwrite content with overflow. **/
	success &= EXPECT_EQL(xsCopy(xs, "Str&overflow", 3), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Str");
	
	/** Append to string with overflow. **/
	success &= EXPECT_EQL(xsConcatenate(xs, "++", 2), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Str++");
	
	/** Replace into formatted string. **/
	success &= EXPECT_EQL(xsPrintf(xs, "Number: %d", 42), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Number: 42");
	
	/** Append formatted text to string. **/
	success &= EXPECT_EQL(xsConcatPrintf(xs, ", String: %s", "test"), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Number: 42, String: test");
	
	/** Forward search with xsFind(). **/
	success &= EXPECT_EQL(xsCopy(xs, "Find me in here!", -1), 0, "%d");
	success &= EXPECT_EQL(xsFind(xs, "me", -1, 0), 5, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Find me in here!"); /* String is not modified. */
	
	/** Backward search with xsFindRev(). **/
	success &= EXPECT_EQL(xsCopy(xs, "find find find", -1), 0, "%d");
	success &= EXPECT_EQL(xsFind(xs, "find", -1, 0), 0, "%d");
	success &= EXPECT_EQL(xsFindRev(xs, "find", -1, 0), 10, "%d");
	success &= EXPECT_XSTR_EQL(xs, "find find find"); /* String is not modified. */
	
	/** Finding items that don't exist. **/
	success &= EXPECT_EQL(xsFind(xs, NULL, -1, 0), -1, "%d");
	success &= EXPECT_EQL(xsFind(xs, "GeorgeNotFound", -1, 0), -1, "%d");
	success &= EXPECT_XSTR_EQL(xs, "find find find"); /* String is not modified. */
	
	/** Find and replace. **/
	success &= EXPECT_EQL(xsCopy(xs, "cat cat cat", -1), 0, "%d");
	success &= EXPECT_EQL(xsReplace(xs, "cat", -1, 0, "dog", -1), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "dog cat cat");
	
	/** Find and replace fails to find. **/
	success &= EXPECT_EQL(xsCopy(xs, "GeorgeNotFound", -1), 0, "%d");
	success &= EXPECT_EQL(xsReplace(xs, "cat", -1, 0, "dog", -1), -1, "%d");
	success &= EXPECT_XSTR_EQL(xs, "GeorgeNotFound");
	
	/** Find and replace overflows buffer. **/
	success &= EXPECT_EQL(xsCopy(xs, "ve^", -1), 0, "%d");
	success &= EXPECT_EQL(xsReplace(xs, "efghijklmnop", 1, 0, "e... e- e... E!!!", 16), 1, "%d");
	success &= EXPECT_XSTR_EQL(xs, "ve... e- e... E!!^");
	
	/** Substitute at offset. **/
	success &= EXPECT_EQL(xsCopy(xs, "Hello World", -1), 0, "%d");
	success &= EXPECT_EQL(xsSubst(xs, 0, 5, "Goodbye", -1), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Goodbye World");
	success &= EXPECT_EQL(xsSubst(xs, 8, 3, "Otherside", 1), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Goodbye Old");
	
	/** Trim right whitespace. **/
	success &= EXPECT_EQL(xsCopy(xs, " \tTest   \n\t  ", -1), 0, "%d");
	success &= EXPECT_EQL(xsRTrim(xs), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, " \tTest");
	
	/** Trim left whitespace. **/
	success &= EXPECT_EQL(xsCopy(xs, "   \t\n  Test\n ", -1), 0, "%d");
	success &= EXPECT_EQL(xsLTrim(xs), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Test\n ");
	
	/** Trim whitespace on both sides. **/
	success &= EXPECT_EQL(xsCopy(xs, "  \t Trimmed \n  ", -1), 0, "%d");
	success &= EXPECT_EQL(xsTrim(xs), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Trimmed");
	
	/** Test xsInsertAfter() at offset. **/
	success &= EXPECT_EQL(xsCopy(xs, "Hello", -1), 0, "%d");
	success &= EXPECT_EQL(xsInsertAfter(xs, "p the fast food restaurant", 7, 3), 10, "%d");
	success &= EXPECT_XSTR_EQL(xs, "Help the flo");
	
	/** Test that xsCheckAlloc() does not destroy content. **/
	success &= EXPECT_EQL(xsCopy(xs, "test", -1), 0, "%d");
	success &= EXPECT_EQL(xsCheckAlloc(xs, 9284), 0, "%d");
	success &= EXPECT_XSTR_EQL(xs, "test");
	
	/** Clean up. **/
	xsFree(xs);
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "xstring-00 Full Test";
    return loop_tests(do_tests);
    }
