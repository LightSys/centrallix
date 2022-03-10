#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_04 ** SHOULD PASS **";
    pXHashTable table = nmSysMalloc(sizeof(XHashTable));    
    typedef struct d_data_node { int key; double value; } freq_node;
    
    exp_fn_i_frequency_table(table, "Hello");
    exp_fn_i_relative_frequency_table(table);

    double magnitude;
    exp_fn_i_magnitude(&magnitude, table, table);
    assert(magnitude >= 0.529150 && magnitude <= 0.529151);
    xhDeInit(table);
    nmSysFree(table);
    return 0;

    }
