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
    	*tname = "xsReplaceWithCharOffset";
	int i, iter;
	setlocale(LC_ALL, "en_US.UTF-8");
	char * find1 = "a\0";
	int findlen = 1;
	char * find2 = "á\0";
	iter = 100000;
	for(i=0;i<iter;i++){
		pXString str1 = xsNew();
		pXString str2 = xsNew();
		pXString str3 = xsNew();
		pXString str4 = xsNew();

		xsConcatenate(str1, "abc", -1);
		xsConcatenate(str2, "bac", -1);
		xsConcatenate(str3, "ábc", -1);
		xsConcatenate(str4, "bác", -1);
	
		int ret = 0;
		ret = xsReplaceWithCharOffset(str1, "c", findlen, 2, "œ", findlen);
		assert(ret == 2);
			
		ret = xsReplaceWithCharOffset(str2, find1, findlen, 1, find2, findlen);
		assert(ret == 1); 

		ret = xsReplaceWithCharOffset(str3, find2, findlen, 0, find1, findlen);
		assert(ret == 0); 


		ret = xsReplaceWithCharOffset(str4, find2, findlen, 1, find1, findlen);
		assert(ret == 1);

		ret = xsReplaceWithCharOffset(str2, "e\0", findlen, 2, find2, findlen);
		assert(ret == -1); 

		ret = xsReplaceWithCharOffset(str3, "eñ\0", findlen+1, 2, find2, findlen);
		assert(ret == -1); 
 

	
	}
    return iter;
    }


