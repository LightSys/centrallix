#include <stdio.h>
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
        int i, ret;
        int iter;
        pBPTree tree = bptNew();

        *tname = "b+tree_56 Testing bptDeInit with the new automatedTree function";

        assert (bptIsEmpty(tree));
        iter = 2000;
        ret = automatedTree(tree, 100);
        assert (ret == 0);
        assert(bptSize(tree) == 100);

        ret = bptDeInit(tree, free_func, NULL);
        assert(ret == 0);
        assert(bptSize(tree) == 0);


        int x = 1;
        for(i=0;i<100001100;i++)
            x++;

        return iter * 2000;
    }



