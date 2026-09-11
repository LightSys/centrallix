#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include <stdbool.h>
#include "test_utils.h"

static bool
doTest(void)
    {
    int rval;
    unsigned char buf[44];

	buf[43] = '\n';
	buf[42] = '\0';
	buf[41] = 0xff;
	buf[40] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	qpfPrintf(NULL, (char*)buf+4, 36, "HTML: '%STR&HTE&25LEN'.", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 36, "HTML: '%STR&HTE&25LEN'.", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 36, "HTML: '%STR&HTE&25LEN'.", "<b c=\"w\">");
	rval = qpfPrintf(NULL, (char*)buf+4, 36, "HTML: '%STR&HTE&25LEN'.", "<b c=\"w\">");
	assert(!strcmp((char*)buf+4, "HTML: '&lt;b c=&quot;w&quot;&gt;'."));
	assert(rval == 34);
	assert(buf[43] == '\n');
	assert(buf[42] == '\0');
	assert(buf[41] == 0xff);
	assert(buf[40] == '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-38 %STR&HTE&NLEN in middle, len equal";
    return loopTest(doTest) * 4;
    }
