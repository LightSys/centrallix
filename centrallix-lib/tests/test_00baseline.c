#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "test_utils.h"

/** No-op operations per pass, enough that the loop dominates the timing. **/
#define OPS_PER_PASS	(1000*1000)

static bool
doTests(void)
    {
    int i;
    int array[2] = {0};

	for(i=0;i<OPS_PER_PASS;i++) array[0] = array[1];

    return true;
    }

long long
test(char** tname)
    {
    *tname = "BASELINE - should pass";
    return loopTests(doTests) * OPS_PER_PASS;
    }
