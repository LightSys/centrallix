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
    int i, j, ret, count, err;
    int iter;
	pBPTree tree = bptNew();
	char* key;
	int len = 10;

	pBPIter tree_iter;

	*tname = "b+tree-58 add and remove many items from the tree";

	assert (bptIsEmpty(tree));
	iter = 2000;
	for(i=0;i<iter;i++)
	    {
		for(j = 0; j < 100; j++)
			{
			key = nmSysMalloc(len + 1);
			int* data = nmMalloc(sizeof(int));
			*data = j;
			sprintf(key, "%010d", j);
			ret = bptAdd(tree, key, len, data);
			nmSysFree(key);
			assert(ret == 0);

			count = 0;
			err = 0;
			tree_iter = bptFront(tree);
			while(!err)
				{
				bptNext(tree_iter, &err);
				count++;
				}
				assert(count == j + 1);
			}
		for(j = 0; j < 100; j++)
			{
			key = nmSysMalloc(len + 1);
			sprintf(key, "%010d", j);
			ret = bptRemove(tree, key, len, free_func, NULL);
			
			nmSysFree(key);
			assert(ret == 0);
			
			count = 0;
			err = 0;
			tree_iter = bptFront(tree);
			while(tree_iter && !err)
				{
				bptNext(tree_iter, &err);
				count++;
				}
				assert(count == 100 - j - 1);
			}
		}
    return iter * 2000;
    }



