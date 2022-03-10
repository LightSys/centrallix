#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_02 ** SHOULD PASS **";
    pXHashTable table = nmSysMalloc(sizeof(XHashTable));    
    typedef struct d_data_node { int key; double value; } freq_node;
    
    exp_fn_i_frequency_table(table, "Hello");
    exp_fn_i_relative_frequency_table(table);

    double dot_product;
    exp_fn_i_dot_product(&dot_product, table, table);
    assert(dot_product == 0.28);
    xhDeInit(table);
    nmSysFree(table);
    return 0;
    }
