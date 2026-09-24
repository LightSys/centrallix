#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"

/** Room for the longest possible output, plus guard bytes on either side. **/
#define GUARD	4
#define AREA	512

static long long ll_values[] =
    {
    0LL,
    -1LL,
    1LL,
    LLONG_MAX,
    LLONG_MIN,
    };

static double dbl_values[] =
    {
    0.0,
    -0.0,
    3.14159,
    DBL_MIN,
    DBL_MAX,
    -DBL_MAX,
    };

/*** Verifies that nothing outside the destination area was touched. ***/
static void
check_guards(const char* buf)
    {
    unsigned int i;

	for(i=0;i<GUARD;i++)
	    {
	    assert((unsigned char)buf[i] == 0xff);
	    assert((unsigned char)buf[GUARD + AREA + i] == 0xff);
	    }

    return;
    }


long long
test(char** tname)
    {
    unsigned int i, v, iter, calls = 0;
    pQPSession s;
    char buf[GUARD + AREA + GUARD];
    char* dst = buf + GUARD;
    char expected[AREA];
    int rval, explen;

	/*** The %LL and %DBL specifiers format their value with snprintf()
	 *** before copying it out, and the length that snprintf() reports is
	 *** what drives the copy.  This test checks the extremes of both
	 *** types, where that reported length is largest: "%lf" of -DBL_MAX
	 *** is 317 characters, one short of the internal buffer's 318.  The
	 *** output must match snprintf() exactly, no error may be recorded,
	 *** and nothing outside the destination may be touched.
	 ***/

	*tname = "qprintf-71 %LL and %DBL at the extremes of their types";
	s = qpfOpenSession();
	assert(s != NULL);
	iter = 1000;
	for(i=0;i<iter;i++)
	    {
	    for(v=0;v<sizeof(ll_values)/sizeof(ll_values[0]);v++)
		{
		explen = snprintf(expected, sizeof(expected), "%lld", ll_values[v]);
		memset(buf, 0xff, sizeof(buf));
		rval = qpfPrintf(s, dst, AREA, "%LL", ll_values[v]);
		assert(rval == explen);
		assert(!strcmp(dst, expected));
		check_guards(buf);
		calls++;
		}

	    for(v=0;v<sizeof(dbl_values)/sizeof(dbl_values[0]);v++)
		{
		explen = snprintf(expected, sizeof(expected), "%lf", dbl_values[v]);
		memset(buf, 0xff, sizeof(buf));
		rval = qpfPrintf(s, dst, AREA, "%DBL", dbl_values[v]);
		assert(rval == explen);
		assert(!strcmp(dst, expected));
		check_guards(buf);
		calls++;
		}

	    /** None of these values may be reported as an error. **/
	    assert(qpfErrors(s) == QPF_ERR_T_NO_ERRORS);
	    }
	qpfCloseSession(s);

    return calls;
    }
