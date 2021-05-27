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
	pBPNode tree, left, right, ll, lr, rl, rr;
	int val;

	*tname = "b+tree-13 bptDeInit works for 3-level tree";

	iter = 80000;
	for(i=0;i<iter;i++)
	 	{
		tree = bpt_i_new_BPNode();
		left = bpt_i_new_BPNode();
		right = bpt_i_new_BPNode();
		ll = bpt_i_new_BPNode();
		lr = bpt_i_new_BPNode();
		rl = bpt_i_new_BPNode();
		rr = bpt_i_new_BPNode();

		bptInit(tree);
		bptInit(left);
		bptInit(right);
		bptInit(ll);
		bptInit(lr);
		bptInit(rl);
		bptInit(rr);

		tree->nKeys = 1;
		tree->IsLeaf = 0;
		tree->Keys[0].Length = 2;
		tree->Keys[0].Value = nmSysMalloc(2); // don't assign double quoted string because deInit frees this
		tree->Children[0].Child = left;
		tree->Children[1].Child = right;

		left->nKeys = 1;
		left->IsLeaf = 0;
		left->Next = right;
		left->Children[0].Child = ll;
		left->Children[1].Child = lr;
		left->Keys[0].Length = 5;
		left->Keys[0].Value = nmSysMalloc(5);

		right->nKeys = 1;
		right->IsLeaf = 0;
		right->Prev = left;
		right->Children[0].Child = rl;
		right->Children[1].Child = rr;
		right->Keys[0].Length = 7;
		right->Keys[0].Value = nmSysMalloc(7);

		ll->nKeys = 2;
		ll->IsLeaf = 1;
		ll->Next = lr;
		ll->Keys[0].Length = 3;
		ll->Keys[0].Value = nmSysMalloc(3);
		ll->Keys[1].Length = 2;
		ll->Keys[1].Value = nmSysMalloc(2);

		lr->nKeys = 2;
		lr->IsLeaf = 1;
		lr->Prev = ll;
		lr->Keys[0].Length = 3;
		lr->Keys[0].Value = nmSysMalloc(3);
		lr->Keys[1].Length = 2;
		lr->Keys[1].Value = nmSysMalloc(2);
		
		rl->nKeys = 2;
		rl->IsLeaf = 1;
		rl->Next = rr;
		rl->Keys[0].Length = 3;
		rl->Keys[0].Value = nmSysMalloc(3);
		rl->Keys[1].Length = 2;
		rl->Keys[1].Value = nmSysMalloc(2);

		rl->nKeys = 2;
		rl->IsLeaf = 1;
		rl->Prev = rl;
		rl->Keys[0].Length = 3;
		rl->Keys[0].Value = nmSysMalloc(3);
		rl->Keys[1].Length = 2;
		rl->Keys[1].Value = nmSysMalloc(2);

		val = bptDeInitNode(tree);
		assert (val == 0);
		assert (tree->Next == NULL);
		assert (tree->Prev == NULL);
		assert (tree->nKeys == 0);	

		assert (left->Next == NULL);
		assert (left->Prev == NULL);
		assert (left->nKeys == 0);	
		assert (right->Next == NULL);
		assert (right->Prev == NULL);
		assert (right->nKeys == 0);	

		assert (ll->Next == NULL);
		assert (ll->Prev == NULL);
		assert (ll->nKeys == 0);
		assert (lr->Next == NULL);
		assert (lr->Prev == NULL);
		assert (lr->nKeys == 0);
		assert (rl->Next == NULL);
		assert (rl->Prev == NULL);
		assert (rl->nKeys == 0);
		assert (rr->Next == NULL);
		assert (rr->Prev == NULL);
		assert (rr->nKeys == 0);
		}
    return iter;
    }


