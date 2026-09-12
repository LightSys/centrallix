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
	buf[5] = 0x7f;	/* should NOT get overwritten by strtcpy() */
	buf[4] = 0xff;	/* should get overwritten by strtcpy() */
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	strtcpy((char*)buf+4, "", 36);
	strtcpy((char*)buf+4, "", 36);
	strtcpy((char*)buf+4, "", 36);
	rval = strtcpy((char*)buf+4, "", 36);
	assert(rval == 1);
	assert(!strcmp((char*)buf+4,""));
	assert(buf[43] == '\n');
	assert(buf[42] == '\0');
	assert(buf[41] == 0xff);
	assert(buf[40] == '\0');
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
    *tname = "strtcpy-10 strtcpy() copy empty string into nonempty buffer";
    return loopTest(doTest) * 4;
    }
