#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <xstring.h>
#include <assert.h>
#include "qprintf.h"

long long
test (char** tname)
	{
	int i, iter;
	char* rStr1;
	char* rStr2;
	char* rStr3;
	
	*tname = "No Overlong";
	iter = 1000000;

	char* byte1 = "\x21";
        char byte2[5];
	byte2[0] = 0xCE;
	byte2[1] = 0x9B; //"\xC0\xA1";
	//byte2[2] = "\0";
        char* euroReg = "\xE2\x82\xAC";
        char* euroLong = "\xF0\x82\x82\xAC";

        printf("%s\n%s\n", byte1, byte2);
        rStr1 = chrNoOverlong(byte1);
	rStr2 = chrNoOverlong(byte2);
	printf("%s\n%s\n", rStr1, rStr2);
	
	rStr3 = chrNoOverlong(euroReg);
	printf("%s\n", rStr3);
	
	printf("%s\n%s\n", euroReg, euroReg);
	fflush(stdout);
	
	for (i = 0; i < iter; i++)
		{
		//printf("A\n");
		//fflush(stdout);
		assert (0 == 0);
		}
	





	return iter*4;
	}
