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

/** Blocks allocated by the most recent pass, used for the op count. **/
static int blocks = 0;

static bool
doTest(void)
    {
    int j,k,l;
    void* alloc[1024];

	j=0;
	while((alloc[j] = smMalloc(region,1 + rand()%8192)) != NULL && j < 1023) j++;
	if (j < 120) return false;
	blocks = j;

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

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-06 malloc/free 1MB, free order=random, size=[1-8192]";

	smInitialize();
	srand(time(NULL));
	region = smCreate(1024*1024);
	if (!region) return -1;

	rval = loopTest(doTest);

	if (rval > 0) rval *= blocks;

	smDestroy(region);
	region = NULL;

    return rval;
    }
