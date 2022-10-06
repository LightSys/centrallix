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
	int i, j;
	int itter = 10000;
	
	*tname = "Util Verify 03: test validateASCII";

	setlocale(LC_ALL, "en_US.utf8");

	/** make sure works with all valid ascii characters **/
	char strBuf[257]; /* room for 64 4-byte characters and a null byte */
	int result;
	char cur[2];

        for(i = 0 ; i < itter ; i++)
	    {
	    for(j = 0 ; j < 128 ; j++)
	        {
	        /* test one char and build up string */
	        strBuf[127 - j] = (char) j; /* need null char last */
	        cur[0] = (char) j;
	        cur[1] = '\0';
        
	        result = verifyASCII(cur);
	        assert(result == UTIL_VALID_CHAR);
	        }
	    result = verifyASCII(strBuf);
	    assert(result == UTIL_VALID_CHAR);
           
            /** ensure invalid values fail **/
	    for(j = 128 ; j < 256 ; j++)
		{
		/** test by self **/
		cur[0] = (char) j;
		cur[1] = '\0';
		result = verifyASCII(cur);
		assert(result == 0);

		/** test in larger string **/
		strBuf[64] = (char) j;
		result = verifyASCII(strBuf);
		assert(result == 64);
		}
	    }
	return itter;
	}
