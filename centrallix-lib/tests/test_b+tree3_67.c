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
    int i, ret, iter;

	*tname = "b+tree3_67 Testing memory leaks: add only";

    iter = 1000;
    for(i = 0; i < iter; i++)
        {
        pBPTree tree = bptNew();
        ret = automatedTree(tree, 30);
        assert (ret == 0);
        printTree(tree->root);
        printf("\n");
        ret = bptRemove(tree, "0000000009", 10, free_func, NULL);
        assert(ret == 0);
        ret = bptFree(tree, free_func, NULL);
        assert(ret == 0);
        }

    //pBPTree tree2 = bptNew();
    //ret = automatedTree(tree2, 1000);
    //assert(ret == 0);

    return iter;
    }