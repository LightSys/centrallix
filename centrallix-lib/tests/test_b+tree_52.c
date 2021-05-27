#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"


int bpt_dummy_freeFn(void* arg, void* ptr) {
    free(ptr);
    return 0;
}

long long
test(char** tname)
    {
    int i;
	int t;
    int iter;
	pBPTree tree = bptNew();
	char* key = malloc(2);
	int *val;


	*tname = "b+tree-52 bptSize updates when removing";

	assert (bptSize(tree) == 0);
	iter = 20000;
	for(i=0;i<iter;i++)
	    {	
		val = malloc(sizeof(int));
		*val = i;
		sprintf(key, "%d", *val);
		bptInsert(tree, key, 5, val);
		}

	assert(bptSize(tree)==20000);
	for(i = iter - 1; i >= 0; i--)
		{
		sprintf(key, "%d", i);
		t = bptRemove(tree, key, 5, bpt_dummy_freeFn, NULL);
		assert(t==0);
		assert (bptSize(tree) == i);
		}

    return iter;
    }



