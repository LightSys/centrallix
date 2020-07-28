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
    	*tname = "xsSubstWithCharOffset";
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
		pXString str7 = xsNew();	
		pXString str8 = xsNew();
		pXString str9 = xsNew();
		pXString str10 = xsNew();
		pXString str11 = xsNew();
	
	
		xsConcatenate(str0, "Hello World!", -1);
		xsConcatenate(str1, "A\0", -1);
		xsConcatenate(str2, "", -1);
		xsConcatenate(str3, "012345677777777777789\0", -1);
		xsConcatenate(str4, "Merry Christmas!!!!!!\0", -1);
		xsConcatenate(str5, "Absolutely not\0", -1);
		xsConcatenate(str6, "Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test\0", -1);
		xsConcatenate(str7, "㉆㉆", -1);
		xsConcatenate(str8, "σειρά\0", -1);
		xsConcatenate(str9, "дощовий дощ піде знову прийде інший день\0", -1);
		xsConcatenate(str10, "耶穌是神\0", -1);
		xsConcatenate(str11, "彼は復活しました\0", -1);

		offset = xsSubstWithCharOffset(str0, 11, 1, "", 0);
		assert (strcmp(str0->String, "Hello World") == 0);
		offset = xsSubstWithCharOffset(str0, 10, 1, "d!", 2); 
		assert (strcmp(str0->String, "Hello World!") == 0);
		offset = xsSubstWithCharOffset(str1, 2, 0, "", 0);  
		assert (offset == -1);
		offset = xsSubstWithCharOffset(str1, -1, 0, "", 0);
                assert (offset == -1);
		offset = xsSubstWithCharOffset(str1, 0, 1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", -1);  
		assert (strcmp(str1->String, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA") == 0);
		offset = xsSubstWithCharOffset(str2, 0, 0, "B", 1);
                assert (strcmp(str2->String, "B") == 0);
		offset = xsSubstWithCharOffset(str3, 7, 12, "7", -1);
                assert (strcmp(str3->String, "0123456789") == 0);
		offset = xsSubstWithCharOffset(str4, 0, -1, "Jesus is born!", -1);
                assert (strcmp(str4->String, "Jesus is born!") == 0);
		offset = xsSubstWithCharOffset(str5, 10, -1, "!", 1);
                assert (strcmp(str5->String, "Absolutely!") == 0);
		offset = xsSubstWithCharOffset(str6, 4, -1, "", 0);
                assert (strcmp(str6->String, "Test") == 0);
		offset = xsSubstWithCharOffset(str7, 1, 1, "AAAAAAA", -1);	
		assert (strcmp(str7->String, "㉆AAAAAAA") == 0);
		offset = xsSubstWithCharOffset(str7, 4, 2, "㉆㉆㉆", -1);
                assert (strcmp(str7->String, "㉆AAA㉆㉆㉆AA") == 0);
		offset = xsSubstWithCharOffset(str7, 5, 2, "ד", -1); 
		assert (strcmp(str7->String, "㉆AAA㉆דAA") == 0);
		offset = xsSubstWithCharOffset(str8, 0, -1, "م", -1);   
		assert (strcmp(str8->String, "م") == 0);
		offset = xsSubstWithCharOffset(str9, 6, -1, "й", -1);  
		assert (strcmp(str9->String, "дощовий") == 0);
		offset = xsSubstWithCharOffset(str10, 4, 0, " hoʻāʻo", -1);
		assert (strcmp(str10->String, "耶穌是神 hoʻāʻo") == 0);
		offset = xsSubstWithCharOffset(str11, 3, 3, "Japanese", 8);
                assert (strcmp(str11->String, "彼は復Japaneseした") == 0);

		xsFree(str0);
		xsFree(str1);
		xsFree(str2);
		xsFree(str3);
		xsFree(str4);
		xsFree(str5);
		xsFree(str6);
		xsFree(str7);
		xsFree(str8);
                xsFree(str9);
                xsFree(str10);
		xsFree(str11);
		
		}

    return iter;
    }

