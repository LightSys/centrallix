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

	*tname = "b+tree-23 REMOVE RIGHT ENTRY FROM ROOT";
	iter = 8000000;
	
	pBPTree root = bptNew();
	pBPTree left = bptNew();
	pBPTree mid = bptNew();
	pBPTree right = bptNew();  
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Tommy\0"; 
	root->nKeys++;	
	root->Keys[1].Length = 8;
        root->Keys[1].Value = "Hot dogs\0";
        root->nKeys++;
	left->Keys[0].Length = 26;
	left->Keys[0].Value = "abcdefghijklmnopqrstuvwxyz\0";
	left->nKeys++;
	left->Keys[1].Length = 1;
	left->Keys[1].Value = "a\0";
	left->nKeys++;
	mid->Keys[0].Length = 7;
	mid->Keys[0].Value = "ALABAMA\0";
	mid->nKeys++;
	right->Keys[0].Length = 10;
	right->Keys[0].Value = "0123456789\0";
	right->nKeys++;
	right->Keys[1].Length = 2;
	right->Keys[1].Value = "!?\0";
	right->nKeys++;
	root->Children[0].Child = left;
	left->Parent = root;
	root->Children[1].Child = mid;
        mid->Parent = root;
	root->Children[2].Child = right;
	right->Parent = root;
	left->Next = right;
	root->IsLeaf = 0;
	char * msg = "Hot dogs";	
	bpt_PrintTree(&root);
	bpt_i_RemoveEntryFromNode(root, msg, 8, mid);
	bpt_PrintTree(&root);
	for(i=0;i<iter;i++)
	 	{
		assert (root->Keys[0].Length == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}

