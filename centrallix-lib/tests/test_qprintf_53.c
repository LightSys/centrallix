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

	*tname = "qprintf-53 Bugtest: &nbsp; following %STR&HTE";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[41] = '\n';
	    buf[40] = '\0';
	    buf[39] = 0xff;
	    buf[38] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 35, "Here is the str: %STR&HTE&nbsp;", "<tag>");
	    qpfPrintf(NULL, buf+4, 35, "Here is the str: %STR&HTE&nbsp;", "<tag>");
	    qpfPrintf(NULL, buf+4, 35, "Here is the str: %STR&HTE&nbsp;", "<tag>");
	    rval = qpfPrintf(NULL, buf+4, 35, "Here is the str: %STR&HTE&nbsp;", "<tag>");
	    assert(!strcmp(buf+4, "Here is the str: &lt;tag&gt;&nbsp;"));
	    assert(rval == 34);
	    assert(buf[41] == '\n');
	    assert(buf[40] == '\0');
	    assert(buf[39] == 0xff);
	    assert(buf[38] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

