#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include "util.h"

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
	    qpfPrintf(NULL, buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, buf+4, 36, "The HTML: '%STR&HTE'.", "<b c=\"w\">");
	    assert(!strcmp(buf+4, "The HTML: '&lt;b c=&quot;w&quot;"));
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

	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /* UTF-8 */
	    rval = qpfPrintf(session, buf+4, 36, "超文: '%STR&HTE'.", "<b c=\"€\">"); /* no split */
	    assert(strcmp(buf+4, "超文: '&lt;b c=&quot;€&quot;") == 0);
	    assert(rval == 38);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    assert(buf[39] == '\n');
	    assert(buf[38] == '\0');
	    assert(buf[37] == 0xff);
	    assert(buf[36] == '\0');
	    assert(buf[35] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    rval = qpfPrintf(session, buf+4, 36, "超文: '%STR&HTE'.", "<b c=\"€\".超>"); /* split */
	    assert(strcmp(buf+4, "超文: '&lt;b c=&quot;€&quot;.") == 0);
	    assert(rval == 39);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
		
	    }

	nmSysFree(session);
    return iter*4;
    }

