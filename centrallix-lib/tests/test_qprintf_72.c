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

    *tname = "qprintf-72 test b64 and hex decode with invalid UTF-8 bytes";
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

        /** no session **/
        rval = qpfPrintf(NULL, (char*)(buf+4), 36, "their, %STR&DHEX, and they're", "7468657265FF"); 
        assert(rval == 26);
        assert(0 == strcmp(buf+4, "their, there\xFF, and they're"));

        rval = qpfPrintf(NULL, (char*)(buf+4), 36, "their, %STR&DB64, and they're", "dGhlcmX/"); 
        assert(rval == 26);
        assert(0 == strcmp(buf+4, "their, there\xFF, and they're"));

        /** with session **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "their, %STR&DHEX, and they're", "7468657265FF"); 
        assert(rval < 0);
        assert(strcmp(buf+4, "their, ") == 0); /* confirm failed in decode **/
        assert(session->Errors == QPF_ERR_T_BADCHAR);
        session->Errors = 0;

        rval = qpfPrintf(session, (char*)(buf+4), 36, "their, %STR&DB64, and they're", "dGhlcmX/"); 
        assert(rval < 0);
        assert(strcmp(buf+4, "their, ") == 0); /* confirm failed in decode **/
        assert(session->Errors == QPF_ERR_T_BADCHAR);
        session->Errors = 0;

        assert(buf[43] == '\n');
        assert(buf[42] == '\0');
        assert(buf[41] == 0xff);
        assert(buf[40] == '\0');
        assert(buf[3] == '\n');
        assert(buf[2] == '\0');
        assert(buf[1] == 0xff);
        assert(buf[0] == '\0');

        /** test again with more utf8 **/
        /** no session **/
        /* ជខ្សែអក្សរ இது ΣEIPA */
        rval = qpfPrintf(NULL, (char*)(buf+4), 36, "អក្សរ %STR&DHEX ΣEIPA", "e0ae87e0aea4e0af81ff"); 
        assert(rval == 33);
        assert(0 == strcmp(buf+4, "អក្សរ இது\xFF ΣEIPA"));

        rval = qpfPrintf(NULL, (char*)(buf+4), 36, "អក្សរ %STR&DB64 ΣEIPA", "4K6H4K6k4K+B/w=="); 
        assert(rval == 33);
        assert(0 == strcmp(buf+4, "អក្សរ இது\xFF ΣEIPA"));

        /** with session **/
        rval = qpfPrintf(session, (char*)(buf+4), 36, "អក្សរ %STR&DHEX ΣEIPA", "e0ae87e0aea4e0af81ff"); 
        assert(rval < 0);
        assert(strcmp(buf+4, "អក្សរ ") == 0); /* confirm failed in decode **/
        assert(session->Errors == QPF_ERR_T_BADCHAR);
        session->Errors = 0;

        rval = qpfPrintf(session, (char*)(buf+4), 36, "អក្សរ %STR&DB64 ΣEIPA", "4K6H4K6k4K+B/w=="); 
        assert(rval < 0);
        assert(strcmp(buf+4, "អក្សរ ") == 0); /* confirm failed in decode **/
        assert(session->Errors == QPF_ERR_T_BADCHAR);
        session->Errors = 0;

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
    return iter*2;
    }

