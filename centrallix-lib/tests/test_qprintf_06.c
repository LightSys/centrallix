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
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = QPF_F_ENFORCE_UTF8;

	*tname = "qprintf-06 constant string into 1-sized buf using qpfPrintf()";
	setlocale(0, "en_US.UTF-8");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;
	    buf[5] = 0x7f;
	    buf[4] = 0xff;
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 1, "this is a string overflow test.");
	    qpfPrintf(NULL, buf+4, 1, "this is a string overflow test.");
	    qpfPrintf(NULL, buf+4, 1, "this is a string overflow test.");
	    qpfPrintf(NULL, buf+4, 1, "this is a string overflow test.");
	    assert(!strcmp(buf+4, ""));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] == 0xff);
	    assert(buf[5] == 0x7f);
	    assert(buf[4] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /* utf8 */
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;
	    buf[5] = 0x7f;
	    buf[4] = 0xff;
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(session, buf+4, 1, "起 初 ， 神 創 造 天 地 。");
	    assert(!strcmp(buf+4, ""));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] == 0xff);
	    assert(buf[5] == 0x7f);
	    assert(buf[4] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }
		
	nmSysFree(session);
    return iter*4;
    }

