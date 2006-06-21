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

	*tname = "BASELINE infinite loop - should show 'lockup'";
	iter = 1000*1000*1000;
	while(1);

    return iter;
    }

