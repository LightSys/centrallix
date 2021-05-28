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


	*tname = "b+tree-51 bptSize updates when inserting";

	assert (bptSize(tree) == 0);
	iter = 20000;
	for(i=0;i<iter;i++)
	    {	
		sprintf(key, "%d", i);
		t = bptAdd(tree, key, 5, &i);
		assert(t==0);
		assert(bptSize(tree) == i+1);
		}

    return iter;
    }



