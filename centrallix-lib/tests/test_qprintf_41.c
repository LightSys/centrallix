#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    unsigned char buf[44];

	*tname = "qprintf-41 %STR&HEX in middle, no overflows";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    buf[36] = '\n';
	    buf[35] = '\0';
	    buf[34] = 0xff;
	    buf[33] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, buf+4, 36, "Encode: '%STR&HEX'.", "<b c=\"w\">");
	    assert(!strcmp(buf+4, "Encode: '3c6220633d2277223e'."));
	    assert(rval == 29);
	    assert(buf[36] == '\n');
	    assert(buf[35] == '\0');
	    assert(buf[34] == 0xff);
	    assert(buf[33] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

