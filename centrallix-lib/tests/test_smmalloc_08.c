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

static bool
doTests(void)
    {
    int j,l,s;
    void* alloc[128];
    int size[128];
    int tsize;

	tsize = 0;

	/** allocate **/
	for(j=0;j<128;j++) 
	    {
	    alloc[j] = smMalloc(region, 1024);
	    size[j] = 1024;
	    tsize += size[j];
	    }

	/** realloc **/
	while(tsize < 512*1024) /* fill up half */ 
	    {
	    l = rand()%j;
	    s = size[l] + rand()%2048;
	    alloc[l] = smRealloc(alloc[l], s);
	    if (!alloc[l]) return false;
	    tsize += (s - size[l]);
	    size[l] = s;
	    }

	/** free **/
	for(j=127;j>=0;j--)
	    {
	    smFree(alloc[j]);
	    }

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-08 block realloc to 512K of 1M - 128 1K blocks";

	smInitialize();
	srand(time(NULL));
	region = smCreate(1024*1024);
	if (!region) return -1;

	rval = loopTests(doTests);

	if (rval > 0) rval *= 128;

	smDestroy(region);
	region = NULL;

    return rval;
    }
