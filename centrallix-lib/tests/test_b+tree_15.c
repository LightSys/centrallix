#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
    {
    int i; 
    int iter;
	pBPNode tree;

	*tname = "b+tree-15 bptFree deinits leaf tree with 3 values";

	iter = 800000;
	for(i=0;i<iter;i++)
	 	{
		tree = bpt_i_new_BPNode();
		bptInit_I_Node(tree);
		tree->Keys[0].Length = 2;
		tree->Keys[0].Value = nmSysMalloc(2); // don't assign double quoted string because deInit frees this
		tree->Keys[1].Length = 5;
		tree->Keys[1].Value = nmSysMalloc(5);
		tree->Keys[2].Length = 3;
		tree->Keys[2].Value = nmSysMalloc(3);
		tree->nKeys = 3;
		tree->IsLeaf = 1;

		bpt_I_FreeNode(tree);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);		
		}
    return iter;
    }