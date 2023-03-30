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
	

	*tname = "qprintf-55 %STR&FILE valid filename";
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
	    qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "myfilename.sbd");
	    qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "myfilename.sbd");
	    qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "myfilename.sbd");
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "myfilename.sbd");
	    assert(!strcmp(buf+4, "/path/to/myfilename.sbd/file"));
	    assert(rval == 28);
	    assert(buf[35] == '\n');
	    assert(buf[34] == '\0');
	    assert(buf[33] == 0xff);
	    assert(buf[32] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
            
	    /** UTF-8 **/
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "イル名.文書");
	    assert(!strcmp(buf+4, "/path/to/イル名.文書/file"));
	    assert(rval == 30);
	    /** cut off normal chars **/
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "イル名.文書test"); 
	    assert(!strcmp(buf+4, "/path/to/イル名.文書test/"));
	    assert(rval == 34);
	    /** split utf-8 char **/
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "tests_イル名.文書"); 
	    assert(!strcmp(buf+4, "/path/to/tests_イル名.文"));
	    assert(rval == 36);
	    /** split without UTF8 **/
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "tests_イル名.文書"); 
	    assert(!strcmp(buf+4, "/path/to/tests_イル名.文\xe6\x9b"));
	    assert(rval == 36);

	    assert(buf[35] == '\n');
	    assert(buf[34] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }