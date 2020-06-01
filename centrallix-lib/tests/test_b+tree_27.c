#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int i, iter;
	char* rval;
		
	*tname = "b+tree-27 test of bptLookup";
	iter = 8000000;
	printf("\n");

	pBPTree root = bptNew();
	pBPTree left = bptNew();
	pBPTree mid = bptNew();
	pBPTree right = bptNew();  
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
	left->Parent = root;
	root->Children[1].Child = mid;
        mid->Parent = root;
	root->Children[2].Child = right;
	right->Parent = root;
	left->Next = right;
	root->IsLeaf = 0;
	
	rval = (char*) bptLookup(root, "Zebra", 5);
	

	for(i=0;i<iter;i++)
	 	{
		assert (strcmp("REF VAL", rval) == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

