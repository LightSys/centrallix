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

	*tname = "qprintf-48 %STR&QUOT in middle without overflow";
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
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&QUOT...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&QUOT...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&QUOT...", "\"ain't\"");
	    rval = qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&QUOT...", "\"ain't\"");
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
	    
            assert(verifyUTF8(buf+4) == 0);

	    /** UTF-8 **/

            qpfPrintf(session, buf+4, 36, "Here is the str: %STR&QUOT...", "\"சோத\"");
	    rval = qpfPrintf(session, buf+4, 36, "Here is the str: %STR&QUOT...", "\"சோத\"");
	    assert(strcmp(buf+4, "Here is the str: '\\\"சோத\\\"'...") == 0);
	    assert(rval == 35);
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

