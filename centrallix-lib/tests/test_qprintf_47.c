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

	buf[31] = '\n';
	buf[30] = '\0';
	buf[29] = 0xff;
	buf[28] = '\0';
	buf[27] = '\0';
	buf[26] = '\0';
	buf[3] = '\n';
	buf[2] = '\0';
	buf[1] = 0xff;
	buf[0] = '\0';
	qpfPrintf(NULL, (char*)buf+4, 27, "Enc: %STR&HEX&16LEN...", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 27, "Enc: %STR&HEX&16LEN...", "<b c=\"w\">");
	qpfPrintf(NULL, (char*)buf+4, 27, "Enc: %STR&HEX&16LEN...", "<b c=\"w\">");
	rval = qpfPrintf(NULL, (char*)buf+4, 27, "Enc: %STR&HEX&16LEN...", "<b c=\"w\">");
	assert(!strcmp((char*)buf+4, "Enc: 3c6220633d227722..."));
	assert(rval == 24);
	assert(buf[31] == '\n');
	assert(buf[30] == '\0');
	assert(buf[29] == 0xff);
	assert(buf[28] == '\0');
	assert(buf[27] != '\0');
	assert(buf[26] != '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

    return true;
    }

long long
test(char** tname)
    {
    *tname = "qprintf-47 %STR&HEX&NLEN in middle, insert overflow(2)";
    return loopTest(doTest) * 4;
    }
