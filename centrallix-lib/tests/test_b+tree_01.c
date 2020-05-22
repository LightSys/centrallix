#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
   	{
	printf("\n");
	int i;
    	int iter;
	int tmp;

	*tname = "b+tree-01 EMPTY TREE";
	iter = 8000000;
	tmp = bpt_PrintTree(NULL);
	for(i=0;i<iter;i++)
	 	{
		//tmp = bpt_PrintTree(NULL);
		assert (tmp == 1);
		}

	printf("\n");
	
    	return iter*4;
    	}

