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
	setlocale(0, "en_US.UTF-8");
	*tname = "qprintf-62 %STR&DB64 integrity test";
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
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "dGVzdC#BkYXRh");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "#dGVzdCBkYXRh");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "dGVzdCBkYXRh#");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "dGVzdCBkY#XRh");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "dGVzdCBkYXRh");
	    assert(strcmp(buf+4,"test data") == 0);
	    assert(rval == 9);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

            assert(chrNoOverlong(buf+4) == 0);

            rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "#4K6a4K+L4K6k4K6p4K+I");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6a4K+L4K6k4K6p4K+I#");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6a4K+L4#K6k4K6p4K+I");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6a4K+L4K6k4K6p#4K+I");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6a4K+L4K6k4K6p4K+I");
	    assert(strcmp(buf+4,"சோதனை") == 0);
	    assert(rval == 15);
            assert(chrNoOverlong(buf+4) == 0);

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*6;
    }

