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

	*tname = "b+tree-05 3 Levels";
	iter = 8000000;
	
	pBPNode root = bpt_i_new_BPNode();
	pBPNode leftMid = bpt_i_new_BPNode();
	pBPNode rightMid = bpt_i_new_BPNode();
	pBPNode leftL = bpt_i_new_BPNode();
	pBPNode leftR = bpt_i_new_BPNode();
	pBPNode rightL = bpt_i_new_BPNode();
	pBPNode rightR = bpt_i_new_BPNode();  
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Tommy\0"; 
	root->nKeys++;	
	leftMid->Keys[0].Length = 26;
	leftMid->Keys[0].Value = "abcdefghijklmnopqrstuvwxyz\0";
	leftMid->nKeys++;
	leftL->Keys[0].Length = 1;
	leftL->Keys[0].Value = "a\0";
	leftL->nKeys++;
	leftR->Keys[0].Length = 3;
	leftR->Keys[0].Value = "HEY\0";
	leftR->nKeys++;
	rightMid->Keys[0].Length = 10;
	rightMid->Keys[0].Value = "0123456789\0";
	rightMid->nKeys++;
	rightL->Keys[0].Length = 2;
	rightL->Keys[0].Value = "!?\0";
	rightL->nKeys++;
	rightR->Keys[0].Length = 3;
	rightR->Keys[0].Value = "*&#\0";
	rightR->nKeys++;
	root->Children[0].Child = leftMid;
	root->Children[1].Child = rightMid;
	leftMid->Children[0].Child = leftL;
	leftMid->Children[1].Child = leftR;
	rightMid->Children[0].Child = rightL;
	rightMid->Children[1].Child = rightR;
	root->IsLeaf = 0;
	leftMid->IsLeaf = 0;
	rightMid->IsLeaf = 0;


	tmp = 0;
	printTree(&root, 3);
	for(i=0;i<iter;i++)
	 	{
		assert (tmp == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

