#include <stdio.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc_memfail.h"


long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;


	*tname = "b+tree-50 bptSize returns 0 after creation of tree";

	iter = 20000;
	for(i=0;i<iter;i++)
	    {
		tree = bptNew();
		
		assert (bptSize(tree) == 0);
		}

    return iter;
    }



