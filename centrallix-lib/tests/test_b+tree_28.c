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
	char* rval1;
	char* rval2;
	char* rval3;
	char* rval4;	

	pBPNode ret_node;

	*tname = "b+tree-28 Bulk Loading: Size = 10";
	iter = 8000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e1.dat";
	FILE* tree = NULL;
	FILE* dict = NULL;
	tree = fopen(fname, "w");
	dict = fopen("tests/dictionary.txt", "r");
	if (dict == NULL)
		perror("dict is null");
	char str[50];
	
	for (a=1; a<=10; a++)
		{
		fscanf(dict, "%s\n", str);
		fprintf(tree, "%08d %s\n", a, str);
		}
	fclose(dict);
	fclose(tree);

//	this = bptBulkLoad(fname, --a);
//	bpt_PrintTree(&this);

	for(i=0;i<iter;i++)
	 	{
		printf("%08d", 0);


		this = bptBulkLoad(fname, 10);
		assert (this != NULL);
		rval1 = (char*) bptLookup(this, "00000001", 8);
		printf("Return for \"00000001\": %s\n", rval1);
		assert (strcmp("A", rval1) == 0);
        /*
		rval2 = (char*) bptSearch(this, "00000009", 8);
        assert (strcmp("Aaron'srod", rval2) == 0);
		rval3 = (char*) bptSearch(this, "00000010", 8);
        assert (strcmp("Ab", rval3) == 0);
		rval4 = (char*) bptSearch(this, "00000011", 8);
        assert (rval4 == NULL);*/
		}
	
    	return iter*4;
    	}

