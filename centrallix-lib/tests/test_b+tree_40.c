#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int i, j, iter, tmp;
	char str[10];
	char trash[50];
	*tname = "b+tree-40 Remove all keys from tree of size 1000";
	iter = 50;
	
	pBPNode this;
	char* fname = "tests/bpt_bl_10e2.dat";
	FILE* f = NULL;
	this = bptBulkLoad(fname, 100);
	//bpt_PrintTreeSmall(this);
	
	for(i=0;i<iter;i++)
		{
		this = bptBulkLoad(fname, 100);
		f = fopen(fname, "r");
		if(f == NULL)
			return 0;
		for(j=0; j < 100; j++)
			{
			//printf("%d\n", j);
			fscanf(f, " %s %s\n", str, trash);
			//printf("%s\n", str);
			//if (j == 900)
			//	 bpt_PrintTreeSmall(this);
			tmp = bptRemove(this, str, 8);
			assert (tmp == 0);
			}
		fclose(f);
		}
	bpt_PrintTreeSmall(this);
	printf("\n");
	
    	return iter*4;
    	}

