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

	*tname = "qprintf-58 %STR&PATH valid pathname";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[25] = '\n';
	    buf[24] = '\0';
	    buf[23] = 0xff;
	    buf[22] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "one/two");
	    qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "one/two");
	    qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "one/two");
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "one/two");
	    assert(strcmp(buf+4,"/path/one/two/name") == 0);
	    assert(rval == 18);
	    assert(buf[25] == '\n');
	    assert(buf[24] == '\0');
	    assert(buf[23] == 0xff);
	    assert(buf[22] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

