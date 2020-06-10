#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time03 testing time of searching tree size 100";
	iter = 2000000;
	
	pBPTree this = bptBulkLoad("tests/bpt_bl_10e2.dat", 100);
	pBPTree dummy = NULL;
	int idx = 0;

	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000020", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000030", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000040", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000056", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000070", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00000080", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000090", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00000095", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00000101", 8, &dummy, &idx);
                assert (tmp == -1);
		}
	
    	return iter;//do I need *4
    	}

