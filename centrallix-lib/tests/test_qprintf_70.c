#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "qprintf.h"

long long
test(char** tname)
    {
    int i, rval;
    int iter;
    unsigned char buf[16];
    
	/*** This test verifies that a bug was successfully fixed:  When
	 *** printing quoted text to a buffer that was only one character
	 *** long using %STR&QUOT, the function would underflow the buffer,
	 *** clobbering the byte before the buffer with 0x27, aka. the '
	 *** character, intended to be the closing quote.
	 ***/
	
	*tname = "qprintf-70 %STR&QUOT buffer size underflow";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    /** Initialize buffer. **/
	    buf[9]  = '\n';
	    buf[8]  = '\0';
	    buf[7]  = 0xff;
	    buf[6]  = 0xff;
	    buf[5]  = 0xff;
	    buf[4]  = 0xff;
	    buf[3]  = '\n';
	    buf[2]  = '\0';
	    buf[1]  = 0xff;
	    buf[0]  = '\0';
	    
	    /** Test qpfPrintf(). **/
	    rval = qpfPrintf(NULL, (char*)buf+4, 1, "%STR&QUOT", "ab");
	    
	    /** The 1-byte buffer have only the null-terminator. **/
	    assert(buf[4] == '\0');
	    
	    /** Bytes outside the provided part of the buffer must not be clobbered. **/
	    assert(buf[9] == '\n');
	    assert(buf[8] == '\0');
	    assert(buf[7] == 0xff);
	    assert(buf[6] == 0xff);
	    assert(buf[5] == 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    
	    /** Return value counts full output: 'ab' = 4 bytes (null-terminator not counted). **/
	    assert(rval == 4);
	    }

    return iter;
    }
