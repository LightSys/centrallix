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
    int j,l,s;
    void* alloc[128];
    int size[128];
    int tsize;

	smInitialize();

	*tname = "smmalloc-08 block realloc to 512K of 1M - 128 1K blocks";
	srand(time(NULL));
	iter = 1000;
	r = smCreate(1024*1024);
	for(i=0;i<iter;i++)
	    {
	    tsize = 0;
	    /** allocate **/
	    for(j=0;j<128;j++) 
		{
		alloc[j] = smMalloc(r, 1024);
		size[j] = 1024;
		tsize += size[j];
		}

	    /** realloc **/
	    while(tsize < 512*1024) /* fill up half */ 
		{
		l = rand()%j;
		s = size[l] + rand()%2048;
		alloc[l] = smRealloc(alloc[l], s);
		if (!alloc[l])
		    {
		    smDestroy(r);
		    return -1;
		    }
		tsize += (s - size[l]);
		size[l] = s;
		}
	    /** free **/
	    for(j=127;j>=0;j--)
		{
		smFree(alloc[j]);
		}
	    }
	smDestroy(r);

    return iter*128;
    }

