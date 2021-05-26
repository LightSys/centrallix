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
	
	*tname = "b+tree-22 Removing Entry From Leaf";
	iter = 8000000;
	
	char* dat0 = "0";
	char* dat1 = "1";
	char* dat2 = "2";
	char* dat3 = "3";
	char* dat4 = "4";
	char* dat5 = "5";

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
	leftL->Keys[1].Length = 1;
        leftL->Keys[1].Value = "b\0";
        leftL->nKeys++;
	leftL->Keys[2].Length = 1;
        leftL->Keys[2].Value = "c\0";
        leftL->nKeys++;
	leftL->Keys[3].Length = 1;
        leftL->Keys[3].Value = "d\0";
        leftL->nKeys++;
	leftL->Children[0].Ref = (void*) dat0;
	leftL->Children[1].Ref = (void*) dat1;
	leftL->Children[2].Ref = (void*) dat2;
	leftL->Children[3].Ref = (void*) dat3;
	leftR->Keys[0].Length = 3;
	leftR->Keys[0].Value = "HEY\0";
	leftR->Children[0].Ref = (void*) dat4;
	leftR->nKeys++;
	rightMid->Keys[0].Length = 10;
	rightMid->Keys[0].Value = "0123456789\0";
	rightMid->nKeys++;
	rightL->Keys[0].Length = 2;
	rightL->Keys[0].Value = "!?\0";
	rightL->Children[0].Ref = (void*) dat5;
	rightL->nKeys++;
	rightR->Keys[0].Length = 3;
	rightR->Keys[0].Value = "*&#\0";
	rightR->Children[0].Ref = (void*) dat5;
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
	leftMid->Parent = root;
	rightMid->Parent = root;
	leftL->Parent = leftMid;
	leftR->Parent = leftMid;
	rightL->Parent = rightMid;
	rightR->Parent = rightMid;
	
	bpt_PrintTreeSmall(root);
	bpt_i_RemoveEntryFromNode(leftL, "b", 1, (pBPNodeVal) dat1);
	bpt_PrintTreeSmall(root);
	
	for(i=0;i<iter;i++)
	 	{
		assert (strcmp((char*)leftL->Children[0].Ref, "0") == 0);
		assert (strcmp((char*)leftL->Children[1].Ref, "2") == 0);
		assert (strcmp((char*)leftL->Children[2].Ref, "3") == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

