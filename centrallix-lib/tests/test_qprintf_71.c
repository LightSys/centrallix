#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
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

    *tname = "qprintf-71 %STR&HEXD overflow test decode hex";
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

        /** short test **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "their, %STR&DHEX, and they're", 
            "7468657265"); 
        assert(rval == 25);
        assert(0 == strcmp(buf+4, "their, there, and they're"));
        /** max len that fits in 36 **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "|%STR&DHEX|",  
            "41206C6F6E672074696D652061676F20696E20612067616C617879206661722066"); 
        assert(rval == 35);
        assert(0 == strcmp(buf+4, "|A long time ago in a galaxy far f|"));
        /** too long **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&DHEX", 
            "41206C6F6E672074696D652061676F20696E20612067616C61787920666172206661"); 
        assert(rval < 0);
        /** make sure NULL's are caught **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&DHEX", 
            "41206C6F6E6\000A72074696D652061676F20696E20612067616C617879"); 
        assert(rval < 0);
        /** odd number of characters **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&DHEX", 
            "41206C6F6E672074696D652061676F20696E20612067616C617879A"); 
        assert(rval < 0);
        /** non-hex char  **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&DHEX", 
            "41206C6F6E672074696D652061H76F20696E20612067616C617879A"); 
        assert(rval < 0);

        assert(buf[43] == '\n');
        assert(buf[42] == '\0');
        assert(buf[41] == 0xff);
        assert(buf[40] == '\0');
        assert(buf[3] == '\n');
        assert(buf[2] == '\0');
        assert(buf[1] == 0xff);
        assert(buf[0] == '\0');

        /* basic UTF-8 */
	session->Flags = QPF_F_ENFORCE_UTF8;
	session->Errors = 0;
        rval = qpfPrintf(session, (char*)(buf+4), 36, "%STR&DHEX", "e0ae9ae0af8be0aea4e0aea9e0af88");
        assert(rval == 15);
        assert(strcmp(buf+4, "சோதனை") == 0);
        assert(session->Errors == 0);
        assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
        /** more complex **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "இது %STR&DHEX உதாரணம்", 
            "e0ae92e0aeb0e0af8120e0aea8e0af80e0aea3e0af8de0ae9f" );
        assert(rval < 67);
        assert(strcmp(buf+4, "இது ஒரு நீண்ட") == 0);
        assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
        assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

        assert(buf[43] == '\n');
        assert(buf[42] == '\0');
        assert(buf[41] == 0xff);
        assert(buf[40] == '\0');
        assert(buf[3] == '\n');
        assert(buf[2] == '\0');
        assert(buf[1] == 0xff);
        assert(buf[0] == '\0');

        session->Errors = 0;
	session->Flags = 0;
        }

    nmSysFree(session);
    return iter*2;
    }

