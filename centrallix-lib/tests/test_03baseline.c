#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

long long
test(char** tname)
    {
    int iter;
    int array[2];

	*tname = "BASELINE sigsegv (11) caused - should crash";
	iter = 1000*1000*1000;
	*((int*)0) = array[1];

    return iter;
    }

