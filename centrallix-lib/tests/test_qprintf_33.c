#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include "util.h"

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

	*tname = "qprintf-33 %STR&ESCQ&NLEN in middle 1 greater than LEN";
	iter = 100000;
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
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: '%STR&ESCQ&9LEN'...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: '%STR&ESCQ&9LEN'...", "\"ain't\"");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: '%STR&ESCQ&9LEN'...", "\"ain't\"");
	    rval = qpfPrintf(NULL, buf+4, 36, "Here is the str: '%STR&ESCQ&9LEN'...", "\"ain't\"");
	    assert(!strcmp(buf+4, "Here is the str: '\\\"ain\\'t'..."));
	    assert(rval == 30);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    /* UTF-8 */
		
	    /* 2 bytes */
	    rval = qpfPrintf(session, buf+4, 36, "εδώ οδός: '%STR&ESCQ&9LEN'...", "\"є.нς\""); /* fits */
	    assert(strcmp(buf+4, "εδώ οδός: '\\\"є.нς'...") == 0);
	    assert(rval == 31);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "εδώ οδός: '%STR&ESCQ&9LEN'...", "\"є..нς\""); /* 1 byte lost */
	    assert(strcmp(buf+4, "εδώ οδός: '\\\"є..н'...") == 0);
	    assert(rval == 30);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /* 3 bytes */
	    rval = qpfPrintf(session, buf+4, 36, "に神は: '%STR&ESCQ&9LEN'...", "\"ひ.と\""); /* fits */
	    assert(strcmp(buf+4, "に神は: '\\\"ひ.と'...") == 0);
	    assert(rval == 25);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "に神は: '%STR&ESCQ&9LEN'...", "\"ひ..と\""); /* 1 byte off */
	    assert(strcmp(buf+4, "に神は: '\\\"ひ..'...") == 0);
	    assert(rval == 23);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "に神は: '%STR&ESCQ&9LEN'...", "\"ひ...と\""); /* 2 bytes off */
	    assert(strcmp(buf+4, "に神は: '\\\"ひ...'...") == 0);
	    assert(rval == 24);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /* 4 bytes */
	    rval = qpfPrintf(session, buf+4, 36, "𓀄𓀅𓀆: '%STR&ESCQ&10LEN'...", "\"𓀇𓅃\""); /* fits */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆: '\\\"𓀇𓅃'...") == 0);
	    assert(rval == 29);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "𓀄𓀅𓀆: '%STR&ESCQ&10LEN'...", "\"𓀇.𓅃\""); /* 1 byte off */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆: '\\\"𓀇.'...") == 0);
	    assert(rval == 26);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "𓀄𓀅𓀆: '%STR&ESCQ&10LEN'...", "\"𓀇..𓅃\""); /* 2 bytes off */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆: '\\\"𓀇..'...") == 0);
	    assert(rval == 27);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(session, buf+4, 36, "𓀄𓀅𓀆: '%STR&ESCQ&10LEN'...", "\"𓀇...𓅃\""); /* 3 bytes off */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆: '\\\"𓀇...'...") == 0);
	    assert(rval == 28);
	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

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

