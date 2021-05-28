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

    int x;
    int i;
    int y;

    *tname = "b+tree3_03 Test bptInit Function";

    /*
    pBPNode newNode; 
    
    newNode = nmMalloc(sizeof(BPNode));


    y = bptInit(newNode);

    assert(y == 0);
    */

   pBPTree newTree = bptNew();

   bptDeInit(newTree);

   y = bptInit(newTree);

   assert(y == 0);




    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }



    return 10;
   	}

