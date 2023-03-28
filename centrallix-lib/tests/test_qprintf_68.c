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
	
    
	*tname = "qprintf-68 %STR&DHEX overflow test";
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
	    rval = qpfPrintf(session, buf+4, 36, "%STR&DHEX", "414243444546");
	    assert(rval == 6);
	    rval = qpfPrintf(session, buf+4, 36, "%STR&DHEX", "4142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F60616263646566676869");
	    assert(rval < 0);
	    rval = qpfPrintf(session, buf+4, 36, "%STR&DHEX", "4142434445464142434445464142434445464142434445464142434445464142434445"); /* exact fit */
	    assert(rval == 35);
	    rval = qpfPrintf(session, buf+4, 36, "%STR&DHEX", "414243444546414243444546414243444546414243444546414243444546414243444546"); /* exact fit */
	    assert(rval < 0);
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

