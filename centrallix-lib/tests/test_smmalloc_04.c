#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"
#include "smmalloc_private.h"

long long
test(char** tname)
    {
    int i;
    pSmRegion r;
    int iter;
    int j,k;
    void* alloc[1024];
    int min_blocks;

	smInitialize();

	*tname = "smmalloc-04 malloc/free 1MB, free order = FIFO, size=1K";
	iter = 200;
	/** Each allocation consumes a block header too, so how many 1K blocks
	 ** fit in a 1MB region depends on the header size, not just on the
	 ** region size.  Leave slack for the region's own overhead.
	 **/
	min_blocks = ((1024*1024 - sizeof(SmRegion)) / (1024 + sizeof(SmBlock))) * 9 / 10;

	r = smCreate(1024*1024);
	for(i=0;i<iter;i++)
	    {
	    j=0;
	    while((alloc[j] = smMalloc(r,1024)) != NULL && j < 1023) j++;
	    if (j < min_blocks)
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
