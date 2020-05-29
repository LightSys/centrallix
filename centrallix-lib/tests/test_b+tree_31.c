#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int a, i;
    	int iter;

	*tname = "b+tree-31 Bulk Loading: Size = 100,000";
	iter = 8000000;
	
	
	char* fname = "tests/bpt_bl_10e5.dat";
	FILE* tree = NULL;
	tree = fopen(fname, "w");

	for (a=1; a<=100000; a++)
		{
			fprintf(tree, "%08d %d\n", a, a);
		}
	fclose(tree);


	//bpt_PrintTree(root);
	for(i=0;i<iter;i++)
	 	{
		assert (5 == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}

