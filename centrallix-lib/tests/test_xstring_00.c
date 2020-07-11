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
    	*tname = "Test chrCharLength Function";
	int i, iter, len;
	setlocale(LC_ALL, "en_US.UTF-8");
	
	iter = 100000;
        for(i=0;i<iter;i++)
		{
		len = chrCharLength("ABC\0");
        	assert(len == 3);

		len = chrCharLength("他复活了！\0");
        	assert(len == 5);	

		len = chrCharLength("پرکھ\0");
		assert(len == 4);

		len = chrCharLength("イエスは主です\0");
		assert(len == 7);

		
		}

    return iter;
    }

