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

	*tname = "b+tree-02 1 Key";
	iter = 8000000;
	
	pBPNode root = bpt_i_new_BPNode();
	//printf("A\n");
	
	/*void* copy;
        copy = nmSysMalloc(5);
        if (!copy) 
		return -1; 
	
	memcpy(copy, "Tommy", 5);
          */      
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Tommy\0"; 
	root->nKeys++;	
	//printf("%d\n", root->nKeys);
	
	//strcpy(root->Keys[0].Value, "Tommy");
	//printf("B\n");
	tmp = 0;
	bpt_PrintTree(&root);
	for(i=0;i<iter;i++)
	 	{
		//tmp = bpt_PrintTree(NULL);
		assert (tmp == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

