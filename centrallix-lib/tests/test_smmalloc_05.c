#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "smmalloc.h"

long long
test(char** tname)
    {
    int i;
    pSmRegion r;
    int iter;
    int j,k,l;
    void* alloc[1024];

	smInitialize();

	*tname = "smmalloc-05 malloc/free 1MB, free order = random, size=1K";
	srand(time(NULL));
	iter = 1000;
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
	    for(k=0;k<j*4/5;k++)
		{
		while (alloc[(l = rand()%j)] == NULL)
		    ;
		smFree(alloc[l]);
		alloc[l] = NULL;
		}
	    for(k=0;k<j;k++)
		{
		if (alloc[k]) smFree(alloc[k]);
		alloc[k] = NULL;
		}
	    }
	smDestroy(r);

    return iter*j;
    }

