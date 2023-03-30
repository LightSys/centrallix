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
	
    *tname = "qprintf-69 Test Len with HEX encode and decode.";
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

	/** HEX trucnate after encode **/
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&HEX&7LEN||", "hello there"); /* invalid length */
	assert(0 == strcmp(buf+4, "||68656c||")); /* should round down to length 6 */
	assert(rval == 10);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "hello there"); /* proper call */
	assert(0 == strcmp(buf+4, "||68656c6c||")); 
	assert(rval == 12);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&HEX&100LEN||", "hello there"); /* too long */
	assert(0 == strcmp(buf+4, "||68656c6c6f207468657265||")); 
	assert(rval == 26);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "你好呀"); /* allows invalid if no utf9 flag */
	assert(0 == strcmp(buf+4, "||e4bda0e5||")); 
	assert(rval == 12);
	/** UTF-8 **/
	/* 2 bytes */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "пр"); /** fits **/
	assert(0 == strcmp(buf+4, "||d0bfd180||")); 
	assert(rval == 12);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&6LEN||", "пр"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||d0bf||")); 
	assert(rval == 8);
	/* 3 bytes */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&12LEN||", "你好"); /** fits **/
	assert(0 == strcmp(buf+4, "||e4bda0e5a5bd||")); 
	assert(rval == 16);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&10LEN||", "你好"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||e4bda0||")); 
	assert(rval == 10);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "你好"); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||e4bda0||")); 
	assert(rval == 10);
	/* 4 bytes */ //
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&16LEN||", "𓂥𓅘"); /** fits **/
	assert(0 == strcmp(buf+4, "||f09382a5f0938598||")); 
	assert(rval == 20);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&14LEN||", "𓂥𓅘"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||f09382a5||")); 
	assert(rval == 12);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&12LEN||", "𓂥𓅘"); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||f09382a5||")); 
	assert(rval == 12);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&10LEN||", "𓂥𓅘"); /** 3 bytes cut off **/
	assert(0 == strcmp(buf+4, "||f09382a5||")); 
	assert(rval == 12);

	/** HEX trucnate before decode **/
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&5LEN&DHEX||", "68656C6C6F207468657265"); /* invalid len */
	assert(rval < 0);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&10LEN&DHEX||", "68656C6C6F207468657265"); /* proper truncate */
	assert(0 == strcmp(buf+4, "||hello||")); 
	assert(rval == 9);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&100LEN&DHEX||", "68656C6C6F207468657265"); /* too long */
	assert(0 == strcmp(buf+4, "||hello there||")); 
	assert(rval == 15);
	/** cut off utf-8 **/
	/* 2 bytes */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&8LEN&DHEX||", "d0bfd180"); /** fits **/
	assert(0 == strcmp(buf+4, "||пр||")); 
	assert(rval == 8);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&6LEN&DHEX||", "d0bfd180"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||п||")); 
	assert(rval == 6);
	/* 3 bytes */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&12LEN&DHEX||", "e4bda0e5a5bde59180"); /** fits **/
	assert(0 == strcmp(buf+4, "||你好||")); 
	assert(rval == 10);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&10LEN&DHEX||", "e4bda0e5a5bde59180"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||你||")); 
	assert(rval == 7);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&8LEN&DHEX||", "e4bda0e5a5bde59180"); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||你||")); 
	assert(rval == 7);
	/* 4 bytes */ //
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&16LEN&DHEX||", "f09382a5f0938598"); /** fits **/
	assert(0 == strcmp(buf+4, "||𓂥𓅘||")); 
	assert(rval == 12);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&14LEN&DHEX||", "f09382a5f0938598"); /** 1 byte cut off **/
	assert(0 == strcmp(buf+4, "||𓂥||")); 
	assert(rval == 8);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&12LEN&DHEX||", "f09382a5f0938598"); /** 2 bytes cut off **/
	assert(0 == strcmp(buf+4, "||𓂥||")); 
	assert(rval == 8);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&10LEN&DHEX||", "f09382a5f0938598"); /** 3 bytes cut off **/
	assert(0 == strcmp(buf+4, "||𓂥||")); 
	assert(rval == 8);

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

