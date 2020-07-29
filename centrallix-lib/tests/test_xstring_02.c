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
    	*tname = "xsFind";
	int i, iter, offset;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 100000;

	pXString str0 = xsNew();
	pXString str1 = xsNew();	
	pXString str2 = xsNew();
	pXString str3 = xsNew();
	pXString str4 = xsNew();
	pXString str5 = xsNew();
	pXString str6 = xsNew();
	pXString str7 = xsNew();

	xsConcatenate(str0, "Hello World!", -1);
	xsConcatenate(str1, "σειρά\0", -1);
	xsConcatenate(str2, "дощовий дощ піде знову прийде інший день\0", -1);
	xsConcatenate(str3, "hoʻāʻo\0", -1);
	xsConcatenate(str4, "Tüm görkem Mesih'e\0", -1);
	xsConcatenate(str5, "耶穌是神\0", -1);
	xsConcatenate(str6, "彼は復活しました\0", -1);


        for(i=0;i<iter;i++)
		{
		offset = xsFind(NULL, "o", 1, 0);
		assert (offset == -1);
		offset = xsFind(str0, NULL, -1, 2);
		assert (offset == -1);
		offset = xsFind(str0, "o", 1, -30);
		assert (offset == 4);
		offset = xsFind(str0, "o", -1, 4);
		assert (offset == 4);
		offset = xsFind(str0, "o", -1, 5);  
		assert (offset == 7);
		offset = xsFind(str0, "o", 1, 8);
		assert (offset == -1);
		offset = xsFind(str0, "o", 1, 30000);
		assert (offset == -1);
		offset = xsFind(str0, "rld!", -1, 0);
                assert (offset == 8);
		offset = xsFind(str0, "rld!!!!", -1, 0);
		assert (offset == -1);
		offset = xsFind(str1, "σειρά", 1, 0);
		assert (offset == 0);
		offset = xsFind(str2, "д", -1, 19);
		assert (offset == 26);
		offset = xsFind(str2, "де знову прий", -1, -1);
		assert (offset == 26);
		offset = xsFind(str3, "āʻo", -1, 0);
		assert (offset == 4);
		offset = xsFind(str4, "ee", -1, 0);
		assert (offset == -1);
		offset = xsFind(str4, "görkem", -1, 8);
		assert (offset == -1);
		offset = xsFind(str5, "是", -1, 6);
		assert (offset == 6);
		offset = xsFind(str5, "", -1, 6);
              	assert (offset == -1);
		offset = xsFind(str6, "し", -1, 0);
		assert (offset == 12);
		offset = xsFind(str6, "した", -1, 0);
		assert (offset == 18);
		offset = xsFind(str7, "した", -1, 0);
		assert (offset == -1);
		}

    return iter;
    }

