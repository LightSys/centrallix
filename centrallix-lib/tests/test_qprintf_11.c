#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include "util.h"
#include "util.h"
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
	
	*tname = "qprintf-11 %STR insertion in middle with overflow after STR";
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
	    qpfPrintf(session, buf+4, 36, "The word %STR is our data... this is an overflow", "STRING");
	    qpfPrintf(session, buf+4, 36, "The word %STR is our data... this is an overflow", "STRING");
	    qpfPrintf(session, buf+4, 36, "The word %STR is our data... this is an overflow", "STRING");
	    rval = qpfPrintf(NULL, buf+4, 36, "The word %STR is our data... this is an overflow", "STRING");
	    assert(!strcmp(buf+4, "The word STRING is our data... this"));
	    assert(rval == 50);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /* utf8 */
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';

	    rval = qpfPrintf(NULL, buf+4, 36, "起 %STR 地 。 आदि में प", "Сотворил"); /* last char fits off next char */
	    assert(!strcmp(buf+4, "起 Сотворил 地 。 आद"));
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    assert(rval == 52);

	    rval = qpfPrintf(NULL, buf+4, 36, "起 %STR 地 。  आदि में प", "Сотворил"); /* cuts off next char */
	    assert(!strcmp(buf+4, "起 Сотворил 地 。  आ"));
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    assert(rval == 53);
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

