#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include "xstring.h"

long long
test(char** tname)
    {
    	*tname = "xsInsertAfterWithCharOffset";
	int i, iter;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 1000000;

        for(i=0;i<iter;i++)
		{
		pXString str = xsNew();
		pXString str1 = xsNew();
		pXString str2 = xsNew();
		pXString str3 = xsNew();
	
		xsConcatenate(str, "Hello World!", -1);
		xsConcatenate(str1, "abc", -1);
		xsConcatenate(str2, "œbc", -1);
		xsConcatenate(str3, "Hi œbc", -1);

		int ret;
		ret = xsInsertAfterWithCharOffset(str, "œ", 1, 4);
		assert(ret==5);

		ret = xsInsertAfterWithCharOffset(str1, "howdy", 5, 2);
		assert(ret==7);
	
		ret = xsInsertAfterWithCharOffset(str2, "aab", 3, 1);
		assert(ret == 4);

	        ret = xsInsertAfterWithCharOffset(str3, str2->String, 6, 3);
		assert(ret == 9);			

		
	}

    return iter;
    }



