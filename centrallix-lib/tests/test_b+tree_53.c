#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

int free_func(void* args, void* ref){
    nmSysFree(ref);
	return 0;
}

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree tree = bptNew();


	*tname = "b+tree-54 bptSize updates when deInit-ing";

	assert (bptSize(tree) == 0);
	iter = 200000;
	for(i=0;i<iter;i++)
	    {	
		pBPNode root = bpt_i_new_BPNode();
		pBPNode left = bpt_i_new_BPNode();
		pBPNode right = bpt_i_new_BPNode();  
        root->Keys[0].Length = 5;
        root->Keys[0].Value = nmSysMalloc(2); 
		root->nKeys++;	
		left->Keys[0].Length = 26;
		left->Keys[0].Value = nmSysMalloc(2);
		left->nKeys++;
		left->Children[0].Ref = NULL;
		left->Children[1].Ref = NULL;
		left->Keys[1].Length = 1;
		left->Keys[1].Value = nmSysMalloc(2);
		left->nKeys++;
		right->Keys[0].Length = 10;
		right->Keys[0].Value = nmSysMalloc(2);
		right->nKeys++;
		right->Keys[1].Length = 2;
		right->Keys[1].Value = nmSysMalloc(2);
		right->Children[0].Ref = NULL;
		right->Children[1].Ref = NULL;
		right->nKeys++;
		root->Children[0].Child = left;
		root->Children[1].Child = right;
		left->Next = right;
		root->IsLeaf = 0;
		tree->root = root;
		tree->size = 5;

		bptDeInit(tree, free_func, NULL);
		assert(bptSize(tree)==0);
		}
	

    return iter;
    }



