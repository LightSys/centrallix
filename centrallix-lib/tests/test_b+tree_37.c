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

	*tname = "b+tree-37 Redistribution in leaves";
	iter = 1;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";

	for(i=0;i<iter;i++)
	 	{
		this = bptBulkLoad(fname, 100);
		tmp = bptRemove(this, "00000087\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000088\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000094\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000095\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000096\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000097\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000098\0", 8);
                assert (tmp == 0);
		}
	bpt_PrintTreeSmall(this);
	
	printf("\n");
	
    	return iter*4;
    	}
;
