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
    	*tname = "xsConcatenate";
	int i, iter;

	setlocale(LC_ALL, "en_US.UTF-8");
	iter = 1000000;

        for(i=0;i<iter;i++)
		{
		pXString str = xsNew();
		assert (str != NULL);
		
		xsConcatenate(str, "Hello World!", -1);
		assert (strcmp(str->String, "Hello World!") == 0);
		assert (str->Length == 12);

		xsConcatenate(str, " ก ขขฌ ณพพ ", -1);
		assert (strcmp(str->String, "Hello World! ก ขขฌ ณพพ ") == 0);
		assert (str->Length == (12 + 25));
		
		xsFree(str);
		}

    return iter;
    }

