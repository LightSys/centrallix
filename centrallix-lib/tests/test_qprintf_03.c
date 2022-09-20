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
    int i;
    int iter;
    unsigned char buf[44];
    pQPSession session;
    session = nmSysMalloc(sizeof(QPSession));
    session->Flags = QPF_F_ENFORCE_UTF8;    
	*tname = "qprintf-03 constant string, 1char overflow, using qpfPrintf()";
	setlocale(0, "en_US.UTF-8");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';
	    qpfPrintf(NULL, buf+4, 36, "this is a string non-overflow test.?");
	    qpfPrintf(NULL, buf+4, 36, "this is a string non-overflow test.?");
	    qpfPrintf(NULL, buf+4, 36, "this is a string non-overflow test.?");
	    qpfPrintf(NULL, buf+4, 36, "this is a string non-overflow test.?");
	    assert(!strcmp(buf+4,"this is a string non-overflow test."));
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    /* utf8 */
	    buf[43] = '\n';
	    buf[42] = '\0';
	    buf[41] = 0xff;
	    buf[40] = '\0';
	    buf[39] = 0xff;	/* should get overwritten by qprintf */
	    buf[3] = '\n';
	    buf[2] = '\0';
	    buf[1] = 0xff;
	    buf[0] = '\0';

	    /** 2 byte cases **/
	    qpfPrintf(session, buf+4, 36, "В Начале Сотворил Бог Небо И Землю."); /* ends at 'Б' char */
	    assert(!strcmp(buf+4, "В Начале Сотворил Б"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[39] == '\0');

	    qpfPrintf(session, buf+4, 36, "В Начале Сотворил *Бог Небо И Землю."); /* cuts off 'Б' char */
	    assert(!strcmp(buf+4, "В Начале Сотворил *"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[38] == '\0');

	    /** check 3 byte cases **/
	    qpfPrintf(session, buf+4, 36, "**に神は、ひとり子をさえ惜しまず与えるほど"); /* ends on 'え' */
	    assert(!strcmp(buf+4, "**に神は、ひとり子をさえ"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[39] == '\0');

	    qpfPrintf(session, buf+4, 36, "**に神は、ひとり子をさ*え惜しまず与えるほど"); /* space cuts off last byte of the 'え' */
	    assert(!strcmp(buf+4, "**に神は、ひとり子をさ*"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[37] == '\0');
	    assert((unsigned char) buf[38] == (unsigned char) '\x81'); /* 2nd byte of the 'え' */

	    qpfPrintf(session, buf+4, 36, "**に神は、ひとり子をさ**え惜しまず与えるほど"); /* space cuts off last 2 bytes of the 'え' */
	    assert(!strcmp(buf+4, "**に神は、ひとり子をさ**"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[38] == '\0');

	    /** check 4 byte cases **/
	    qpfPrintf(session, buf+4, 36, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓅃𓀈𓀉"); /* ends on the '𓅃' */
	    assert(!strcmp(buf+4, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓅃"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[39] == '\0');

	    qpfPrintf(session, buf+4, 36, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇*𓅃𓀈𓀉"); /* cuts off last byte of '𓅃' */
	    assert(!strcmp(buf+4, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇*"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[36] == '\0');
	    assert((unsigned char) buf[37] == (unsigned char) '\x93'); /* 2nd byte */
	    assert((unsigned char) buf[38] == (unsigned char) '\x85'); /* 3rd byte */

	    qpfPrintf(session, buf+4, 36, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇**𓅃𓀈𓀉"); /* cuts off last 2 bytes of '𓅃' */
	    assert(!strcmp(buf+4, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇**"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[37] == '\0');
	    assert((unsigned char) buf[38] == (unsigned char) '\x93'); /* 2nd byte */    
	    qpfPrintf(session, buf+4, 36, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇***𓅃𓀈𓀉"); /* cuts off last 3 bytes of '𓅃' */
	    assert(!strcmp(buf+4, "***𓀁𓀂𓀃𓀄𓀅𓀆𓀇***"));
	    assert(verifyUTF8(buf+4) == 0);
	    assert(buf[38] == '\0');

	    /** make sure stayed in bounds **/
	    assert(buf[38] == '\0'); 
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[39] != 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

	nmSysFree(session);
    return iter*4;
    }

