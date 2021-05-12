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
		pXString str4 = xsNew();
		pXString str5 = xsNew();
		pXString str6 = xsNew();
		pXString str7 = xsNew();

	
		xsConcatenate(str, "Hello World!", -1);
		xsConcatenate(str1, "abc", -1);
		xsConcatenate(str2, "œbc", -1);
		xsConcatenate(str3, "Hi œbc", -1);
		xsConcatenate(str4, " e f g ", -1);
		xsConcatenate(str5, "", -1);
		xsConcatenate(str6, " ", -1);
		//str7 not used here intentionally		

		int ret;
		ret = xsInsertAfterWithCharOffset(str, "œ", 1, 4);
		assert(ret==5);

		ret = xsInsertAfterWithCharOffset(str1, "howdy", 5, 2);
		assert(ret==7);
	
		ret = xsInsertAfterWithCharOffset(str2, "aab", 3, 1);
		assert(ret == 4);

	        ret = xsInsertAfterWithCharOffset(str3, str2->String, 6, 3);
		assert(ret == 9);			
		
		ret = xsInsertAfterWithCharOffset(str4, "gg", 2, 2);
		assert(ret==4);

		ret = xsInsertAfterWithCharOffset(str5, "howdy", 5, 0);
		assert(ret == 5);		

		ret = xsInsertAfterWithCharOffset(str6, " 1 ", 3, 0);
		assert(ret == 3);

		ret = xsInsertAfterWithCharOffset(str7, ":)", 2, 0);
		assert(ret == 2);		


		xsFree(str1);
		xsFree(str2);
		xsFree(str3);
		xsFree(str4);
		xsFree(str);
		xsFree(str5);
		xsFree(str6);
		xsFree(str7);



		
	}

    return iter;
    }



