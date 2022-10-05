#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include "util.h"
#include "util.h"

long long
test (char** tname)
	{
	int i, j, k, l;
	
	*tname = "Overlong 00: Ensure valid characters pass";

	setlocale(LC_ALL, "en_US.utf8");

	/** make sure works with all ascii characters **/
	char strBuf[257]; /* room for 64 4-byte characters and a null byte */
	int result;

	for(i = 0 ; i < 128 ; i++)
		{
		/* test one char and build up string */
		strBuf[127 - i] = (char) i; /* need null char last */
		char cur[2];
		cur[0] = (char) i;
		cur[1] = '\0';

		result = verifyUTF8(cur);
		assert(result == UTIL_VALID_CHAR);
		}
	result = verifyUTF8(strBuf);
	assert(result == UTIL_VALID_CHAR);


	/** test all 2 character utf8 chars  **/
	for(i = 0xC2 ; i <= 0xDF ; i++) /* of the form  110xxxxx, but C0 and C1 are overlongs, so skip */
		{
		/* there are 64 values, each with two bytes = 128 bytes, Plus a null. Will fit in str buf */
		for(j = 0x80 ; j <= 0xBF ; j++) /* of the form 10xxxxxx */
			{
			strBuf[(j-0x80)*2]   = i;
			strBuf[(j-0x80)*2+1] = j;
			strBuf[(j-0x80)*2+2] = '\0'; /* endable single char test */

			char *cur = strBuf+((j-0x80)*2);
			result = verifyUTF8(cur);
			assert(result == UTIL_VALID_CHAR);
			}
			
		result = verifyUTF8(strBuf);
		assert(result == UTIL_VALID_CHAR);
		}


	/** test 3 byte chars **/
	/*E0-EF, 80-BF, 80-BF*/
	for(i = 0xE0 ; i <= 0xEF ; i++) /* of the form  1110xxxx */
		{
		
		for(j = 0x80 ; j <= 0xBF ; j++) /* of the form 10xxxxxx */
			{
			if(i == 0xE0 && j <= 0x9F) j = 0xA0; /* E0 8x - E0 9x is overlong */
			if(i == 0xED && j >= 0xA0) /* ED A0 80 - ED BF BF is invalid */
				{
				i = 0xEE;
				j = 0x80;
				}

			/* there are 64 values, each with 3 bytes = 192 bytes, Plus a null. Will fit in str buf */
			/* NOTE: there are too many 3 byte chars to test them all at once, so do in sets of 63 */
			for(k = 0x80 ; k <= 0xBF ; k++) /* of the form 10xxxxxx */
				{
				
				strBuf[(k-0x80)*3]   = i;
				strBuf[(k-0x80)*3+1] = j;
				strBuf[(k-0x80)*3+2] = k;
				strBuf[(k-0x80)*3+3] = '\0'; /* endable single char test */

				char *cur = strBuf+((k-0x80)*3);
				
				result = verifyUTF8(cur);
				assert(result == UTIL_VALID_CHAR);
				}
				
			result = verifyUTF8(strBuf);
			assert(result == UTIL_VALID_CHAR);
			}
		}


	/** test 4 byte chars **/
	/*F0-F7, 80-BF, 80-BF, 80-BF*/
	for(i = 0xF0 ; i <= 0xF4 ; i++) /* of the form  11110xxx */
		{		
		for(j = 0x80 ; j <= 0xBF ; j++) /* of the form 10xxxxxx */
			{
			if(i == 0xF0 && j == 0x80) j = 0x90; /** all of F08x is overlong **/
			if(i == 0xF4 && j == 0x90)
				{
				i = 0xF5; /* largest unicode character is F4 8F BF BF in utf8 (u+10FFFF) */
				break;
				}

			/* there are 64 values, each with 4 bytes = 256 bytes, Plus a null. Will fit in str buf */
			for(k = 0x80 ; k <= 0xBF ; k++) /* of the form 10xxxxxx */
				{

				for(l = 0x80 ; l <= 0xBF ; l++)
					{
					strBuf[(l-0x80)*4]   = i;
					strBuf[(l-0x80)*4+1] = j;
					strBuf[(l-0x80)*4+2] = k;
					strBuf[(l-0x80)*4+3] = l;
					strBuf[(l-0x80)*4+4] = '\0'; /* endable single char test */

					char *cur = strBuf+((l-0x80)*4);
					result = verifyUTF8(cur);
					assert(result == UTIL_VALID_CHAR);
					}
				result = verifyUTF8(strBuf);
				assert(result == UTIL_VALID_CHAR);
				}
			}
		}

	return 0;
	}
