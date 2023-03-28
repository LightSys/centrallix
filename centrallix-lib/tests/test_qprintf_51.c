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
    unsigned char buf[44];
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = 0;

	setlocale(0, "en_US.UTF-8");
	qpfInitialize(); 
	

	*tname = "qprintf-51 %STR&QUOT at end with overflow(1)";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[39] = '\n';
	    buf[38] = '\0';
	    buf[37] = 0xff;
	    buf[36] = '\0';
	    buf[35] = '\0';
	    buf[34] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(session, buf+4, 32, "Here is the str...: %STR&QUOT", "\"ain't\"");
	    qpfPrintf(session, buf+4, 32, "Here is the str...: %STR&QUOT", "\"ain't\"");
	    qpfPrintf(session, buf+4, 32, "Here is the str...: %STR&QUOT", "\"ain't\"");
	    rval = qpfPrintf(session, buf+4, 32, "Here is the str...: %STR&QUOT", "\"ain't\"");
	    assert(!strcmp(buf+4, "Here is the str...: '\\\"ain\\'t'"));
	    assert(rval == 32);
	    assert(buf[39] == '\n');
	    assert(buf[38] == '\0');
	    assert(buf[37] == 0xff);
	    assert(buf[36] == '\0');
	    assert(buf[35] == '\0');
	    assert(buf[34] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /** UTF-8 **/
	    rval = qpfPrintf(NULL, buf+4, 32, "Str...: %STR&QUOT", "\"சோத.சோத.\"");
	    assert(strcmp(buf+4, "Str...: '\\\"சோத.சோத'") == 0);
            assert(rval == 33);
	    assert(strlen(buf+4) == 31);
    
	    rval = qpfPrintf(NULL, buf+4, 32, "Str...: %STR&QUOT", "\"சோத..சோத\"");
	    assert(strcmp(buf+4, "Str...: '\\\"சோத..சோ'") == 0);
            assert(rval == 33);
	    assert(strlen(buf+4) == 29); /*added 1 char, which cut off 3 bytes (to not split char) so 2 shorter */

	    assert(buf[39] == '\n');
	    assert(buf[38] == '\0');
	    assert(buf[37] == 0xff);
	    assert(buf[36] == '\0');
	    assert(buf[35] == '\0');
	    assert(buf[34] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    nmSysFree(session);
    return iter*4;
    }

