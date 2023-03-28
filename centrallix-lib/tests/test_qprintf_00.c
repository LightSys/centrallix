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
    int i;
    int iter;
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = QPF_F_ENFORCE_UTF8;
    unsigned char buf[44];
    
	setlocale(0, "en_US.UTF-8");
	qpfInitialize(); 
	
	*tname = "qprintf-00 constant string using snprintf()";
	setlocale(0, "en_US.UTF-8");
	iter = 2000000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    snprintf(buf+4, 36, "this is a string non-overflow test.");
	    snprintf(buf+4, 36, "this is a string non-overflow test.");
	    snprintf(buf+4, 36, "this is a string non-overflow test.");
	    snprintf(buf+4, 36, "this is a string non-overflow test.");
	    assert(!strcmp(buf+4,"this is a string non-overflow test."));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

		/* test with utf8 */
		buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    snprintf(buf+4, 36, "В Начале Сотворил Б");
	    assert(!strcmp(buf+4,"В Начале Сотворил Б"));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

        nmSysFree(session);
    return iter*4;
    }

