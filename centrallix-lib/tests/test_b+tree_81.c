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
    
	*tname = "b+tree_81 Testing bptFromLookup when the tree has only a root node";
    
    int status = 0;
    int i, j, iter, ret;
	pBPIter iterator;
    pBPTree tree = bptNew();


    iter = 10000;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        ret = automatedTree(tree, 8);
        assert (ret == 0);

        iterator = bptFromLookup(tree, 0, "0000000000", 10);
        assert(0 == *((int*)(iterator->Ref)));

        ret = bptFree(tree, free_func, NULL);
        assert (ret == 0);
        }

    return iter;
    }


