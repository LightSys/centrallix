#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int i;
    	int iter;
	int tmp;

	*tname = "b+tree-04 2 Children";
	iter = 8000000;
	
	BPNode* root = bpt_i_new_BPNode();
	BPNode* left = bpt_i_new_BPNode();
	BPNode* right = bpt_i_new_BPNode();  
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Tommy\0"; 
	root->nKeys++;	
	left->Keys[0].Length = 26;
	left->Keys[0].Value = "abcdefghijklmnopqrstuvwxyz\0";
	left->nKeys++;
	left->Keys[1].Length = 1;
	left->Keys[1].Value = "a\0";
	left->nKeys++;
	right->Keys[0].Length = 10;
	right->Keys[0].Value = "0123456789\0";
	right->nKeys++;
	right->Keys[1].Length = 2;
	right->Keys[1].Value = "!?\0";
	right->nKeys++;
	root->Children[0].Child = left;
	root->Children[1].Child = right;
	left->Next = right;
	root->IsLeaf = 0;
	
	tmp = 0;
	for(i=0;i<iter;i++)
	 	{
		assert (tmp == 0);
		}

	
    	return iter*4;
    	}

