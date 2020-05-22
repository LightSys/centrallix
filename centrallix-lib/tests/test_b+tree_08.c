#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree;
	int val;

	*tname = "b+tree-08 bptInit returns 0, clears ptrs, resets vars";

	iter = 800000;
	for(i=0;i<iter;i++)
	 	{
		tree = bptNew();
		tree->Parent = tree;
		tree->Next = tree;
		tree->Prev = tree;
		tree->nKeys = 4;
		tree->IsLeaf = 0;
		val = bptInit(tree);
		assert (val == 0);
		assert (tree->Parent == NULL);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);
		assert (tree->IsLeaf == 1);
		}
    return iter;
    }

