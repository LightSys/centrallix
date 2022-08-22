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

    *tname = "qprintf-69 Test Len with B64 and HEX encode and decode.";
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
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&7LEN||", "hello there"); /* invalid length */
	assert(0 == strcmp(buf+4, "||68656c||")); /* should round down to length 6 */
	assert(rval == 10);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "hello there"); /* proper call */
	assert(0 == strcmp(buf+4, "||68656c6c||")); 
	assert(rval == 12);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&100LEN||", "hello there"); /* too long */
	assert(0 == strcmp(buf+4, "||68656c6c6f207468657265||")); 
	assert(rval == 26);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "你好呀"); /* allows invalid if no utf9 flag */
	assert(0 == strcmp(buf+4, "||e4bda0e5||")); 
	assert(rval == 12);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "||%STR&HEX&8LEN||", "你好呀"); /* chop utf-8 */
	assert(0 == strcmp(buf+4, "||e4bda0||")); 
	assert(rval == 10);
	
	/** B64 trucnate after encode **/
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&B64&7LEN||", "hello there"); 
	assert(0 == strcmp(buf+4, "||aGVsbG8gdGhlcmU=||")); /* does not truncate at all */

	/** HEX trucnate before decode **/
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&5LEN&DHEX||", "68656C6C6F207468657265"); /* invalid len */
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&10LEN&DHEX||", "68656C6C6F207468657265"); /* proper truncate */
	assert(0 == strcmp(buf+4, "||hello||")); 
	assert(rval == 9);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&100LEN&DHEX||", "68656C6C6F207468657265"); /* too long */
	assert(0 == strcmp(buf+4, "||hello there||")); 
	assert(rval == 15);

	/** B64 trucnate before decode **/
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&7LEN&DB64||", "aGVsbG8gdGhlcmU="); /* invalid len */
	assert(rval < 0);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "aGVsbG8gdGhlcmU="); /* proper truncate */
	assert(0 == strcmp(buf+4, "||hello ||")); 
	assert(rval == 10);
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&100LEN&DB64||", "aGVsbG8gdGhlcmU="); /* too long */
	assert(0 == strcmp(buf+4, "||hello there||")); 
	assert(rval == 15);

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

