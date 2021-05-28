#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPNode tree;
	int val;

	*tname = "b+tree-08 bptInit_I_Node returns 0, clears ptrs, resets vars";

	iter = 800000;
	for(i=0;i<iter;i++)
	 	{
		tree = bpt_i_new_BPNode();
		tree->Next = tree;
		tree->Prev = tree;
		tree->nKeys = 4;
		tree->IsLeaf = 0;
		val = bptInit_I_Node(tree);
		assert (val == 0);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);
		assert (tree->IsLeaf == 1);
		}
    return iter;
    }

