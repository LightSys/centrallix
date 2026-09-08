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
doTests(void)
    {
    unsigned char buf[44];

	buf[43] = '\n';
	buf[42] = '\0';
	buf[41] = 0xff;
	buf[40] = '\0';
	buf[39] = 0xff;
	buf[5] = 0x7f;
	buf[4] = 0xff;
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	qpfPrintf(NULL, (char*)buf+4, 1, "this is a string overflow test.");
	qpfPrintf(NULL, (char*)buf+4, 1, "this is a string overflow test.");
	qpfPrintf(NULL, (char*)buf+4, 1, "this is a string overflow test.");
	qpfPrintf(NULL, (char*)buf+4, 1, "this is a string overflow test.");
	assert(!strcmp((char*)buf+4, ""));
	assert(buf[43] == '\n');
	assert(buf[42] == '\0');
	assert(buf[41] == 0xff);
	assert(buf[40] == '\0');
	assert(buf[39] == 0xff);
	assert(buf[5] == 0x7f);
	assert(buf[4] != 0xff);
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-06 constant string into 1-sized buf using qpfPrintf()";
    return loopTests(doTests) * 4;
    }
