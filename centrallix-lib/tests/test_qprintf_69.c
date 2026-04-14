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
    unsigned char buf[16];
    
	/*** This test verifies that, when printing quoted text to a buffer that
	 *** is too small using %STR&QUOT, the string quoting is handled properly
	 *** and the string is still null-terminated, even if the buffer is not
	 *** large enough for any of the text.
	 ***/
	
	*tname = "qprintf-69 %STR&QUOT small buffer quote & null termination";
	iter = 100000;
	for(i=0;i<iter;i++)
	    {
	    /** Initialize buffer around buf+4. **/
	    buf[9]  = '\n';
	    buf[8]  = '\0';
	    buf[7]  = 0xff;
	    buf[6]  = 0xff;
	    buf[3]  = '\n';
	    buf[2]  = '\0';
	    buf[1]  = 0xff;
	    buf[0]  = '\0';
	    
	    /** Test qpfPrintf(). **/
	    rval = qpfPrintf(NULL, (char*)buf+4, 2, "%STR&QUOT", "ab");
	    
	    /** The 2-byte buffer have the first quote and the null-terminator. **/
	    assert(buf[4] == '\'');
	    assert(buf[5] == '\0');
	    
	    /** Bytes outside the provided part of the buffer must not be clobbered. **/
	    assert(buf[9] == '\n');
	    assert(buf[8] == '\0');
	    assert(buf[7] == 0xff);
	    assert(buf[6] == 0xff);
	    assert(buf[3] == '\n');
	    assert(buf[2] == '\0');
	    assert(buf[1] == 0xff);
	    assert(buf[0] == '\0');
	    
	    /** Return value counts full output: 'ab' = 4 bytes (null-terminator not counted). **/
	    assert(rval == 4);
	    }

    return iter;
    }
