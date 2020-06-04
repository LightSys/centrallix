#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int i, iter, a, tmp;

	*tname = "b+tree-35 Remove smallest node";
	iter = 8000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";

	bptRemove(this, "00000001\0", 8);

	for(i=0;i<iter;i++)
	 	{
		this = bptBulkLoad(fname, 100);
		assert (strcmp((char*)bptLookup(this, "00000001", 8), "A") == 0);
		tmp = bptRemove(this, "00000001\0", 8);
		assert (tmp == 0);
		assert (bptLookup(this, "00000001", 8) == NULL);
		tmp = bptRemove(this, "00000001\0", 8);
		assert (tmp == -1);
		}

	printf("\n");
	
    	return iter*4;
    	}
;
