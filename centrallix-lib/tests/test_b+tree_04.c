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
	
	pBPTree root = bptNew();
	pBPTree left = bptNew();
	pBPTree right = bptNew();  
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
	left->Parent = root;
	root->Children[1].Child = right;
	right->Parent = root;
	left->Next = right;
	root->IsLeaf = 0;
	
	tmp = 0;
	bpt_PrintTree(&root);
	for(i=0;i<iter;i++)
	 	{
		assert (tmp == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}
