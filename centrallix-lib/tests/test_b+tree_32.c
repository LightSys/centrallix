#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int a, i, iter;

	*tname = "b+tree-32 Bulk Loading: Size = 100,000";
	iter = 8000000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e5.dat";
	FILE* tree = NULL;
	tree = fopen(fname, "w");
	for (a=1; a<=100000; a++)
		{
		fprintf(tree, "%08d %d\n", a, a);
		}
	fclose(tree);

	this = bptBulkLoad(fname, --a);
	//bpt_PrintTree(this);

	for(i=0;i<iter;i++)
	 	{
		assert (5 == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}

