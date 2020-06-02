#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int a, i, iter, tmp;

	*tname = "b+tree-34 Remove";
	iter = 8000000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";
	char* str1 = "Ab";	

	this = bptBulkLoad(fname, --a);
	bpt_PrintTree(this);
	
	
	tmp = bptRemove(this, str1, strlen(str1));

	for(i=0;i<iter;i++)
	 	{
		assert (tmp == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

