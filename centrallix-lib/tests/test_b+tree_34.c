#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int i;
    	int iter;
	pBPNode root, left, rootNull, rootLeaf, rootToAdjust, leftToAdjust;


	*tname = "b+tree-34 bpt_i_AdjustRoot";
	iter = 80000;
	

	for (i=0; i<iter; i++)
		{
		root = bpt_i_new_BPNode();
		left = bpt_i_new_BPNode(); 
	        root->Keys[0].Length = 5;
       		root->Keys[0].Value = "00005\0"; 
		root->nKeys++;	
		left->Keys[0].Length = 5;
		left->Keys[0].Value = "00000\0";
		left->nKeys++;
		left->Keys[1].Length = 5;
		left->Keys[1].Value = "00003\0";
		left->nKeys++;
		root->Children[0].Child = left;
		left->Parent = root;
		root->IsLeaf = 0;
		root = bpt_i_AdjustRoot(root);	
		assert (strcmp(root->Keys[0].Value, "00005\0") == 0);
		assert (strcmp(root->Children[0].Child->Keys[0].Value, "00000\0") == 0);
		assert (strcmp(root->Children[0].Child->Keys[1].Value, "00003\0") == 0);
		
		rootNull = NULL;
		rootNull = bpt_i_AdjustRoot(rootNull);
		assert (rootNull == NULL);

		rootLeaf = bpt_i_new_BPNode();
		rootLeaf = bpt_i_AdjustRoot(rootLeaf);
		assert (rootLeaf == NULL);		
		
		rootToAdjust = bpt_i_new_BPNode();
		leftToAdjust = bpt_i_new_BPNode();
		leftToAdjust->Keys[0].Length = 5; 
		leftToAdjust->Keys[0].Value = "00000\0";
                leftToAdjust->nKeys++;
		leftToAdjust->Parent = rootToAdjust;
		rootToAdjust->IsLeaf = 0;
		rootToAdjust->Children[0].Child = leftToAdjust;
		rootToAdjust = bpt_i_AdjustRoot(rootToAdjust);
                assert (rootToAdjust->Parent == NULL);
		assert (strcmp(rootToAdjust->Keys[0].Value, "00000\0") == 0);
		}

    	return iter*4;
    	}

