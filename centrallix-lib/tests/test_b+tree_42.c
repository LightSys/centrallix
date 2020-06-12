#include <stdio.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc_memfail.h"

int fail_malloc;

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;


	*tname = "b+tree-42 bptNew returns NULL on nmMalloc fail";

	iter = 8000000;
	for(i=0;i<iter;i++)
	    {
		tree = (pBPTree)(1);
		fail_malloc = 1;
		tree = bptNew();
		fail_malloc = 0;
		assert (tree == NULL);
		}

    return iter;
    }



