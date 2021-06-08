#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
	return 0;
}

long long
test(char** tname)
    {
    int i, j, iter, ret;
	pBPTree tree = bptNew();

	*tname = "b+tree_80 Testing a specific branch of bptRemove";
    int status = 0;
    
    //Note: this is empirically based on the current implementation of the tree and a branching factor of 8
    
    iter = 1000;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        for(j = 0; j < 137; j++)
            {
            char* key = nmSysMalloc(11);
            int* data = nmMalloc(sizeof(int));
            *data = 1+j;
            sprintf(key, "%010d", *data);
            ret = bptAdd(tree, key, 10, data);
            assert (ret == 0);
            }

        ret = bptRemove(tree, "0000000137", 10, free_func, NULL);
        assert (ret == 0);
        ret = bptRemove(tree, "0000000136", 10, free_func, NULL);
        assert (ret == 0);
        ret = bptRemove(tree, "0000000135", 10, free_func, NULL);
        assert (ret == 0);
        ret = bptRemove(tree, "0000000134", 10, free_func, NULL);
        assert (ret == 0);
        ret = bptFree(tree, free_func, NULL);
        assert (ret == 0);
        }

    return iter;
    }


