#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_03 ** SHOULD PASS **";

    double *table = nmMalloc(36 * sizeof(double));
    exp_fn_i_frequency_table(table, "Hello World");
    //exp_fn_i_relative_frequency_table(table);

    assert(table[11] == 0.3);

    nmFree(table, 36 * sizeof(double));
    return 0;
    }
