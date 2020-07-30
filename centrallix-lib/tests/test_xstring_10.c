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
    	*tname = "xsInsertAfter";
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
		ret = xsInsertAfter(str, "œ", 1, 1);
		assert(ret==2);

		ret = xsInsertAfter(str1, "howdy",5, 2);
		assert(ret==7);
	
		ret = xsInsertAfter(str2, "aab",3, 1);
		assert(ret == 4);

	        ret = xsInsertAfter(str3, str2->String,6, 3);
		assert(ret == 9);	
		

		/*pXString str4 = xsNew();
		xsConcatenate(str4, "oh", -1);
		ret = xsInsertAfter(str4, "more", 4, 1);
		assert(ret == 5);	*/	

		
	}


/*	for(i=0;i<iter;i++){
		int ret;
		pXString str4 = xsNew();
		xsConcatenate(str4, "oh", -1);
		ret = xsInsertAfter(str4, "more", 4, 1);
		assert(ret == 5);		
	}

*/
    return iter;
    }



