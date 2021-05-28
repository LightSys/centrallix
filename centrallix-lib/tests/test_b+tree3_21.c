#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    free(ref);
    return 0;
}

long long
test(char** tname)
   	{
    
    *tname = "b+tree3_21 Test bptDeInit Function when allocating values with malloc";

    int i;
    int iter = 20000;
    for (i = 0; i < iter; i++) {
        pBPTree tree1 = bptNew();
    
        int *info1 = malloc(sizeof(int));
        int *info2 = malloc(sizeof(int));
        int *info3 = malloc(sizeof(int));
        int *info4 = malloc(sizeof(int));
        int *info5 = malloc(sizeof(int));

        bptAdd(tree1, "0001", 4, info1);
        bptAdd(tree1, "0002", 4, info2);
        bptAdd(tree1, "0003", 4, info3);
        bptAdd(tree1, "0004", 4, info4);
        bptAdd(tree1, "0005", 4, info5);

        int r = bptDeInit(tree1, free_func, NULL);
        assert(r==0);
    }

    return iter;
   	}