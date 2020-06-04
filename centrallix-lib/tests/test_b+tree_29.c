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

	*tname = "b+tree-29 Bulk Loading: Size = 100";
	iter = 8000000;
	printf("1\n");	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";
	FILE* tree = NULL;
	FILE* dict = NULL;
	tree = fopen(fname, "w");
	dict = fopen("tests/dictionary.txt", "r");
	printf("2\n");
	if (dict == NULL)
		perror("dict is null");
	char str[50];
	printf("3\n");
	for (a=1; a<=100; a++)
		{
		fscanf(dict, "%s\n", str);
		fprintf(tree, "%08d %s\n", a, str);
		}
	fclose(dict);
	fclose(tree);
	printf("4\n");
	this = bptBulkLoad(fname, --a);
	printf("after bulk\n");
	bpt_PrintTreeSmall(this);

	for(i=0;i<iter;i++)
	 	{
		assert (5 == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}

