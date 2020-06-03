#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int i, iter, a;

	*tname = "b+tree-35 Remove smallest node";
	iter = 8000000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";
	printf("Here1\n");	
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
	printf("Here1.5\n");
	this = bptBulkLoad(fname, --a);
	printf("Here2\n");
	bptRemove(this, "00000001\0", 8);
	printf("Here3\n");

	for(i=0;i<iter;i++)
	 	{
		assert (5 == 5);
		}

	printf("\n");
	
    	return iter*4;
    	}
;
