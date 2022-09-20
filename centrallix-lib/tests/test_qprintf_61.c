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

	*tname = "qprintf-61 %STR&PATH&nLEN length-limited insert tests";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[26] = '\n';
	    buf[25] = '\0';
	    buf[24] = 0xff;
	    buf[23] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "file/..\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "files/../");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "one/t/..");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "one/t/...");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "one/two/");
	    assert(strcmp(buf+4,"/path/one/two//name") == 0);
	    assert(rval == 19);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/%STR&PATH&8LEN/name", "one/two//");
	    assert(strcmp(buf+4,"/path/one/two//name") == 0);
	    assert(rval == 19);
	    assert(buf[26] == '\n');
	    assert(buf[25] == '\0');
	    assert(buf[24] == 0xff);
	    assert(buf[23] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /** UTF-8 **/
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&PATH&8LEN/file", "וק/..\0");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&PATH&8LEN/file", "דוק/../");
	    assert(rval < 0);
	    rval = qpfPrintf(NULL, buf+4, 31, "/path/to/%STR&PATH&8LEN/file", "\0דוק");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&PATH&8LEN/f", "𓂥_𓅘"); /** chops first char, cut short **/
	    assert(strcmp( "/path/to/𓂥_/f", buf+4) == 0);
	    assert(rval == 19);
	    rval = qpfPrintf(session, buf+4, 31, "/path/to/%STR&PATH&8LEN/f", "𓂥𓅘");
	    assert(verifyUTF8(buf+4) == 0);
	    assert(rval == 19);

	    assert(buf[26] == '\n');
	    assert(buf[25] == '\0');
	    assert(buf[24] == 0xff);
	    assert(buf[23] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*6;
    }

