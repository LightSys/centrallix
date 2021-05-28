#include <stdio.h>
#include <assert.h>
#include "b+tree.h"


long long
test(char** tname)
    {
    int i;
	int t;
    int iter;
	pBPTree tree = bptNew();
	char key[] = "-1\0";


	*tname = "b+tree-55 bptIsEmpty updates when inserting";

	assert(bptIsEmpty(tree));
	iter = 20000;
	for(i=0;i<iter;i++)
	    {	
		sprintf(key, "%d", i);
		t = bptAdd(tree, key, 5, &i);
		assert(t==0);
		assert(!bptIsEmpty(tree));
		}

    return iter;
    }



