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

	buf[35] = '\n';
	buf[34] = '\0';
	buf[33] = 0xff;
	buf[32] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "..");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "../otherdir");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "dir/..");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "file/subfile");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "a/../b");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", ".");
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)buf+4, 31, "/path/to/%STR&FILE/file", "");
	assert(rval < 0);
	assert(buf[35] == '\n');
	assert(buf[34] == '\0');
	assert(buf[33] == 0xff);
	assert(buf[32] == '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-56 %STR&FILE various invalid filenames";
    return loopTest(doTest) * 7;
    }
