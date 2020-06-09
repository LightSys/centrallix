#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int i, iter, tmp;

	*tname = "b+tree-36 Coalesce left leaf twice";
	iter = 1000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";

	for(i=0;i<iter;i++)
		{
		this = bptBulkLoad(fname, 100);
		tmp = bptRemove(this, "00000001\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000002\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000003\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000004\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000005\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000006\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000007\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000008\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000009\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000010\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000011\0", 8);
		assert (tmp == 0);
		tmp = bptRemove(this, "00000012\0", 8);
		assert (tmp == 0);
		}
	//bpt_PrintTreeSmall(this);
	printf("\n");
	
    	return iter*4;
    	}
;
