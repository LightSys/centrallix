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
    int j,k;
    void* alloc[1024];

	smInitialize();

	*tname = "smmalloc-04 malloc/free 1MB, free order = FIFO, size=1K";
	iter = 300;
	r = smCreate(1024*1024);
	for(i=0;i<iter;i++)
	    {
	    j=0;
	    while((alloc[j] = smMalloc(r,1024)) != NULL && j < 1023) j++;
	    if (j < 950)
		{
		smDestroy(r);
		return -1;
		}
	    for(k=0;k<j;k++)
		{
		smFree(alloc[k]);
		}
	    }
	smDestroy(r);

    return iter*j;
    }
