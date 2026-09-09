#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"
#include "test_qprintf.h"
#include "test_utils.h"
#include "expect.h"

const char* BAD_BASE64S[] = {
    "    ",
    "....",
    "!!!!",
    "@@@@",
    "####",
    "$$$$",
    "%%%%",
    "^^^^",
    "&&&&",
    "****",
    "((((",
    "))))",
    "----",
    "====",
    "::::",
    ",,,,",
    "....",
    "[[[[",
    "]]]]",
    "{{{{",
    "}}}}",
    "<<<<",
    ">>>>",
    "!@#$%^&*",
    ",.{}[]<>",
    "-=: ",
    "A",
    "AB",
    "ABC",
    "ABCDE",
    "ABCDEF",
    "ABCDEFG",
    "ABCDEFGHI",
    "ABCDEFGHIJ",
    "ABCDEFGHIJK",
    "ABCDEFGHIJKLM",
    "ABCDEFGHIJKLMN",
    "ABCDEFGHIJKLMNO",
    "ABCDEFGHIJKLMNOPQ",
    "ABCDEFGHIJKLMNOPQR",
    "ABCDEFGHIJKLMNOPQRS",
    "ABCDEFGHIJKLMNOPQRSTU",
    "ABCDEFGHIJKLMNOPQRSTUV",
    "ABCDEFGHIJKLMNOPQRSTUVW",
    "ABCDEFGHIJKLMNOPQRSTUVWXY",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/-", /* Priority vs. QPF_ERR_T_MEMORY. */
};
#define N_BAD_BASE64S ((sizeof BAD_BASE64S) / sizeof BAD_BASE64S[0])

static bool test_bad_char(const char* bad_base64)
    {
    bool success = true;
    
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	char buf[12];
	memset(buf, 'X', sizeof(buf));
	
	/** Test a bad format. **/
	const int rval = qpfPrintf(s, buf, 10, "%STR&DB64", bad_base64);
	
	/** Verify results. **/
	success &= EXPECT_TRUE(s->Errors & QPF_ERR_T_BADCHAR);
	s->Errors &= ~QPF_ERR_T_BADCHAR; /* Clear the expected error. */
	success &= EXPECT_NO_ERRORS(s);
	success &= EXPECT_EQL(rval, -22, "%d");
	success &= EXPECT_STR_EQL_N(buf, "\0XXXXXXXXXXX", sizeof(buf));
	
	/** Clean up. **/
	qpfCloseSession(s);
    
    return success;
    }

static bool doTest(void)
    {
    bool success = true;
	
	/** Test all specified bad formats. **/
	for (size_t i = 0u; i < N_BAD_BASE64S; i++)
	    {
	    const char* bad_base64 = BAD_BASE64S[i];
	    if (UNLIKELY(!test_bad_char(bad_base64)))
		{
		fprintf(stderr, "Test failed for file path: \"%s\"\n", bad_base64);
		success = false;
		break;
		}
	    }
    
    return success;
    }

long long
test(char** tname)
    {
	*tname = "qprintf-90 Error detection: Bad Char";

    return loopTest(doTest) * N_BAD_BASE64S;
    }
