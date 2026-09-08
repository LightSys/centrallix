#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "smmalloc.h"
#include <stdbool.h>
#include "test_utils.h"

/** Region shared by every pass; created and destroyed by test(). **/
static pSmRegion region = NULL;

/** References freed by the most recent pass, used for the op count. **/
static int refs = 0;

static bool
doTests(void)
    {
    int j,k,l,t;
    void* alloc[1024];
    int cnt[1024];

	/** allocate **/
	j=0;
	while((alloc[j] = smMalloc(region,1 + rand()%8192)) != NULL && j < 1023) 
	    {
	    cnt[j++] = 1;
	    }
	if (j < 120) return false;

	/** link **/
	for(k=0;k<1024;k++) 
	    {
	    l = rand()%j;
	    smLinkTo(alloc[l]);
	    cnt[l]++;
	    }

	/** free **/
	t = j + 1024;
	refs = t;
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

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-07 reference counting (randomized free order)";

	smInitialize();
	srand(time(NULL));
	region = smCreate(1024*1024);
	if (!region) return -1;

	rval = loopTests(doTests);

	if (rval > 0) rval *= refs;

	smDestroy(region);
	region = NULL;

    return rval;
    }
