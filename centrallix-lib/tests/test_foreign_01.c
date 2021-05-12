#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include <wchar.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    
	setlocale(LC_CTYPE, "");
        wchar_t str[] = L"ﺎﻟﺮﺧﺎﻣ";
        //int bytes = (int)strlen(str);
        //printf("bytes: %d\n", bytes);
        wprintf(str);
	//fflush(stdout);
	wprintf(L"\n");
        wchar_t ch1 = L'ﺍ';
        wchar_t ch2 = L'ﻝ';
        wchar_t ch3 = L'ﺭ';
        wchar_t ch4 = L'ﺥ';
        wchar_t ch5 = L'ﺍ';
        wchar_t ch6 = L'ﻡ';
        wchar_t word[13];
        word[0] = ch1;
        word[1] = ch2;
        word[2] = ch3;
        word[3] = ch4;
        word[4] = ch5;
        word[5] = ch6;
        word[6] = '\0';
        wchar_t rev[7];
        rev[0] = ch6;
        rev[1] = ch5;
        rev[2] = ch4;
        rev[3] = ch3;
        rev[4] = ch2;
        rev[5] = ch1;
        rev[6] = '\0';

        //wprintf("str: %ls\n", str);
        wprintf(word);
	wprintf(L"\n");
        wprintf(rev);
	wprintf(L"\n");




	*tname = "Printing Arabic";
	iter = 10000000;
	for(i=0;i<iter;i++) assert (0 == 0);

    return iter;
    }

