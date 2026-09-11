#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"
#include "test_qprintf.h"
#include "test_utils.h"
#include "expect.h"

const char* BAD_FILE_NAMES[] = {
    "",
    ".",
    "..",
    "path/",
    "%/",
    "buffer overflow/", /* Tests priority vs. QPF_ERR_T_BUFOVERFLOW. */
};
#define N_BAD_FILE_NAMES (sizeof(BAD_FILE_NAMES) / sizeof(BAD_FILE_NAMES[0]))

static bool testBadFileName(const char* bad_file_name)
    {
    bool success = true;
    
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	char buf[8];
	memset(buf, 'X', sizeof(buf));
	
	/** Test a bad format. **/
	const int rval = qpfPrintf(s, buf, 6, "%STR&FILE", bad_file_name);
	
	/** Verify results. **/
	success &= EXPECT_TRUE(s->Errors & QPF_ERR_T_BADFILE);
	s->Errors &= ~QPF_ERR_T_BADFILE; /* Clear the expected error. */
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
	for (size_t i = 0u; i < N_BAD_FILE_NAMES; i++)
	    {
	    const char* bad_file_name = BAD_FILE_NAMES[i];
	    if (UNLIKELY(!testBadFileName(bad_file_name)))
		{
		fprintf(stderr, "Test failed for file name: \"%s\"\n", bad_file_name);
		success = false;
		break;
		}
	    }
    
    return success;
    }

long long
test(char** tname)
    {
	*tname = "qprintf-88 Error detection: Bad Files";

    return loopTest(doTest) * N_BAD_FILE_NAMES;
    }
