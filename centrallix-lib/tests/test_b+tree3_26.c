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
	*tname = "b+tree3_26 Test that bptRemove Cannot Remove a Value that does not Exist";
	
   int ret;
   pBPTree this = bptNew();

   int *info1 = nmMalloc(sizeof(int));
   int *info2 = nmMalloc(sizeof(int));
   int *info3 = nmMalloc(sizeof(int));

   *info1 = 10;
   *info2 = 20;
   *info3 = 30;

    bptAdd(this, "Key1", 4, info1);
    bptAdd(this, "Key2", 4, info2);
    bptAdd(this, "Key3", 4, info3);
    ret = bptRemove(this, "Key9", 4, bpt_dummy_freeFn, NULL);


   assert(ret == -1);

     //This next part is just to avoid a floating point error
    int i;
    int x;
    x = 1;
    for (i = 0; i < 10000000; i++) {
        x++;
    }

    return x;

   

   	}



