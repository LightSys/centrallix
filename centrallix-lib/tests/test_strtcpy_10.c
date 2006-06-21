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

	*tname = "strtcpy-10 strtcpy() copy empty string into nonempty buffer";
	iter = 800000;
	for(i=0;i<iter;i++)
	    {
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
	    strtcpy(buf+4, "", 36);
	    strtcpy(buf+4, "", 36);
	    strtcpy(buf+4, "", 36);
	    rval = strtcpy(buf+4, "", 36);
	    assert(rval == 1);
	    assert(!strcmp(buf+4,""));
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
	    }

    return iter*4;
    }

