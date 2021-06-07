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

    int x;
    int i;
    int y;

    *tname = "b+tree_03 Test bptInit Function";

   int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

   pBPTree newTree = bptNew();

   bptDeInit(newTree, free_func, NULL);

   y = bptInit(newTree);

   assert(y == 0);

   y = bptAdd(newTree, "001", 3, info1); //This is to make sure that the bptInit worked

   assert(y == 0);




    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }



    return 10;
   	}

