#include <stdio.h>
#include <assert.h>
#include "b+tree.h"


long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;


	*tname = "b+tree-54 bptIsEmpty returns true after creation of tree";

	iter = 20000;
	for(i=0;i<iter;i++)
	    {
		tree = bptNew();
		
		assert (bptIsEmpty(tree));
		}

    return iter;
    }



