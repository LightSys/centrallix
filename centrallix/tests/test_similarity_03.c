#include <assert.h>
#include <stdio.h>
#include "cxlib/newmalloc.h"
#include "cxlib/xhash.h"
long long
test(char** name)
    {
    *name = "similarity_03 ** SHOULD PASS **";
    pXHashTable table = nmSysMalloc(sizeof(XHashTable));    
    typedef struct d_data_node { int key; double value; } freq_node;
    
    exp_fn_i_frequency_table(table, "Hello");
    exp_fn_i_relative_frequency_table(table);

    int key = 'L';
    void *data = xhLookup(table, (char *)&key);
    freq_node *data_entry = (freq_node *)data;
    assert(data_entry->value == 0.4);
    xhDeInit(table);
    nmSysFree(table);
    return 0;
    }
