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
    
    int y;


    *tname = "b+tree3_11 Test bptDeInit Function at 2 levels";
    pBPTree tree = bptNew();

    

    int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

    int *info2 = nmMalloc(sizeof(int));
    *info1 = 20;

    int *info3 = nmMalloc(sizeof(int));
    *info1 = 30;

    bptAdd(tree, "0001", 4, info1);
    bptAdd(tree, "0002", 4, info2);
    bptAdd(tree, "0003", 4, info3);

    y = bptDeInit(tree, free_func, NULL);

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

