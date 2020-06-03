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
	*tname = "b+tree-27 test of bptLookup";
	iter = 800000;
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
	
	rval1 = (char*) bptLookup(root, "Zebra", 5);
	
	pBPTree this;
	this = bptBulkLoad("tests/bpt_bl_10e1.dat", 10);
	char* hold = (char*) this->Children[0].Child->Children[0].Ref;
	printf("MANUAL: %s\n", hold);
	bpt_PrintTreeSmall(this);
	printf("1\n");
	rval2 = (char*) bptLookup(this, "00000001", 8);
	printf("%s\n", rval2);
	//rval3 = (char*) bptLookup(this, "00000057", 8);
	//printf("%s\n", rval3);	

	for(i=0;i<iter;i++)
	 	{
//		assert (strcmp("REF VAL", rval1) == 0);
		assert (strcmp("A", rval2) == 0);
//		assert (strcmp("Abashing", rval3) == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

