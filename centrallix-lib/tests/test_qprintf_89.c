#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"
#include "test_qprintf.h"
#include "test_utils.h"
#include "expect.h"

const char* BAD_FILE_PATHS[] = {
    "",
    "..",
    "../",
    "a/.."
    "a/../a",
    "../% ",
    "%/..",
    "%/../%",
    "../buffer overflow", /* Tests priority vs. QPF_ERR_T_BUFOVERFLOW. */
    "buffer overflow/..", /* Tests priority vs. QPF_ERR_T_BUFOVERFLOW. */
    "buffer/../overflow", /* Tests priority vs. QPF_ERR_T_BUFOVERFLOW. */
};
#define N_BAD_FILE_PATHS (sizeof(BAD_FILE_PATHS) / sizeof(BAD_FILE_PATHS[0]))

static bool test_bad_file_path(const char* bad_file_path)
    {
    bool success = true;
    
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	char buf[12];
	memset(buf, 'X', sizeof(buf));
	
	/** Test a bad format. **/
	const int rval = qpfPrintf(s, buf, 10, "%STR&PATH", bad_file_path);
	
	/** Verify results. **/
	success &= EXPECT_TRUE(s->Errors & QPF_ERR_T_BADPATH);
	s->Errors &= ~QPF_ERR_T_BADPATH; /* Clear the expected error. */
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
	for (size_t i = 0u; i < N_BAD_FILE_PATHS; i++)
	    {
	    const char* bad_file_path = BAD_FILE_PATHS[i];
	    if (UNLIKELY(!test_bad_file_path(bad_file_path)))
		{
		fprintf(stderr, "Test failed for file path: \"%s\"\n", bad_file_path);
		success = false;
		break;
		}
	    }
    
    return success;
    }

long long
test(char** tname)
    {
	*tname = "qprintf-89 Error detection: Bad Paths";

    return loopTest(doTest) * N_BAD_FILE_PATHS;
    }
