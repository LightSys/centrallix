#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include "charsets.h"

long long
test (char** tname)
	{
	int i, j, k, l;
	char strBuf[10]; 
	char * result;
	
	*tname = "Overlong 01: Ensure invalid chars fail";

	setlocale(LC_ALL, "en_US.utf8");
	
	/* fill buffer with valid chars */
	memset(strBuf, 'A', 9);
	strBuf[9] = '\n';

	/** test all 2 byte overlong: C0-C1 (2 byte) **/
	for(i = 0xC0 ; i <= 0xC1 ; i++) /* of the form  110xxxxx, but C0 and C1 are overlongs, so skip */
		{
		/* there are 64 values, each with two bytes = 128 bytes, Plus a null. Will fit in str buf */
		for(j = 0x80 ; j < 0xBF ; j++) /* of the form 10xxxxxx */
			{
			strBuf[3] = i;
			strBuf[4] = j;
			strBuf[5] = '\0'; /* endable single char test */

			char *cur = strBuf+3;
			result = chrNoOverlong(cur);
			assert(result == NULL);
			
			/* now test in the middle of a string */
			strBuf[5] = 'A';
			result = chrNoOverlong(strBuf);
			assert(result == 0);
			}
		}


	/** test all 3 byte invalid **/
	/* E08x/E09x (3 byte)  are all overlongs 
	 * ED includes U+D800–U+DFFF, which are "invalid UTF-16 surrogate halves"
	 */

	for(i = 0xE0 ; i <= 0xED ; i++)
		{
		if(i == 0xE1) i = 0xED; /* skip the valid values in the middle */

		for(j = 0x80 ; j < 0x9F ; j++) 
			{
			if(i == 0xED && j == 0x80) j = 0xA0; /* skip valid values */

			for(k = 0x80 ; k < 0xBF ; k++) /* of the form 10xxxxxx */
				{
				strBuf[3] = i;
				strBuf[4] = j;
				strBuf[5] = k;
				strBuf[6] = '\0';

				char *cur = strBuf+3;
				result = chrNoOverlong(cur);
				assert(result == NULL);
				
				/* now test in the middle of a string */
				strBuf[6] = 'A';
				result = chrNoOverlong(strBuf);
				assert(result == 0);
				}
			}
		}

	/** test 4 byte invalid chars **/
	/* F08x are all overlong (4 byte)
	 * Values above F4 90 and above (so up to FF) are too large for utf
	 */
	for(i = 0xF0 ; i < 0xFF; i++) 
		{		
		for(j = 0x80 ; j < 0xBF ; j++) /* of the form 10xxxxxx */
			{
			if(i == 0xF0 && j == 0x90)
				{
				i = 0xF4; /* skip over valid values */
				}

			for(k = 0x80 ; k < 0xBF ; k++) /* of the form 10xxxxxx */
				{
				for(l = 0x80 ; l < 0xBF ; l++)
					{
					strBuf[3] = i;
					strBuf[4] = j;
					strBuf[5] = k;
					strBuf[6] = l;
					strBuf[7] = '\0';

					char *cur = strBuf+3;
					result = chrNoOverlong(cur);
					assert(result == NULL);
					
					/* now test in the middle of a string */
					strBuf[7] = 'A';
					result = chrNoOverlong(strBuf);
					assert(result == 0);
					}
				}
			}
		}

	return 0;
	}
