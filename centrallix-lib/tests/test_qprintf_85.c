#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"
#include "test_qprintf.h"
#include "test_utils.h"

static bool doTest(void)
    {
    bool success = true;
	
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	char buf[8];
	memset(buf, 'X', sizeof(buf));
	
	/** Test string buffer overflow. **/
	const int rval = qpfPrintf(s, buf, 6, "%STR&ESCQ&ESCQ&ESCQ&ESCQ&ESCQ&ESCQ&ESCQ&ESCQ", "Str");
	
	/** Verify results. **/
	success &= EXPECT_TRUE(s->Errors & QPF_ERR_T_RESOURCE);
	s->Errors &= ~QPF_ERR_T_RESOURCE; /* Clear the expected error. */
	success &= EXPECT_NO_ERRORS(s);
	success &= EXPECT_EQL(rval, -12, "%d");
	success &= EXPECT_STR_EQL_N(buf, "\0XXXXXXX", sizeof(buf));
	
	/** Clean up. **/
	qpfCloseSession(s);
    
    return success;
    }


long long
test(char** tname)
    {
	*tname = "qprintf-85 Error detection: Resource Exhaustion";

    return loopTest(doTest);
    }
