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

	*tname = "qprintf-42 %STR&HEX at end, overflow(1) in insert";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    buf[32] = '\n';
	    buf[31] = '\0';
	    buf[30] = 0xff;
	    buf[29] = '\0';
	    buf[28] = '\0';
	    buf[27] = '\0';
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    qpfPrintf(NULL, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    rval = qpfPrintf(NULL, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    assert(!strcmp(buf+4, "Encode: 3c6220633d227722"));
	    assert(rval == 26);
	    assert(buf[32] == '\n');
	    assert(buf[31] == '\0');
	    assert(buf[30] == 0xff);
	    assert(buf[29] == '\0');
	    assert(buf[28] == '\0');
	    assert(buf[27] != '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(chrNoOverlong(buf+4) == 0);

	    /** UTF-8 **/
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';

	    /** should chop off last full char despite being able to fit the first byte **/
	    /** 2 byte chars **/
	    rval = qpfPrintf(session, buf+4, 36, "код: %STR&HEX", "Test тест"); /* fits */
	    assert(strcmp(buf+4, "код: 5465737420d182d0b5d181d182") == 0);
	    assert(rval == 34);
	    assert(session->Errors == 0);
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "код: %STR&HEX", "Test: тест"); /* 1 byte over */
	    assert(strcmp(buf+4, "код: 546573743a20d182d0b5d181") == 0);
	    assert(rval == 36);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
            assert(chrNoOverlong(buf+4) == 0);
	    
	    /** 3 byte chars **/
	    rval = qpfPrintf(session, buf+4, 36, "编码: %STR&HEX", "Testing测试"); /* fits */
	    assert(strcmp(buf+4, "编码: 54657374696e67e6b58be8af95") == 0);
	    assert(rval == 34);
	    assert(session->Errors == 0);
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "编码: %STR&HEX", "Testing:测试"); /* 1 byte over */
	    assert(strcmp(buf+4, "编码: 54657374696e673ae6b58b") == 0);
	    assert(rval == 36);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "编码: %STR&HEX", "Testing: 测试"); /* 2 bytes over */
	    assert(strcmp(buf+4, "编码: 54657374696e673a20e6b58b") == 0);
	    assert(rval == 38);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
            assert(chrNoOverlong(buf+4) == 0);

	    /** 4 byte chars **/
	    rval = qpfPrintf(session, buf+4, 36, "𓅅𓂀 %STR&HEX", "Test 𓁳𓀒"); /* fits */
	    assert(strcmp(buf+4, "𓅅𓂀 5465737420f09381b3f0938092") == 0);
	    assert(rval == 35);
	    assert(session->Errors == 0);
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "𓅅𓂀: %STR&HEX", "Test 𓁳𓀒"); /* 1 byte over */
	    assert(strcmp(buf+4, "𓅅𓂀: 5465737420f09381b3") == 0);
	    assert(rval == 36);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "𓅅𓂀 %STR&HEX", "Test: 𓁳𓀒"); /* 2 bytes over */
	    assert(strcmp(buf+4, "𓅅𓂀 546573743a20f09381b3") == 0);
	    assert(rval == 37);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
            assert(chrNoOverlong(buf+4) == 0);
	    rval = qpfPrintf(session, buf+4, 36, "𓅅𓂀: %STR&HEX", "Test: 𓁳𓀒"); /* 3 bytes over */
	    assert(strcmp(buf+4, "𓅅𓂀: 546573743a20f09381b3") == 0);
	    assert(rval == 38);
	    assert(session->Errors == QPF_ERR_T_BUFOVERFLOW);
	    session->Errors = 0;
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

	nmSysFree(session);
    return iter*4;
    }

