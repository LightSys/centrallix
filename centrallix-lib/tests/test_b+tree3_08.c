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
    
    int *y;


    *tname = "b+tree3_08 Test bptLookup Function";
    pBPTree tree = bptNew();

    int *info1 = malloc(sizeof(int));
    *info1 = 10;

    int *info2 = malloc(sizeof(int));
    *info1 = 20;

    int *info3 = malloc(sizeof(int));
    *info1 = 30;

    int *info4 = malloc(sizeof(int));
    *info1 = 40;

    int *info5 = malloc(sizeof(int));
    *info1 = 50;

    bptAdd(tree, "0001", 4, info1);
    bptAdd(tree, "0002", 4, info2);
    bptAdd(tree, "0003", 4, info3);
    bptAdd(tree, "0004", 4, info4);
    bptAdd(tree, "0005", 4, info5);

    y = bptLookup(tree, "0001", 4);
    assert(y == info1);

    y = bptLookup(tree, "0002", 4);
    assert(y == info2);

    y = bptLookup(tree, "0003", 4);
    assert(y == info3);

    y = bptLookup(tree, "0004", 4);
    assert(y == info4);

    y = bptLookup(tree, "0005", 4);
    assert(y == info5);

    
    //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }



    return 10;
   	}

