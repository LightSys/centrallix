#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include "util.h"
#include <locale.h>

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    unsigned char buf[44];
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = 0;

	setlocale(0, "en_US.UTF-8");
	qpfInitialize(); 
	
	*tname = "qprintf-32 %STR&ESCQ&NLEN in middle eq to LEN";
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
	    qpfPrintf(session, buf+4, 36, "Here is the str: '%STR&ESCQ&10LEN'...", "\"ain't\"");
	    qpfPrintf(session, buf+4, 36, "Here is the str: '%STR&ESCQ&10LEN'...", "\"ain't\"");
	    qpfPrintf(session, buf+4, 36, "Here is the str: '%STR&ESCQ&10LEN'...", "\"ain't\"");
	    rval = qpfPrintf(session, buf+4, 36, "Here is the str: '%STR&ESCQ&10LEN'...", "\"ain't\"");
	    assert(!strcmp(buf+4, "Here is the str: '\\\"ain\\'t\\\"'..."));
	    assert(rval == 32);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /* UTF-8 */
	    rval = qpfPrintf(NULL, buf+4, 36, "εδώ οδός: '%STR&ESCQ&10LEN'...", "\"є'н\"");
	    assert(strcmp(buf+4, "εδώ οδός: '\\\"є\\'н\\\"'...") == 0);
	    assert(rval == 32);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
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

