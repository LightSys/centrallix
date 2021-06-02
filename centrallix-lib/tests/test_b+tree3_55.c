#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmFree(ref, sizeof(int));
	return 0;
}

long long
test(char** tname)
    {
    int i, j, ret;
    int iter;
	pBPTree tree = bptNew();
	char* key;
	int len = 10;
	int innerIter;

	*tname = "b+tree3_55 add and remove many items from the tree";

	assert (bptIsEmpty(tree));
	iter = 2000;
	innerIter = 100;
    for(j = 0; j < innerIter; j++)
			{
			key = nmSysMalloc(len + 1);
			int* data = nmMalloc(sizeof(int));
			*data = j;
			sprintf(key, "%03d", j);
			ret = bptAdd(tree, key, len, data);
			nmSysFree(key);
			assert(ret == 0);
			}

            assert(bptSize(tree) == innerIter);


    int x = 1;
	for(i=0;i<100000000;i++)
	    {
        x++;
		
		}
    return iter * 2000;
    }



