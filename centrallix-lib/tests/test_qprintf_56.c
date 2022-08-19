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

	*tname = "qprintf-56 %STR&FILE various invalid filenames";
	iter = 100000;
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
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "..");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "../otherdir");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "dir/..");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "file/subfile");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "a/../b");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", ".");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "");
	    assert(rval < 0);
	    assert(buf[35] == '\n');
	    assert(buf[34] == '\0');
	    assert(buf[33] == 0xff);
	    assert(buf[32] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /** utf-8 **/
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "イル\xFF名.文書");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&FILE/file", "イル名.文\xe6\x9b");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&FILE/file", "イル名.文\xe6\x9b"); /* without flag, will pass */
	    assert(rval > 0);
            
	    assert(buf[35] == '\n');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    
	    }

    return iter*7;
    }

