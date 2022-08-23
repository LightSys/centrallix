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

    *tname = "qprintf-70 Test Len with B64 encode and decode.";
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

	
	/** B64 trucnate after encode **/
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&B64&7LEN||", "hello there"); /* does not truncate at all */
//printf("%s, %d\n", buf+4, rval);
	assert(0 == strcmp(buf+4, "||aGVsbG8gdGhlcmU=||")); 
	assert(rval == 20);

	/** B64 trucnate before decode **/
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&7LEN&DB64||", "aGVsbG8gdGhlcmU="); /* invalid len */
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "aGVsbG8gdGhlcmU="); /* proper truncate */
	assert(0 == strcmp(buf+4, "||hello ||")); 
	assert(rval == 10);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&100LEN&DB64||", "aGVsbG8gdGhlcmU="); /* too long */
	assert(0 == strcmp(buf+4, "||hello there||")); 
	assert(rval == 15);
	/** cut off utf-8 **/
	/* 2 bytes */
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "0L/RgA=="); /** fits **/
	assert(0 == strcmp(buf+4, "||пр||")); 
	assert(rval == 8);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&4LEN&DB64||", "0L/RgA=="); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||п||")); 
	assert(rval == 6);
	/* 3 bytes */
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "5L2g5aW9"); /** fits **/
	assert(0 == strcmp(buf+4, "||你好||")); 
	assert(rval == 10);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "5L2gLuWlvQ=="); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||你.||")); 
	assert(rval == 8);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "5L2gLi7lpb0="); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||你..||")); 
	assert(rval == 9);
	/* 4 bytes */ //
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&12LEN&DB64||", "8JOCpS7wk4WY"); /** fits **/
	assert(0 == strcmp(buf+4, "||𓂥.𓅘||")); 
	assert(rval == 13);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&12LEN&DB64||", "8JOCpS4u8JOFmA=="); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||𓂥..||")); 
	assert(rval == 10);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&12LEN&DB64||", "8JOCpS4uLvCThZg="); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||𓂥...||")); 
	assert(rval == 11);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "8JOCpfCThZg="); /** 3 bytes cut off **/
	assert(0 == strcmp(buf+4, "||𓂥||")); 
	assert(rval == 8);
	/* alt method (2  bytes) */
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%8STR&DB64||", "0L/RgA=="); /** fits **/
	assert(0 == strcmp(buf+4, "||пр||")); 
	assert(rval == 8);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%4STR&DB64||", "0L/RgA=="); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||п||")); 
	assert(rval == 6);

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

