#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;

	*tname = "b+tree-06 bptNew returns a (non-NULL) pointer";

	iter = 800000;
	for(i=0;i<iter;i++)
	    {
		tree = NULL;
		tree = bptNew();
		assert (tree != NULL);
		}

    return iter;
    }

