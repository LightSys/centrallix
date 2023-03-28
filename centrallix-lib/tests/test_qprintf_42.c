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
    pQPSession asciiSes, utf8Ses;
    utf8Ses = nmSysMalloc(sizeof(QPSession));
    utf8Ses->Flags = QPF_F_ENFORCE_UTF8;
    asciiSes = nmSysMalloc(sizeof(QPSession));
    asciiSes->Flags = 0;

	setlocale(0, "en_US.UTF-8");
	qpfInitialize(); 
	
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
	    qpfPrintf(asciiSes, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    qpfPrintf(asciiSes, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    qpfPrintf(asciiSes, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
	    rval = qpfPrintf(asciiSes, buf+4, 26, "Encode: %STR&HEX", "<b c=\"w\">");
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

	    assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /** UTF-8 **/
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';

	    /** should chop off last full char despite being able to fit the first byte **/
	    /** 2 byte chars **/
	    rval = qpfPrintf(NULL, buf+4, 36, "код: %STR&HEX", "Test тест"); /* fits */
	    assert(strcmp(buf+4, "код: 5465737420d182d0b5d181d182") == 0);
	    assert(rval == 34);
	    assert(utf8Ses->Errors == 0);
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "код: %STR&HEX", "Test: тест"); /* 1 byte over */
	    assert(strcmp(buf+4, "код: 546573743a20d182d0b5d181") == 0);
	    assert(rval == 36);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    
	    /** 3 byte chars **/
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "编码: %STR&HEX", "Testing测试"); /* fits */
	    assert(strcmp(buf+4, "编码: 54657374696e67e6b58be8af95") == 0);
	    assert(rval == 34);
	    assert(utf8Ses->Errors == 0);
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "编码: %STR&HEX", "Testing:测试"); /* 1 byte over */
	    assert(strcmp(buf+4, "编码: 54657374696e673ae6b58b") == 0);
	    assert(rval == 36);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "编码: %STR&HEX", "Testing: 测试"); /* 2 bytes over */
	    assert(strcmp(buf+4, "编码: 54657374696e673a20e6b58b") == 0);
	    assert(rval == 38);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /** 4 byte chars **/
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "𓅅𓂀 %STR&HEX", "Test 𓁳𓀒"); /* fits */
	    assert(strcmp(buf+4, "𓅅𓂀 5465737420f09381b3f0938092") == 0);
	    assert(rval == 35);
	    assert(utf8Ses->Errors == 0);
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "𓅅𓂀: %STR&HEX", "Test 𓁳𓀒"); /* 1 byte over */
	    assert(strcmp(buf+4, "𓅅𓂀: 5465737420f09381b3") == 0);
	    assert(rval == 36);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "𓅅𓂀 %STR&HEX", "Test: 𓁳𓀒"); /* 2 bytes over */
	    assert(strcmp(buf+4, "𓅅𓂀 546573743a20f09381b3") == 0);
	    assert(rval == 37);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);
	    rval = qpfPrintf(utf8Ses, buf+4, 36, "𓅅𓂀: %STR&HEX", "Test: 𓁳𓀒"); /* 3 bytes over */
	    assert(strcmp(buf+4, "𓅅𓂀: 546573743a20f09381b3") == 0);
	    assert(rval == 38);
	    assert(utf8Ses->Errors == QPF_ERR_T_BUFOVERFLOW);
	    utf8Ses->Errors = 0;
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR);

	    /** can split if no enforce **/
	    rval = qpfPrintf(asciiSes, buf+4, 36, "код: %STR&HEX", "Test: тест"); /* cuts off 1 byte */
	    assert(strcmp(buf+4, "код: 546573743a20d182d0b5d181d1") == 0);
	    assert(rval == 36);
	    assert(asciiSes->Errors == QPF_ERR_T_BUFOVERFLOW); /* detects overflow, but allows split */
            assert(verifyUTF8(buf+4) == UTIL_VALID_CHAR); /* invalid char is hex encoded, so passes */

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }
	nmSysFree(asciiSes);
	nmSysFree(utf8Ses);
    return iter*4;
    }

