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
	pBPNode tree, left, right;

	*tname = "b+tree-16 bptFree runs deinit on for 2-level tree";

	iter = 80000;
	for(i=0;i<iter;i++)
	 	{
		tree = bpt_i_new_BPNode();
		left = bpt_i_new_BPNode();
		right = bpt_i_new_BPNode();
		bptInit(tree);
		bptInit(left);
		bptInit(right);

		tree->nKeys = 1;
		tree->IsLeaf = 0;
		tree->Keys[0].Length = 2;
		tree->Keys[0].Value = nmSysMalloc(2); // don't assign double quoted string because deInit frees this
		tree->Children[0].Child = left;
		tree->Children[1].Child = right;

		left->nKeys = 2;
		left->IsLeaf = 1;
		left->Next = right;
		left->Keys[0].Length = 5;
		left->Keys[0].Value = nmSysMalloc(5);
		left->Keys[1].Length = 4;
		left->Keys[1].Value = nmSysMalloc(4);

		right->nKeys = 2;
		right->IsLeaf = 1;
		right->Prev = left;
		right->Keys[0].Length = 7;
		right->Keys[0].Value = nmSysMalloc(7);
		right->Keys[1].Length = 2;
		right->Keys[1].Value = nmSysMalloc(2);

		bptFreeNode(tree);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);		
		assert (left->Next == NULL);
		assert (left->Prev == NULL);
		assert (left->nKeys == 0);
		assert (right->Next == NULL);
		assert (right->Prev == NULL); //issue
		assert (right->nKeys == 0);	
		}
    return iter;
    }

