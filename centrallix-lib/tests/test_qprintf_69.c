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

    *tname = "qprintf-69 %STR&B64 overflow test with len";
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
	/* TODO: modify these to use &LEN */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "||%STR&8LEN&DB64||", "dGhlIHF1aWNrIGJyb3duIGZveCBqdW1w"); /* max len that fits in 36 */
printf("%s, %d\n", buf+4, rval);
	assert(0 == strcmp(buf+4, "||dGhlIHF1aWNrIGJyb3duIGZveCBqdW1w||"));

	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "%STR&B64", "the quick brown fox jumps over the fence");
	assert(rval < 0);
	/* has enough room for some chars, but not the next 4 */
	rval = qpfPrintf(NULL, (char*)(buf+4), 36, "%STR&B64", "the quick brown fox jumps");
	assert(rval < 0);
	assert(buf[43] == '\n');
	assert(buf[42] == '\0');
	assert(buf[41] == 0xff);
	assert(buf[40] == '\0');
	assert(buf[3] == '\n');
	assert(buf[2] == '\0');
	assert(buf[1] == 0xff);
	assert(buf[0] == '\0');

        assert(chrNoOverlong(buf+4) == 0);
	rval = qpfPrintf(session, (char*)(buf+4), 36, "%STR&B64", "சோதனை");
	assert(rval == strlen("4K6a4K+L4K6k4K6p4K+I"));
	rval = qpfPrintf(session, (char*)(buf+4), 36, "%STR&B64", "இது ஒரு நீண்ட உதாரணம்");
	assert(rval < 0);
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
    return iter*2;
    }

