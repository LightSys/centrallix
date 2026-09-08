#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"
#include "smmalloc_private.h"
#include <stdbool.h>
#include "test_utils.h"

/** Region shared by every pass; created and destroyed by test(). **/
static pSmRegion region = NULL;

/** Blocks allocated by the most recent pass, used for the op count. **/
static int blocks = 0;

static bool
doTests(void)
    {
    int j,k;
    void* alloc[1024];
    int min_blocks;

	/** Each allocation consumes a block header too, so how many 1K blocks
	 ** fit in a 1MB region depends on the header size, not just on the
	 ** region size.  Leave slack for the region's own overhead.
	 **/
	min_blocks = ((1024*1024 - sizeof(SmRegion)) / (1024 + sizeof(SmBlock))) * 9 / 10;

	j=0;
	while((alloc[j] = smMalloc(region,1024)) != NULL && j < 1023) j++;
	if (j < min_blocks) return false;
	blocks = j;

	k = j;
	while(k > 0)
	    {
	    k--;
	    smFree(alloc[k]);
	    }

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-03 malloc/free 1MB, free order = LIFO, size=1K";

	smInitialize();
	region = smCreate(1024*1024);
	if (!region) return -1;

	rval = loopTests(doTests);

	if (rval > 0) rval *= blocks;

	smDestroy(region);
	region = NULL;

    return rval;
    }
