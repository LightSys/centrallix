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

    

	*tname = "b+tree3_10 Test bptDeInit function on an empty tree";
    pBPTree tree = bptNew();

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