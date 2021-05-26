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
	int val;

	*tname = "b+tree-11 bptDeInit works for leaf tree with 3 values";

	iter = 800000;
	for(i=0;i<iter;i++)
	 	{
		tree = bpt_i_new_BPNode();
		bptInit(tree);
		tree->Keys[0].Length = 2;
		tree->Keys[0].Value = nmSysMalloc(2); // don't assign double quoted string because deInit frees this
		tree->Keys[1].Length = 5;
		tree->Keys[1].Value = nmSysMalloc(5);
		tree->Keys[2].Length = 3;
		tree->Keys[2].Value = nmSysMalloc(3);
		tree->nKeys = 3;
		tree->IsLeaf = 1;

		val = bptDeInit(tree);
		assert (val == 0);
		assert (tree->Parent == NULL);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);		
		}
    return iter;
    }
