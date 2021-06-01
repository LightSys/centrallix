#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
    
    int y;
    int k;


    *tname = "b+tree3_07 Test bptIsEmpty Function";
    pBPTree tree1 = bptNew();
    pBPTree tree2 = bptNew();

    int *info1 = nmMalloc(sizeof(int));
    *info1 = 10;

    int *info2 = nmMalloc(sizeof(int));
    *info1 = 20;

    int *info3 = nmMalloc(sizeof(int));
    *info1 = 30;

    int *info4 = nmMalloc(sizeof(int));
    *info1 = 40;

    int *info5 = nmMalloc(sizeof(int));
    *info1 = 50;

    bptAdd(tree2, "0001", 4, info1);
    bptAdd(tree2, "0002", 4, info2);
    bptAdd(tree2, "0003", 4, info3);
    bptAdd(tree2, "0004", 4, info4);
    bptAdd(tree2, "0005", 4, info5);

    y = bptIsEmpty(tree1); //Testing to make sure it identifies an empty tree as empty
    k = bptIsEmpty(tree2); //Testing to make sure it identifies a not empty tree as not empty

    assert(y == 1);

    assert(k != 1);


    //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }


    return 10;
   	}