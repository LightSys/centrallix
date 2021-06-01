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
    *tname = "b+tree3_15 Test bptDeInit Function at a lot of levels, as well as testing adding in a lot of nodes to the tree";
    pBPTree tree = bptNew();
    int y;
    int as;

    int i;
    int size = 1000;

    int* info[size];

    for(i = 0; i < size; i++) {
        info[i] = nmMalloc(sizeof(int));
        *info[i] = i + 10;
        char k[12];
        sprintf(k, "%d", i);
        as = bptAdd(tree, k, strlen(k), info[i]);
        assert(as == 0);
    }

    //There has been an issue with bptDeInit. It seems to originate in bpt_i_clear, based on the error message
    y = bptDeInit(tree, free_func, NULL);

    printf("%d\n", y);

    assert(y == 0);

    /*
    //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }
*/


    return 10;
   	}

