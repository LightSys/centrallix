#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include <errno.h>

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    unsigned char buf[44];

	*tname = "qprintf-16 %POS insertion without overflow, negative";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(buf+4, 36, "Here is the integer: %POS...", -12345);
	    qpfPrintf(buf+4, 36, "Here is the integer: %POS...", -12345);
	    qpfPrintf(buf+4, 36, "Here is the integer: %POS...", -12345);
	    rval = qpfPrintf(buf+4, 36, "Here is the integer: %POS...", -12345);
	    assert(strlen(buf+4) <= 30);
	    assert(rval == -EINVAL);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

