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

	*tname = "strtcpy-03 strtcpy() full-size non-overflow";
	iter = 800000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;	/* should get overwritten by strtcpy() */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    strtcpy(buf+4, "this is a string non-overflow test.", 36);
	    strtcpy(buf+4, "this is a string non-overflow test.", 36);
	    strtcpy(buf+4, "this is a string non-overflow test.", 36);
	    rval = strtcpy(buf+4, "this is a string non-overflow test.", 36);
	    assert(rval == 36);
	    assert(!strcmp(buf+4,"this is a string non-overflow test."));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

