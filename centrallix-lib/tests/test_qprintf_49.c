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
    setlocale(0, "en_US.UTF-8");

	*tname = "qprintf-49 %STR&DQUOT in middle without overflow";
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
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"ain't\"");
	    rval = qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"ain't\"");
	    assert(!strcmp(buf+4, "Here is the str: \"\\\"ain\\'t\\\"\"..."));
	    assert(rval == 32);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

            assert(chrNoOverlong(buf+4) == 0);

            qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"சோத\"");
            rval = qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&DQUOT...", "\"சோத\"");
            assert(rval == 35);
            assert(strcmp(buf+4, "Here is the str: \"\\\"சோத\\\"\"...") == 0);
	    
            assert(chrNoOverlong(buf+4) == 0);

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

