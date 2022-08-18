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

	*tname = "qprintf-12 %STR insertion in middle with overflow in STR";
	setlocale(0, "en_US.UTF-8");
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
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is data word %STR is our......", "STRING");
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is data word %STR is our......", "STRING");
	    qpfPrintf(NULL, buf+4, 36, "The overflow this is data word %STR is our......", "STRING");
	    rval = qpfPrintf(NULL, buf+4, 36, "The overflow this is data word %STR is our......", "STRING");
	    assert(!strcmp(buf+4, "The overflow this is data word STRI"));
	    assert(rval == 50);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';

	    rval = qpfPrintf(session, buf+4, 36, "起 地 。 आदि: %STR में परमेश्वर ने आकाश और पृथ्वी को बनाया", "Сотворил"); /* last char fits */
	    assert(!strcmp(buf+4, "起 地 。 आदि: Сотвор"));
	    assert(rval == 143);

	    rval = qpfPrintf(session, buf+4, 36, "起 地 。 आदि:  %STR में परमेश्वर ने आकाश और पृथ्वी को बनाया", "Сотворил"); /* cut off р */
	    assert(!strcmp(buf+4, "起 地 。 आदि:  Сотво"));
	    assert(rval == 144);

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

	nmSysFree(session);
    return iter*4;
    }

