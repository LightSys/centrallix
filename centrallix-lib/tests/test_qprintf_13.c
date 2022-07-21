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
    int i, rval, numBytesAttempted;
    int iter;
    unsigned char buf[44];

	*tname = "qprintf-13 %STR insertion in middle with overflow before STR";
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
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is...... data word %STR is our", "STRING");
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is...... data word %STR is our", "STRING");
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is...... data word %STR is our", "STRING");
	    rval = qpfPrintf(NULL, buf+4, 36, "The overflow this is...... data word %STR is our", "STRING");
	    assert(!strcmp(buf+4, "The overflow this is...... data wor"));
	    assert(rval == 50);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /* UTF-8 */
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    numBytesAttempted = qpfPrintf(NULL, buf+4, 36, "溢出这是......数据 %STR 是我们的....", "ΣEIPA");
	    assert(strcmp(buf+4, "溢出这是......数据 ΣEIPA 是我") < 0);
	    assert(strcmp(buf+4, "溢出这是......数据 ΣEIPA 是我们的..") == 0);
	    assert(numBytesAttempted == 38);
	    // assert(chrNoOverlong(buf+4) == 0);
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

