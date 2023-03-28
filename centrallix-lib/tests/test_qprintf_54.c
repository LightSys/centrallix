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
	

	*tname = "qprintf-54 Bugtest: %[ %] with static data";
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[38] = '\n';
	    buf[37] = '\0';
	    buf[36] = 0xff;
	    buf[35] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(session, buf+4, 31, "Conditional: yes=%[yes%STR%] no=%[no%STR%]%STR", 1, "!YES!", 0, "!NO!", ".");
	    qpfPrintf(session, buf+4, 31, "Conditional: yes=%[yes%STR%] no=%[no%STR%]%STR", 1, "!YES!", 0, "!NO!", ".");
	    qpfPrintf(session, buf+4, 31, "Conditional: yes=%[yes%STR%] no=%[no%STR%]%STR", 1, "!YES!", 0, "!NO!", ".");
	    rval = qpfPrintf(session, buf+4, 31, "Conditional: yes=%[yes%STR%] no=%[no%STR%]%STR", 1, "!YES!", 0, "!NO!", ".");
	    assert(!strcmp(buf+4, "Conditional: yes=yes!YES! no=."));
	    assert(rval == 30);
	    assert(buf[38] == '\n');
	    assert(buf[37] == '\0');
	    assert(buf[36] == 0xff);
	    assert(buf[35] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
 
            /** test utf-8 overflow **/
 	    rval = qpfPrintf(NULL, buf+4, 31, "Conditional: test=%[utf8: %STR%]", 0, "ジー・ウィズ・ポップス");
	    assert(!strcmp(buf+4, "Conditional: test="));
	    assert(rval == 18);

	    rval = qpfPrintf(NULL, buf+4, 31, "Conditional: test =%[utf8: %STR%]", 1, "ジー・ウィズ・ポップス");
	    assert(!strcmp(buf+4, "Conditional: test =utf8: ジ"));
	    assert(rval == 58);

	    assert(buf[38] == '\n');
	    assert(buf[37] == '\0');
	    assert(buf[36] == 0xff);
	    assert(buf[35] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    }

    return iter*4;
    }

