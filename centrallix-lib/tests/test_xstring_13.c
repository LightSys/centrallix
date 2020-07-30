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
    	*tname = "xsInsertAfterWithCharOffset part 2";
	int i, iter;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 1000000;

        for(i=0;i<iter;i++)
		{
		pXString str = xsNew();
		pXString str1 = xsNew();
		pXString str2 = xsNew();
		pXString str3 = xsNew();
	
		//xsConcatenate(str, "Hello World!", -1);
		xsConcatenate(str1, "abc", -1);
		xsConcatenate(str2, "", -1);
		xsConcatenate(str3, " ", -1);

		int ret;
		//empty string - never had contents
		ret = xsInsertAfterWithCharOffset(str, "œ", 1, 0);
		assert(ret==1);

		ret = xsInsertAfterWithCharOffset(str1, "howdy",5, 2);
		assert(ret==7);
	
		//initialized to empty string
		ret = xsInsertAfterWithCharOffset(str2, "aab",3, 0);
		assert(ret == 3);


		//initialized to single space
	        ret = xsInsertAfterWithCharOffset(str3, "efg",3, 0);
		assert(ret == 3);	

		
	}
	return iter;

}

