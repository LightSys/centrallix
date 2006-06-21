#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"

long long
test(char** tname)
    {
    int i;
    pSmRegion r;
    int iter;

	smInitialize();

	*tname = "smmalloc-01 create/destroy region";
	iter = 30000;
	for(i=0;i<iter;i++)
	    {
	    r = smCreate(1024*1024);
	    if (!r) return -1;
	    smDestroy(r);
	    }

    return iter;
    }

