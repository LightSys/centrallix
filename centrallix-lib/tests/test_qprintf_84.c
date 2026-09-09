#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"
#include "test_qprintf.h"
#include "test_utils.h"
#include "expect.h"

const char* BAD_FORMATS[] = {
    "% ",
    "%str",
    "%sTR",
    "%XML",
    "%LS",
    "%E",
    "% Other Data",
};
#define N_BAD_FORMATS (sizeof(BAD_FORMATS) / sizeof(BAD_FORMATS[0]))

static bool test_bad_format(const char* bad_format)
    {
    bool success = true;
    
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	char buf[8];
	memset(buf, 'X', sizeof(buf));
	
	/** Test a bad format. **/
	const int rval = qpfPrintf(s, buf, 6, bad_format);
	
	/** Verify results. **/
	success &= EXPECT_TRUE(s->Errors & QPF_ERR_T_BADFORMAT);
	s->Errors &= ~QPF_ERR_T_BADFORMAT; /* Clear the expected error. */
	success &= EXPECT_NO_ERRORS(s);
	success &= EXPECT_EQL(rval, -22, "%d");
	success &= EXPECT_STR_EQL_N(buf, "\0XXXXXXX", sizeof(buf));
	
	/** Clean up. **/
	qpfCloseSession(s);
    
    return success;
    }

static bool doTest(void)
    {
    bool success = true;
	
	/** Test all specified bad formats. **/
	for (size_t i = 0u; i < N_BAD_FORMATS; i++)
	    {
	    const char* bad_format = BAD_FORMATS[i];
	    if (UNLIKELY(!test_bad_format(bad_format)))
		{
		fprintf(stderr, "Test failed for format: \"%s\"\n", bad_format);
		success = false;
		break;
		}
	    }
    
    return success;
    }

long long
test(char** tname)
    {
	*tname = "qprintf-84 Error detection: Bad Formats";

    return loopTest(doTest) * N_BAD_FORMATS;
    }
