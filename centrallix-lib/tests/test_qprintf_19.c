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
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = QPF_F_ENFORCE_UTF8;
    setlocale(0, "en_US.UTF-8");

	*tname = "qprintf-19 %*STR insertion in middle with insert overflow";
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
	    qpfPrintf(NULL, buf+4, 36, "Here is the string: %*STR...", 8, "STRINGSTR");
	    qpfPrintf(NULL, buf+4, 36, "Here is the string: %*STR...", 8, "STRINGSTR");
	    qpfPrintf(NULL, buf+4, 36, "Here is the string: %*STR...", 8, "STRINGSTR");
	    rval = qpfPrintf(NULL, buf+4, 36, "Here is the string: %*STR...", 8, "STRINGSTR");
	    assert(!strcmp(buf+4, "Here is the string: STRINGST..."));
	    assert(rval == 31);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(chrNoOverlong(buf+4) == 0);

	    /* UTF-8 */
	    rval = qpfPrintf(session, buf+4, 36, "ជខ្សែអក្សរ: %*STR....", 8, "ΣEIPA");
	    assert(strcmp(buf+4, "ជខ្សែអក្សរ: ΣE") == 0);
	    assert(rval == 44);
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
		
	nmSysFree(session);
    return iter*4;
    }

