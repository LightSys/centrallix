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
	pBPTree this;

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
	
	this = bptBulkLoad("tests/bpt_bl_10e2.dat", 100);

	for (i=0; i<iter; i++)
		{
		rval1 = (char*) bptLookup(root, "Zebra", 5);
		rval2 = (char*) bptLookup(this, "00000001", 8);
		//rval3 = (char*) bptLookup(this, "00000057", 8);
		rval4 =	(char*) bptLookup(this, "00000100", 8);
		rval5 = (char*) bptLookup(this, "00000000", 8);
		rval6 = (char*) bptLookup(this, "00000037", 8);
		rval7 = (char*) bptLookup(this, "00000072", 8);
		assert (strcmp("REF VAL", rval1) == 0);
		assert (strcmp("A", rval2) == 0);
//		assert (strcmp("Abashing", rval3) == 0);
		assert (strcmp("ABC", rval4) == 0);
		assert (rval5 == NULL);
		assert (strcmp("Abandoned", rval6) == 0);
		assert (strcmp("Abattoir", rval7) == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}
