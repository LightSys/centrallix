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

	buf[25] = '\n';
	buf[24] = '\0';
	buf[23] = 0xff;
	buf[22] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "one/../two");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "..");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "../one");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "one/..");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "/..");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "../");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/%STR&PATH/name", "/../");
	assert(rval < 0);
	assert(buf[25] == '\n');
	assert(buf[24] == '\0');
	assert(buf[23] == 0xff);
	assert(buf[22] == '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-59 %STR&PATH various invalid pathnames";
    return loopTests(doTests) * 7;
    }
