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

	*tname = "qprintf-45 %STR&HEX&NLEN in middle, no overflow";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    buf[33] = '\n';
	    buf[32] = '\0';
	    buf[31] = 0xff;
	    buf[30] = '\0';
	    buf[29] = '\0';
	    buf[28] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 27, "Enc: %STR&HEX&18LEN...", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 27, "Enc: %STR&HEX&18LEN...", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 27, "Enc: %STR&HEX&18LEN...", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, buf+4, 27, "Enc: %STR&HEX&18LEN...", "<b c=\"w\">");
	    assert(!strcmp(buf+4, "Enc: 3c6220633d2277223e..."));
	    assert(rval == 26);
	    assert(buf[33] == '\n');
	    assert(buf[32] == '\0');
	    assert(buf[31] == 0xff);
	    assert(buf[30] == '\0');
	    assert(buf[29] != '\0');
	    assert(buf[28] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(verifyUTF8(buf+4) == 0);

	    /** UTF-8 **/
            qpfPrintf(session, buf+4, 27, "编: %STR&HEX&18LEN...", "<b c=\"w\">");	    
            rval = qpfPrintf(session, buf+4, 27, "编: %STR&HEX&18LEN...", "<b c=\"w\">");
	    assert(strcmp(buf+4, "编: 3c6220633d2277223e...") == 0);
	    assert(verifyUTF8(buf+4) == 0);
            assert(rval == 26);
	    assert(buf[33] == '\n');
	    assert(buf[32] == '\0');
	    assert(buf[31] == 0xff);
	    assert(buf[30] == '\0');
	    assert(buf[29] != '\0');
	    assert(buf[28] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

	nmSysFree(session);
    return iter*4;
    }

