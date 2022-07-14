#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_04 ** SHOULD PASS **";

    double *table = nmMalloc(36 * sizeof(double));
    exp_fn_i_frequency_table(table, "Hello");
    //exp_fn_i_relative_frequency_table(table);

    double dot_product = 0.0;
    exp_fn_i_dot_product(&dot_product, table, table);
    
    double magnitude;
    exp_fn_i_magnitude(&magnitude, table, table);
    assert(magnitude >= 0.529150 && magnitude <= 0.529151);

    nmFree(table, 36 * sizeof(double));
    return 0;

    }
