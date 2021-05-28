#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

// dummy free function
int bpt_dummy_freeFn(void* arg, void* ptr) {
    free(ptr);
    return 0;
}

long long
test(char** tname)
   	{
	*tname = "b+tree3_13 Size Updates as you Remove and Add";
	
   int y;
   pBPTree this = bptNew();

   int *info1 = malloc(sizeof(int));
   int *info2 = malloc(sizeof(int));
   int *info3 = malloc(sizeof(int));
   int *info4 = malloc(sizeof(int));

   *info1 = 10;
   *info2 = 20;
   *info3 = 30;
   *info4 = 40;

    bptAdd(this, "Key1", 4, info1);
    bptAdd(this, "Key2", 4, info2);
    bptAdd(this, "Key3", 4, info3);
    
    y = bptRemove(this, "Key1", 4, bpt_dummy_freeFn, NULL);
    assert(y == 0);

    y = bptSize(this);
    assert(y == 2);

    y = bptAdd(this, "Key4", 4, info4);
    assert(y == 0);

     y = bptSize(this);
    assert(y == 3);


    y = bptRemove(this, "Key2", 4, bpt_dummy_freeFn, NULL);
    assert(y == 0);

    y = bptSize(this);
    assert(y == 2);






    //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }

    return 10;
   	}



