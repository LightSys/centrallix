#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../../centrallix/include/charsets.h"

long long
test(char** tname)
    {
    int i;
    int iter;
    size_t len;

	char* str2 = "Χαίρετε";
	len = chrCharLength(str2);
	printf("num chars = %d", len);
	assert(len == 7);

	*tname = "Accessing centrallix";
	iter = 10000000;
	for(i=0;i<iter;i++) assert (0 == 0);

    return iter;
    }

