#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "smmalloc.h"
#include <stdbool.h>
#include "test_utils.h"

static bool
doTests(void)
    {
    pSmRegion r;

	r = smCreate(1024*1024);
	if (!r) return false;
	smDestroy(r);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "smmalloc-01 create/destroy region";
    smInitialize();
    return loopTests(doTests);
    }
