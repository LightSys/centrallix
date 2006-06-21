#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    int array[2];

	*tname = "BASELINE - should pass";
	iter = 1000*1000*300;
	for(i=0;i<iter;i++) array[0] = array[1];

    return iter;
    }

