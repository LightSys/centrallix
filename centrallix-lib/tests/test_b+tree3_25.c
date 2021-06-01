#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    return 0;
}

long long
test(char** tname)
   	{
    
    *tname = "b+tree3_25 Make Sure that You Cannot Put in a NULL";

    int y;
        pBPTree tree1 = bptNew();

        int info1[10];
        int info2[10];
        int info3[10];

        y = bptAdd(tree1, "0001", 4, NULL);
        assert(y == -1);
        y = bptAdd(NULL, "0002", 4, info1);
        assert(y == -1);
        bptAdd(tree1, NULL, 4, info2);
        assert(y == -1);
        bptAdd(tree1, "0003", NULL, info3);
        assert(y == -1);



     //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }

    return x;
   	}