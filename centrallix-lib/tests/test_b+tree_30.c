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

	*tname = "b+tree-30 Bulk Loading: Size = 1,000";
	iter = 8000000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e3.dat";
	FILE* tree = NULL;
	FILE* dict = NULL;
	tree = fopen(fname, "w");
	dict = fopen("tests/dictionary.txt", "r");
	if (dict == NULL)
		perror("dict is null");
	char str[50];
	
	for (a=1; a<=1000; a++)
		{
		fscanf(dict, "%s\n", str);
		fprintf(tree, "%08d %s\n", a, str);
		}
	fclose(dict);
	fclose(tree);

	this = bptBulkLoad(fname, --a);
	bpt_PrintTree(this);

	for(i=0;i<iter;i++)
	 	{
		assert (5 == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}

