#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include <locale.h>

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = 0;
    unsigned char buf[44];

	setlocale(0, "en_US.UTF-8");
	qpfInitialize(); 
	

	*tname = "qprintf-59 %STR&PATH various invalid pathnames";
	iter = 100000;
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
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "one/../two");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "..");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "../one");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "one/..");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "/..");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "../");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/%STR&PATH/name", "/../");
	    assert(rval < 0);
	    /** utf-8 **/
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "test\xFF");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH/name", "Γειά σο\xCF");
	    assert(rval < 0);
	    assert(buf[25] == '\n');
	    assert(buf[24] == '\0');
	    assert(buf[23] == 0xff);
	    assert(buf[22] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*7;
    }

