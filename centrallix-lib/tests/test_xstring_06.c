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
    	*tname = "xsSubst";
	int i, iter, offset;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 50000;

	for (i = 0; i < iter; i++)
		{
		pXString str0 = xsNew();
		pXString str1 = xsNew();	
		pXString str2 = xsNew();
		pXString str3 = xsNew();
		pXString str4 = xsNew();
		pXString str5 = xsNew();
		pXString str6 = xsNew();
	
		xsConcatenate(str0, "Hello World!", -1);
		xsConcatenate(str1, "A\0", -1);
		xsConcatenate(str2, "", -1);
		xsConcatenate(str3, "012345677777777777789\0", -1);
		xsConcatenate(str4, "Merry Christmas!!!!!!\0", -1);
		xsConcatenate(str5, "Absolutely not\0", -1);
		xsConcatenate(str6, "Test Test Test Test Test\0", -1);

		offset = xsSubst(str0, 11, 1, "", 0);
		assert (strcmp(str0->String, "Hello World") == 0);
		offset = xsSubst(str0, 10, 1, "d!", 2); 
		assert (strcmp(str0->String, "Hello World!") == 0);
		offset = xsSubst(str1, 2, 0, "", 0);  
		assert (assert == -1);
		offset = xsSubst(str1, -1, 0, "", 0);
                assert (assert == -1);
		offset = xsSubst(str1, 0, 1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", -1);  
		assert (strcmp(str0->String, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA") == 0);
			



		}

    return iter;
    }

