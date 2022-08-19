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
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = QPF_F_ENFORCE_UTF8;
    unsigned char buf[44];
    setlocale(0, "en_US.UTF-8");

	*tname = "qprintf-57 %nSTR&FILE tests with fixed length insert";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[35] = '\n';
	    buf[34] = '\0';
	    buf[33] = 0xff;
	    buf[32] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "file\0ame");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "filenam\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "\0ilename");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%2STR&FILE/file", "...");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "filename");
	    assert(rval == 22);
	    assert(buf[35] == '\n');
	    assert(buf[34] == '\0');
	    assert(buf[33] == 0xff);
	    assert(buf[32] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "וק\0ד.");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "דוק\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%8STR&FILE/file", "\0דוק");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%8STR&FILE/file", "𓂥.𓅘"); /** chops first char, cut short **/
	    assert(strcmp( "/path/to/𓂥./file", buf+4) == 0);
	    assert(rval == 22);
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%8STR&FILE/file", "𓂥𓅘");
	    assert(chrNoOverlong(buf+4) == 0);
	    assert(rval == 22);
	    assert(buf[35] == '\n');
	    assert(buf[34] == '\0');
	    assert(buf[33] == 0xff);
	    assert(buf[32] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*5;
    }

