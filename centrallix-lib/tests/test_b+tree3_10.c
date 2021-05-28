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

    

	*tname = "b+tree3_10 Test bptDeInit function";
    pBPTree tree = bptNew();

    y = bptDeInit(tree);

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

