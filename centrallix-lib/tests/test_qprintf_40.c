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

	*tname = "qprintf-40 %STR&HTE&NLEN in middle, len 2 less";
	iter = 100000;
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
	    qpfPrintf(NULL, buf+4, 36, "HTML: '%STR&HTE&23LEN'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "HTML: '%STR&HTE&23LEN'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "HTML: '%STR&HTE&23LEN'.", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, buf+4, 36, "HTML: '%STR&HTE&23LEN'.", "<b c=\"w\">");
	    assert(!strcmp(buf+4, "HTML: '&lt;b c=&quot;w&quot;'."));
	    assert(rval == 30);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(verifyUTF8(buf+4) == 0);

	    /* UTF-8 */
	    rval = qpfPrintf(session, buf+4, 36, "超: '%STR&HTE&25LEN'.", "<b c=\"€\">");
	    assert(strcmp(buf+4, "超: '&lt;b c=&quot;€&quot;'.") == 0); /* no char split */
	    assert(rval == 31);
	    assert(verifyUTF8(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "超: '%STR&HTE&25LEN'.", "<b c=\"€\"超>");
	    assert(strcmp(buf+4, "超: '&lt;b c=&quot;€&quot;'.") == 0); /* char split */
	    assert(rval == 31);
	    assert(verifyUTF8(buf+4) == 0);
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

