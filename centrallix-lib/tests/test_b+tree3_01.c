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


//This shows the tree printed out initially
   printTree(this->root, 2);
   printf("\n");

   ret = bptRemove(this, "Key1", 4, bpt_dummy_freeFn, NULL);

//This shows the tree printed out after the leaf has been removed
   printTree(this->root, 2);
   printf("\n");

//I added this in to debug, so I can see whether or 
   printf("%d", ret);
   printf("\n");

   assert(ret == 0);

    return 10;
   	}



