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
	int i, j, k, l, m;
	char strBuf[10]; 
	int result;
	
	*tname = "Overlong 02: Ensure broken chars fail";

	setlocale(LC_ALL, "en_US.utf8");
	/* fill buffer with valid chars */
	memset(strBuf, 'A', 9);
	strBuf[9] = '\n';

	/** test chars with too few bytes **/
	for(i = 0xC0 ; i <= 0xFF ; i++)
		{
		/* all should have at least 2 */
		strBuf[3] = i;
		strBuf[4] = '\0';

		char *cur = strBuf+3;
		result = chrNoOverlong(cur);
		assert(result != 0);

		/* now test in the middle of a string */
		strBuf[4] = 'A';
		result = chrNoOverlong(strBuf);
		assert(result != 0);

		for(j = 0x80 ; j < 0x9F ; j++) 
			{
			/* only test 2 for headers 0xE0 and above */
			if(i >= 0xE0)
				{
				strBuf[3] = i;
				strBuf[4] = j;
				strBuf[5] = '\0'; /* endable single char test */

				char *cur = strBuf+3;
				result = chrNoOverlong(cur);
				assert(result != 0);

				/* now test in the middle of a string */
				strBuf[5] = 'A';
				result = chrNoOverlong(strBuf);
				assert(result != 0);
				}

			for(k = 0x80 ; k < 0xBF ; k++) /* of the form 10xxxxxx */
				{
				/* only test 3 on F0 and above  */
				if(i >= 0xF0)
					{
					strBuf[3] = i;
					strBuf[4] = j;
					strBuf[5] = k;
					strBuf[6] = '\0'; /* endable single char test */

					char *cur = strBuf+3;
					result = chrNoOverlong(cur);
					assert(result != 0);
					
					/* now test in the middle of a string */
					strBuf[6] = 'A';
					result = chrNoOverlong(strBuf);
					assert(result != 0);
					}	
				}
			}
		}

	/** test chars with too many bytes **/
	/* note that for cases where the first byte is normal ascii, this also 
	 * covers cases of lone body bytes (of the form 8x/9x)
	 */
	for(i = 0x1 ; i < 0xFF; i++) 
		{		
		for(j = 0x80 ; j < 0x90 ; j++) /* of the form 10xxxxxx */
			{
			/* only ascii (essentially lone data bytes) */
			if(i < 0xC0)
				{
				strBuf[3] = i;
				strBuf[4] = j;
				strBuf[5] = '\0'; /* endable single char test */

				char *cur = strBuf+3;
				result = chrNoOverlong(cur);
				assert(result != 0);
					
				/* now test in the middle of a string */
				strBuf[5] = 'A';
				result = chrNoOverlong(strBuf);
				assert(result != 0);
				}

			for(k = 0x80 ; k < 0x90 ; k++) /* of the form 10xxxxxx */
				{
				/* test up to 2 byte headers */
				if(i < 0xE0)
					{
					strBuf[3] = i;
					strBuf[4] = j;
					strBuf[5] = k;
					strBuf[6] = '\0'; /* endable single char test */

					char *cur = strBuf+3;
					result = chrNoOverlong(cur);
					assert(result != 0);
					
					/* now test in the middle of a string */
					strBuf[6] = 'A';
					result = chrNoOverlong(strBuf);
					assert(result != 0);
					}

				for(l = 0x80 ; l < 0x90 ; l++)
					{
					/* test up to 3 byte headers */
					if(i < 0xF0)
						{
						strBuf[3] = i;
						strBuf[4] = j;
						strBuf[5] = k;
						strBuf[6] = l;
						strBuf[7] = '\0'; /* endable single char test */

						char *cur = strBuf+3;
						result = chrNoOverlong(cur);
						assert(result != 0);
						
						/* now test in the middle of a string */
						strBuf[7] = 'A';
						result = chrNoOverlong(strBuf);
						assert(result != 0);
						}
					
					for(m = 0x80 ; m < 0x90 ; m++)
						{
						/* there are no valid 5 byte chars, so test with all */
						strBuf[3] = i;
						strBuf[4] = j;
						strBuf[5] = k;
						strBuf[6] = l;
						strBuf[7] = m;
						strBuf[8] = '\0'; /* endable single char test */

						char *cur = strBuf+3;
						result = chrNoOverlong(cur);
						assert(result != 0);
						
						/* now test in the middle of a string */
						strBuf[8] = 'A';
						result = chrNoOverlong(strBuf);
						assert(result != 0);
						}
					}
				}
			}
		}

	return 0;
	}
