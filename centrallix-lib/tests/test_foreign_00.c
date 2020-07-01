#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    
	char* str1 = "Glück";
	char* str2 = "Χαίρετε";
	char* str3 = "\u0393";
	char* str4 = "\xce\x93";
	printf("%s\n%s\n%s\n%s\n", str1, str2, str3, str4);
	
	*tname = "Printing foreign characters";
	iter = 10000000;
	for(i=0;i<iter;i++) assert (0 == 0);

    return iter;
    }

