#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "charsets.h"

long long
test (char** tname)
	{
	char* rStr1;
	char* rStr2;
	char* rStr3;
	
	*tname = "No Overlong";

	setlocale(0, "en_US.UTF-8");

	/** make sure works with all ascii characters **/
	char strBuf[257]; /* 256+1 */
	char * result;
	int i;
	for(i = 0 ; i < 128 ; i++)
		{
		/* test one char and build up string */
		strBuf[127 - i] = (char) i; /* need null char last */
		char cur[2];
		cur[0] = (char) i;
		cur[1] = '\0';

		result = chrNoOverlong(cur);
		assert(strcmp(cur, result) == 0);
		nmSysFree(result);
		}
	result = chrNoOverlong(strBuf);
	assert(strcmp(strBuf, result) == 0);
	nmSysFree(result);

	/** test all 2 character utf8 chars  **/
	for(i = 0xC0 ; i < 0xDF ; i++) /* of the form  110xxxxx */
		{
		int j;
		/* there are 64 values, each with two bytes = 128 bytes, Plus a null. Will fit in str buf */
		printf("\xC0\x80\n");
		for(j = 0x80 ; j < 0xBF ; j++) /* of the form 10xxxxxx */
			{
			strBuf[(j-0x80)*2]   = i;
			strBuf[(j-0x80)*2+1] = j;
			strBuf[(j-0x80)*2+2] = '\0'; /* endable single char test */

			char *cur = strBuf+((j-0x80)*2);
			result = chrNoOverlong(cur);
			printf("|%s| ", cur);
			assert(result != NULL);
			assert(strcmp(cur, result) == 0);
			nmSysFree(result);
			}
			
			printf("\n%s\n", strBuf);
			result = chrNoOverlong(strBuf);
			assert(strcmp(strBuf, result) == 0);
			nmSysFree(result);
		}

	/** test 3 byte chars **/
	/*E0-EF, 80-BF, 80-BF*/
	for(i = 0xE0 ; i < 0xEF ; i++) /* of the form  1110xxxx */
		{
		int j;
		
		for(j = 0x80 ; j < 0xBF ; j++) /* of the form 10xxxxxx */
			{
			int k;

			/* there are 64 values, each with 3 bytes = 192 bytes, Plus a null. Will fit in str buf */
			/* NOTE: there are too many 3 byte chars to test them all at once, so do in sets of 63 */
			for(k = 0x80 ; k < 0xBF ; k++) /* of the form 10xxxxxx */
				{
				strBuf[(k-0x80)*3]   = i;
				strBuf[(k-0x80)*3+1] = j;
				strBuf[(k-0x80)*3+2] = k;
				strBuf[(k-0x80)*3+3] = '\0'; /* endable single char test */

				char *cur = strBuf+((j-0x80)*3);
				result = chrNoOverlong(cur);
				printf("|%s| ", cur);
				assert(result != NULL);
				assert(strcmp(cur, result) == 0);
				nmSysFree(result);
				}
				
			printf("\n%s\n", strBuf);
			result = chrNoOverlong(strBuf);
			assert(strcmp(strBuf, result) == 0);
			nmSysFree(result);
			}
		}

	/** test 4 byte chars **/
	/*F0-F7, 80-BF, 80-BF, 80-BF*/
	for(i = 0xE0 ; i < 0xEF ; i++) /* of the form  11110xxx */
		{
		int j;
		
		for(j = 0x80 ; j < 0xBF ; j++) /* of the form 10xxxxxx */
			{
			int k;

			/* there are 64 values, each with 4 bytes = 256 bytes, Plus a null. Will fit in str buf */
			for(k = 0x80 ; k < 0xBF ; k++) /* of the form 10xxxxxx */
				{
				int l;

				for(l = 0x80 ; l < 0xBF ; l++)
					{
					strBuf[(k-0x80)*4]   = i;
					strBuf[(k-0x80)*4+1] = j;
					strBuf[(k-0x80)*4+2] = k;
					strBuf[(k-0x80)*4+3] = l;
					strBuf[(k-0x80)*4+4] = '\0'; /* endable single char test */

					char *cur = strBuf+((j-0x80)*3);
					result = chrNoOverlong(cur);
					printf("|%s| ", cur);
					assert(result != NULL);
					assert(strcmp(cur, result) == 0);
					nmSysFree(result);
					}
				
				printf("\n%s\n", strBuf);
				result = chrNoOverlong(strBuf);
				assert(strcmp(strBuf, result) == 0);
				nmSysFree(result);
				}
			}
		}

    char* euroReg = "\xE2\x82\xAC";
    char* euroLong = "\xF0\x82\x82\xAC";

	
	rStr3 = chrNoOverlong(euroReg); 
	printf("str3: %s.\n", rStr3);
	
	printf("reg: %s, over: %s.\n", euroReg, euroLong);
	fflush(stdout);


	return 0;
	}
