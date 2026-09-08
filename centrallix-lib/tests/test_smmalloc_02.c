#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"
#include <stdbool.h>
#include "test_utils.h"

/** Region shared by every pass; created and destroyed by test(). **/
static pSmRegion region = NULL;

static bool
doTests(void)
    {
    void* ptr;

	ptr = smMalloc(region, 1024);
	if (!ptr) return false;
	smFree(ptr);

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "smmalloc-02 malloc/free 1024 bytes";

	smInitialize();
	region = smCreate(1024*1024);
	if (!region) return -1;

	rval = loopTests(doTests);

	smDestroy(region);
	region = NULL;

    return rval;
    }
