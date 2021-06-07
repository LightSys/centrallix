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
    int i, j, ret, err;
    int iter;
	pBPTree tree = bptNew();
	char *key;
	int len = 10;
	int *val, *prev_val;

	pBPIter tree_iter;

	*tname = "b+tree3_58 test leaf linked list is in order (Taken from original test 59)";

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
			assert(ret == 0);

			prev_val = nmMalloc(sizeof(int));
			*prev_val = -1;
			err = 0;
			tree_iter = bptFront(tree);
			while(!err)
				{
				val = tree_iter->Ref;
				assert(*prev_val < *val);
				prev_val = val;
				bptNext(tree_iter, &err);
				}
			}
		for(j = 0; j < 100; j++)
			{
			key = nmSysMalloc(len + 1);
			sprintf(key, "%010d", j);
			ret = bptRemove(tree, key, len, free_func, NULL);
			
			assert(ret == 0);
			
			prev_val = nmMalloc(sizeof(int));
			*prev_val = -1;
			err = 0;
			tree_iter = bptFront(tree);
			while(tree_iter && !err)
				{
				val = tree_iter->Ref;
				assert(*prev_val < *val);
				prev_val = val;
				bptNext(tree_iter, &err);
				}
			}
		}
    return iter * 2000;
    }




