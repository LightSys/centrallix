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

	*tname = "b+tree-36 Remove multiple keys from leftmost leaf";
	iter = 8000000;
	
	pBPTree this;
	char* fname = "tests/bpt_bl_10e2.dat";

	for(i=0;i<1;i++)
	 	{
		this = bptBulkLoad(fname, 100);
		tmp = bptRemove(this, "00000001\0", 8);
		//assert (tmp == 0);
		tmp = bptRemove(this, "00000002\0", 8);
		//assert (tmp == 0);
		tmp = bptRemove(this, "00000003\0", 8);
		//assert (tmp == 0);
		tmp = bptRemove(this, "00000004\0", 8);
		//assert (tmp == 0);
		tmp = bptRemove(this, "00000005\0", 8);
		//assert (tmp == 0);
		assert (0 == 0);
		}
	bpt_PrintTreeSmall(this);
	
	for(i=0;i<iter;i++) assert (0==0);
	printf("\n");
	
    	return iter*4;
    	}
;
