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
    int rval;
    unsigned char buf[44];

	buf[36] = '\n';
	buf[35] = '\0';
	buf[34] = 0xff;
	buf[33] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	qpfPrintf(NULL, (char*)buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	rval = qpfPrintf(NULL, (char*)buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	assert(!strcmp((char*)buf+4, "Encode: '3c6220633d2277223e'."));
	assert(rval == 29);
	assert(buf[36] == '\n');
	assert(buf[35] == '\0');
	assert(buf[34] == 0xff);
	assert(buf[33] == '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-41 %STR&HEX in middle, no overflows";
    return loopTests(doTests) * 4;
    }
