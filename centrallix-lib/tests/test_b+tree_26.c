#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int i, iter, tmp, idx;
	pBPTree locate;
	
	*tname = "b+tree-26 full test of bpt_i_Find";
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
	right->nKeys++;
	root->Children[0].Child = left;
	left->Parent = root;
	root->Children[1].Child = mid;
        mid->Parent = root;
	root->Children[2].Child = right;
	right->Parent = root;
	left->Next = right;
	root->IsLeaf = 0;
	
	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(root, "Az", 2, locate, &idx);
		//assert (locate == left);
		assert (tmp == 0);
		assert(idx == 1);
		tmp = bpt_i_Find(root, "Do", 2, locate, &idx);
                //assert (locate == left);
                assert (tmp == 0);
                assert(idx == 3);
		tmp = bpt_i_Find(root, "Happier", 7, locate, &idx);
                //assert (locate == mid);
                assert (tmp == 0);
                assert(idx == 0);
		tmp = bpt_i_Find(root, "Zebra", 5, locate, &idx);
                //assert (locate == right);
                assert (tmp == 0);
                assert(idx == 1);
		tmp = bpt_i_Find(root, "Zebras", 6, locate, &idx);
                //assert (locate == right);
                assert (tmp == -1);
                assert(idx == 2);
		tmp = bpt_i_Find(root, "AAAAAAA", 7, locate, &idx);
                //assert (locate == left);
                assert (tmp == -1);
                assert(idx == 0);
		tmp = bpt_i_Find(root, "Heat", 4, locate, &idx);
                //assert (locate == mid);
                assert (tmp == -1);
                assert(idx == 1);
		tmp = bpt_i_Find(root, "Bad", 3, locate, &idx);
                //assert (locate == mid);
                assert (tmp == -1);
		assert(idx == 2);
		}

	printf("\n");
	
    	return iter*4;
    	}

