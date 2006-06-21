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
    int j,k,l,t;
    void* alloc[1024];
    int cnt[1024];

	smInitialize();

	*tname = "smmalloc-07 reference counting (randomized free order)";
	srand(time(NULL));
	iter = 300;
	r = smCreate(1024*1024);
	for(i=0;i<iter;i++)
	    {
	    j=0;
	    /** allocate **/
	    while((alloc[j] = smMalloc(r,1 + rand()%8192)) != NULL && j < 1023) 
		{
		cnt[j++] = 1;
		}
	    if (j < 120)
		{
		smDestroy(r);
		return -1;
		}
	    /** link **/
	    for(k=0;k<1024;k++) 
		{
		l = rand()%j;
		smLinkTo(alloc[l]);
		cnt[l]++;
		}
	    /** free **/
	    t = j + 1024;
	    for(k=0;k<t*4/5;k++)
		{
		while (cnt[(l = rand()%j)] == 0)
		    ;
		smFree(alloc[l]);
		cnt[l]--;
		if (!cnt[l]) alloc[l] = NULL;
		}
	    for(k=0;k<j;k++)
		{
		if (alloc[k]) 
		    {
		    for(l=0;l<cnt[k];l++) smFree(alloc[k]);
		    }
		alloc[k] = NULL;
		}
	    }
	smDestroy(r);

    return iter*t;
    }

