#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
    return 0;
}

long long
test(char** tname)
   	{
    
    *tname = "b+tree3_42 Testing bptIterFree on a back iterator";
    pBPTree tree = bptNew();

    int status = 0;
    int y;

    

    int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

    int *info2 = nmMalloc(sizeof(int));
    *info2 = 20;

    int *info3 = nmMalloc(sizeof(int));
    *info3 = 30;

    int *info4 = nmMalloc(sizeof(int));
    *info4 = 40;

    int *info5 = nmMalloc(sizeof(int));
    *info5 = 50;

    int *info6 = nmMalloc(sizeof(int));
    *info6 = 60;

    bptAdd(tree, "0001", 4, info1);
    bptAdd(tree, "0002", 4, info2);
    bptAdd(tree, "0003", 4, info3);
    bptAdd(tree, "0004", 4, info4);
    bptAdd(tree, "0005", 4, info5);
    bptAdd(tree, "0006", 4, info6);

    pBPIter iterator = bptBack(tree);

    y = bptIterFree(iterator);

    assert(y == 0);

    
    //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }



    return 10;
   	}

