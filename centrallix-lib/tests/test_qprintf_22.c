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
    setlocale(0, "en_US.UTF-8");

	*tname = "qprintf-22 %STR&NLEN in middle with insert overflow";
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
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&6LEN...", "STRINGSTR");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&6LEN...", "STRINGSTR");
	    qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&6LEN...", "STRINGSTR");
	    rval = qpfPrintf(NULL, buf+4, 36, "Here is the str: %STR&6LEN...", "STRINGSTR");
	    assert(!strcmp(buf+4, "Here is the str: STRING..."));
	    assert(rval == 26);
	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');

	    assert(chrNoOverlong(buf+4) == 0);

	    /*** UTF-8 ***/

		/** 2 byte combos **/
	    rval = qpfPrintf(NULL, buf+4, 36, "εδώ οδός: %STR&6LEN...", "рядок"); /* full  */
	    assert(strcmp(buf+4, "εδώ οδός: ряд...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 26);

		rval = qpfPrintf(NULL, buf+4, 36, "εδώ οδός: %STR&5LEN...", "рядок"); /* cut off last byte */
	    assert(strcmp(buf+4, "εδώ οδός: ря...") == 0); 
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 25);

		/** 3 byte combos **/
		rval = qpfPrintf(NULL, buf+4, 36, "に神は、 %STR&6LEN...", "ひとり子をさえ惜し"); /* full  */
	    assert(strcmp(buf+4, "に神は、 ひと...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 22);

		rval = qpfPrintf(NULL, buf+4, 36, "に神は、 %STR&5LEN...", "ひとり子をさえ惜し"); /* cut off last byte  */
	    assert(strcmp(buf+4, "に神は、 ひ...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 21);

		rval = qpfPrintf(NULL, buf+4, 36, "に神は、 %STR&4LEN...", "ひとり子をさえ惜し"); /* cut off 2nd byte  */
	    assert(strcmp(buf+4, "に神は、 ひ...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 20);

		/** 4 byte combos **/
		rval = qpfPrintf(NULL, buf+4, 36, "𓀄𓀅𓀆 %STR&8LEN...", "𓀇𓅃𓀈𓀉"); /* full */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆 𓀇𓅃...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 24);

		rval = qpfPrintf(NULL, buf+4, 36, "𓀄𓀅𓀆 %STR&7LEN...", "𓀇𓅃𓀈𓀉"); /* cut off 4th byte */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆 𓀇...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 23);

		rval = qpfPrintf(NULL, buf+4, 36, "𓀄𓀅𓀆 %STR&6LEN...", "𓀇𓅃𓀈𓀉"); /* cut off 3rd byte */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆 𓀇...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 22);

		rval = qpfPrintf(NULL, buf+4, 36, "𓀄𓀅𓀆 %STR&5LEN...", "𓀇𓅃𓀈𓀉"); /* cut off 2nd byte */
	    assert(strcmp(buf+4, "𓀄𓀅𓀆 𓀇...") == 0);
	    assert(chrNoOverlong(buf+4) == 0);
		assert(rval == 21);

	    assert(buf[43] == '\n');
	    assert(buf[42] == '\0');
	    assert(buf[41] == 0xff);
	    assert(buf[40] == '\0');
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    }

    return iter*4;
    }

