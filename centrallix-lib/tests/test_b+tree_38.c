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

	*tname = "b+tree-38 Coalesce inodes";
	iter = 100;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e3.dat";
	
	this = bptBulkLoad(fname, 1000);
	//bpt_PrintTreeSmall(this);
	
	for(i=0;i<iter;i++)
		{
		this = bptBulkLoad(fname, 1000);
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
		tmp = bptRemove(this, "00000586\0", 8);
                assert (tmp == 0);
		}
	//bpt_PrintTreeSmall(this);
	printf("\n");
	
    	return iter*4;
    	}
;
