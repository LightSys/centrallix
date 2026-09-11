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
    int rval;
	
	/** Initialize data structures. **/
	pQPSession s = qpfOpenSession();
	const size_t buf_len = ALL_SPECS_RESULT_LEN + 4;
	char buf[buf_len];
	memset(buf, 'X', sizeof(buf));
	buf[buf_len - 1] = '\0';
	
	/** Test not ignored. **/
	rval = qpfPrintf(s, buf, ALL_SPECS_RESULT_LEN + 2, "%["ALL_SPECS"%]", true, ALL_SPECS_VALUES);
	
	/** Verify results. **/
	success &= EXPECT_NO_ERRORS(s);
	success &= EXPECT_EQL(rval, (int)ALL_SPECS_RESULT_LEN - 1, "%d");
	success &= EXPECT_STR_EQL_N(buf, ALL_SPECS_RESULT"\0XXX", buf_len);
	
	/** Reset structs for 2nd test case. **/
	memset(buf, 'X', sizeof(buf));
	buf[buf_len - 1] = '\0';
	char original_buf[buf_len];
	memcpy(original_buf, buf, buf_len);
	
	/** Test ignored. **/
	rval = qpfPrintf(s, buf, ALL_SPECS_RESULT_LEN + 2, "%["ALL_SPECS"%]", false, ALL_SPECS_VALUES);
	
	/** Verify results. **/
	success &= EXPECT_NO_ERRORS(s);
	success &= EXPECT_EQL(rval, 0, "%d");
	success &= EXPECT_EQL(buf[0], '\0', "%c");
	buf[0] = 'Y';
	success &= EXPECT_STR_EQL(buf + 1, original_buf + 1);
	
	/** Clean up. **/
	qpfCloseSession(s);
    
    return success;
    }


long long
test(char** tname)
    {
	*tname = "qprintf-78 Characters should not escape conditionals";

    return loopTest(doTest) * 2;
    }
