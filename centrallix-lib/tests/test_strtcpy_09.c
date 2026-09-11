#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "strtcpy.h"
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
	buf[33] = 0x7f;	/* should NOT get overwritten by strtcpy() */
	buf[32] = 0xff;	/* should get overwritten by strtcpy() */
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	strtcpy((char*)buf+4, "this is a non-overflow test.", 36);
	strtcpy((char*)buf+4, "this is a non-overflow test.", 36);
	strtcpy((char*)buf+4, "this is a non-overflow test.", 36);
	rval = strtcpy((char*)buf+4, "this is a non-overflow test.", 36);
	assert(rval == 29);
	assert(!strcmp((char*)buf+4,"this is a non-overflow test."));
	assert(buf[43] == '\n');
	assert(buf[42] == '\0');
	assert(buf[41] == 0xff);
	assert(buf[40] == '\0');
	assert(buf[33] == 0x7f);
	assert(buf[32] != 0xff);
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "strtcpy-09 strtcpy() use less than entire buffer";
    return loopTest(doTest) * 4;
    }
