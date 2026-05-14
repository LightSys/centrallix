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

	*tname = "qprintf-36 %STR&HTE in middle, overflow 1 char";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    buf[39] = '\n';
	    buf[38] = '\0';
	    buf[37] = 0xff;
	    buf[36] = '\0';
	    buf[35] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, (char*)buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    qpfPrintf(NULL, (char*)buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    qpfPrintf(NULL, (char*)buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, (char*)buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    assert(!strcmp((char*)buf+4, "The HTML: '&lt;b c=&quot;w&quot;"));
	    assert(rval == 38);
	    assert(buf[39] == '\n');
	    assert(buf[38] == '\0');
	    assert(buf[37] == 0xff);
	    assert(buf[36] == '\0');
	    assert(buf[35] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }
