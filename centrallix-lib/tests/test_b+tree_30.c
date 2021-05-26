#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int a, i, tmp;
    	int iter;
	char* rval1;
        char* rval8;
	
	*tname = "b+tree-30 Bulk Loading: Size = 1,000";
	iter = 8000;
	
	pBPNode this;
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

	//this = bptBulkLoad(fname, 1000);
	//bpt_PrintTreeSmall(this);
	int idx;
	pBPNode locate;
	for(i=0;i<iter;i++)
	 	{
		this = bptBulkLoad(fname, 100);
                assert (this != NULL);
                rval1 = (char*) bptLookup(this, "00000001", 8);
                assert (strcmp("A", rval1) == 0);
                printf("A\n");
		//rval2 = (char*) bptLookup(this, "00000009", 8);
                //assert (strcmp("Aaron'srod", rval2) == 0);
                //rval3 = (char*) bptLookup(this, "00000057", 8);
                //assert (strcmp("Abashing", rval3) == 0);
                //rval4 = (char*) bptLookup(this, "00000497", 8);
                //assert (strcmp("Absumption", rval4) == 0);
                //rval5 = (char*) bptLookup(this, "00000700", 8);
                //assert (strcmp("Acclimatize", rval5) == 0);
                //rval6 = (char*) bptLookup(this, "00001000", 8);
                //assert (strcmp("Acipenser", rval6) == 0);
                tmp = bpt_i_Find(this, "00001000", 8, &locate, &idx);
		assert (tmp == 0);
		//printf("B\n");
		//rval7 = (char*) bptLookup(this, "00000100", 8);
                //assert (strcmp("ABC", rval7) == 0);
                rval8 = (char*) bptLookup(this, "00001001", 8);
                assert (rval8 == NULL);	
		printf("C\n");
		}

	printf("\n");
	
    	return iter*4;
    	}

