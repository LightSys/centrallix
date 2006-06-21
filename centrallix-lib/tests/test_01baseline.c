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

	*tname = "BASELINE assert() failed - should abort";
	iter = 1000*1000*1000;
	assert(0);

    return iter;
    }

