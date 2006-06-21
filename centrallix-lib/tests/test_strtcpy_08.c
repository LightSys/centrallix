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

	/**
	 ** Main reason to have both a 1 char and 2 char test is that
	 ** the compiler's optimization seems to partly unroll the
	 ** loop into a copy-first-char and then copy-subsequent-chars
	 ** sections (since 1st char requires no index offsets)
	 **/
	*tname = "strtcpy-08 strtcpy() copy to 2 char buffer";
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
	    strtcpy(buf+4, "this is a string non-overflow test.?", 2);
	    strtcpy(buf+4, "this is a string non-overflow test.?", 2);
	    strtcpy(buf+4, "this is a string non-overflow test.?", 2);
	    rval = strtcpy(buf+4, "this is a string non-overflow test.?", 2);
	    assert(rval == -2);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == 0x7f);
	    assert(buf[39] == 0xff);
	    assert(buf[6] == 0x7f);
	    assert(buf[5] == 0x00);
	    assert(buf[4] == 't');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

