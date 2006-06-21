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

	*tname = "BASELINE failed, return value - should fail";
	iter = 1000*1000*1000;

    return -1;
    }

