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
    int i;
    int iter;
    unsigned char buf[44];

	*tname = "qprintf-04 empty string using qpfPrintf()";
	setlocale(0, "en_US.UTF-8");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;
	    buf[4] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 36, "");
	    qpfPrintf(NULL, buf+4, 36, "");
	    qpfPrintf(NULL, buf+4, 36, "");
	    qpfPrintf(NULL, buf+4, 36, "");
	    assert(!strcmp(buf+4,""));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] == 0xff);
	    assert(buf[4] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

		/* utf8 */
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;
	    buf[4] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 36, "起 初 ");
	    qpfPrintf(NULL, buf+4, 36, "起 初 ");
	    qpfPrintf(NULL, buf+4, 36, "起 初 ");
	    qpfPrintf(NULL, buf+4, 36, "起 初 ");
	    assert(!strcmp(buf+4,"起 初 "));
		assert(chrNoOverlong(buf+4) != 0);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] == 0xff);
	    assert(buf[4] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

    return iter*4;
    }

