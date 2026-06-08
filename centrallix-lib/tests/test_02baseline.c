#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

long long
test(char** tname)
    {
	*tname = "BASELINE failed, return value - should fail";

    return -1;
    }
