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
    	*tname = "xsFindRevWithCharOffset";
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
	
	xsConcatenate(str0, "Hello World!", -1);
	xsConcatenate(str1, "σειρά\0", -1);
	xsConcatenate(str2, "дощовий дощ піде знову прийде інший день\0", -1);
	xsConcatenate(str3, "hoʻāʻo\0", -1);
	xsConcatenate(str4, "Tüm görkem Mesih'e\0", -1);
	xsConcatenate(str5, "耶穌是神\0", -1);
	xsConcatenate(str6, "彼は復活しました\0", -1);

        for(i=0;i<iter;i++)
		{
		offset = xsFindRevWithCharOffset(str0, "o", 1, -30);
		assert (offset == 7);
		offset = xsFindRevWithCharOffset(str0, "o", -1, 0);
		assert (offset == 7);
		offset = xsFindRevWithCharOffset(str0, "o", -1, 5);  
		assert (offset == 4);
		offset = xsFindRevWithCharOffset(str0, "o", 1, 8);
		assert (offset == -1);
		offset = xsFindRevWithCharOffset(str0, "o", 1, 30000);
		assert (offset == -1);
		offset = xsFindRevWithCharOffset(str0, "H", 1, 12);  
		assert (offset == -1);
		offset = xsFindRevWithCharOffset(str0, "rld!", -1, 0);
                assert (offset == 8);
		offset = xsFindRevWithCharOffset(str0, "rld!!!!", -1, 0);
		assert (offset == -1);
		offset = xsFindRevWithCharOffset(str1, "σειρά", -1,0);
		assert (offset == 0);
		offset = xsFindRevWithCharOffset(str1, "σ", -1, 4);
		assert (offset == 0);
		offset = xsFindRevWithCharOffset(str2, "д", -1, 29);
		assert (offset == 8);
		offset = xsFindRevWithCharOffset(str2, "де знову прий", -1, -1);
		assert (offset == 14);
		offset = xsFindRevWithCharOffset(str3, "āʻo", -1, 1);
		assert (offset == 3);
		offset = xsFindRevWithCharOffset(str4, "ee", -1, 0);
		assert (offset == -1);
		offset = xsFindRevWithCharOffset(str4, "görkem", -1, 6);
		assert (offset == 4);
		offset = xsFindRevWithCharOffset(str5, "是", -1, 1);
		assert (offset == 2);
		offset = xsFindRevWithCharOffset(str6, "し", -1, 2);
		assert (offset == 4);
		offset = xsFindRevWithCharOffset(str6, "した", -1, 0);
		assert (offset == 6);
		}

    return iter;
    }
