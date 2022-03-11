#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_01 ** SHOULD PASS **";
    // pXHashTable table = nmSysMalloc(sizeof(XHashTable));    
    // typedef struct d_data_node { int key; double value; } freq_node;

    double *table = nmMalloc(36 * sizeof(double));
    exp_fn_i_frequency_table(table, "Hello World");

    // int key = 'L';
    // void *data = xhLookup(table, (char *)&key);
    // freq_node *data_entry = (freq_node *)data;
    // assert(data_entry->value == 3.0);
    assert(table[11] == 3.0);
    // xhDeInit(table);
    // nmSysFree(table);
    nmFree(table, 36 * sizeof(double));
    return 0;
    }
