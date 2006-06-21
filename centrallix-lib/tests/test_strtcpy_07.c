#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "strtcpy.h"
#include <assert.h>

long long
test(char** tname)
    {
    int i,rval;
    int iter;
    unsigned char buf[44];

	*tname = "strtcpy-07 strtcpy() copy to 1 char buffer";
	iter = 800000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = 0x7f;
	    buf[39] = 0xff;
	    buf[6] = 0x7f;
	    buf[5] = 0xff;
	    buf[4] = 0x7f;
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    strtcpy(buf+4, "this is a string non-overflow test.?", 1);
	    strtcpy(buf+4, "this is a string non-overflow test.?", 1);
	    strtcpy(buf+4, "this is a string non-overflow test.?", 1);
	    rval = strtcpy(buf+4, "this is a string non-overflow test.?", 1);
	    assert(rval == -1);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == 0x7f);
	    assert(buf[39] == 0xff);
	    assert(buf[6] == 0x7f);
	    assert(buf[5] == 0xff);
	    assert(buf[4] == 0x00);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

