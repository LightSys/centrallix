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

	*tname = "qprintf-63 %STR&DB64 overflow test";
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
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "dGVzdCBkYXRh");
	    assert(rval == 9);
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXphYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5eg");
	    assert(rval < 0);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

            assert(verifyUTF8(buf+4) == 0);
    
            rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6a4K+L4K6k4K6p4K+I");
	    assert(rval == 15);
	    assert(strcmp("à®à¯à®¤à®©à¯", buf+4) != 0);
    
	    /** this used to result in a buffer overread. Confirm bug is fixed **/
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "Li4");
	    char * test = "QW55IGRhdGEgSSB3YW50"; /* this is the data that used to be read */
	    assert(rval < 0);
	    assert(strcmp("..@Any data I want", buf+4) != 0);

	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "4K6H4K6k4K+BIOCukuCusOCvgSDgrqjgr4DgrqPgr43grp8g4K6J4K6k4K6+4K6w4K6j4K6u4K+N");
	    assert(rval < 0);
            assert(verifyUTF8(buf+4) == 0);
	    
	    /** test cutting off utf-8 chars **/
	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "0JLQtdC00Ywg0JHQvtCzINGC0LDQutC/0L7Qu9GO0LHQuNC7"); /* 1 byte too long */
	    assert(rval < 0);
            assert(verifyUTF8(buf+4) == 0);

	    rval = qpfPrintf(NULL, buf+4, 36, "%STR&DB64", "0JLQtdC00Ywg0JHQvtCz0YLQsNC60L/QvtC70Y7QsdC40Ls="); /*fits */
	    assert(rval == 35);
            assert(verifyUTF8(buf+4) == 0);
	    assert(strcmp("Ведь Богтакполюбил", buf+4) == 0);
	    

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*2;
    }

