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
    	*tname = "xsFindWithCharOffset";
	int i, iter, offset;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 10000;

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
		offset = xsFindWithCharOffset(NULL, "o", 1, 0);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str0, NULL, -1, 2);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str0, "o", 1, -30);
		assert (offset == 4);
		offset = xsFindWithCharOffset(str0, "o", -1, 4);
		assert (offset == 4);
		offset = xsFindWithCharOffset(str0, "o", -1, 5);  
		assert (offset == 7);
		offset = xsFindWithCharOffset(str0, "o", 1, 8);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str0, "o", 1, 30000);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str0, "rld!", -1, 0);
                assert (offset == 8);
		offset = xsFindWithCharOffset(str0, "rld!!!!", -1, 0);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str1, "σειρά", -1,0);
		assert (offset == 0);
		offset = xsFindWithCharOffset(str2, "д", -1, 10);
		assert (offset == 14);
		offset = xsFindWithCharOffset(str2, "де знову прий", -1, -1);
		assert (offset == 14);
		offset = xsFindWithCharOffset(str3, "āʻo", -1, 0);
		assert (offset == 3);
		offset = xsFindWithCharOffset(str4, "ee", -1, 0);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str4, "görkem", -1, 6);
		assert (offset == -1);
		offset = xsFindWithCharOffset(str5, "是", -1, 2);
		assert (offset == 2);
		offset = xsFindWithCharOffset(str5, "", -1, 6);
              	assert (offset == -1);
		offset = xsFindWithCharOffset(str6, "し", -1, 0);
		assert (offset == 4);
		offset = xsFindWithCharOffset(str6, "した", -1, 0);
		assert (offset == 6);
		offset = xsFindWithCharOffset(str7, "した", -1, 0);
		assert (offset == -1);
		}

    return iter;
    }

