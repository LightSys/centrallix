#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int i, iter;
	char* rval1;
	char* rval2;
	char* rval3;
	char* rval4;
	char* rval5;
	char* rval6;
	char* rval7;
	pBPNode this;

	pBPNode rnode1;

	*tname = "b+tree-27 test of bptSearch";
	iter = 800000;
	printf("\n");

	pBPNode root = bpt_i_new_BPNode();
	pBPNode left = bpt_i_new_BPNode();
	pBPNode mid = bpt_i_new_BPNode();
	pBPNode right = bpt_i_new_BPNode();  
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Green\0"; 
	root->nKeys++;	
	root->Keys[1].Length = 8;
        root->Keys[1].Value = "Hot dogs\0";
        root->nKeys++;
	left->Keys[0].Length = 26;
	left->Keys[0].Value = "Abcdefghijklmnopqrstuvwxyz\0";
	left->nKeys++;
	left->Keys[1].Length = 2;
	left->Keys[1].Value = "Az\0";
	left->nKeys++;
	left->Keys[2].Length = 7;
        left->Keys[2].Value = "Bananas\0";
	left->nKeys++;
        left->Keys[3].Length = 2; 
        left->Keys[3].Value = "Do\0";         
	left->nKeys++;
	mid->Keys[0].Length = 7;
	mid->Keys[0].Value = "Happier\0";
	mid->nKeys++;
	right->Keys[0].Length = 10;
	right->Keys[0].Value = "Watermelon\0";
	right->nKeys++;
	right->Keys[1].Length = 5;
	right->Keys[1].Value = "Zebra\0";
	right->Children[1].Ref = (void*) "REF VAL\0";
	right->nKeys++;
	root->Children[0].Child = left;
	root->Children[1].Child = mid;
	root->Children[2].Child = right;
	left->Next = right;
	root->IsLeaf = 0;
	
	for (i=0; i<iter; i++)
		{
		rnode1 = bptLookup(root, "Zebra", 5);
		assert(rnode1 != NULL);
		int i = 0;
		while(strcmp(rnode1->Keys[i].Value, "Zebra") != 0){
			i++;
		}
		rval1 = rnode1->Children[i].Ref;
		assert (strcmp("REF VAL\0", rval1) == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

