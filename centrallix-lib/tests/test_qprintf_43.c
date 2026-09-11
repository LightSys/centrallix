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

	buf[32] = '\n';
	buf[31] = '\0';
	buf[30] = 0xff;
	buf[29] = '\0';
	buf[28] = '\0';
	buf[27] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	qpfPrintf(NULL, (char*)buf+4, 25, "Encode: %STR&HEX", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 25, "Encode: %STR&HEX", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 25, "Encode: %STR&HEX", "<b c=\"w\">");
	rval = qpfPrintf(NULL, (char*)buf+4, 25, "Encode: %STR&HEX", "<b c=\"w\">");
	assert(!strcmp((char*)buf+4, "Encode: 3c6220633d227722"));
	assert(rval == 26);
	assert(buf[32] == '\n');
	assert(buf[31] == '\0');
	assert(buf[30] == 0xff);
	assert(buf[29] == '\0');
	assert(buf[28] == '\0');
	assert(buf[27] != '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-43 %STR&HEX at end, overflow(2) in insert";
    return loopTest(doTest) * 4;
    }
