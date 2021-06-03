#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

// dummy free function
int bpt_dummy_freeFn(void* arg, void* ptr) {
    nmFree(ptr, sizeof(int));
    return 0;
}

long long
test(char** tname)
   	{
	*tname = "b+tree3_01 Test bptRemove function";
	
   int ret;
   int i;
   pBPTree this = bptNew();

   int *info1 = nmMalloc(sizeof(int));
   int *info2 = nmMalloc(sizeof(int));
   int *info3 = nmMalloc(sizeof(int));

   *info1 = 10;
   *info2 = 20;
   *info3 = 30;

   //I have this for loop for no other reason other than to avoid floating point errors. C's testing framework is really stupid
   for(i = 0; i < 200000; i++) {
        bptAdd(this, "Key1", 4, info1);
        bptAdd(this, "Key2", 4, info2);
        bptAdd(this, "Key3", 4, info3);
   }


   ret = bptRemove(this, "Key1", 4, bpt_dummy_freeFn, NULL);


   assert(ret == 0);

    return 10;
   	}



