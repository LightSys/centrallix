#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_01 ** SHOULD PASS **";

    double *table = nmMalloc(36 * sizeof(double));
    exp_fn_i_frequency_table(table, "Hello World");

    assert(table[11] == 3.0);
    
    nmFree(table, 36 * sizeof(double));
    return 0;
    }
