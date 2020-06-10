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
	char* rval5;
        char* rval6;
        char* rval7;
        char* rval8;

	*tname = "b+tree-29 Bulk Loading: Size = 100";
	iter = 800;

	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";
	FILE* tree = NULL;
	FILE* dict = NULL;
	tree = fopen(fname, "w");
	dict = fopen("tests/dictionary.txt", "r");

	if (dict == NULL)
		perror("dict is null");
	char str[50];

	for (a=1; a<=100; a++)
		{
		fscanf(dict, "%s\n", str);
		fprintf(tree, "%08d %s\n", a, str);
		}
	fclose(dict);
	fclose(tree);

	for(i=0;i<iter;i++)
	 	{
		this = bptBulkLoad(fname, 100);
                assert (this != NULL);
                rval1 = (char*) bptLookup(this, "00000001", 8);
                assert (strcmp("A", rval1) == 0);
                //rval2 = (char*) bptLookup(this, "00000009", 8);
                //assert (strcmp("Aaron'srod", rval2) == 0);
                //rval3 = (char*) bptLookup(this, "00000057", 8);
                //assert (strcmp("Abashing", rval3) == 0);
               // rval4 = (char*) bptLookup(this, "00000033", 8);
               // assert (strcmp("Abalienation", rval4) == 0);	
		rval5 = (char*) bptLookup(this, "00000070", 8);
                assert (strcmp("Abator", rval5) == 0);
                rval6 = (char*) bptLookup(this, "00000088", 8);
                assert (strcmp("Abbey", rval6) == 0);
                rval7 = (char*) bptLookup(this, "00000100", 8);
		assert (strcmp("ABC", rval7) == 0);
		rval8 = (char*) bptLookup(this, "00000101", 8);
		assert (rval8 == NULL);
		}

	printf("\n");
	
    	return iter*4;
    	}

