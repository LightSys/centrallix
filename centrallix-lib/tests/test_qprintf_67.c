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

	*tname = "qprintf-67 %STR&DHEX integrity test";
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
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "4142434D4E4F#");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "414243#4D4E4F");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "41424#34D4E4F");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "#4142434D4E4F");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "4142434D4E4");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "4142434D4E4F");
	    assert(strcmp(buf+4,"ABCMNO") == 0);
	    assert(rval == 6);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DHEX", "4142434d4e4f");
	    assert(strcmp(buf+4,"ABCMNO") == 0);
	    assert(rval == 6);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*7;
    }

